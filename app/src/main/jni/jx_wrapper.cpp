#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "jx_wrapper", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "jx_wrapper", __VA_ARGS__))

#include <jx.h>
#include <jx_result.h>

namespace jx_wrapper {

	void* assetManager;
	int (*readFile)(void*, const char*, char**);

	void setReadFileParts(void* assetManagerInstance, int (*readFileFunction)(void*, const char*, char**)) {
		assetManager = assetManagerInstance;
		readFile = readFileFunction;
	}

	void assetReadSync(JXValue *results, int argc) {
		const char *filename = JX_GetString(&results[0]);

		if (readFile != NULL || assetManager != NULL) {
			char* data;
			int read_len = readFile(assetManager, filename, &data);
			if (read_len >= 0) {
				JX_SetBuffer(&results[argc], data, read_len);
				return;
			}
		}

		const char *err = "File doesn't exist";
		JX_SetError(&results[argc], err, strlen(err));
	}


	void (*cacheTextureCallback)(char*, char*);

	void cacheTexture(JXValue *results, int argc) {
		char* lable = (char*) JX_GetString(&results[0]);
		char* path = (char*) JX_GetString(&results[1]);

		cacheTextureCallback(lable, path);
	}

	void setCacheTextureCallback(void (*callback)(char*, char*)) {
		cacheTextureCallback = callback;

		JX_DefineExtension("cacheTexture", cacheTexture);
	}


	static bool JXCoreInitialized = false;

	void initForCurrentThread(char* startScript) {
		JX_InitializeNewEngine();
		JX_DefineExtension("assetReadSync", assetReadSync);
		JX_DefineMainFile(startScript);
		JX_StartEngine();
	}

	int init() {
		if (!JXCoreInitialized) {
			JX_Initialize("some_data", NULL);
			JXCoreInitialized = true;

			initForCurrentThread((char *) "global.require = require;");
		}

		return 0;
	}

	void evaluate(char* script) {
		JXValue tempValue;
		JX_Evaluate(script,
				"evaluateScript", &tempValue);
		JX_Free(&tempValue);
	}

	void test() {
		JXValue tempValue, tempProperty;
		JX_Evaluate(
				"console.log(\"Trying to load module...\");"
				"var value = 0;"
				"try {value = global.require('/meaningOfLife.js');} catch(e){value = \"error\"+e.toString();}"
				"console.log(\"It\'s value is \"+value);"
				"[value];",
				"processInput", &tempValue);

		JX_GetIndexedProperty(&tempValue, 0, &tempProperty);
		char* stringValue =  JX_GetString(&tempProperty);

		JX_Free(&tempProperty);
		JX_Free(&tempValue);

		JX_Loop();

		LOGI("Is it SpiderMonkey? %s!!!", JX_IsSpiderMonkey() ? "Yes" : "No");
		LOGI("Is it V8? %s!!!", JX_IsV8() ? "Yes" : "No");

		JX_ForceGC();
	}

	void destroy() {
		JX_StopEngine();
	}

}