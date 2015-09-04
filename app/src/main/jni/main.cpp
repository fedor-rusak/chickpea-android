#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <android/sensor.h>
#include <android/log.h>

#include <integration_enums.h>
#include <integration_struct.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGZ(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ZZZ", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ZZZ", __VA_ARGS__))

#include <stdio.h>

#include <opengl_wrapper.h>
#include <jx_wrapper.h>

/**
 * Our saved state data.
 */
struct saved_state {
	int32_t x;
	int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine_struct {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	int animating;

	struct saved_state state;
};

namespace engine_ns {

	static int engine_init_display(engine_struct* engine) {
		return opengl_wrapper::init(engine->app->native_stuff.window);
	}

	static void engine_term_display(engine_struct* engine) {
		opengl_wrapper::destroy();

		engine->animating = 0;
	}

	/**
	 * Process the next input event.
	 */
	static int activeId = -1;

	static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
		engine_struct* engine = (engine_struct*)app->userData;
		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
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
		return 0;
	}


	static float grey = 0.0;

	static void engine_draw_frame(engine_struct* engine) {
		grey += 0.01f;
		if (grey > 1.0f) {
			grey = 0.0f;
		}

		opengl_wrapper::render(grey);

		opengl_wrapper::swapBuffers();
	}

	/**
	 * Process the next main command.
	 */
	static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
		engine_struct* engine = (engine_struct*)app->userData;
		switch (cmd) {
			case APP_CMD_SAVE_STATE:
				// The system has asked us to save our current state.  Do so.
				engine->app->savedState = malloc(sizeof(struct saved_state));
				*((struct saved_state*)engine->app->savedState) = engine->state;
				engine->app->savedStateSize = sizeof(struct saved_state);
				break;
			case APP_CMD_INIT_WINDOW:
				// The window is being shown, get it ready.
				if (engine->app->native_stuff.window != NULL) {
					engine_init_display(engine);

					opengl_wrapper::initProgram();

					engine->animating = 1;
					engine_draw_frame(engine);
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
				engine_draw_frame(engine);
				break;
		}
	}

	/**
	 * This is the main entry point of a native application that is using
	 * android_threaded_adapter. It runs in its own thread, with its own
	 * event loop for receiving input events and doing other things.
	 */
	void android_main(struct android_app* state) {
		function_to_prevent_stripping();

		jx_wrapper::init();
		jx_wrapper::test();


		ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
		ALooper_addFd(looper, state->io_stuff.msgread, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, NULL, NULL);
		state->io_stuff.looper = looper;

		engine_struct engine;
		memset(&engine, 0, sizeof(engine));
		state->userData = &engine;
		engine.app = state;


		while (1) {
			int uselessEvents;

			int ident = ALooper_pollAll(0, NULL, &uselessEvents, NULL);

			LOGZ("uselessEvents - %i", uselessEvents);

			if (ident >= 0) {
				if (ident == LOOPER_ID_MAIN) {
					process_cmd(state, engine_handle_cmd);
				}

				if (ident == LOOPER_ID_INPUT) {
					process_input(state, engine_handle_input);
				}

				if (state->destroyRequested != 0) {
					engine_term_display(&engine);
					return;
				}
			}

			if (engine.animating) {
				engine_draw_frame(&engine);
			}
		}

	}

}