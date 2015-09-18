#include <poll.h>
#include <pthread.h>
#include <sched.h>

#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>

#ifdef __cplusplus
extern "C" {
#endif

struct io_struct {

	int msgread;
	int msgwrite;

	ALooper* looper;

	// When non-NULL, this is the input queue from which the app will
	// receive user input events.
	AInputQueue* inputQueue;
	AInputQueue* pendingInputQueue;

};

struct native_struct {

	// The ANativeActivity object instance that this app is running in.
	ANativeActivity* activity;

	// The current configuration the app is running in.
	AConfiguration* config;

	// When non-NULL, this is the window surface that the app can draw in.
	ANativeWindow* window;

	ANativeWindow* pendingWindow;

};

struct flags_struct {

	// Current state of the app's activity.  May be either APP_CMD_START,
	// APP_CMD_RESUME, APP_CMD_PAUSE, or APP_CMD_STOP; see below.
	int activityState;

	// This is non-zero when the application's NativeActivity is being
	// destroyed and waiting for the app thread to complete.
	int destroyRequested;

	int stateSaved;
	int destroyed;

};

struct appdata_struct {

	// The application can place a pointer to its own state object
	// here if it likes.
	void* internal;


	// This is the last instance's saved state, as provided at creation time.
	// It is NULL if there was no state.  You can use this as you need; the
	// memory will remain around until you call android_app_exec_cmd() for
	// APP_CMD_RESUME, at which point it will be freed and savedState set to NULL.
	// These variables should only be changed when processing a APP_CMD_SAVE_STATE,
	// at which point they will be initialized to NULL and you can malloc your
	// state and place the information here.  In that case the memory will be
	// freed for you later.
	void* savedState;
	size_t savedStateSize;

};

struct threading_struct {

	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_t instance;

};

/**
 * This is the interface for the standard glue code of a threaded
 * application.  In this model, the application's code is running
 * in its own thread separate from the main thread of the process.
 * It is not required that this thread be associated with the Java
 * VM, although it will need to be in order to make JNI calls any
 * Java objects.
 */
struct global_struct {

	io_struct io_stuff;

	native_struct native_stuff;

	flags_struct flags;

	appdata_struct appdata;

	threading_struct threading;

};

/**
 * Dummy function you can call to ensure glue code isn't stripped.
 */
void function_to_prevent_stripping();


/* These must be used by application for processing events */

void process_cmd(global_struct*, void (*)(global_struct*, int32_t));

void process_input(global_struct*, int32_t (*)(global_struct*, AInputEvent*));


/**
 * This is the function that application code must implement, representing
 * the main entry to the app.
 */
namespace engine_ns {
	void android_main(global_struct*, char*);
}

#ifdef __cplusplus
}
#endif