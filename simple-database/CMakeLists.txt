cmake_minimum_required(VERSION 3.6)

project(DarumaDB C CXX)

if (NOT TARGET simple-msg)
    if (NOT DEFINED HUB_MSG_SCHEMA_DIR)
        set(HUB_MSG_SCHEMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test-schemas")
    endif()
    add_subdirectory(lib/messages)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(clang_tidy_checks
      clang-diagnostic-*
      clang-analyzer-*
      -clang-analyzer-security.insecureAPI.strcpy
      -clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling

      llvm-*
      -llvm-header-guard
      -llvm-include-order
      -llvm-namespace-comment

      bugprone-*
      -bugprone-reserved-identifier
      -bugprone-easily-swappable-parameters
    )

string(JOIN "," clang_tidy_checks ${clang_tidy_checks})

set(CLANG_TIDY_ARGS
  clang-tidy;
  -header-filter=.;
  -checks=${clang_tidy_checks};
  --warnings-as-errors=*;
  )

# warnings
if ( MSVC )
    set(WFLAGS "/W4 /WX")
else()
    set(WFLAGS "-Wall -Wextra -Wno-unused-function -Wno-unused-variable -Werror")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS} -Wvla")

# fatfs
if(NOT DEFINED USE_FF)
    set(USE_FF 0)
endif()

if(${USE_FF})
    file(GLOB_RECURSE dir "lib/FatFS/*.hpp" "lib/FatFS/*.h")
    set(fatfs_includes "")
    foreach(file_path ${dir})
        get_filename_component(dir_path ${file_path} PATH)
        set(fatfs_includes ${fatfs_includes} ${dir_path})
    endforeach()
    list(REMOVE_DUPLICATES fatfs_includes)

    file(GLOB_RECURSE fatfs_sources "lib/FatFS/*.cpp" "lib/FatFS/*.c" "lib/FatFS/*.h" "lib/FatFS/*.hpp")
    add_library(
        target_fatfs_objects
        OBJECT
        ${fatfs_sources}
        )
    target_include_directories(target_fatfs_objects PUBLIC ${fatfs_includes})
    set(target_objects ${target_objects} "$<TARGET_OBJECTS:target_fatfs_objects>")

    add_definitions(-DUSE_FF=1)
endif()

# Get the includes
file(GLOB_RECURSE dir "src/*.hpp" "src/*.h")
set(include_dirs "")
foreach(file_path ${dir})
    get_filename_component(dir_path ${file_path} PATH)
    set(include_dirs ${include_dirs} ${dir_path})
endforeach()
list(REMOVE_DUPLICATES include_dirs)

# Get sources
file(GLOB_RECURSE sources "src/*.cpp" "src/*.c" "src/*.h" "src/*.hpp")

add_library(
    target_db_objects
    OBJECT
    ${sources}
)

# Record cache
if(NOT DEFINED DB_RECORD_CACHE)
    set(DB_RECORD_CACHE 1)
endif()
target_compile_definitions(target_db_objects PRIVATE DB_RECORD_CACHE=${DB_RECORD_CACHE})

target_link_libraries(target_db_objects simple-msg)
target_include_directories(target_db_objects PUBLIC ${include_dirs} ${fatfs_includes})

if (${CLANG_TIDY})
    set_target_properties(
        target_db_objects
        PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}"
    )
endif()

set(target_objects ${target_objects} "$<TARGET_OBJECTS:target_db_objects>")

add_library(simple-db STATIC ${target_objects})
target_include_directories(simple-db PUBLIC ${include_dirs} ${fatfs_includes})
target_link_libraries(simple-db)
