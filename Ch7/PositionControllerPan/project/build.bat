@ECHO OFF
rem ----Usage----
rem build [clean|noclean]
rem vs2008 for compiling with visual studio 2008
rem clean to force a full rebuild
rem noclean to force a build without clean
rem noprompt to avoid all prompts
CLS
COLOR 1B
TITLE 20-sim Dynamic DLL build script
rem -------------------------------------------------------------
set CURPATH=%~dp0
rem Start searching for the newest compiler
set comp=vs2022
set promptlevel=prompt
set exitcode=0
set buildmode=clean
set DLL=PositionControllerPan.dll
FOR %%b in (%1, %2, %3, %4, %5) DO (
	IF %%b==vs2005 set comp=vs2005
	IF %%b==vs2008 set comp=vs2008
	IF %%b==vs2010 set comp=vs2010
	IF %%b==vs2012 set comp=vs2012
	IF %%b==vs2013 set comp=vs2013
	IF %%b==vs2015 set comp=vs2015
	IF %%b==vs2017 set comp=vs2017
	IF %%b==vs2019 set comp=vs2019
	IF %%b==vs2022 set comp=vs2022
	IF %%b==clean set buildmode=clean
	IF %%b==noclean set buildmode=noclean
	IF %%b==noprompt set promptlevel=noprompt
)

set buildconfig=Release
set DEVENV=""
set VSVARS32=""

ECHO ------------------------------------------------------------
ECHO 20-sim Dynamic DLL for 'PositionControllerPan'
ECHO ------------------------------------------------------------
ECHO Searching for Visual C++ compiler...

rem Search for VS 2022
:VS2022
IF NOT %comp%==vs2022 goto VS2019
setlocal
rem Search for VSWhere first
set "InstallerPath=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%InstallerPath%" set "InstallerPath=%ProgramFiles%\Microsoft Visual Studio\Installer"
if not exist "%InstallerPath%" goto :no-vswhere_2022

set VSWHERE_ARGS=-latest -products * %VSWHERE_REQ% %VSWHERE_PRP% %VSWHERE_LMT%
set VSWHERE_REQ=-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
set VSWHERE_PRP=-property installationPath
set VSWHERE_LMT=-version "[17.0,18.0)"
set VSWHERE_ARGS=-latest -products * %VSWHERE_REQ% %VSWHERE_PRP% %VSWHERE_LMT%
set PATH=%PATH%;%InstallerPath%
for /f "usebackq tokens=*" %%i in (`vswhere %VSWHERE_ARGS%`) do (
	endlocal
	set "VCINSTALLDIR=%%i\VC\"
	set "VS170COMNTOOLS=%%i\Common7\Tools\"
)
endlocal
:no-vswhere_2022:
	IF EXIST "%VS170COMNTOOLS%\VsDevCmd.bat" (
		set VSVARS32="%VS170COMNTOOLS%\VsDevCmd.bat"
		ECHO Found Visual C++ 2022
		set PROJ_DIR=VS2022
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
		set VSVARS32="%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
		ECHO Found Visual C++ 2022
		set PROJ_DIR=VS2022
	) ELSE IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
		set VSVARS32="%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
		ECHO Found Visual C++ 2022
		set PROJ_DIR=VS2022
	) ELSE (
		rem Try an older compiler
		set comp=vs2019
	)
)


rem Search for VS 2019
:VS2019
IF NOT %comp%==vs2019 goto VS2017
setlocal
rem Search for VSWhere first
set "InstallerPath=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%InstallerPath%" set "InstallerPath=%ProgramFiles%\Microsoft Visual Studio\Installer"
if not exist "%InstallerPath%" goto :no-vswhere_2019

