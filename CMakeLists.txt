cmake_minimum_required(VERSION 3.23.0)
project(Spotify VERSION 0.1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Spotify
	index.cpp
	base64.cpp
	tf2-logfile.cpp
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)