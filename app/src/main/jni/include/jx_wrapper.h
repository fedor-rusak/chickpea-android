namespace jx_wrapper {

	void setReadFileParts(void*, int (*)(void*, const char*, char**));

	int init();

	void initForCurrentThread(char*);

	void test();

	void destroy();

}