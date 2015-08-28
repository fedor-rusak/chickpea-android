/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGZ(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ZZZ", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ZZZ", __VA_ARGS__))

#include <stdio.h>

#include <jx.h>
#include <jx_result.h>

#include <simple.h>

/**
 * Our saved state data.
 */
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
	// initialize OpenGL ES and EGL

	/*
	 * Here specify the attributes of the desired configuration.
	 * Below, we select an EGLConfig with at least 8 bits per color
	 * component compatible with on-screen windows
	 */
	const EGLint attribs[] = {
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_NONE
	};
	EGLint w, h, dummy, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	int* attrib_list1 = new int[3];
	attrib_list1[0]=EGL_CONTEXT_CLIENT_VERSION;
	attrib_list1[1]=2;
	attrib_list1[2]=EGL_NONE;
	context = eglCreateContext(display, config, NULL, attrib_list1);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	char str[31];
	sprintf(str, "%s %i %i", "Width&Height ", w,h);
	LOGZ(str);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;

	// Initialize GL state.
	// glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	// glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	return 0;
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int activeId = -1;

int32_t findIndex( const AInputEvent* event, int32_t id )
{
	int32_t count = AMotionEvent_getPointerCount( event );
	for( uint32_t i = 0; i < count; ++i )
	{
		LOGZ("index %i id %i", i, AMotionEvent_getPointerId( event, i ));
		if( id == AMotionEvent_getPointerId( event, i ) )
			return i;
	}
	return -1;
}

static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		switch(AInputEvent_getSource(event)){
			case AINPUT_SOURCE_TOUCHSCREEN:
				int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
				int pointerIndex = (AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
				switch(action){
					case AMOTION_EVENT_ACTION_DOWN: {
						activeId = AMotionEvent_getPointerId(event, 0);

						int index = findIndex(event, activeId);
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

static bool JXCoreInitialized = false;

void JXCoreInitialize() {
	if (!JXCoreInitialized) {
		JX_Initialize("some_data", NULL);
		JX_InitializeNewEngine();
		JXCoreInitialized = true;
		LOGZ("It is alive!");
	}
	else {
		LOGZ("JX init no MORE!");
	}
}

void JXStuff() {
	JX_DefineMainFile("2+2;");
	JX_StartEngine();

	JXValue tempValue, tempProperty;
	JX_Evaluate(
			"[2]",
			"processInput", &tempValue);

	JX_GetIndexedProperty(&tempValue, 0, &tempProperty);
	char* stringValue =  JX_GetString(&tempProperty);
	JX_Free(&tempProperty);
	JX_Free(&tempValue);

	JX_StopEngine();

	LOGZ("Value from array - %s", stringValue);
}

struct Vertex {
	GLfloat pos[2];
	GLubyte rgba[4];
};

const Vertex QUAD[4] = {
	// Square with diagonal < 2 so that it fits in a [-1 .. 1]^2 square
	// regardless of rotation.
	{{-0.7f, -0.7f}, {0x00, 0xFF, 0x00}},
	{{ 0.7f, -0.7f}, {0x00, 0x00, 0xFF}},
	{{-0.7f,  0.7f}, {0xFF, 0x00, 0x00}},
	{{ 0.7f,  0.7f}, {0xFF, 0xFF, 0xFF}},
};


bool checkGlError(const char* funcName) {
	GLint err = glGetError();
	if (err != GL_NO_ERROR) {
		LOGE("GL error after %s(): 0x%08x\n", funcName, err);
		return true;
	}
	return false;
}

GLuint createShader(GLenum shaderType, const char* src) {
	GLuint shader = glCreateShader(shaderType);
	if (!shader) {
		checkGlError("glCreateShader");
		return 0;
	}
	glShaderSource(shader, 1, &src, NULL);

	GLint compiled = GL_FALSE;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLogLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen > 0) {
			GLchar* infoLog = (GLchar*)malloc(infoLogLen);
			if (infoLog) {
				glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
				LOGE("Could not compile %s shader:\n%s\n",
						shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
						infoLog);
				free(infoLog);
			}
		}
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static const char VERTEX_SHADER[] =
	"#version 100\n"
	"attribute vec4 vPosition;\n"
	"void main() {\n"
	"  gl_Position = vPosition;\n"
	"}\n";

static const char FRAGMENT_SHADER[] =
	"#version 100\n"
	"precision mediump float;\n"
	"void main() {\n"
	"  gl_FragColor = vec4(0.2, 1.0, 0.0, 1.0);\n"
	"}\n";

GLuint createProgram(const char* vtxSrc, const char* fragSrc) {
	GLuint vtxShader = 0;
	GLuint fragShader = 0;
	GLuint program = 0;
	GLint linked = GL_FALSE;

	vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
	if (!vtxShader)
		goto exit;

	fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
	if (!fragShader)
		goto exit;

	program = glCreateProgram();
	if (!program) {
		checkGlError("glCreateProgram");
		goto exit;
	}
	glAttachShader(program, vtxShader);
	glAttachShader(program, fragShader);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		LOGE("Could not link program");
		GLint infoLogLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen) {
			GLchar* infoLog = (GLchar*)malloc(infoLogLen);
			if (infoLog) {
				glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
				LOGE("Could not link program:\n%s\n", infoLog);
				free(infoLog);
			}
		}
		glDeleteProgram(program);
		program = 0;
	}

exit:
	glDeleteShader(vtxShader);
	glDeleteShader(fragShader);
	return program;
}

	GLuint mProgram;
	GLuint gvPositionHandle;

	const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
	static float grey = 0.0f;

/**
 * Just the current frame in the display.
 */

static void engine_draw_frame(struct engine* engine) {
	if (engine->display == NULL) {
		LOGZ("ND");
		return;
	}

	// Just fill the screen with a color.
	// glClearColor(((float)engine->state.x)/engine->width, engine->state.angle,
	//         ((float)engine->state.y)/engine->height, 1);

	grey += 0.01f;
	if (grey > 1.0f) {
		grey = 0.0f;
	}
	glClearColor(grey, grey, grey, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(mProgram);

	glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
	glEnableVertexAttribArray(gvPositionHandle);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// glBindBuffer(GL_ARRAY_BUFFER, mVB);
	// glVertexAttribPointer(mPosAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, pos));
	// glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, rgba));
	// glEnableVertexAttribArray(mPosAttrib);
	// glEnableVertexAttribArray(mColorAttrib);

	// int numInstances = 12;
	// for (unsigned int i = 0; i < numInstances; i++) {
	//     glUniformMatrix2fv(mScaleRotUniform, 1, GL_FALSE, mScaleRot + 4*i);
	//     glUniform2fv(mOffsetUniform, 1, mOffsets + 2*i);
	//     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// }

	eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
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
				JXStuff();
				mProgram = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
				gvPositionHandle = glGetAttribLocation(mProgram, "vPosition");
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
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
	LOGZ("Meaning of life is - %i", meaningOfLifeIs());
	JXCoreInitialize();

	struct engine engine;

	// Make sure glue isn't stripped.
	app_dummy();

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