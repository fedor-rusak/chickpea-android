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
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

	struct saved_state state;
};

namespace engine_ns {

	/**
	 * Initialize an EGL context for the current display.
	 */
	static int engine_init_display(engine_struct* engine) {
		return opengl_wrapper::init(
			engine->app->window,
			engine->display,
			engine->context,
			engine->surface);
	}

	/**
	 * Tear down the EGL context currently associated with the display.
	 */
	static void engine_term_display(engine_struct* engine) {
		opengl_wrapper::destroy(
			engine->display,
			engine->context,
			engine->surface);

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
		if (engine->display == NULL) {
			LOGZ("ND");
			return;
		}

		grey += 0.01f;
		if (grey > 1.0f) {
			grey = 0.0f;
		}

		opengl_wrapper::render(grey);

		opengl_wrapper::swapBuffers(engine->display, engine->surface);
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
				if (engine->app->window != NULL) {
					engine_init_display(engine);
					jx_wrapper::test();
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
				// When our app gains focus, we start monitoring the accelerometer.
				if (engine->accelerometerSensor != NULL) {
					ASensorEventQueue_enableSensor(engine->sensorEventQueue,
							engine->accelerometerSensor);
					// We'd like to get 60 events per second (in us).
					ASensorEventQueue_setEventRate(engine->sensorEventQueue,
							engine->accelerometerSensor, (1000L/60)*1000);
				}
				break;
			case APP_CMD_LOST_FOCUS:
				// When our app loses focus, we stop monitoring the accelerometer.
				// This is to avoid consuming battery while not being used.
				if (engine->accelerometerSensor != NULL) {
					ASensorEventQueue_disableSensor(engine->sensorEventQueue,
							engine->accelerometerSensor);
				}
				// Also stop animating.
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
		jx_wrapper::init();

		engine_struct engine;

		function_to_prevent_stripping();

		memset(&engine, 0, sizeof(engine));
		state->userData = &engine;
		state->onAppCmd = engine_handle_cmd;
		state->onInputEvent = engine_handle_input;
		engine.app = state;

		// Prepare to monitor accelerometer
		engine.sensorManager = ASensorManager_getInstance();
		engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
				ASENSOR_TYPE_ACCELEROMETER);
		engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
				state->looper, LOOPER_ID_USER, NULL, NULL);

		while (1) {
			int events;
			struct android_poll_source* source;

			int ident = ALooper_pollAll(0, NULL, &events, (void**)&source);

			if (ident >= 0) {
				if (source != NULL) {
					source->process(state, source);
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