Use CYGWIN or some Unix-like distribution because shell-scripts and make-files are used.

Install latest Android NDK and JXCore sources.

For cross-compiling official JXCore documentation describes usage of NDK standalone toolchain. A self-sufficient copy of NDK subset.

Official solution.

Go to root folder of jxcore project and;
```bash
$> build_scripts/android-configure.sh ~/androidNDK/ optional-android-api-number
```

Multiple standalone toolchains will be created. One for each cpu architecture - ARM, x86, MIPS and etc.

If you want only one - you have to fix script by yourself. This is an idea behind script for ARM acrhitecture:

```bash
if [ $# -eq 0 ]
then
  echo "no argument provided."
  echo "usage: android_configure <ndk_path> <optional ndk version number>\n"
  exit
fi

ANDROID_TARGET=android-9
if [ $# -gt 1 ]
then
  ANDROID_TARGET="android-$2"
fi

export TOOLCHAIN=$PWD/android-toolchain-arm
rm -rf $TOOLCHAIN
mkdir -p $TOOLCHAIN
$1/build/tools/make-standalone-toolchain.sh \
    --toolchain=arm-linux-androidabi-4.9 \
    --arch=arm \
    --install-dir=$TOOLCHAIN \
    --platform=$ANDROID_TARGET
    
rm $TOOLCHAIN/bin/python

echo "Android tools are ready. Now call 'build_scripts/android_compile.sh'"
```

Don't rush pasting part of this script. Because python symlink has to be deleted too. And it is easier to understand in form of full script.

Official solution gives you choice of format and underlying engine. This project has no usage of standalone binary of jx so I will continue to static libraries.

You have choice of underlying JS engine - V8 or SpiderMonkey. It is achieved through using for V8:

```bash
$> build_scripts/android_compile_v8.sh ~/androidNDK/
```

Or for SpiderMonkey:

```bash
$> build_scripts/android_compile.sh ~/androidNDK/
```

If you have used my shorter version of standalone-chain-creation-script. Then you should make the same changes for compile script:

