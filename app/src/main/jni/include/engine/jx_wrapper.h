namespace jx_wrapper {

	void setReadFileParts(void*, int (*)(void*, const char*, char**));

	void setCacheTextureCallback(void (*)(char*, char*));

	int init();

	void initForCurrentThread(char*);

	void evaluate(char*);

	void test();

	void destroy();

}