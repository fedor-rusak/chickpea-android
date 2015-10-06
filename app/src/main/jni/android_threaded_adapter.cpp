#include <jni.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include <android/log.h>

#include "integration_contract.h"
#include "integration_enums.h"

#include <engine_technical.hpp>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "threaded_app", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "threaded_app", __VA_ARGS__))
#define LOGY(...) ((void)__android_log_print(ANDROID_LOG_INFO, "YYY", __VA_ARGS__))

static void free_saved_state(global_struct* global) {
	pthread_mutex_lock(&global->threading.mutex);
	if (global->appdata.savedState != NULL) {
		free(global->appdata.savedState);
		global->appdata.savedState = NULL;
		global->appdata.savedStateSize = 0;
	}
	pthread_mutex_unlock(&global->threading.mutex);
}

int8_t global_struct_read_cmd(global_struct* global) {
	int8_t cmd;
	if (read(global->io_stuff.msgread, &cmd, sizeof(cmd)) == sizeof(cmd)) {
		switch (cmd) {
			case APP_CMD_SAVE_STATE:
				free_saved_state(global);
				break;
		}
		return cmd;
	}
	else {
		LOGE("No data on command pipe!");
	}
	return -1;
}

static void print_cur_config(global_struct* global) {
	AConfiguration* config = global->native_stuff.config;
	char lang[2], country[2];
	AConfiguration_getLanguage(config, lang);
	AConfiguration_getCountry(config, country);

	LOGI("Config: mcc=%d mnc=%d lang=%c%c cnt=%c%c orien=%d touch=%d dens=%d "
			"keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d long=%d "
			"modetype=%d modenight=%d",
			AConfiguration_getMcc(config),
			AConfiguration_getMnc(config),
			lang[0], lang[1], country[0], country[1],
			AConfiguration_getOrientation(config),
			AConfiguration_getTouchscreen(config),
			AConfiguration_getDensity(config),
			AConfiguration_getKeyboard(config),
			AConfiguration_getNavigation(config),
			AConfiguration_getKeysHidden(config),
			AConfiguration_getNavHidden(config),
			AConfiguration_getSdkVersion(config),
			AConfiguration_getScreenSize(config),
			AConfiguration_getScreenLong(config),
			AConfiguration_getUiModeType(config),
			AConfiguration_getUiModeNight(config));
}

void global_struct_pre_exec_cmd(global_struct* global, int8_t cmd) {
	io_struct& io_stuff = global->io_stuff;

	switch (cmd) {
		case APP_CMD_INPUT_CHANGED:
			LOGI("APP_CMD_INPUT_CHANGED\n");
			pthread_mutex_lock(&global->threading.mutex);
			if (io_stuff.inputQueue != NULL) {
				AInputQueue_detachLooper(io_stuff.inputQueue);
			}
			io_stuff.inputQueue = io_stuff.pendingInputQueue;
			if (io_stuff.inputQueue != NULL) {
				LOGI("Attaching input queue to looper");
				AInputQueue_attachLooper(io_stuff.inputQueue,
						io_stuff.looper, LOOPER_ID_INPUT, NULL, NULL);
			}
			pthread_cond_broadcast(&(global->threading.cond));
			pthread_mutex_unlock(&(global->threading.mutex));
			break;

		case APP_CMD_INIT_WINDOW:
			LOGI("APP_CMD_INIT_WINDOW\n");
			pthread_mutex_lock(&global->threading.mutex);
			global->native_stuff.window = global->native_stuff.pendingWindow;
			pthread_cond_broadcast(&global->threading.cond);
			pthread_mutex_unlock(&global->threading.mutex);
			break;

		case APP_CMD_TERM_WINDOW:
			LOGI("APP_CMD_TERM_WINDOW\n");
			pthread_cond_broadcast(&global->threading.cond);
			break;

		case APP_CMD_RESUME:
		case APP_CMD_START:
		case APP_CMD_PAUSE:
		case APP_CMD_STOP:
			LOGI("activityState=%d\n", cmd);
			pthread_mutex_lock(&global->threading.mutex);
			global->flags.activityState = cmd;
			pthread_cond_broadcast(&global->threading.cond);
			pthread_mutex_unlock(&global->threading.mutex);
			break;

		case APP_CMD_CONFIG_CHANGED:
			LOGI("APP_CMD_CONFIG_CHANGED\n");
			AConfiguration_fromAssetManager(global->native_stuff.config,
					global->native_stuff.activity->assetManager);
			print_cur_config(global);
			break;

		case APP_CMD_DESTROY:
			LOGI("APP_CMD_DESTROY\n");
			global->flags.destroyRequested = 1;
			break;
	}
}

