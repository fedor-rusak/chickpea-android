#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "jx_wrapper", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "jx_wrapper", __VA_ARGS__))

#include <jx.h>
#include <jx_result.h>

#include <string>

namespace jx_wrapper {

	void* assetManager;
	int (*readFile)(void*, const char*, char**);

	void setReadFileParts(void* assetManagerInstance, int (*readFileFunction)(void*, const char*, char**)) {
		assetManager = assetManagerInstance;
		readFile = readFileFunction;
	}

	void assetReadSync(JXValue *results, int argc) {
		const char *filename = JX_GetString(&results[0]);

		if (readFile != NULL || assetManager != NULL) {
			char* data;
			int read_len = readFile(assetManager, filename, &data);
			if (read_len >= 0) {
				JX_SetBuffer(&results[argc], data, read_len);
				return;
			}
		}

		const char *err = "File doesn't exist";
		JX_SetError(&results[argc], err, strlen(err));
	}


	void (*cacheTextureCallback)(char*, char*);

	void cacheTexture(JXValue *results, int argc) {
		char* lable = (char*) JX_GetString(&results[0]);
		char* path = (char*) JX_GetString(&results[1]);

		cacheTextureCallback(lable, path);
	}

	void setCacheTextureCallback(void (*callback)(char*, char*)) {
		cacheTextureCallback = callback;

		JX_DefineExtension("cacheTexture", cacheTexture);
	}

	void (*renderCallback)(char*, float, float, float);

	void render(JXValue *results, int argc) {
		char* label = (char*) JX_GetString(&results[0]);

		float offsetX = 0;
		float offsetY = 0;
		float offsetZ = 0;

		if (argc > 1)
			offsetX = (float) JX_GetDouble(&results[1]);

		if (argc > 2)
			offsetY = (float) JX_GetDouble(&results[2]);

		if (argc > 3)
			offsetZ = (float) JX_GetDouble(&results[3]);

		renderCallback(label, offsetX, offsetY, offsetZ);
	}

	void setRenderCallback(void (*callback)(char*, float, float, float)) {
		renderCallback = callback;
		JX_DefineExtension("render", render);
	}

	void (*setCameraCallback)(float, float, float);

	void setCamera(JXValue *results, int argc) {
		float offsetX = 0;
		float offsetY = 0;
		float offsetZ = 0;

		if (argc > 0)
			offsetX = (float) JX_GetDouble(&results[0]);

		if (argc > 1)
			offsetY = (float) JX_GetDouble(&results[1]);

		if (argc > 2)
			offsetZ = (float) JX_GetDouble(&results[2]);

		setCameraCallback(offsetX, offsetY, offsetZ);
	}

	void setSetCameraCallback(void (*callback)(float, float, float)) {
		setCameraCallback = callback;
		JX_DefineExtension("setCamera", setCamera);
	}

	void (*getScreenDimensionsCallback)(int*, int*);

	void getScreenDimensions(JXValue *results, int argc) {
		int width, height;
		getScreenDimensionsCallback(&width, &height);

		char data[40];
		sprintf(data, "{\"width\": %i, \"height\": %i}", width, height);

		std::string result = "";
		result += data;		

		const char* str = result.c_str();

		JX_SetJSON(&results[argc], str, strlen(str));
	}

	void setGetScreenDimensionsCallback(void (*callback)(int*, int*)) {
		getScreenDimensionsCallback = callback;
		JX_DefineExtension("getScreenDimensions", getScreenDimensions);
	}

	void (*clearScreenCallback)(float, float, float);

	void clearScreen(JXValue *results, int argc) {
		float colorR = (float) JX_GetDouble(&results[0]);
		float colorG = (float) JX_GetDouble(&results[1]);
		float colorB = (float) JX_GetDouble(&results[2]);

		clearScreenCallback(colorR, colorG, colorB);
	}