```bash
#!/bin/bash

# Copyright & License details are available under JXCORE_LICENSE file

NORMAL_COLOR='\033[0m'
RED_COLOR='\033[0;31m'
GREEN_COLOR='\033[0;32m'
GRAY_COLOR='\033[0;37m'

LOG() {
  COLOR="$1"
  TEXT="$2"
  echo -e "${COLOR}$TEXT ${NORMAL_COLOR}"
}


ERROR_ABORT() {
  if [[ $? != 0 ]]
  then
    LOG $RED_COLOR "compilation aborted\n"
    exit  
  fi
}


ERROR_ABORT_MOVE() {
  if [[ $? != 0 ]]
  then
    $($1)
    LOG $RED_COLOR "compilation aborted for $2 target\n"
    exit  
  fi
}


if [ $# -eq 2 ]
then
  CONF_EXTRAS=$2
fi

ARM7=out_arm_droid
FATBIN=out_android/android
    
MAKE_INSTALL() {
  TARGET_DIR="out_$1_droid"
  mv $TARGET_DIR out
  ./configure --static-library --dest-os=android --dest-cpu=$1 --engine-mozilla $CONF_EXTRAS
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  make -j 2
  ERROR_ABORT_MOVE "mv out $TARGET_DIR" $1
  
  PREFIX_DIR="out/Release"
  $STRIP -d $PREFIX_DIR/libcares.a
  mv $PREFIX_DIR/libcares.a "$PREFIX_DIR/libcares_$1.a"
  
  $STRIP -d $PREFIX_DIR/libchrome_zlib.a
  mv $PREFIX_DIR/libchrome_zlib.a "$PREFIX_DIR/libchrome_zlib_$1.a"
  
  $STRIP -d $PREFIX_DIR/libhttp_parser.a
  mv $PREFIX_DIR/libhttp_parser.a "$PREFIX_DIR/libhttp_parser_$1.a"
  
  $STRIP -d $PREFIX_DIR/libjx.a
  mv $PREFIX_DIR/libjx.a "$PREFIX_DIR/libjx_$1.a"
  
  $STRIP -d $PREFIX_DIR/libmozjs.a
  mv $PREFIX_DIR/libmozjs.a "$PREFIX_DIR/libmozjs_$1.a"
  
  $STRIP -d $PREFIX_DIR/libopenssl.a
  mv $PREFIX_DIR/libopenssl.a "$PREFIX_DIR/libopenssl_$1.a"
  
  $STRIP -d $PREFIX_DIR/libuv.a
  mv $PREFIX_DIR/libuv.a "$PREFIX_DIR/libuv_$1.a"
  
  $STRIP -d $PREFIX_DIR/libsqlite3.a
  mv $PREFIX_DIR/libsqlite3.a "$PREFIX_DIR/libsqlite3_$1.a"
  
if [ "$CONF_EXTRAS" == "--embed-leveldown" ]
then
  $STRIP -d $PREFIX_DIR/libleveldown.a
  mv $PREFIX_DIR/libleveldown.a "$PREFIX_DIR/libleveldown_$1.a"
  
  $STRIP -d $PREFIX_DIR/libsnappy.a
  mv $PREFIX_DIR/libsnappy.a "$PREFIX_DIR/libsnappy_$1.a"
  
  $STRIP -d $PREFIX_DIR/libleveldb.a
  mv $PREFIX_DIR/libleveldb.a "$PREFIX_DIR/libleveldb_$1.a"
fi
  
  mv out $TARGET_DIR
}

COMBINE() {
if [ $MIPS != 0 ]
  mv "$ARM7/Release/$1_arm.a" "$FATBIN/bin/"
  ERROR_ABORT
}

mkdir out_arm_droid
mkdir out_android

rm -rf out

OLD_PATH=$PATH

export TOOLCHAIN=$PWD/android-toolchain-arm
export PATH=$TOOLCHAIN/bin:$OLD_PATH
export AR=arm-linux-androideabi-ar
export CC=arm-linux-androideabi-gcc
export CXX=arm-linux-androideabi-g++
export LINK=arm-linux-androideabi-g++
export STRIP=arm-linux-androideabi-strip

LOG $GREEN_COLOR "Compiling Android ARM7\n"
MAKE_INSTALL arm

LOG $GREEN_COLOR "Preparing FAT binaries\n"
rm -rf $FATBIN
mkdir -p $FATBIN/bin

COMBINE "libcares"
COMBINE "libchrome_zlib"
COMBINE "libhttp_parser"
COMBINE "libjx"
COMBINE "libmozjs"
COMBINE "libopenssl"
COMBINE "libuv"
COMBINE "libsqlite3"

if [ "$CONF_EXTRAS" == "--embed-leveldown" ]
then
  COMBINE "libleveldown"
  COMBINE "libsnappy"
  COMBINE "libleveldb"
fi

cp src/public/*.h $FATBIN/bin

LOG $GREEN_COLOR "JXcore Android binaries are ready under $FATBIN\n"
```

As a minor bonus this script does not need useless path to NDK folder. Results can be found here:

```bash
cd out_android/android/bin
```

This folder would contain *.a files for corresponding parts of JXCore library for chosen architectures.

If you used scripts I have given to you. Please remove _arm prefixes from files. Result should be this:

```bash
jx.h
jx_result.h
libcares.a
libchrome_zlib.a
libhttp_parser.a
libjx.a
libmozjs.a
libopenssl.a
libuv.a
libsqlite3.a
```

For facilitation of build process I decided to make a shared library from this static pile.

Idea behind script:

```bash
android-toolchain-arm/bin/arm-linux-androideabi-g++ -o libjxcore.so -Lout_android/android/bin -Wl,--whole-archive -ljx  -Wl,--no-whole-archive -llog -lcares -lsqlite3 -luv -lmozjs  -lopenssl -lhttp_parser -lchrome_zlib -lgnustl_shared
```

It uses scecial compiler directive --whole-archive. It requires shared libraries log and gnustl_shared from NDK (and I just copied them to bin-folder to make thing easier).

If everything worked right then you should get an error like undefined reference to 'main'. This is correct because we should add -shared option for building complete library. But it may hide some compile problems from you like undefined references to libraries which later will result in app crashes.

Final script:

```bash
android-toolchain-arm/bin/arm-linux-androideabi-g++ -shared -o libjxcore.so -Lout_android/android/bin -Wl,--whole-archive -ljx  -Wl,--no-whole-archive -llog -lcares -lsqlite3 -luv -lmozjs  -lopenssl -lhttp_parser -lchrome_zlib -lgnustl_shared
```

Now you have a libjxcore.so compile with g++-4.9 for ARM architecture using ANDROID-19 level API. Before using it you should load gnustl_shared library and don't forget to use g++-4.9 for compiling all the connected c++ code.

Cheers.
