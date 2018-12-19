NVIDIA Cg 2.2 February 2010 README  Copyright (C) 2002-2010 NVIDIA Corp.
==================================

This distribution contains
--------------------------

- NVIDIA Cg toolkit documentation
  in the docs directory

- NVIDIA Cg compiler (cgc)
  in the bin directory

- NVIDIA Cg runtime libraries
  in the lib directory

- Example Cg applications
  in the examples directory

- Under Microsoft Windows, a Cg language syntax highlighter
  for Microsoft Visual Studio is provided in the
  msdev_syntax_highlighting directory

- Under Microsoft Windows, if selected at install time, 64-bit
  binaries and libraries are in the bin.x64 and lib.x64 directories.

See the release notes (docs/CgReleaseNotes.pdf) for detailed
information about this release.

The Cg toolkit is available for a number of different hardware and
OS platforms.  As of this writing, supported platforms include:

  - Microsoft NT 4, 2000, and Windows XP & Vista on IA32/x86/x86-64 (Intel, AMD)
  - Linux on IA32/x86 (Intel, AMD)
  - Linux for x64 (AMD64 and EMT64)
  - MacOS X 10.4 and 10.5 (Tiger and Leopard)
  - Solaris (x86/x86_64)

Visit the NVIDIA Cg website at http://developer.nvidia.com/page/cg_main.html
for updates and complete compatibility information.

Changes since Cg 2.2 October 2009
---------------------------------
- Bug fixes
  - Require EXT_gpu_shader4 in GLSL when using bit shift/mask instructions
  - Modified example gs_simple to explicitly use the GLSL profiles if supported
  - HLSL semantic VFACE is now accepted as an alias for semantic FACE
  - Improved our handling of extensions on older versions of OpenGL
  - Various performance improvements
  - Enhanced cgfxcat to work for program files as well as effect files
  - Fixed some compiler crashes with malformed shaders
  - Fixed a crash in cgGetParameterBufferIndex and cgGetParameterBufferOffset
  - Fixed a bug in cgGetPassProgram for combined programs
  - Fixed a problem with geometry shaders on Solaris

Changes since Cg 2.2 April 2009
-------------------------------
- New features
  - Allow compiler options in effect compile statements
      - e.g. VertexProgram = compile vp40 "-po PosInv=1" shader();
  - Better performance when running on multicore CPUs
  - Choosing the "latest" profile is now deferred until effect validation
  - Now using MesaGLUT-7.5 for GLUT on Windows
- Bug fixes
  - Improved the inverse matrix computation in cgGLSetStateMatrixParameter
  - Better memory behavior when a program is repeatedly recompiled
  - Fixed an issue when using PSIZE semantic with ps_3_0 and ps_4_0 profiles
  - cgCombinePrograms now works with CG_OBJECT programs
  - cgGetNextProgram was always returning 0
  - Fixed a problem with effect parameters and cgGLGetTextureEnum
  - Allow comments prior to the shader version in D3D asm blocks of an effect
  - HLSL10: Mark globally scoped temporaries 'static' to keep them out of constant buffers
  - HLSL10: Allow any semantic for varyings provided the semantics match between stages
  - HLSL10: Fix handling of TEXUNITn semantic
  - HLSL10: Added support for arrays of samplers
  - HLSL10: Empty structs for uniform parameters crashed the D3D compiler
  - Fixed a problem when connecting parameters of type string
  - Corrected issues in the fp20 profile on Solaris

Changes since Cg 2.2 beta February 2009
---------------------------------------
- New features
  - Support for pack_matrix() pragma
  - Arrays of shaders can now be used in CgFX files
  - Support for 64-bit Solaris
  - Bug fixes (see release notes for details)

Changes since Cg 2.1 November 2008
----------------------------------
- New features
  - DirectX10 and GLSL geometry profiles (gs_4_0 AND glslg)
  - Support for "latest" profile keyword in CgFX compile statements
  - Additional API routines (see release notes for a complete list)
  - Migrated the OpenGL examples onto GLEW
- New examples
  - Direct3D10/advanced/combine_programs
  - Direct3D10/advanced/gs_shrinky
  - Direct3D10/advanced/gs_simple
  - OpenGL/advanced/cgfx_latest
  - Tools/cgfxcat
  - Tools/cginfo
- New documentation
  - Updated reference manual for new profiles and entry points