	void setClearScreenCallback(void (*callback)(float, float, float)) {
		clearScreenCallback = callback;
		JX_DefineExtension("clearScreen", clearScreen);
	}

	void (*unprojectCallback)(int, int, float*, float*);

	void unproject(JXValue *results, int argc) {
		int screenX = JX_GetInt32(&results[0]);
		int screenY = JX_GetInt32(&results[1]);

		float worldX, worldY;
		unprojectCallback(screenX, screenY, &worldX, &worldY);

		char data[30];
		sprintf(data, "[%f, %f]", worldX, worldY);

		std::string result = "";
		result += data;		

		const char* str = result.c_str();

		JX_SetJSON(&results[argc], str, strlen(str));
	}

	void setUnprojectCallback(void (*callback)(int, int, float*, float*)) {
		unprojectCallback = callback;
		JX_DefineExtension("unproject", unproject);
	}

	void* assetManagerForSound;
	void (*backgroundCacheSound)(void*, const char*, char*);
	void (*actionCacheSound)(void*, const char*, char*);

	void cacheSound(JXValue *results, int argc) {
		char* tagValue = JX_GetString(&results[0]);
		char* path = JX_GetString(&results[1]);

		LOGI("tag!!! %s!!!", tagValue);
		if (strcmp((char*) "background", tagValue) == 0) {
			backgroundCacheSound(assetManagerForSound, (const char*)path, tagValue);
		}
		else if (strcmp((char*) "action", tagValue) == 0) {
			actionCacheSound(assetManagerForSound, (const char*)path, tagValue);
		}
	}

	void setCacheSoundCallbacks(void* assetManagerForSoundValue, void (*backgroundCacheSoundCallback)(void*, const char*, char*), void (*actionCacheSoundCallback)(void*, const char*, char*)) {
		assetManagerForSound = assetManagerForSoundValue;
		backgroundCacheSound = backgroundCacheSoundCallback;
		actionCacheSound = actionCacheSoundCallback;

		JX_DefineExtension("cacheSound", cacheSound);
	}


	void (*playSoundCallback)(bool);

	void playSound(JXValue *results, int argc) {
		playSoundCallback(true);
	}

	void setPlaySoundCallback(void (*callback)(bool)) {
		playSoundCallback = callback;

		JX_DefineExtension("playSound", playSound);
	}


	static bool JXCoreInitialized = false;

	void initForCurrentThread(char* startScript) {
		JX_InitializeNewEngine();
		JX_DefineExtension("assetReadSync", assetReadSync);
		JX_DefineMainFile(startScript);
		JX_StartEngine();
	}

	int init() {
		if (!JXCoreInitialized) {
			JX_Initialize("some_data", NULL);
			JXCoreInitialized = true;

			initForCurrentThread((char *) "global.require = require;");
		}

		return 0;
	}

	void evaluate(char* script) {
		JXValue tempValue;
		JX_Evaluate(script,
				"evaluateScript", &tempValue);
		JX_Free(&tempValue);
	}

	void test() {
		JXValue tempValue, tempProperty;
		JX_Evaluate(
				"console.log(\"Trying to load module...\");"
				"var value = 0;"
				"try {value = global.require('/meaningOfLife.js');} catch(e){value = \"error\"+e.toString();}"
				"console.log(\"It\'s value is \"+value);"
				"[value];",
				"processInput", &tempValue);

		JX_GetIndexedProperty(&tempValue, 0, &tempProperty);
		char* stringValue =  JX_GetString(&tempProperty);

		JX_Free(&tempProperty);
		JX_Free(&tempValue);

		JX_Loop();

		LOGI("Is it SpiderMonkey? %s!!!", JX_IsSpiderMonkey() ? "Yes" : "No");
		LOGI("Is it V8? %s!!!", JX_IsV8() ? "Yes" : "No");

		JX_ForceGC();
	}

	void destroy() {
		JX_StopEngine();
	}

}