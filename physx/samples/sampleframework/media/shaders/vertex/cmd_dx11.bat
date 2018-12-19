cd ../../
mkdir compiledshaders
cd compiledshaders
mkdir dx11feature11
cd dx11feature11
mkdir vertex
mkdir geometry
cd ..
mkdir dx11feature9
cd dx11feature9
mkdir vertex
cd ../../shaders/vertex

CALL :SHADER staticmesh
CALL :SHADER mouse
CALL :SHADER particle_fog
CALL :SHADER pointsprite
CALL :SHADER pointsprite2
CALL :SHADER screenquad
CALL :SHADER skeletalmesh_1bone
CALL :SHADER skeletalmesh_4bone
CALL :SHADER staticmesh
CALL :SHADER text
CALL :SHADER turbulencesprites
CALL :GEOMETRYSHADER pointsprite
exit /b

:SHADER

CALL :COMPILE %1 %1
set inputDef=/DUSE_POSITION=1 /DUSE_COLOR=1
set inputDefName=POSITION_COLOR
CALL :INPUT_COMPILE %1 %1 %inputDefName%
set inputDef=/DUSE_POSITION=1 /DUSE_COLOR=1 /DUSE_TEXCOORD0=1
set inputDefName=POSITION_COLOR_TEXCOORD0
CALL :INPUT_COMPILE %1 %1 %inputDefName%
set inputDef=/DUSE_POSITION=1 /DUSE_NORMAL=1 /DUSE_TEXCOORD0=1
set inputDefName=POSITION_NORMAL_TEXCOORD0
CALL :INPUT_COMPILE %1 %1 %inputDefName%
set inputDef=/DUSE_POSITION=1 /DUSE_NORMAL=1 /DUSE_TEXCOORD0=1
set inputDefName=TEXCOORD0_POSITION_NORMAL
CALL :INPUT_COMPILE %1 %1 %inputDefName%
set inputDef=/DUSE_POSITION=1 /DUSE_NORMAL=1 /DUSE_TEXCOORD0=1 /DUSE_COLOR=1
set inputDefName=NORMAL_TEXCOORD0_POSITION_COLOR
CALL :INPUT_COMPILE %1 %1 %inputDefName%
set inputDef=/DUSE_POSITION=1 /DUSE_NORMAL=1 /DUSE_COLOR=1
set inputDefName=POSITION_COLOR_NORMAL
CALL :INPUT_COMPILE %1 %1 %inputDefName%
exit /b

:GEOMETRYSHADER

CALL :GEOMETRYCOMPILE %1 %1
exit /b

:COMPILE
set definesdx9=/DRENDERER_VERTEX=1 /DRENDERER_D3D11=1 /DPX_WINDOWS=1 /DRENDERER_INSTANCED=0 /DRENDERER_DISPLACED=0 /DENABLE_VFACE=0 /DENABLE_VFACE_SCALE=0 /DRENDERER_WIN8ARM=1 /DENABLE_TESSELLATION=0 /DADAPTIVE_TESSELLATION=0 /DSEMANTIC_TANGENT=TANGENT /DUSE_ALL=1
set definesdx11=/DRENDERER_VERTEX=1 /DRENDERER_D3D11=1 /DPX_WINDOWS=1 /DRENDERER_INSTANCED=0 /DRENDERER_DISPLACED=0 /DENABLE_VFACE=1 /DENABLE_VFACE_SCALE=1 /DENABLE_TESSELLATION=0 /DADAPTIVE_TESSELLATION=0 /DSEMANTIC_TANGENT=TANGENT /DUSE_ALL=1

"fxc.exe" /E"vmain" /Zpr /Gec /I"../include" %definesdx9% /Fo"../../compiledshaders/dx11feature9/vertex/%2.cg.cso" /T vs_4_0_level_9_1 /nologo %1.cg
if errorlevel 1 pause
"fxc.exe" /E"vmain" /Zpr /Gec /I"../include" %definesdx11% /Fo"../../compiledshaders/dx11feature11/vertex/%2.cg.cso" /T vs_4_0 /nologo %1.cg
if errorlevel 1 pause
exit /b

:INPUT_COMPILE
set definesdx9=/DRENDERER_VERTEX=1 /DRENDERER_D3D11=1 /DPX_WINDOWS=1 /DRENDERER_INSTANCED=0 /DRENDERER_DISPLACED=0 /DENABLE_VFACE=0 /DENABLE_VFACE_SCALE=0 /DRENDERER_WIN8ARM=1 /DENABLE_TESSELLATION=0 /DADAPTIVE_TESSELLATION=0 /DSEMANTIC_TANGENT=TANGENT
set definesdx11=/DRENDERER_VERTEX=1 /DRENDERER_D3D11=1 /DPX_WINDOWS=1 /DRENDERER_INSTANCED=0 /DRENDERER_DISPLACED=0 /DENABLE_VFACE=1 /DENABLE_VFACE_SCALE=1 /DENABLE_TESSELLATION=0 /DADAPTIVE_TESSELLATION=0 /DSEMANTIC_TANGENT=TANGENT

"fxc.exe" /E"vmain" /Zpr /Gec /I"../include" %definesdx9% %inputDef% /Fo"../../compiledshaders/dx11feature9/vertex/%2.cg_%3.cso" /T vs_4_0_level_9_1 /nologo %1.cg
if errorlevel 1 pause
"fxc.exe" /E"vmain" /Zpr /Gec /I"../include" %definesdx11% %inputDef% /Fo"../../compiledshaders/dx11feature11/vertex/%2.cg_%3.cso" /T vs_4_0 /nologo %1.cg
if errorlevel 1 pause
exit /b

:GEOMETRYCOMPILE
set definesdx11=/DRENDERER_GEOMETRY=1 /DRENDERER_D3D11=1 /DPX_WINDOWS=1 /DSEMANTIC_TANGENT=TANGENT /DUSE_ALL=1

"fxc.exe" /E"gmain" /Zpr /Gec /I"../include" %definesdx11% /Fo"../../compiledshaders/dx11feature11/geometry/%2.cg.cso" /T gs_4_0 /nologo %1.cg
if errorlevel 1 pause
exit /b

set definesdx9=
set definesdx11=