void global_struct_post_exec_cmd(global_struct* global, int8_t cmd) {
	switch (cmd) {
		case APP_CMD_TERM_WINDOW:
			LOGI("APP_CMD_TERM_WINDOW\n");
			pthread_mutex_lock(&global->threading.mutex);
			global->native_stuff.window = NULL;
			pthread_cond_broadcast(&global->threading.cond);
			pthread_mutex_unlock(&global->threading.mutex);
			break;

		case APP_CMD_SAVE_STATE:
			LOGI("APP_CMD_SAVE_STATE\n");
			pthread_mutex_lock(&global->threading.mutex);
			global->flags.stateSaved = 1;
			pthread_cond_broadcast(&global->threading.cond);
			pthread_mutex_unlock(&global->threading.mutex);
			break;

		case APP_CMD_RESUME:
			free_saved_state(global);
			break;
	}
}

static void global_struct_destroy(global_struct* global) {
	LOGI("global_destroy!");
	free_saved_state(global);
	pthread_mutex_lock(&global->threading.mutex);
	if (global->io_stuff.inputQueue != NULL) {
		AInputQueue_detachLooper(global->io_stuff.inputQueue);
	}
	AConfiguration_delete(global->native_stuff.config);
	global->flags.destroyed = 1;
	pthread_cond_broadcast(&global->threading.cond);
	pthread_mutex_unlock(&global->threading.mutex);
	// Can't touch global_struct struct after this.
}

void process_input(global_struct* global, int32_t (*onInputEvent)(global_struct*, AInputEvent*)) {
	AInputEvent* event = NULL;
	while (AInputQueue_getEvent(global->io_stuff.inputQueue, &event) >= 0) {
		LOGI("New input event: type=%d\n", AInputEvent_getType(event));
		if (AInputQueue_preDispatchEvent(global->io_stuff.inputQueue, event)) {
			continue;
		}

		int32_t handled = 0;
		if (onInputEvent != NULL) handled = onInputEvent(global, event);
		AInputQueue_finishEvent(global->io_stuff.inputQueue, event, handled);
	}
}

void process_cmd(global_struct* global, void (*onAppCmd)(global_struct*, int32_t)) {
	int8_t cmd = global_struct_read_cmd(global);
	global_struct_pre_exec_cmd(global, cmd);
	if (onAppCmd != NULL) onAppCmd(global, cmd);
	global_struct_post_exec_cmd(global, cmd);
}


int readStringFileAsset(void* currentAssetManager, const char* filename, char** result) {
	AAsset *asset = AAssetManager_open((AAssetManager*) currentAssetManager, filename, AASSET_MODE_UNKNOWN);
	if (asset) {
		off_t fileSize = AAsset_getLength(asset);
		void* data = malloc(fileSize+1);
		int read_len = AAsset_read(asset, data, fileSize);
		AAsset_close(asset);
		((char*) data)[fileSize] = '\0';
		*result = (char*) data;

		return read_len;
	}
	else {
		return -1;
	}
}

int readBinaryFileAsset(void* currentAssetManager, const char* filename, unsigned char** result) {
	AAsset *asset = AAssetManager_open((AAssetManager*) currentAssetManager, filename, AASSET_MODE_UNKNOWN);
	if (asset) {
		off_t fileSize = AAsset_getLength(asset);
		void* data = malloc(fileSize);
		int read_len = AAsset_read(asset, data, fileSize);
		AAsset_close(asset);
		*result = (unsigned char*) data;

		return read_len;
	}
	else {
		return -1;
	}
}

