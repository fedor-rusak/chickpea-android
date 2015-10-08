#include <android/native_activity.h>

namespace opensles_wrapper {
	void createEngine();
	void createBufferQueueAudioPlayer();
	void createAssetAudioPlayer(void*, const char*);
	void setPlayingAssetAudioPlayer(bool);
	void selectClip();

	void shutdown();
}