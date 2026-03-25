cmake %~dp0 -Wno-dev -DCMAKE_TOOLCHAIN_FILE=C:/opt/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64w %*
cmake --build .
cmake --build . --config Release
