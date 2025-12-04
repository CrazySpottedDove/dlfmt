cmake_minimum_required(VERSION 3.16)
project(dl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

find_package(spdlog CONFIG REQUIRED)

add_library(dl_core STATIC
    src/tokenizer.cpp
)

target_include_directories(dl_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dl_core PUBLIC spdlog::spdlog)

add_executable(dl main.cpp)
target_link_libraries(dl PRIVATE dl_core)