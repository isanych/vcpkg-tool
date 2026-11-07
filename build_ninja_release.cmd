call %~dp0\env.cmd
if not defined VCPKG_ROOT set VCPKG_ROOT=C:/opt/vcpkg
if not defined VCPKG_DEFAULT_TRIPLET set VCPKG_DEFAULT_TRIPLET=x64ws
if not defined VCPKG_VERSION for /f "tokens=*" %%i in ('git describe --match v0') do set VCPKG_VERSION=%%i
set VCPKG_VERSION=%VCPKG_VERSION:v0-=%
cmake %~dp0 -Wno-author -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%VCPKG_DEFAULT_TRIPLET% -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -GNinja -DCMAKE_BUILD_TYPE=Release -DVCPKG_VERSION=%VCPKG_VERSION% %*
ninja
ninja test
vcpkg --version
if "%ART_PUBLISH%"=="0" exit /b 0
tar czf vcpkg-tool-2026.4-x64w.tgz vcpkg.exe
artc put vcpkg-tool-2026.4-x64w.tgz /vcpkg-tool/
