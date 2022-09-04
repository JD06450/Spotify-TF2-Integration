cmake_minimum_required(VERSION 3.23.0)
project(Spotify VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL REQUIRED)
add_executable(Spotify index.cpp base64.cpp curl-util.cpp spotify-api/spotify-api.cpp)

target_link_libraries(Spotify PRIVATE nlohmann_json::nlohmann_json PRIVATE CURL::libcurl)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)