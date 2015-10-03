#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGZ(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ZZZ", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ZZZ", __VA_ARGS__))

#include <stdio.h>

#include <integration_contract.h>
#include <integration_enums.h>

#include <engine/opengl_wrapper.h>

#include <engine/jx_wrapper.h>

#include <engine/opensles_wrapper.h>

/**
 * Our saved state data.
 */
struct saved_state {
	float value;
	int32_t x;
	int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine_struct {
	global_struct* app;

	int animating;

	saved_state state;
};


static float trygetAxis(const AInputEvent* event) {
	float result = 0;
	int i = 1;
	static const int32_t AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 11, AXIS_RZ = 14, AXIS_HAT_X = 15, AXIS_HAT_Y = 16;
	float pos[2];
	for (i = 0; i < 3; i++) {
		switch(i) {
			case 0:
				pos[0] = AMotionEvent_getAxisValue(event, AXIS_X, 0);
				pos[1] = AMotionEvent_getAxisValue(event, AXIS_Y, 0);
				result = pos[0];
				if (result < 0) {result *= -1;}
			break;
				pos[0] = AMotionEvent_getAxisValue(event, AXIS_Z, 0);
				pos[1] = AMotionEvent_getAxisValue(event, AXIS_RZ, 0);
			break;
			case 2:
				pos[0] = AMotionEvent_getAxisValue(event, AXIS_HAT_X, 0);
				pos[1] = AMotionEvent_getAxisValue(event, AXIS_HAT_Y, 0);
			break;
		}
		LOGZ("DATA%i: %f, %f", i, pos[0], pos[1]);
	}

	return result;
}


namespace chickpea {

	static int engine_init_display(engine_struct* engine) {
		return opengl_wrapper::init(engine->app);
	}

	void engine_term_display(engine_struct* engine) {
		opengl_wrapper::destroy();

		jx_wrapper::destroy();

		engine->animating = 0;
	}

	/**
	 * Process the next input event.
	 */
	static int activeId = -1;

	int32_t engine_handle_input(global_struct* global, AInputEvent* event) {
		engine_struct* engine = (engine_struct*)global->appdata.internal;
		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
			engine->state.value = trygetAxis(event);

			switch(AInputEvent_getSource(event)){
				case AINPUT_SOURCE_TOUCHSCREEN:
					int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
					int pointerIndex = (AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
					switch(action){
						case AMOTION_EVENT_ACTION_DOWN: {
							activeId = AMotionEvent_getPointerId(event, 0);

							int index = 0;
							engine->state.x = AMotionEvent_getX(event, index);
							engine->state.y = AMotionEvent_getY(event, index);
							LOGZ("x %i, y %i", engine->state.x, engine->state.y);

							LOGZ("DOWN");
						}
						break;
						case AMOTION_EVENT_ACTION_POINTER_DOWN: {
							activeId = AMotionEvent_getPointerId(event, pointerIndex);
							LOGZ("POINTER DOWN");
						}
						break;
						case AMOTION_EVENT_ACTION_UP:
							activeId = -1;
							LOGZ("UP");
						break;
						case AMOTION_EVENT_ACTION_POINTER_UP: {
							int pointerId = AMotionEvent_getPointerId(event, pointerIndex);

							if (pointerId == activeId) {
								int newPointerIndex = pointerIndex == 0 ? 1 : 0;

								activeId = AMotionEvent_getPointerId(event, newPointerIndex);
							}
							LOGZ("POINTER UP");
						}
						break;
						case AMOTION_EVENT_ACTION_MOVE: {
							for (int i = 0; i < AMotionEvent_getPointerCount(event); i++) {
								int index = i;
								engine->state.x = AMotionEvent_getX(event, index);
								engine->state.y = AMotionEvent_getY(event, index);
								LOGZ("index: %i, id: %i, x: %i, y: %i", i, AMotionEvent_getPointerId( event, i ), engine->state.x, engine->state.y);
							}
							LOGZ("MOVE", pointerIndex);
						}
						break;
					}
				break;
			}

			return 1;
		}
		else {
			LOGZ("EventType is %i", AInputEvent_getType(event));
			LOGZ("Source is %i", AInputEvent_getSource(event));
			LOGZ("Action is %i",AKeyEvent_getAction(event));
			LOGZ("KeyCode is %i",AKeyEvent_getKeyCode(event));
		}
		return 0;
	}