set VSWHERE_ARGS=-latest -products * %VSWHERE_REQ% %VSWHERE_PRP% %VSWHERE_LMT%
set VSWHERE_REQ=-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
set VSWHERE_PRP=-property installationPath
set VSWHERE_LMT=-version "[16.0,17.0)"
set VSWHERE_ARGS=-latest -products * %VSWHERE_REQ% %VSWHERE_PRP% %VSWHERE_LMT%
set PATH=%PATH%;%InstallerPath%
for /f "usebackq tokens=*" %%i in (`vswhere %VSWHERE_ARGS%`) do (
	endlocal
	set "VCINSTALLDIR=%%i\VC\"
	set "VS160COMNTOOLS=%%i\Common7\Tools\"
)
endlocal
:no-vswhere_2019:
	IF EXIST "%VS160COMNTOOLS%\VsDevCmd.bat" (
		set VSVARS32="%VS160COMNTOOLS%\VsDevCmd.bat"
		ECHO Found Visual C++ 2019
		set PROJ_DIR=VS2019
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
		set VSVARS32="%ProgramFiles%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
		ECHO Found Visual C++ 2019
		set PROJ_DIR=VS2019
	) ELSE IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
		set VSVARS32="%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
		ECHO Found Visual C++ 2019
		set PROJ_DIR=VS2019
	) ELSE (
		rem Try an older compiler
		set comp=vs2017
	)
)

rem Search for VS 2017
:VS2017
IF NOT %comp%==vs2017 goto VS2015
setlocal
rem Search for VSWhere first
set "InstallerPath=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%InstallerPath%" set "InstallerPath=%ProgramFiles%\Microsoft Visual Studio\Installer"
if not exist "%InstallerPath%" goto :no-vswhere

set VSWHERE_ARGS=-latest -products * %VSWHERE_REQ% %VSWHERE_PRP% %VSWHERE_LMT%
set VSWHERE_REQ=-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
set VSWHERE_PRP=-property installationPath
set VSWHERE_LMT=-version "[15.0,16.0)"
set VSWHERE_ARGS=-latest -products * %VSWHERE_REQ% %VSWHERE_PRP% %VSWHERE_LMT%
set PATH=%PATH%;%InstallerPath%
for /f "usebackq tokens=*" %%i in (`vswhere %VSWHERE_ARGS%`) do (
	endlocal
	set "VCINSTALLDIR=%%i\VC\"
	set "VS150COMNTOOLS=%%i\Common7\Tools\"
)
endlocal
:no-vswhere:
	IF EXIST "%VS150COMNTOOLS%\VsDevCmd.bat" (
		set VSVARS32="%VS150COMNTOOLS%\VsDevCmd.bat"
		ECHO Found Visual C++ 2017
		set PROJ_DIR=VS2017
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
		set VSVARS32="%ProgramFiles%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
		ECHO Found Visual C++ 2017
		set PROJ_DIR=VS2017
	) ELSE IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
		set VSVARS32="%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
		ECHO Found Visual C++ 2017
		set PROJ_DIR=VS2017
	) ELSE (
		rem Try an older compiler
		set comp=vs2015
	)
)

rem Search for VS 2015
:VS2015
IF %comp%==vs2015 (
	set PROJ_DIR=VS2015
	IF EXIST "%VS140COMNTOOLS%\vsvars32.bat" (
		set VSVARS32="%VS140COMNTOOLS%\vsvars32.bat"
		ECHO Found Visual C++ 2015
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat" (
		set VSVARS32="%ProgramFiles%\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat"
		ECHO Found Visual C++ 2015
	) ELSE (
		rem Try an older compiler
		set comp=vs2013
	)
)

rem Search for VS 2013 / VS 2013 Express / VS 2013 Community edition
IF %comp%==vs2013 (
	set PROJ_DIR=VS2013
	IF EXIST "%VS120COMNTOOLS%\vsvars32.bat" (
		set VSVARS32="%VS120COMNTOOLS%\vsvars32.bat"
		ECHO Found Visual C++ 2013
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio 12.0\Common7\Tools\vsvars32.bat" (
		set VSVARS32="%ProgramFiles%\Microsoft Visual Studio 12.0\Common7\Tools\vsvars32.bat"
		ECHO Found Visual C++ 2013
	) ELSE (
		rem Try an older compiler
		set comp=vs2012
	)
)

