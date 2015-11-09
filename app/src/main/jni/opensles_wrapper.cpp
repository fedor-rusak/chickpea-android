#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <jni.h>
#include <string.h>
#include <android/native_activity.h>
#include <assert.h>

namespace opensles_wrapper {
	static SLObjectItf engineObject = NULL;
	static SLEngineItf engineEngine;

	static SLObjectItf outputMixObject = NULL;

	static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

	static const SLEnvironmentalReverbSettings reverbSettings =
	    SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

	// create the engine and output mix objects
	void createEngine() {
	    SLresult result;

	    // create engine
	    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // realize the engine
	    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the engine interface, which is needed in order to create other objects
	    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // create output mix, with environmental reverb specified as a non-required interface
	    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
	    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
	    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // realize the output mix
	    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the environmental reverb interface
	    // this could fail if the environmental reverb effect is not available,
	    // either because the feature is not present, excessive CPU load, or
	    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
	    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
	            &outputMixEnvironmentalReverb);
	    if (SL_RESULT_SUCCESS == result) {
	        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
	                outputMixEnvironmentalReverb, &reverbSettings);
	        (void)result;
	    }
	    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example
	}

	// pointer and size of the next player buffer to enqueue, and number of remaining buffers
	static short *nextBuffer;
	static unsigned nextSize;
	static int nextCount;

	static SLObjectItf bqPlayerObject = NULL;

	static SLPlayItf bqPlayerPlay;

	static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

	// this callback handler is called every time a buffer finishes playing
	void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
	{
	    assert(bq == bqPlayerBufferQueue);
	    assert(NULL == context);
	    // for streaming playback, replace this test by logic to find and fill the next buffer
	    if (--nextCount > 0 && NULL != nextBuffer && 0 != nextSize) {
	        SLresult result;
	        // enqueue another buffer
	        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
	        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
	        // which for this code example would indicate a programming error
	        assert(SL_RESULT_SUCCESS == result);
	        (void)result;
	    }
	}

	static SLEffectSendItf bqPlayerEffectSend;

	static SLVolumeItf bqPlayerVolume;

	void createBufferQueueAudioPlayer() {
	    SLresult result;

	    // configure audio source
	    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
	        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
	        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
	    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

	    // configure audio sink
	    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
	    SLDataSink audioSnk = {&loc_outmix, NULL};

	    // create audio player
	    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
	            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
	    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
	            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
	    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
	            3, ids, req);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // realize the player
	    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the play interface
	    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the buffer queue interface
	    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
	            &bqPlayerBufferQueue);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // register callback on the buffer queue
	    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the effect send interface
	    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
	            &bqPlayerEffectSend);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
	    // get the mute/solo interface
	    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;
	#endif

	    // get the volume interface
	    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // set the player's state to playing
	    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;
	}

	static SLObjectItf fdPlayerObject = NULL;
	static SLPlayItf fdPlayerPlay;

	static SLSeekItf fdPlayerSeek;
	static SLMuteSoloItf fdPlayerMuteSolo;
	static SLVolumeItf fdPlayerVolume;

	// create asset audio player
	bool createAssetAudioPlayer(void* mgr, const char* utf8, char* tag) {
	    SLresult result;

	    assert(NULL != mgr);
	    AAsset* asset = AAssetManager_open((AAssetManager*) mgr, utf8, AASSET_MODE_UNKNOWN);


	    // the asset might not be found
	    if (NULL == asset) {
	        return JNI_FALSE;
	    }

	    // open asset as file descriptor
	    off_t start, length;
	    int fd = AAsset_openFileDescriptor(asset, &start, &length);
	    assert(0 <= fd);
	    AAsset_close(asset);

	    // configure audio source
	    SLDataLocator_AndroidFD loc_fd = {SL_DATALOCATOR_ANDROIDFD, fd, start, length};
	    SLDataFormat_MIME format_mime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
	    SLDataSource audioSrc = {&loc_fd, &format_mime};

	    // configure audio sink
	    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
	    SLDataSink audioSnk = {&loc_outmix, NULL};

	    // create audio player
	    const SLInterfaceID ids[3] = {SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME};
	    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
	    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &fdPlayerObject, &audioSrc, &audioSnk,
	            3, ids, req);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // realize the player
	    result = (*fdPlayerObject)->Realize(fdPlayerObject, SL_BOOLEAN_FALSE);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the play interface
	    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_PLAY, &fdPlayerPlay);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the seek interface
	    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_SEEK, &fdPlayerSeek);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the mute/solo interface
	    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_MUTESOLO, &fdPlayerMuteSolo);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the volume interface
	    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_VOLUME, &fdPlayerVolume);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // enable whole file looping
	    result = (*fdPlayerSeek)->SetLoop(fdPlayerSeek, SL_BOOLEAN_TRUE, 0, SL_TIME_UNKNOWN);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    return JNI_TRUE;
	}

	static SLObjectItf fd2PlayerObject = NULL;
	static SLPlayItf fd2PlayerPlay;

	static SLSeekItf fd2PlayerSeek;
	static SLMuteSoloItf fd2PlayerMuteSolo;
	static SLVolumeItf fd2PlayerVolume;

	// create asset audio player
	bool createAssetAudioPlayer2(void* mgr, const char* utf8, char* tag) {
	    SLresult result;

	    assert(NULL != mgr);
	    AAsset* asset = AAssetManager_open((AAssetManager*) mgr, utf8, AASSET_MODE_UNKNOWN);


	    // the asset might not be found
	    if (NULL == asset) {
	        return JNI_FALSE;
	    }

	    // open asset as file descriptor
	    off_t start, length;
	    int fd = AAsset_openFileDescriptor(asset, &start, &length);
	    assert(0 <= fd);
	    AAsset_close(asset);

	    // configure audio source
	    SLDataLocator_AndroidFD loc_fd = {SL_DATALOCATOR_ANDROIDFD, fd, start, length};
	    SLDataFormat_MIME format_mime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
	    SLDataSource audioSrc = {&loc_fd, &format_mime};

	    // configure audio sink
	    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
	    SLDataSink audioSnk = {&loc_outmix, NULL};

	    // create audio player
	    const SLInterfaceID ids[3] = {SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME};
	    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
	    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &fd2PlayerObject, &audioSrc, &audioSnk,
	            3, ids, req);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // realize the player
	    result = (*fdPlayerObject)->Realize(fd2PlayerObject, SL_BOOLEAN_FALSE);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the play interface
	    result = (*fdPlayerObject)->GetInterface(fd2PlayerObject, SL_IID_PLAY, &fd2PlayerPlay);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the seek interface
	    result = (*fdPlayerObject)->GetInterface(fd2PlayerObject, SL_IID_SEEK, &fd2PlayerSeek);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the mute/solo interface
	    result = (*fdPlayerObject)->GetInterface(fd2PlayerObject, SL_IID_MUTESOLO, &fd2PlayerMuteSolo);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // get the volume interface
	    result = (*fdPlayerObject)->GetInterface(fd2PlayerObject, SL_IID_VOLUME, &fd2PlayerVolume);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    // enable whole file looping
	    result = (*fdPlayerSeek)->SetLoop(fd2PlayerSeek, SL_BOOLEAN_FALSE, 0, SL_TIME_UNKNOWN);
	    assert(SL_RESULT_SUCCESS == result);
	    (void)result;

	    return JNI_TRUE;
	}


	static const char hello[] =
	#include "hello_clip.h"
	;


	// select the desired clip and play count, and enqueue the first buffer if idle
	jboolean selectClip() {
	    nextBuffer = (short *) hello;
	    nextSize = sizeof(hello);
	    nextCount = 3;
	    if (nextSize > 0) {
	        // here we only enqueue one buffer because it is a long clip,
	        // but for streaming playback we would typically enqueue at least 2 buffers to start
	        SLresult result;
	        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
	        if (SL_RESULT_SUCCESS != result) {
	            return JNI_FALSE;
	        }
	    }

	    return JNI_TRUE;
	}

	void setPlayingAssetAudioPlayer(bool isPlaying) {
	    SLresult result;

	    // make sure the asset audio player was created
	    if (NULL != fdPlayerPlay) {

	        // set the player's state
	        result = (*fdPlayerPlay)->SetPlayState(fdPlayerPlay, isPlaying ?
	            SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
	        assert(SL_RESULT_SUCCESS == result);
	        (void)result;
	    }
	}


	void setPlayingAssetAudioPlayer2(bool isPlaying) {
	    SLresult result;

	    // make sure the asset audio player was created
	    if (NULL != fdPlayerPlay) {
	    	
	        // set the player's state
	        result = (*fdPlayerPlay)->SetPlayState(fd2PlayerPlay, isPlaying ?
	            SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
	        assert(SL_RESULT_SUCCESS == result);
	        (void)result;
	    }
	}


	void shutdown() {

	    // destroy buffer queue audio player object, and invalidate all associated interfaces
	    if (bqPlayerObject != NULL) {
	        (*bqPlayerObject)->Destroy(bqPlayerObject);
	        bqPlayerObject = NULL;
	        bqPlayerPlay = NULL;
	        bqPlayerBufferQueue = NULL;
	        bqPlayerEffectSend = NULL;
	        bqPlayerVolume = NULL;
	    }

	    // destroy file descriptor audio player object, and invalidate all associated interfaces
	    if (fdPlayerObject != NULL) {
	        (*fdPlayerObject)->Destroy(fdPlayerObject);
	        fdPlayerObject = NULL;
	        fdPlayerPlay = NULL;
	        fdPlayerSeek = NULL;
	        fdPlayerMuteSolo = NULL;
	        fdPlayerVolume = NULL;
	    }

	     // destroy output mix object, and invalidate all associated interfaces
	    if (outputMixObject != NULL) {
	        (*outputMixObject)->Destroy(outputMixObject);
	        outputMixObject = NULL;
	        outputMixEnvironmentalReverb = NULL;
	    }

	    // destroy engine object, and invalidate all associated interfaces
	    if (engineObject != NULL) {
	        (*engineObject)->Destroy(engineObject);
	        engineObject = NULL;
	        engineEngine = NULL;
	    }

	}

}