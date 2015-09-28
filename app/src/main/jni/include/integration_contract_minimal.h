#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is the interface for the standard glue code of a threaded
 * application.  In this model, the application's code is running
 * in its own thread separate from the main thread of the process.
 * It is not required that this thread be associated with the Java
 * VM, although it will need to be in order to make JNI calls any
 * Java objects.
 */
struct global_struct;

/**
 * This is the function that application code must implement, representing
 * the main entry to the app.
 */
void android_main(global_struct*, char*);

#ifdef __cplusplus
}
#endif