/* Spawns a thread */
static global_struct* setupGlobalStruct(
	ANativeActivity* activity,
	void* savedState, size_t savedStateSize,
	void* (*threadEntryCode)(void*)) {

	global_struct* global = (global_struct*)malloc(sizeof(global_struct));
	memset(global, 0, sizeof(global_struct));


	global->native_stuff.activity = activity;

	global->native_stuff.config = AConfiguration_new();
	AConfiguration_fromAssetManager(global->native_stuff.config, activity->assetManager);
	print_cur_config(global);

	global->native_stuff.process_cmd = process_cmd;
	global->native_stuff.process_input = process_input;
	global->native_stuff.readStringFile = readStringFileAsset;
	global->native_stuff.readBinaryFile = readBinaryFileAsset;
	global->native_stuff.assetManager = (void*) activity->assetManager;



	int msgpipe[2];
	if (pipe(msgpipe)) {
		LOGE("could not create pipe: %s", strerror(errno));
		return NULL;
	}
	global->io_stuff.msgread = msgpipe[0];
	global->io_stuff.msgwrite = msgpipe[1];

	pthread_mutex_init(&global->threading.mutex, NULL);
	pthread_cond_init(&global->threading.cond, NULL);

	if (savedState != NULL) {
		global->appdata.savedState = malloc(savedStateSize);
		global->appdata.savedStateSize = savedStateSize;
		memcpy(global->appdata.savedState, savedState, savedStateSize);
	}


	pthread_attr_t attr; 
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&global->threading.instance, &attr, threadEntryCode, global);


	return global;
}

static void* global_struct_entry(void* param) {
	global_struct* global = (global_struct*) param;

	char* startScript;
    readStringFileAsset((void*)global->native_stuff.activity->assetManager, (char*) "init.js", &startScript);

	android_main(global, startScript);

	global_struct_destroy(global);

	return NULL;
}


/* Spawns a thread */

static void global_struct_write_cmd(global_struct* global, int8_t cmd) {
	if (write(global->io_stuff.msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd)) {
		LOGE("Failure writing cmd: %s\n", strerror(errno));
	}
}

static void global_struct_set_input(global_struct* global, AInputQueue* inputQueue) {
	pthread_mutex_lock(&global->threading.mutex);
	global->io_stuff.pendingInputQueue = inputQueue;
	global_struct_write_cmd(global, APP_CMD_INPUT_CHANGED);
	while (global->io_stuff.inputQueue != global->io_stuff.pendingInputQueue) {
		pthread_cond_wait(&global->threading.cond, &global->threading.mutex);
	}
	pthread_mutex_unlock(&global->threading.mutex);
}

static void global_struct_set_window(global_struct* global, ANativeWindow* window) {
	pthread_mutex_lock(&global->threading.mutex);
	if (global->native_stuff.pendingWindow != NULL) {
		global_struct_write_cmd(global, APP_CMD_TERM_WINDOW);
	}
	global->native_stuff.pendingWindow = window;
	if (window != NULL) {
		global_struct_write_cmd(global, APP_CMD_INIT_WINDOW);
	}
	while (global->native_stuff.window != global->native_stuff.pendingWindow) {
		pthread_cond_wait(&global->threading.cond, &global->threading.mutex);
	}
	pthread_mutex_unlock(&global->threading.mutex);
}


static void global_struct_set_activity_state(struct global_struct* global, int8_t cmd) {
	pthread_mutex_lock(&global->threading.mutex);
	global_struct_write_cmd(global, cmd);
	while (global->flags.activityState != cmd) {
		pthread_cond_wait(&global->threading.cond, &global->threading.mutex);
	}
	pthread_mutex_unlock(&global->threading.mutex);
}

static void global_struct_free(struct global_struct* global) {
	pthread_mutex_lock(&global->threading.mutex);
	global_struct_write_cmd(global, APP_CMD_DESTROY);
	while (!global->flags.destroyed) {
		pthread_cond_wait(&global->threading.cond, &global->threading.mutex);
	}
	pthread_mutex_unlock(&global->threading.mutex);

	close(global->io_stuff.msgread);
	close(global->io_stuff.msgwrite);
	pthread_cond_destroy(&global->threading.cond);
	pthread_mutex_destroy(&global->threading.mutex);
	free(global);
}


