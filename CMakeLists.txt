cmake_minimum_required(VERSION 3.23.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VCPKG_INSTALLATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg CACHE STRING "" FORCE)
set(VCPKG_INSTALLATION_ROOT ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux CACHE STRING "" FORCE)

set(CMAKE_TOOLCHAIN_FILE ${VCPKG_INSTALLATION_DIR}/scripts/buildsystems/vcpkg.cmake CACHE STRING "" FORCE)

project(Spotify VERSION 0.1.0)

add_subdirectory(Cpp-Spotify-API)
add_subdirectory(Cpp-RCON)

add_executable(Spotify
	index.cpp
	base64.cpp
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

target_include_directories(Spotify PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Cpp-Spotify-API ${CMAKE_CURRENT_SOURCE_DIR}/Cpp-RCON/include)
target_link_libraries(Spotify PRIVATE Cpp-Spotify-API Lib-Cpp-RCON nlohmann_json::nlohmann_json CURL::libcurl)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)