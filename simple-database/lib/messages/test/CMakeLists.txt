cmake_minimum_required(VERSION 3.6)

project(HubCppMessageTests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DCRYPTOCURRENCY=1)
add_definitions(-DCONFIDENTIAL_COMPUTE=1)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../" msgSrc)

# GTEST
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# warnings
set(WFLAGS "")
if ( CMAKE_COMPILER_IS_GNUCC )
    set(WFLAGS "-Wall -Wextra -Wno-unused-function -Wno-unused-variable -Werror")
endif()
if ( MSVC )
    set(WFLAGS "/W4 /WX")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS}")

file(GLOB_RECURSE test_sources "./suites/*.cpp")
add_executable(msgTests ${test_sources})
# target_include_directories(msgTests PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(msgTests pthread simple-msg gtest_main)
