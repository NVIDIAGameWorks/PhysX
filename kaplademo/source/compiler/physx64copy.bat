@echo off
SETLOCAL

IF NOT "%7"=="-postbuildevent" GOTO RUN_MSG

SET PXFROMDIR=%1
SET PXTODIR=%2
SET EXTERNALSFROMDIR=%3
SET GLUTFROMDIR=%4
SET CGFROMDIR=%5
SET HBAODIR=%6

CALL :UPDATE_PX_TARGET PhysXCooking_64.dll
CALL :UPDATE_PX_TARGET PhysX_64.dll
CALL :UPDATE_PX_TARGET PhysXGpu_64.dll
CALL :UPDATE_PX_TARGET PhysXDevice64.dll
CALL :UPDATE_PX_TARGET PhysXCommon_64.dll
REM CALL :UPDATE_PX_TARGET PhysXCharacterKinematic_64.dll
CALL :UPDATE_PX_TARGET PhysXFoundation_64.dll

rem CALL :UPDATE_PX_TARGET nvToolsExt64_1.dll
rem CALL :UPDATE_PX_TARGET cudart64_*.dll
rem CALL :UPDATE_PX_TARGET glut32.dll

CALL :UPDATE_TARGET %CGFROMDIR% %PXTODIR% cg.dll
CALL :UPDATE_TARGET %CGFROMDIR% %PXTODIR% cgGL.dll
CALL :UPDATE_TARGET %CGFROMDIR% %PXTODIR% cgD3d9.dll

CALL :UPDATE_TARGET %EXTERNALSFROMDIR%/nvToolsExt/1/bin/x64 %PXTODIR% nvToolsExt64_1.dll

CALL :UPDATE_TARGET %GLUTFROMDIR% %PXTODIR% glut32.dll

CALL :UPDATE_TARGET ../../../bin %PXTODIR% resourcePath.txt

IF "%6"=="" GOTO END

CALL :UPDATE_TARGET %HBAODIR% %PXTODIR% GFSDK_SSAO_GL.win64.dll

ENDLOCAL
GOTO END


REM ********************************************
REM NO CALLS TO :UPDATE*_TARGET below this line!!
REM ********************************************

:UPDATE_TARGET
IF NOT EXIST %1\%3 (
	echo File doesn't exist %1\%3
) ELSE (
	mkdir "%2\DEBUG"
	mkdir "%2\RELEASE"
	mkdir "%2\CHECKED"
	mkdir "%2\PROFILE"
    XCOPY "%1\%3" "%2\DEBUG" /R /C /Y > nul
	XCOPY "%1\%3" "%2\RELEASE" /R /C /Y > nul
	XCOPY "%1\%3" "%2\CHECKED" /R /C /Y > nul
	XCOPY "%1\%3" "%2\PROFILE" /R /C /Y > nul
)

:UPDATE_PX_TARGET
IF NOT EXIST %PXFROMDIR%\DEBUG\%1 (
	echo File doesn't exist %PXFROMDIR%\DEBUG\%1
) ELSE (
	mkdir "%PXTODIR%\DEBUG"
    XCOPY "%PXFROMDIR%\DEBUG\%1" "%PXTODIR%\DEBUG" /R /C /Y > nul
)
IF NOT EXIST %PXFROMDIR%\RELEASE\%1 (
	echo File doesn't exist %PXFROMDIR%\RELEASE\%1
) ELSE (
	mkdir "%PXTODIR%\RELEASE"
    XCOPY "%PXFROMDIR%\RELEASE\%1" "%PXTODIR%\RELEASE" /R /C /Y > nul
)
IF NOT EXIST %PXFROMDIR%\CHECKED\%1 (
	echo File doesn't exist %PXFROMDIR%\CHECKED\%1
) ELSE (
	mkdir "%PXTODIR%\CHECKED"
    XCOPY "%PXFROMDIR%\CHECKED\%1" "%PXTODIR%\CHECKED" /R /C /Y > nul
)
IF NOT EXIST %PXFROMDIR%\PROFILE\%1 (
	echo File doesn't exist %PXFROMDIR%\PROFILE\%1
) ELSE (
	mkdir "%PXTODIR%\PROFILE"
    XCOPY "%PXFROMDIR%\PROFILE\%1" "%PXTODIR%\PROFILE" /R /C /Y > nul
)
GOTO END

:RUN_MSG
echo This script doesn't need to be run manually. Please compile the kapla demo after compiling PhysX in the corresponding configuration, and the copy script will be executed by the build automatically
pause

:END