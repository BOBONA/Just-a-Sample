# Building 
I've run into a lot of frustration while trying to build on different platforms with the necessary dependencies.
Here's a list of things to watch out for. This is not meant to be a guide for beginners, just a reference.

On all platforms, check that the paths in the Projucer project are set correctly for your system. This includes:
- Global Paths (Path to JUCE, JUCE Modules, User Modules)
- Exporter Paths (External Libraries to Link)
- Debug/Release paths (Header Search Paths)

## All Platforms

Notes:
- The VST Reaper integration can cause an extraneous `jassert` that will be triggered in Debug notes.
- When building Bungee set `maxPitchOctaves = 4` in `Timing.cpp`.

## Windows


### Bungee

Build for release:
```shell
cmake .. -DBUNGEE_BUILD_SHARED_LIBRARY=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
cmake --build . --config Release
```

Build for debug:
```shell
cmake .. -DBUNGEE_BUILD_SHARED_LIBRARY=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug
cmake --build . --config Debug
```

These may show `fatal error LNK1149`, but this is not an issue as long as `bungee.lib` is created.

- `fftpack.c` may require `#define M_PI 3.14159265358979323846` at the top of the file to compile.
- `Assert.cpp` is not necessary for building, however `#include <unistd.h>` only works on Unix, so you may need to remove it.
- `Resample.h` uses `__attribute__((noinline))`, which is not supported by MSVC. You can replace it with `__declspec(noinline)`.

## Linux

### Bungee

Build for release:
```shell
cmake -DBUNGEE_BUILD_SHARED_LIBRARY=OFF _-DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_C_FLAGS "-fPIC" ..
cmake --build . --config Release
```
Build for debug:
```shell
cmake -DBUNGEE_BUILD_SHARED_LIBRARY=OFF -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_C_FLAGS "-fPIC" ..
cmake --build . --config Debug
```

### JAS
```shell
make CONFIG=Release
```