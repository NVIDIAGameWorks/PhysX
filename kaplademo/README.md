# NVIDIA PhysX Kapla Demo

## Build process

1. Build the PhysX SDK in the required config (e.g. vc14win64/vc15win64 debug/profile/checked/release)
2. Build corresponding build config of the demo (./source/compiler/vc15win64-PhysX_4.1/KaplaDemo.sln). The post-build step will copy across the required DLLs from the PhysX build.
3. Run the demo

Per-demo help screens can be displayed by pressing F1. Possible command line args are displayed on the console.

The demo requires a minimum spec of a Kepler class NVIDIA GPU although best performance is achieved with a Maxwell or Pascal GPU, e.g. GTX970 or above.

## Known Issues

The Win32 build may intermittently boot to a black screen. If this occurs, exit the demo and try again. This is caused by the 32-bit GL library failing to allocate a large buffer. This issue does not occur on 64-bit builds.

## Acknowledgements

This depot contains external third party open source software, copyright belonging to their respective owners:
* OpenGL Extension Wrangler Library
* glext
* glut
* FrameBufferObject by Aaron Lefohn, Robert Strzodka, and Adam Moerschell
