cmake_minimum_required(VERSION 2.8)

project(libxv)

set(CMAKE_C_STANDARD 11)

enable_testing()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_C_COMPILER "gcc")
    set(C_FLAGS
            -g
            -std=gnu11
            -Wall -Werror
            -static-libgcc -static-libstdc++
            -fno-omit-frame-pointer
            #-fsanitize=address
            )
else()
    set(CMAKE_C_COMPILER "clang")
    set(C_FLAGS
            -g
            -std=c11
            -Wall -Werror
            -fsanitize=address,undefined,null
            )
endif()

string(REPLACE ";" " " CMAKE_C_FLAGS "${C_FLAGS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(tests)

