cmake_minimum_required(VERSION 3.15)
project(dylibbundler)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include_directories(src)

add_executable(dylibbundler
    src/Dependency.cpp
    src/Dependency.h
    src/DylibBundler.cpp
    src/DylibBundler.h
    src/main.cpp
    src/Settings.cpp
    src/Settings.h
    src/Utils.cpp
    src/Utils.h
)
