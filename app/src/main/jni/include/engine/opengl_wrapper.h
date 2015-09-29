#include <EGL/egl.h>

namespace opengl_wrapper {

	uint createProgram(char*, char*);

	uint createProgramBasic();

	void swapBuffers();

	int init(ANativeWindow*);

	void destroy();

	void initProgram();

	void render(float);

}