rem Seach for VS 2012 / VS 2012 Express
IF %comp%==vs2012 (
	set PROJ_DIR=VS2012
	IF EXIST "%VS110COMNTOOLS%\..\IDE\devenv.exe" (
		set DEVENV="%VS110COMNTOOLS%\..\IDE\devenv.exe"
		ECHO Found Visual C++ 2012
	) ELSE IF EXIST "%VS110COMNTOOLS%\..\IDE\VCExpress.exe" (
		set DEVENV="%VS110COMNTOOLS%\..\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2012
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio 11.0\Common7\IDE\VCExpress.exe" (
		set DEVENV="%ProgramFiles%\Microsoft Visual Studio 11.0\Common7\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2012
	) ELSE (
		rem Try an older compiler
		set comp=vs2010
	)
)

rem Search for VS 2010 / VS 2010 Express
IF %comp%==vs2010 (
	set PROJ_DIR=VS2010
	IF EXIST "%VS100COMNTOOLS%\..\IDE\devenv.exe" (
		set DEVENV="%VS100COMNTOOLS%\..\IDE\devenv.exe"
		ECHO Found Visual C++ 2010
	) ELSE IF EXIST "%VS100COMNTOOLS%\..\IDE\VCExpress.exe" (
		set DEVENV="%VS100COMNTOOLS%\..\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2010
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\VCExpress.exe" (
		set DEVENV="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2010
	) ELSE (
		rem Try an older compiler
		set comp=vs2008
	)
)

rem Seach for VS 2008 / VS 2008 Express
IF %comp%==vs2008 (
	set PROJ_DIR=VS2008
	IF EXIST "%VS90COMNTOOLS%\..\IDE\devenv.exe" (
		set DEVENV="%VS90COMNTOOLS%\..\IDE\devenv.exe"
		ECHO Found Visual C++ 2008
	) ELSE IF EXIST "%VS90COMNTOOLS%\..\IDE\VCExpress.exe" (
		set DEVENV="%VS90COMNTOOLS%\..\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2008
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio 9.0\Common7\IDE\VCExpress.exe" (
		set DEVENV="%ProgramFiles%\Microsoft Visual Studio 9.0\Common7\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2008
	) ELSE (
		rem Try an older compiler
		set comp=vs2005
	)
)

rem Seach for VS 2005 / VS 2005 Express
IF %comp%==vs2005 (
	set PROJ_DIR=VS2005
	IF EXIST "%VS80COMNTOOLS%\..\IDE\devenv.exe" (
		set DEVENV="%VS80COMNTOOLS%\..\IDE\devenv.exe"
		ECHO Found Visual C++ 2005
	) ELSE IF EXIST "%VS80COMNTOOLS%\..\IDE\VCExpress.exe" (
		set DEVENV="%VS80COMNTOOLS%\..\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2005
	) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio 8\Common7\IDE\VCExpress.exe" (
		set DEVENV="%ProgramFiles%\Microsoft Visual Studio 8\Common7\IDE\VCExpress.exe"
		ECHO Found Visual C++ Express 2005
	) ELSE (
		echo No suitable Visual C++ compiler found
		set PROJ_DIR=
	)
)

REM Remove potential " from the path, otherwise loading vsvar32 fails
set PATH=%PATH:"=%

IF DEFINED VSVARS32 (
	echo Loading vsvar32
	call %VSVARS32%
) ELSE (
	IF DEFINED DEVENV (
		IF NOT EXIST %DEVENV% (
			echo Could not find a suitable Visual C++ compiler. Supported versions: 2005, 2008, 2010, 2012, 2013 Desktop/Express, 2015, 2017.
			echo You can use the generated Makefile to compile the DLL using the MinGW compiler
			echo.
			echo You can find the generated source code in
			cd %~dp0\..\..\
			echo   %CD%
			echo.
			echo The corresponding 20-sim submodel to use this DLL in 20-sim is:
			echo   PositionControllerPan.emx
			goto END
		)
	)
)

set OPTS_DLL="%PROJ_DIR%\PositionControllerPan.sln" /build "%buildconfig%"
set CLEAN_DLL="%PROJ_DIR%\PositionControllerPan.sln" /clean "%buildconfig%"

ECHO ------------------------------------------------------------
rem	CONFIG END
rem -------------------------------------------------------------

