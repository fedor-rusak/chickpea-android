armv7a-19-g++ -shared -Isrc/main/jni/include -Lsrc/main/jniLibs/armeabi-v7a src/main/jni/opengl_wrapper.cpp -lGLESv3 -lEGL -landroid -lstlport_shared -llog -o src/main/jniLibs/armeabi-v7a/libopengl-wrapper.so