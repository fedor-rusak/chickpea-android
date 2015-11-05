#include <EGL/egl.h>
#include <integration_contract_minimal.h>

namespace opengl_wrapper {

	uint createProgram(char*, char*);

	uint createProgramBasic();

	void cacheTexture(char*, char*);

	void unprojectOnZeroLevel(int, int, float*, float*);

	void render(char*, float, float, float);

	void clearScreen(float, float, float);

	void setCamera(float, float, float);

	void getScreenDimensions(int*, int*);

	void swapBuffers();

	int init(global_struct*);

	void destroy();

	void initProgram();

}