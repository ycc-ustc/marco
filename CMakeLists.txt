cmake_minimum_required(VERSION 2.8)
project(marco)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(.)
include_directories(/apps/marco/include)

link_directories(/apps/marco/lib)
find_library(YAMLCPP yaml-cpp)

set(LIB_SRC
    marco/log.cc
    marco/util.cc
    marco/config.cc
)

add_library(marco SHARED ${LIB_SRC})

add_executable(test tests/test.cc)
add_dependencies(test marco)
target_link_libraries(test marco ${YAMLCPP})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config marco)
target_link_libraries(test_config marco ${YAMLCPP})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)