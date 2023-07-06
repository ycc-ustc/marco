cmake_minimum_required(VERSION 2.8)
project(marco)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
include_directories(.)
include_directories(/apps/marco/include)

link_directories(/apps/marco/lib)
#find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)
#set(LIBS ${LIBS} sylar)
#set(LIBS ${LIBS} dl)
#set(LIBS ${LIBS} ${YAMLCPP})
#set(LIBS ${LIBS} ${PTHREAD})

set(LIB_SRC
    marco/log.cc
    marco/util.cc
    marco/config.cc
    marco/thread.cc
    marco/fiber.cc
    marco/scheduler.cc
    marco/iomanager.cc
    marco/timer.cc
    marco/hook.cc
    marco/fd_manager.cc
)

set(LIBS
    marco
    dl
    pthread
    yaml-cpp
)

add_library(marco SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(marco)

add_executable(test tests/test.cc)
add_dependencies(test marco)
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config marco)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread marco)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util marco)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIBS})


add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber marco)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler marco)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager marco)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook marco)
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIBS})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)