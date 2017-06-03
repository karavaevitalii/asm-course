cmake_minimum_required(VERSION 2.8)

project(words-count)

set(CMAKE_CXX_STANDART 11)

set(CMAKE_CXX_FLAGS_DEBUG "${CMKAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -W -Wfloat-equal -Wconversion \
        -Wunreachable-code -Winline -Werror -pedantic-errors -fsanitize=address -g -time")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

add_executable(count main.cpp)
