#include <integration_contract_minimal.h>
#include <engine.hpp>

/**
 * This is the main entry point of a native application that is using
 * android_threaded_adapter. It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(global_struct* global, char* startScript) {
	chickpea::init(global, startScript);

	while (true) {
		chickpea::processInput(global);

		if (chickpea::shouldExit(global)) {
			break;
		}

		chickpea::engine_draw_frame(global);
	}

	chickpea::destroy(global);
}