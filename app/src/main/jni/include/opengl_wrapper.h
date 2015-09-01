#include <EGL/egl.h>

namespace opengl_wrapper {

	uint createProgram(char*, char*);

	uint createProgramBasic();

	void swapBuffers(EGLDisplay, EGLSurface);

	int init(ANativeWindow*, EGLDisplay&, EGLContext&, EGLSurface&);

	void destroy(EGLDisplay&, EGLContext&, EGLSurface&);

	void initProgram();

	void render(float);

}