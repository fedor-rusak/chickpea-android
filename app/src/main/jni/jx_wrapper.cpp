#include <jx.h>
#include <jx_result.h>

namespace jx_wrapper {

	static bool JXCoreInitialized = false;

	int init() {
		if (!JXCoreInitialized) {
			JX_Initialize("some_data", NULL);
			JXCoreInitialized = true;

			JX_InitializeNewEngine();
			JX_StartEngine();
		}

		return 0;
	}

	void test() {
		JX_DefineMainFile("2+2;");

		JXValue tempValue, tempProperty;
		JX_Evaluate(
				"[2]",
				"processInput", &tempValue);

		JX_GetIndexedProperty(&tempValue, 0, &tempProperty);
		char* stringValue =  JX_GetString(&tempProperty);
		JX_Free(&tempProperty);
		JX_Free(&tempValue);
	}

	void destroy() {
		JX_StopEngine();
	}
}