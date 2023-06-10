cmake_minimum_required(VERSION 2.8)
project(macro)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    macro/log.cc
    macro/util.cc
)

add_library(macro SHARED ${LIB_SRC})

add_executable(test tests/test.cc)
add_dependencies(test macro)
target_link_libraries(test macro)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)