/* Boring callbacks for Android events */

static void onDestroy(ANativeActivity* activity) {
	LOGI("Destroy: %p\n", activity);
	global_struct_free((global_struct*)activity->instance);
}

static void onStart(ANativeActivity* activity) {
	LOGI("Start: %p\n", activity);
	global_struct_set_activity_state((global_struct*)activity->instance, APP_CMD_START);
}

static void onResume(ANativeActivity* activity) {
	LOGI("Resume: %p\n", activity);
	global_struct_set_activity_state((global_struct*)activity->instance, APP_CMD_RESUME);
}

static void* onSaveInstanceState(ANativeActivity* activity, size_t* outLen) {
	global_struct* global = (global_struct*)activity->instance;
	void* savedState = NULL;

	LOGI("SaveInstanceState: %p\n", activity);
	pthread_mutex_lock(&global->threading.mutex);
	global->flags.stateSaved = 0;
	global_struct_write_cmd(global, APP_CMD_SAVE_STATE);
	while (!global->flags.stateSaved) {
		pthread_cond_wait(&global->threading.cond, &global->threading.mutex);
	}

	if (global->appdata.savedState != NULL) {
		savedState = global->appdata.savedState;
		*outLen = global->appdata.savedStateSize;
		global->appdata.savedState = NULL;
		global->appdata.savedStateSize = 0;
	}

	pthread_mutex_unlock(&global->threading.mutex);

	return savedState;
}

static void onPause(ANativeActivity* activity) {
	LOGI("Pause: %p\n", activity);
	global_struct_set_activity_state((global_struct*)activity->instance, APP_CMD_PAUSE);
}

static void onStop(ANativeActivity* activity) {
	LOGI("Stop: %p\n", activity);
	global_struct_set_activity_state((global_struct*)activity->instance, APP_CMD_STOP);
}

static void onConfigurationChanged(ANativeActivity* activity) {
	LOGI("ConfigurationChanged: %p\n", activity);
	global_struct_write_cmd((global_struct*) activity->instance, APP_CMD_CONFIG_CHANGED);
}

static void onLowMemory(ANativeActivity* activity) {
	LOGI("LowMemory: %p\n", activity);
	global_struct_write_cmd((global_struct*)activity->instance, APP_CMD_LOW_MEMORY);
}

static void onWindowFocusChanged(ANativeActivity* activity, int focused) {
	LOGI("WindowFocusChanged: %p -- %d\n", activity, focused);
	global_struct_write_cmd((global_struct*)activity->instance,
			focused ? APP_CMD_GAINED_FOCUS : APP_CMD_LOST_FOCUS);
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
	LOGI("NativeWindowCreated: %p -- %p\n", activity, window);
	global_struct_set_window((global_struct*)activity->instance, window);
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
	LOGI("NativeWindowDestroyed: %p -- %p\n", activity, window);
	global_struct_set_window((global_struct*)activity->instance, NULL);
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
	LOGI("InputQueueCreated: %p -- %p\n", activity, queue);
	global_struct_set_input((global_struct*)activity->instance, queue);
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
	LOGI("InputQueueDestroyed: %p -- %p\n", activity, queue);
	global_struct_set_input((global_struct*)activity->instance, NULL);
}

/* Starting point */
void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
	LOGI("Creating: %p\n", activity);

	chickpea::preInitSetup((void*) activity->assetManager, readStringFileAsset);

	activity->callbacks->onDestroy = onDestroy;
	activity->callbacks->onStart = onStart;
	activity->callbacks->onResume = onResume;
	activity->callbacks->onSaveInstanceState = onSaveInstanceState;
	activity->callbacks->onPause = onPause;
	activity->callbacks->onStop = onStop;
	activity->callbacks->onConfigurationChanged = onConfigurationChanged;
	activity->callbacks->onLowMemory = onLowMemory;
	activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
	activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
	activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
	activity->callbacks->onInputQueueCreated = onInputQueueCreated;
	activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;

	activity->instance = setupGlobalStruct(activity, savedState, savedStateSize, global_struct_entry);
}