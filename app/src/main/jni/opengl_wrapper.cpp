#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

namespace opengl_wrapper {

	static char VERTEX_SHADER[] =
		"#version 100\n"
		"attribute vec4 vPosition;\n"
		"void main() {\n"
		"  gl_Position = vPosition;\n"
		"}\n";

	static char FRAGMENT_SHADER[] =
		"#version 100\n"
		"precision mediump float;\n"
		"void main() {\n"
		"  gl_FragColor = vec4(0.2, 1.0, 0.0, 1.0);\n"
		"}\n";

	char* getVertexShaderText() {
		return VERTEX_SHADER;
	}

	char* getFragmentShaderText() {
		return FRAGMENT_SHADER;
	}

	bool checkGlError(const char* funcName) {
		GLint err = glGetError();
		if (err != GL_NO_ERROR) {
			// LOGE("GL error after %s(): 0x%08x\n", funcName, err);
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
					// LOGE("Could not compile %s shader:\n%s\n",
							// shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
							// infoLog);
					free(infoLog);
				}
			}
			glDeleteShader(shader);
			return 0;
		}

		return shader;
	}

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
			// LOGE("Could not link program");
			GLint infoLogLen = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
			if (infoLogLen) {
				GLchar* infoLog = (GLchar*)malloc(infoLogLen);
				if (infoLog) {
					glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
					// LOGE("Could not link program:\n%s\n", infoLog);
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

	GLuint createProgramBasic() {
		return createProgram(getVertexShaderText(), getFragmentShaderText());
	}


	static EGLDisplay display;
	static EGLSurface surface;
	static EGLContext context;

	int init(ANativeWindow* window) {
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

		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

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

		ANativeWindow_setBuffersGeometry(window, 0, 0, format);

		surface = eglCreateWindowSurface(display, config, window, NULL);
		int* attrib_list1 = new int[3];
		attrib_list1[0]=EGL_CONTEXT_CLIENT_VERSION;
		attrib_list1[1]=2;
		attrib_list1[2]=EGL_NONE;
		context = eglCreateContext(display, config, NULL, attrib_list1);

		if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
			// LOGW("Unable to eglMakeCurrent");
			return -1;
		}

		eglQuerySurface(display, surface, EGL_WIDTH, &w);
		eglQuerySurface(display, surface, EGL_HEIGHT, &h);

		// char str[31];
		// sprintf(str, "%s %i %i", "Width&Height ", w,h);
		// LOGZ(str);

		// Initialize GL state.
		// glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		glEnable(GL_CULL_FACE);
		// glShadeModel(GL_SMOOTH);
		glDisable(GL_DEPTH_TEST);

		return 0;
	}


	void destroy() {
		if (display != EGL_NO_DISPLAY) {
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if (context != EGL_NO_CONTEXT) {
				eglDestroyContext(display, context);
			}
			if (surface != EGL_NO_SURFACE) {
				eglDestroySurface(display, surface);
			}
			eglTerminate(display);
		}

		display = EGL_NO_DISPLAY;
		context = EGL_NO_CONTEXT;
		surface = EGL_NO_SURFACE;
	}


	static GLuint mProgram;
	static GLuint gvPositionHandle;

	const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };

	void initProgram() {
		mProgram = createProgramBasic();
		gvPositionHandle = glGetAttribLocation(mProgram, "vPosition");
	}

	void render(float color) {
		glClearColor(1.0-color, 1.0-color, color, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		glUseProgram(mProgram);

		glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
		glEnableVertexAttribArray(gvPositionHandle);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	void swapBuffers() {
		eglSwapBuffers(display, surface);
	}

}