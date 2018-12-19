REM Rebuilds the cloth CUDA kernel that calculates smooth normals, the resulting fatbin
REM will be converted to a C header file that is included in the SampleRenderer

set CUDA_PATH=..\..\..\..\..\..\..\..\..\externals\CUDA\6.0.2-17056404
set SDK_ROOT=..\..\..\..\..
set COMPILER_DIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools

"%CUDA_PATH%\bin\nvcc.exe" -m32 -g -gencode=arch=compute_11,code=sm_11 -gencode=arch=compute_20,code=sm_20 -gencode=arch=compute_30,code=sm_30 -gencode=arch=compute_50,code=sm_50 -Xopencc -woffall -prec-div=false -prec-sqrt=false -ftz=true -use_fast_math -D_USE_MATH_DEFINES -DNDEBUG --compiler-bindir="%COMPILER_DIR%" --compiler-options=/W3,/nologo,/Zi,/MT,/Ox -I"%CUDA_PATH%\include" -I"%SDK_ROOT%\Include" -I"%SDK_ROOT%\Include\foundation" -I"%SDK_ROOT%\Source\LowLevelCloth\include" -I"%SDK_ROOT%\Source\LowLevelCloth\src" --fatbin -o RenderClothNormals_x86.fatbin RenderClothNormals.cu
"%CUDA_PATH%\bin\bin2c.exe" --name computeSmoothNormals RenderClothNormals_x86.fatbin > RenderClothNormals_x86.cuh

"%CUDA_PATH%\bin\nvcc.exe" -m64 -g -gencode=arch=compute_11,code=sm_11 -gencode=arch=compute_20,code=sm_20 -gencode=arch=compute_30,code=sm_30 -gencode=arch=compute_50,code=sm_50 -Xopencc -woffall -prec-div=false -prec-sqrt=false -ftz=true -use_fast_math -D_USE_MATH_DEFINES -DNDEBUG --compiler-bindir="%COMPILER_DIR%" --compiler-options=/W3,/nologo,/Zi,/MT,/Ox -I"%CUDA_PATH%\include" -I"%SDK_ROOT%\Include" -I"%SDK_ROOT%\Include\foundation" -I"%SDK_ROOT%\Source\LowLevelCloth\include" -I"%SDK_ROOT%\Source\LowLevelCloth\src" --fatbin -o RenderClothNormals_x64.fatbin RenderClothNormals.cu
"%CUDA_PATH%\bin\bin2c.exe" --name computeSmoothNormals RenderClothNormals_x64.fatbin > RenderClothNormals_x64.cuh

"%CUDA_PATH%\bin\nvcc.exe" -m32 -g -gencode=arch=compute_11,code=sm_11 -gencode=arch=compute_20,code=sm_20 -gencode=arch=compute_30,code=sm_30 -gencode=arch=compute_50,code=sm_50 -Xopencc -woffall -prec-div=false -prec-sqrt=false -ftz=true -use_fast_math -D_USE_MATH_DEFINES -DNDEBUG --compiler-bindir="%COMPILER_DIR%" --compiler-options=/W3,/nologo,/Zi,/MT,/Ox -I"%CUDA_PATH%\include" -I"%SDK_ROOT%\Include" -I"%SDK_ROOT%\Include\foundation" -I"%SDK_ROOT%\Source\LowLevelCloth\include" -I"%SDK_ROOT%\Source\LowLevelCloth\src" --fatbin -o RenderParticles_x86.fatbin RenderParticles.cu
"%CUDA_PATH%\bin\bin2c.exe" --name updateParticleVertexBuffer RenderParticles_x86.fatbin > RenderParticles_x86.cuh

"%CUDA_PATH%\bin\nvcc.exe" -m64 -g -gencode=arch=compute_11,code=sm_11 -gencode=arch=compute_20,code=sm_20 -gencode=arch=compute_30,code=sm_30 -gencode=arch=compute_50,code=sm_50 -Xopencc -woffall -prec-div=false -prec-sqrt=false -ftz=true -use_fast_math -D_USE_MATH_DEFINES -DNDEBUG --compiler-bindir="%COMPILER_DIR%" --compiler-options=/W3,/nologo,/Zi,/MT,/Ox -I"%CUDA_PATH%\include" -I"%SDK_ROOT%\Include" -I"%SDK_ROOT%\Include\foundation" -I"%SDK_ROOT%\Source\LowLevelCloth\include" -I"%SDK_ROOT%\Source\LowLevelCloth\src" --fatbin -o RenderParticles_x64.fatbin RenderParticles.cu
"%CUDA_PATH%\bin\bin2c.exe" --name updateParticleVertexBuffer RenderParticles_x64.fatbin > RenderParticles_x64.cuh

pause

