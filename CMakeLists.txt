cmake_minimum_required(VERSION 3.20)
project(TradeChartEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(TCE_BUILD_TESTS "Build unit tests" ON)
option(TCE_BUILD_SHARED "Build shared library" OFF)

add_subdirectory(core)

if(TCE_BUILD_TESTS AND NOT IOS AND NOT ANDROID AND NOT EMSCRIPTEN)
    enable_testing()
    add_subdirectory(core/tests)
endif()
