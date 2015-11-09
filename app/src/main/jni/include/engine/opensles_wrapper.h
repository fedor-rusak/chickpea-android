#include <android/native_activity.h>

namespace opensles_wrapper {
	void createEngine();
	void createBufferQueueAudioPlayer();
	void createAssetAudioPlayer(void*, const char*, char*);
	void createAssetAudioPlayer2(void*, const char*, char*);
	void setPlayingAssetAudioPlayer(bool);
	void setPlayingAssetAudioPlayer2(bool);
	void selectClip();

	void shutdown();
}