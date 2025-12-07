cmake_minimum_required(VERSION 3.16)
project(dl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

find_package(spdlog CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
add_library(dl_core STATIC
    src/tokenizer.cpp
    src/ast_tools.cpp
    src/parser.cpp
)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
target_include_directories(dl_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dl_core PUBLIC spdlog::spdlog magic_enum::magic_enum OpenMP::OpenMP_CXX)

add_executable(dlfmt target/dlfmt.cpp)
target_link_libraries(dlfmt PRIVATE dl_core)

# add_executable(dlc target/dlc.cpp)
# target_link_libraries(dlc PRIVATE dl_core)