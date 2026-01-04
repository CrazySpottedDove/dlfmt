# Previous configuration is unchanged
# 1. Set the target system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 2. Specify the cross-compilers
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_C_FLAGS_INIT "-static -static-libgcc")

set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_CXX_FLAGS_INIT "-static -static-libgcc -static-libstdc++")

set(CMAKE_REQUIRED_LIBRARIES "pthread")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++")

# 3. Configure the search behavior for external dependencies
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# New Additions
set(MINGW TRUE)
set(VCPKG_TARGET_TRIPLET x64-mingw-static)
include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")