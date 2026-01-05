cmake_minimum_required(VERSION 3.16)
project(dl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  # MSVC warnings
  add_compile_options(/W4)
  # MSVC 的 OpenMP 标志为 /openmp（如果需要）
  # add_compile_options(/openmp)
else()
  # gcc/clang warnings
  add_compile_options(-Wall -Wextra -Wpedantic)
  # gcc/clang 的 OpenMP 标志为 -fopenmp（如果需要）
  # add_compile_options(-fopenmp)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

find_package(spdlog CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(dl_core STATIC
    src/parser.cpp
)
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()
target_include_directories(dl_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dl_core PUBLIC spdlog::spdlog magic_enum::magic_enum OpenMP::OpenMP_CXX nlohmann_json::nlohmann_json)

add_executable(dlfmt target/dlfmt/main.cpp target/dlfmt/dlfmt_core.cpp)
target_link_libraries(dlfmt PRIVATE dl_core)

# add_executable(dlc target/dlc.cpp)
# target_link_libraries(dlc PRIVATE dl_core)