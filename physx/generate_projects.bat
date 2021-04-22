:: Reset errorlevel status so we are not inheriting this state from the calling process:
@call :CLEAN_EXIT
@echo off

pushd %~dp0
set PHYSX_ROOT_DIR=%CD%
popd
SET PHYSX_ROOT_DIR=%PHYSX_ROOT_DIR:\=/%
SET PM_VSWHERE_PATH=%PHYSX_ROOT_DIR%/../externals/VsWhere
SET PM_CMAKEMODULES_PATH=%PHYSX_ROOT_DIR%/../externals/CMakeModules
SET PM_PXSHARED_PATH=%PHYSX_ROOT_DIR%/../pxshared
SET PM_TARGA_PATH=%PHYSX_ROOT_DIR%/../externals/targa
SET PM_PATHS=%PM_CMAKEMODULES_PATH%;%PM_TARGA_PATH%

if exist "%PHYSX_ROOT_DIR%/../externals/cmake/x64/bin/cmake.exe" (
    SET "PM_CMAKE_PATH=%PHYSX_ROOT_DIR%/../externals/cmake/x64"
    GOTO CMAKE_EXTERNAL    
)

where /q cmake
IF ERRORLEVEL 1 (    
	ECHO Cmake is missing, please install cmake version 3.12 and up.
    set /p DUMMY=Hit ENTER to continue...
	exit /b 1
)

:CMAKE_EXTERNAL

:: Use the Python launcher if it exists
py --version 2>NUL
IF ERRORLEVEL 0 (
    set PM_PYTHON=py
)

IF ERRORLEVEL 1 (
    python --version 2>NUL
    IF ERRORLEVEL 1 (
        if "%PM_python_PATH%" == "" (        
            ECHO Python is missing, please install python version 2.7.6 and up. If Python is installed but not in the PATH, then set the env variable PM_python_PATH pointing to python root directory.
            set /p DUMMY=Hit ENTER to continue...
            exit /b 1
        )
    )
    IF ERRORLEVEL 0 (
        if "%PM_python_PATH%" == "" (    
        set PM_PYTHON=python.exe
        ) else (
            set PM_PYTHON="%PM_python_PATH%\python.exe"
        )
    )
)

IF %1.==. GOTO ADDITIONAL_PARAMS_MISSING

for /f "usebackq tokens=*" %%i in (`"%PM_vswhere_PATH%\VsWhere.exe  -version [15.0,16.0) -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath"`) do (
	set Install2017Dir=%%i
	set VS150PATH="%%i"
)

for /f "usebackq tokens=*" %%i in (`"%PM_vswhere_PATH%\VsWhere.exe  -version [16.0,17.0) -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath"`) do (
  set Install2019Dir=%%i
	set VS160PATH="%%i"	
)	

if exist "%Install2017Dir%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt" (
  pushd "%Install2017Dir%\VC\Auxiliary\Build\"
  set /p Version=<Microsoft.VCToolsVersion.default.txt
  for /f "delims=" %%x in (Microsoft.VCToolsVersion.default.txt) do (
	if not %%x=="" (
	  rem Example hardcodes x64 as the host and target architecture, but you could parse it from arguments
	  set VS150CLPATH="%Install2017Dir%\VC\Tools\MSVC\%%x\bin\HostX64\x64\cl.exe"
	)
  )  
  popd
)

if exist "%Install2019Dir%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt" (
  pushd "%Install2019Dir%\VC\Auxiliary\Build\"
  set /p Version=<Microsoft.VCToolsVersion.default.txt
  for /f "delims=" %%x in (Microsoft.VCToolsVersion.default.txt) do (
	if not %%x=="" (
	  rem Example hardcodes x64 as the host and target architecture, but you could parse it from arguments
	  set VS160CLPATH="%Install2019Dir%\VC\Tools\MSVC\%%x\bin\HostX64\x64\cl.exe"
	)
  )  
  popd
)

:ADDITIONAL_PARAMS_MISSING
pushd %~dp0
%PM_PYTHON% "%PHYSX_ROOT_DIR%/buildtools/cmake_generate_projects.py" %1
popd
if %ERRORLEVEL% neq 0 (
    set /p DUMMY=Hit ENTER to continue...
    exit /b %errorlevel%
) else (
    goto CLEAN_EXIT
)

:CLEAN_EXIT
exit /b 0