cmake_minimum_required(VERSION 3.6)

project(HubCppMessages C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(NOT ${Python3_FOUND})
    message(FATAL_ERROR "Python3 is required to generate the message classes")
endif()

find_program(ClangFormat_EXECUTABLE NAMES clang-format)
find_package_handle_standard_args(ClangFormat REQUIRED_VARS ClangFormat_EXECUTABLE)

if(NOT ${ClangFormat_FOUND})
    message(WARNING "clang-format not found, will not format generated messages")
endif()

if (NOT DEFINED HUB_MSG_SCHEMA_DIR)
    set(HUB_MSG_SCHEMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test-schemas")
endif()

set(GENERATED_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/generated")

set(MESSAGE_GENERATE_COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/message_generator.py"
    "${HUB_MSG_SCHEMA_DIR}"
    "${GENERATED_CODE_DIR}"
    )

execute_process(
    COMMAND ${MESSAGE_GENERATE_COMMAND} --dry-run
    OUTPUT_VARIABLE GENERATED_FILES
    RESULT_VARIABLE RETURN_VALUE
)
if (NOT RETURN_VALUE EQUAL 0)
    message(FATAL_ERROR "Failed to get the generated message source list from schemas in: \n" ${HUB_MSG_SCHEMA_DIR})
endif()

set(GENERATOR_CODE "")
file(GLOB_RECURSE GENERATOR_CODE "${CMAKE_CURRENT_SOURCE_DIR}/generator/*")

set(SCHEMA_FILES "")
file(GLOB_RECURSE SCHEMA_FILES "${HUB_MSG_SCHEMA_DIR}/*.json")

add_custom_command(
    COMMAND ${MESSAGE_GENERATE_COMMAND}
    DEPENDS ${SCHEMA_FILES} ${GENERATOR_CODE}
    OUTPUT ${GENERATED_FILES}
    COMMENT "Generating the messages"
)

# Re run cmake when the message schemas change
set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${SCHEMA_FILES} ${GENERATOR_CODE}
)

set_source_files_properties(${GENERATED_FILES} GENERATED)

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
      -bugprone-macro-parentheses

      readability-*
      -readability-convert-member-functions-to-static
      -readability-make-member-function-const
      -readability-magic-numbers
      -readability-implicit-bool-conversion
      -readability-isolate-declaration
      -readability-braces-around-statements
      -readability-else-after-return
      -readability-misleading-indentation # clang-tidy doesn't understand constexpr
    )

string(JOIN "," clang_tidy_checks ${clang_tidy_checks})

set(CLANG_TIDY_ARGS
  clang-tidy;
  -header-filter=.;
  -checks=${clang_tidy_checks};
  --warnings-as-errors=*;
  )

# warnings
set(WFLAGS "")

if ( MSVC )
    set(WFLAGS "/W4 /WX")
else()
    set(WFLAGS "-Wall -Wextra -Wno-unused-function -Wno-unused-variable -Werror")    
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WFLAGS}")

# includes
file(GLOB_RECURSE HEADERS "./src/*.hpp" "./src/*.h")
set(INCLUDES "")
foreach(FILE_PATH ${HEADERS})
    get_filename_component(DIR_PATH ${FILE_PATH} PATH)
    set(INCLUDES ${INCLUDES} ${DIR_PATH})
endforeach()

# generated headers
foreach(FILE_PATH ${GENERATED_FILES})
    if(${FILE_PATH} MATCHES ".*\\.hpp$" OR ${FILE_PATH} MATCHES ".*\\.h$")
        get_filename_component(DIR_PATH ${FILE_PATH} PATH)
        set(INCLUDES ${INCLUDES} ${DIR_PATH})
    endif()
endforeach()

list(REMOVE_DUPLICATES INCLUDES)


# sources
set(SOURCES "")
file(GLOB_RECURSE SOURCES "./src/*.cpp" "./src/*.c" "./src/*.h" "./src/*.hpp")

add_library(simple-msg STATIC ${SOURCES} ${GENERATED_FILES})
target_include_directories(simple-msg PUBLIC ${INCLUDES})

if (${CLANG_TIDY})
    set_target_properties(
        simple-msg
        PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}"
        C_CLANG_TIDY "${CLANG_TIDY_ARGS}"
        )
endif()
