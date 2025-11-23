cmake_minimum_required(VERSION 3.16)

project(DarumaDBTests C CXX)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../" dbSrc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# GTEST
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

if(NOT DEFINED USE_FF)
    set(USE_FF 0)
endif()
if(${USE_FF})
    add_definitions(-DUSE_FF=1)
endif()

file(GLOB_RECURSE dir "src/*.hpp" "src/*.h")
set(include_dirs "")
foreach(file_path ${dir})
    get_filename_component(dir_path ${file_path} PATH)
    set(include_dirs ${include_dirs} ${dir_path})
endforeach()
list(REMOVE_DUPLICATES include_dirs)

file(GLOB_RECURSE test_sources "src/*.cpp" "src/*.h" "src/*.hpp")

add_executable(dbTests ${test_sources})
target_include_directories(dbTests PRIVATE ${include_dirs})
target_link_libraries(dbTests pthread simple-db simple-msg gtest_main)
