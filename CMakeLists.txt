cmake_minimum_required(VERSION 3.23.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

set(API_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Cpp-Spotify-API")
set(API_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/Cpp-Spotify-API")

ExternalProject_Add(Cpp-Spotify-API
	SOURCE_DIR ${API_SRC_DIR}
	BINARY_DIR ${API_BIN_DIR}
	CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${API_BIN_DIR}"
	INSTALL_COMMAND ""
)

project(Spotify VERSION 0.1.0)

add_executable(Spotify
	index.cpp
	base64.cpp
)

add_dependencies(Spotify Cpp-Spotify-API)

target_include_directories(Spotify PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Cpp-Spotify-API)
target_link_directories(Spotify PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Cpp-Spotify-API)
target_link_libraries(Spotify PRIVATE ${API_BIN_DIR}/libCpp-Spotify-API.so)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)