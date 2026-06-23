call %~dp0\env.cmd
cmake %~dp0 -Wno-dev -DCMAKE_TOOLCHAIN_FILE=C:/opt/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64ws -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -GNinja -DCMAKE_BUILD_TYPE=Release %*
ninja