:DLL_COMPILE
  IF EXIST %PROJ_DIR%\%buildconfig%\buildlog.html del %PROJ_DIR%\%buildconfig%\buildlog.html /q
  IF %buildmode%==clean goto COMPILE_DLL
  IF %buildmode%==noclean goto COMPILE_NO_CLEAN_DLL
  rem ---------------------------------------------
  rem	check for existing dll
  rem ---------------------------------------------
  
  IF EXIST ..\%DLL% (
    echo "Found existing DLL %DLL%"
    goto DLL_EXIST
  )
  goto COMPILE_DLL

:DLL_EXIST
  IF %promptlevel%==noprompt goto COMPILE_DLL
  ECHO Found a previous compiled WIN32 DLL.
  ECHO [1] a NEW DLL will be compiled.
  ECHO [2] existing DLL will be updated (quick mode compile).
  ECHO ------------------------------------------------------------
  set /P COMPILE_ANSWER=Compile a new EXE? [1/2]:
  if /I %COMPILE_ANSWER% EQU 1 goto COMPILE_DLL
  if /I %COMPILE_ANSWER% EQU 2 goto COMPILE_NO_CLEAN_DLL
  
:COMPILE_DLL
  IF %DEVENV% NEQ "" (
    ECHO Cleaning Solution...
	%DEVENV% "%PROJ_DIR%\PositionControllerPan.sln" /clean "%buildconfig%"
    ECHO Compiling 20-sim Dynamic DLL for submodel "PositionControllerPan"
    %DEVENV% "%PROJ_DIR%\PositionControllerPan.sln" /build "%buildconfig%"
  ) ELSE (
	ECHO Compiling 20-sim Dynamic DLL for submodel "PositionControllerPan"
    msbuild.exe "%PROJ_DIR%\PositionControllerPan.vcxproj" /p:Configuration=Release /t:Rebuild /verbosity:minimal
  )
  
  IF NOT EXIST ..\%DLL% (
  	set DIETEXT="PositionControllerPan.dll failed to build!  See ..\%PROJ_DIR%\%buildconfig%\BuildLog.htm for details."
  	goto DIE
  )
  ECHO Done.
  ECHO ------------------------------------------------------------
  set buildmode=clean
  GOTO MAKE_BUILD_DLL
  
:COMPILE_NO_CLEAN_DLL
  IF %DEVENV% NEQ "" (
    ECHO Compiling 20-sim Dynamic DLL for submodel "PositionControllerPan"
    %DEVENV% "%PROJ_DIR%\PositionControllerPan.sln" /build "%buildconfig%"
  ) ELSE (
	ECHO Compiling 20-sim Dynamic DLL for submodel "PositionControllerPan"
    msbuild.exe "%PROJ_DIR%\PositionControllerPan.vcxproj" /p:Configuration=Release /t:Build /verbosity:minimal
  )

  IF NOT EXIST ..\%DLL% (
  	set DIETEXT="PositionControllerPan.dll failed to build!  See ..\%PROJ_DIR%\%buildconfig%\BuildLog.htm for details."
  	goto DIE
  )
  ECHO Done.
  ECHO ------------------------------------------------------------
  GOTO MAKE_BUILD_DLL

:MAKE_BUILD_DLL
  cd ..
  ECHO Your PositionControllerPan Dynamic DLL is ready and can be found in:
  ECHO   %CD%
  ECHO The name of your DLL is:
  ECHO   %DLL%
  ECHO The corresponding 20-sim submodel to use this DLL in 20-sim is:
  ECHO   PositionControllerPan.emx
  ECHO ------------------------------------------------------------
  cd %CURPATH%
  GOTO END
  
:DIE
  set DIETEXT=Error: %DIETEXT%
  echo %DIETEXT%
  SET exitcode=1
  ECHO ------------------------------------------------------------

:END
  IF %promptlevel% NEQ noprompt (
    IF "%XXSIM_SCRIPT_MODE%" == "1" goto END_NO_PROMPT
    ECHO Press any key to exit...
    pause > NUL
  )
:END_NO_PROMPT
  IF %exitcode% NEQ 0 EXIT /B %exitcode%
  EXIT