	static float grey = 0.0;

	void engine_draw_frame(global_struct* global) {
		engine_struct* engine = (engine_struct*) global->appdata.internal;

		if (!engine->animating) {return;}

		grey += 0.01f;
		if (grey > 1.0f - engine->state.value) {
			grey = 0.0f;
		}

		opengl_wrapper::render(grey);

		opengl_wrapper::swapBuffers();
	}

	/**
	 * Process the next main command.
	 */
	void engine_handle_cmd(global_struct* app, int32_t cmd) {
		engine_struct* engine = (engine_struct*)app->appdata.internal;
		switch (cmd) {
			case APP_CMD_SAVE_STATE:
				// The system has asked us to save our current state.  Do so.
				app->appdata.savedState = malloc(sizeof(struct saved_state));
				*((struct saved_state*)app->appdata.savedState) = engine->state;
				app->appdata.savedStateSize = sizeof(struct saved_state);
				break;
			case APP_CMD_INIT_WINDOW:
				// The window is being shown, get it ready.
				if (app->native_stuff.window != NULL) {
					engine_init_display(engine);

					opengl_wrapper::initProgram();

					engine->animating = 1;
					engine_draw_frame(app);
				}
				break;
			case APP_CMD_TERM_WINDOW:
				// The window is being hidden or closed, clean it up.
				engine_term_display(engine);
				break;
			case APP_CMD_GAINED_FOCUS:
				break;
			case APP_CMD_LOST_FOCUS:
				engine->animating = 0;
				engine_draw_frame(app);
				break;
		}
	}

	void init(global_struct* global, char* startScript) {
		jx_wrapper::initForCurrentThread(startScript);
		jx_wrapper::test();

		opensles_wrapper::createEngine();
		opensles_wrapper::createBufferQueueAudioPlayer();
		opensles_wrapper::createAssetAudioPlayer(global->native_stuff.activity->assetManager, "sound/background.mp3");
		opensles_wrapper::setPlayingAssetAudioPlayer(true);
		//opensles_wrapper::selectClip();

		ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
		ALooper_addFd(looper, global->io_stuff.msgread, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, NULL, NULL);
		global->io_stuff.looper = looper;

		engine_struct* engine_value = (engine_struct*) malloc(sizeof(engine_struct));
		memset(engine_value, 0, sizeof(engine_struct));
		global->appdata.internal = engine_value;
		engine_value->app = global;
	}

	int getInputIdentifier() {
		int uselessEvents;

		return ALooper_pollAll(0, NULL, &uselessEvents, NULL);
	}

	bool shouldExit(global_struct* global) {
		return global->flags.destroyRequested != 0;
	}

	void processInput(global_struct* global) {
		int identifier;

		while ((identifier = getInputIdentifier()) >= 0) {

			if (identifier == LOOPER_ID_MAIN) {
				global->native_stuff.process_cmd(global, engine_handle_cmd);
			}

			if (identifier == LOOPER_ID_INPUT) {
				global->native_stuff.process_input(global, engine_handle_input);
			}
		}
	}

	void preInitSetup(void* assetManager, int (*readFile)(void*, const char*, char**)) {
		jx_wrapper::setReadFileParts(assetManager, readFile);
		jx_wrapper::init();
		jx_wrapper::test();
	}

	void destroy(global_struct* global) {
		engine_term_display((engine_struct*) global->appdata.internal);
		opensles_wrapper::shutdown();
		free((engine_struct*) global->appdata.internal);
	}

}