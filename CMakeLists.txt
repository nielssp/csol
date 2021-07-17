project(csol C)

cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_C_FLAGS "--std=c89 -Wall -pedantic -DUSE_GETOPT")

configure_file(csolrc csolrc COPYONLY)
file(COPY games DESTINATION .)
file(COPY themes DESTINATION .)

set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)
include_directories("$(CURSES_INCLUDE_DIR)")

include_directories(${CMAKE_BINARY_DIR}/src src)

file(GLOB SRC_LIST src/*.c)

add_executable(csol ${SRC_LIST} csolrc)

target_link_libraries(csol ${CURSES_LIBRARIES})

install(TARGETS csol DESTINATION bin COMPONENT binaries)
install(FILES "${CMAKE_BINARY_DIR}/csolrc" DESTINATION /etc/xdg/csol COMPONENT config)
install(DIRECTORY "${CMAKE_BINARY_DIR}/themes" DESTINATION /etc/xdg/csol COMPONENT config)
install(DIRECTORY "${CMAKE_BINARY_DIR}/games" DESTINATION /etc/xdg/csol COMPONENT config)
install(FILES "${CMAKE_BINARY_DIR}/doc/csol.1" DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)
