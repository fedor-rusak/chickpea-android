#include <android/log.h>

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

		__android_log_print(ANDROID_LOG_INFO, "jxcore-log", "Is it SpiderMonkey? %s!!!", JX_IsSpiderMonkey() ? "Yes" : "No");
		__android_log_print(ANDROID_LOG_INFO, "jxcore-log", "Is it V8? %s!!!", JX_IsV8() ? "Yes" : "No");

		JX_ForceGC();
	}

	void destroy() {
		JX_StopEngine();
	}

}