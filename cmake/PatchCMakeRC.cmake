# Script mode (cmake -P) replacement for applying CMakeRC_cmake_4.patch without needing git.
# Run with the CMakeRC source directory as the working directory.
# Allows CMakeRC to be configured with CMake 4.0+ by extending the cmake_minimum_required ranges.

function(vcpkg_patch_cmakerc_file file_name old_text new_text)
    file(READ "${file_name}" contents)
    string(FIND "${contents}" "${old_text}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Failed to patch ${file_name}: '${old_text}' was not found.")
    endif()
    string(REPLACE "${old_text}" "${new_text}" contents "${contents}")
    file(WRITE "${file_name}" "${contents}")
endfunction()

vcpkg_patch_cmakerc_file("CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.6)"
    "cmake_minimum_required(VERSION 3.6...4.0)")
vcpkg_patch_cmakerc_file("CMakeRC.cmake"
    "cmake_minimum_required(VERSION 3.3)"
    "cmake_minimum_required(VERSION 3.3...4.0)")
