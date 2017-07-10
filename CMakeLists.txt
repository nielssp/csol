project(csol)
set(CSOL_VERSION "0.1.0")

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

configure_file(src/config.h.in src/config.h)
configure_file(src/csolrc csolrc COPYONLY)

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
