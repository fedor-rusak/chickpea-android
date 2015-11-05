namespace jx_wrapper {

	void setReadFileParts(void*, int (*)(void*, const char*, char**));

	void setCacheTextureCallback(void (*)(char*, char*));

	void setRenderCallback(void (*)(char*, float, float, float));

	void setSetCameraCallback(void (*)(float, float, float));

	void setGetScreenDimensionsCallback(void (*)(int*, int*));

	void setClearScreenCallback(void (*)(float, float, float));

	void setUnprojectCallback(void (*callback)(int, int, float*, float*));

	int init();

	void initForCurrentThread(char*);

	void evaluate(char*);

	void test();

	void destroy();

}