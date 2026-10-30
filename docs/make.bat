@ECHO OFF

pushd %~dp0

REM FromSoheWare Engine docs build script
REM   make.bat html   : build HTML (_build\html\index.html)
REM   make.bat clean  : remove build output
REM   make.bat live   : live preview server (rebuilds on save)

if "%SPHINXBUILD%" == "" (
	set SPHINXBUILD=sphinx-build
)
set SOURCEDIR=.
set BUILDDIR=_build

if "%1" == "" goto help
if "%1" == "live" goto live

%SPHINXBUILD% >NUL 2>NUL
if errorlevel 9009 (
	echo.
	echo sphinx-build was not found. Install the tools with:
	echo     pip install -r requirements.txt
	exit /b 1
)

%SPHINXBUILD% -M %1 %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%
goto end

:live
sphinx-autobuild %SOURCEDIR% %BUILDDIR%\html --ignore _build --open-browser %SPHINXOPTS%
goto end

:help
%SPHINXBUILD% -M help %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%

:end
popd
