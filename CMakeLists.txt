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
option(BUILD_TEST "ON for complile test" OFF)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

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
    marco/address.cc
    marco/socket.cc
    marco/bytearray.cc
    marco/http/http.cc
    marco/http/http_parser.cc
    marco/tcp_server.cc
    marco/stream.cc
    marco/socket_stream.cc
    marco/http/http_session.cc
    marco/http/servlet.cc
    marco/http/http_server.cc
    marco/http/http_connection.cc
)

ragelmaker(marco/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/marco/http)
ragelmaker(marco/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/marco/http)
ragelmaker(marco/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/marco)

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

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address marco)
force_redefine_file_macro_for_sources(test_address)
target_link_libraries(test_address ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray marco)
force_redefine_file_macro_for_sources(test_bytearray)
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket marco)
force_redefine_file_macro_for_sources(test_socket)
target_link_libraries(test_socket ${LIBS})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http marco)
force_redefine_file_macro_for_sources(test_http)
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser marco)
force_redefine_file_macro_for_sources(test_http_parser)
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server marco)
force_redefine_file_macro_for_sources(test_tcp_server)
target_link_libraries(test_tcp_server ${LIBS})

add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server marco)
force_redefine_file_macro_for_sources(echo_server)
target_link_libraries(echo_server ${LIBS})

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server marco)
force_redefine_file_macro_for_sources(test_http_server)
target_link_libraries(test_http_server ${LIBS})

add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection marco)
force_redefine_file_macro_for_sources(test_http_connection)
target_link_libraries(test_http_connection ${LIBS})

add_executable(test_uri tests/test_uri.cc)
add_dependencies(test_uri marco)
force_redefine_file_macro_for_sources(test_uri)
target_link_libraries(test_uri ${LIBS})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)