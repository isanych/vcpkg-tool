for /f "delims=" %%i in ('vswhere -all -latest -products * -prerelease -property installationPath') do set _p=%%i
if not "%_p%" == "" call "%_p%\VC\Auxiliary\Build\vcvarsall.bat" x64
