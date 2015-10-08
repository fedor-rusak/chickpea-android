#include <EGL/egl.h>
#include <integration_contract_minimal.h>

namespace opengl_wrapper {

	uint createProgram(char*, char*);

	uint createProgramBasic();

	void cacheTexture(char*, char*);

	void swapBuffers();

	int init(global_struct*);

	void destroy();

	void initProgram();

	void render(float);

}