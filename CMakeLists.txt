cmake_minimum_required(VERSION 3.15)
project(amarantine VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g -march=native")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Zi")
endif()

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================================
# Third Party Libraries for Benchmarking
# ============================================================================

# Check third_party directory
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# Detect fetch script for manual use only
if(WIN32)
    set(FETCH_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fetch_libs.ps1")
    set(FETCH_CMD powershell -ExecutionPolicy Bypass -File)
else()
    set(FETCH_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fetch_libs.sh")
    find_program(BASH bash)
    if(BASH)
        set(FETCH_CMD bash)
    else()
        set(FETCH_CMD sh)
    endif()
endif()

set(USE_SYSTEM_RE2 OFF CACHE BOOL "Use system-installed RE2 instead of third_party")
set(USE_SYSTEM_PCRE2 OFF CACHE BOOL "Use system-installed PCRE2 instead of third_party")
set(USE_SYSTEM_HYPERSCAN OFF CACHE BOOL "Use system-installed Hyperscan instead of third_party")

# ============================================================================
# RE2 (if available in third_party or system)
# ============================================================================
set(RE2_FOUND FALSE)
set(RE2_TARGET "")
# Only check if re2/re2.h exists (indicates submodule is initialized)
if(EXISTS "${THIRD_PARTY_DIR}/re2/re2.h" OR EXISTS "${THIRD_PARTY_DIR}/re2/src/re2/re2.h")
    message(STATUS "Found RE2 in third_party")

    set(RE2_LIB_DIR "${THIRD_PARTY_DIR}/re2/obj")

    # Check if RE2 is already built
    if(NOT EXISTS "${RE2_LIB_DIR}/libre2.a" AND NOT EXISTS "${RE2_LIB_DIR}/libre2.so")
        message(STATUS "Building RE2 with CMake+Ninja...")

        # Configure RE2 with CMake - find system Abseil
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RE2_LIB_DIR}"
            RESULT_VARIABLE RE2_MKDIR_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )
        if(RE2_MKDIR_RESULT EQUAL 0)
            execute_process(
                COMMAND ${CMAKE_COMMAND} .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_FIND_APPBUNDLE=LAST -G Ninja
                WORKING_DIRECTORY "${RE2_LIB_DIR}"
                RESULT_VARIABLE RE2_CONFIG_RESULT
                OUTPUT_QUIET ERROR_QUIET
            )
            if(RE2_CONFIG_RESULT EQUAL 0)
                execute_process(
                    COMMAND ninja
                    WORKING_DIRECTORY "${RE2_LIB_DIR}"
                    RESULT_VARIABLE RE2_BUILD_RESULT
                    OUTPUT_QUIET ERROR_QUIET
                )
                if(RE2_BUILD_RESULT EQUAL 0)
                    message(STATUS "RE2 built successfully")
                else()
                    message(WARNING "Failed to build RE2: ninja failed")
                endif()
            else()
                message(WARNING "Failed to build RE2: CMake configuration failed")
            endif()
        else()
            message(WARNING "Failed to create build directory for RE2")
        endif()
    else()
        message(STATUS "RE2 already built at ${RE2_LIB_DIR}")
    endif()

    # Add compile definition and include path (only if library exists)
    if(EXISTS "${RE2_LIB_DIR}/libre2.a" OR EXISTS "${RE2_LIB_DIR}/libre2.so")
        # Now use find_package to properly get RE2 and its dependencies
        set(CMAKE_PREFIX_PATH "${RE2_LIB_DIR};${CMAKE_PREFIX_PATH}")
        find_package(absl QUIET)
        if(absl_FOUND)
            add_compile_definitions(HAVE_RE2)
            include_directories("${THIRD_PARTY_DIR}/re2")
            list(APPEND BENCHMARK_LIBS "${RE2_LIB_DIR}/libre2.a")
            list(APPEND BENCHMARK_LIBS absl::base absl::strings absl::status absl::log absl::synchronization absl::str_format absl::flags absl::hash)
            set(RE2_FOUND TRUE)
            message(STATUS "RE2 + Abseil properly configured")
        else()
            message(WARNING "Abseil not found, RE2 support disabled")
        endif()
    else()
        message(WARNING "RE2 library not found, RE2 support disabled")
    endif()
elseif(USE_SYSTEM_RE2)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(RE2 re2)
        if(RE2_FOUND)
            message(STATUS "Found system RE2")
            add_compile_definitions(HAVE_RE2)
            include_directories(${RE2_INCLUDE_DIRS})
            link_directories(${RE2_LIBRARY_DIRS})
            list(APPEND BENCHMARK_LIBS ${RE2_LIBRARIES})
            set(RE2_FOUND TRUE)
        endif()
    endif()
endif()
if(NOT RE2_FOUND)
    message(STATUS "RE2 not found - place it in third_party/re2 for benchmarking")
endif()

# ============================================================================
# PCRE2 (from third_party only)
# ============================================================================
set(PCRE2_FOUND FALSE)
# Only enable if both library and header exist
if(EXISTS "${THIRD_PARTY_DIR}/pcre2/build/libpcre2-8.a" OR EXISTS "${THIRD_PARTY_DIR}/pcre2/build/libpcre2-8.so")
    if(EXISTS "${THIRD_PARTY_DIR}/pcre2/src/pcre2.h" OR EXISTS "${THIRD_PARTY_DIR}/pcre2/src/pcre2.h.in")
        message(STATUS "Found built PCRE2 in third_party")
        add_compile_definitions(HAVE_PCRE2)
        include_directories("${THIRD_PARTY_DIR}/pcre2/src")
        list(APPEND BENCHMARK_LIBS "${THIRD_PARTY_DIR}/pcre2/build/libpcre2-8.a")
        set(PCRE2_FOUND TRUE)
    endif()
endif()
if(NOT PCRE2_FOUND)
    message(STATUS "PCRE2 not found or not built - run 'git submodule update --init --recursive' and fetch script")
endif()

# ============================================================================
# CTRE (Compile Time Regular Expression from third_party)
# ============================================================================
set(CTRE_FOUND FALSE)
# Only enable if ctre.hpp actually exists and contains content
if(EXISTS "${THIRD_PARTY_DIR}/ctre/include/ctre.hpp")
    # Check file size to ensure it's not empty (empty submodule placeholder)
    file(SIZE "${THIRD_PARTY_DIR}/ctre/include/ctre.hpp" CTHRE_SIZE)
    if(CTHRE_SIZE GREATER 100)
        message(STATUS "Found CTRE in third_party")
        add_compile_definitions(HAVE_CTRE)
        include_directories("${THIRD_PARTY_DIR}/ctre/include")
        set(CTRE_FOUND TRUE)
    endif()
endif()
if(NOT CTRE_FOUND)
    message(STATUS "CTRE not found - run 'git submodule update --init --recursive'")
endif()

# ============================================================================
# Examples
# ============================================================================

add_executable(amarantine_demo examples/amarantine_demo.cc)
add_executable(simple_demo examples/simple_demo.cc)

# ============================================================================
# Benchmarks
# ============================================================================

add_executable(benchmark benchmarks/benchmark.cc)

# Link regex libraries to benchmark if found
if(DEFINED BENCHMARK_LIBS)
    target_link_libraries(benchmark PRIVATE ${BENCHMARK_LIBS})
endif()

# ============================================================================
# Tests
# ============================================================================

enable_testing()

add_executable(test_simple tests/test_simple.cc)
add_executable(test_compile tests/test_compile.cc)
add_executable(test_debug tests/test_debug.cc)
add_executable(test_bytecode tests/test_bytecode.cc)
add_executable(test_trace tests/test_trace.cc)

add_test(NAME SimpleTest COMMAND test_simple)
add_test(NAME CompileTest COMMAND test_compile)
add_test(NAME DebugTest COMMAND test_debug)
add_test(NAME BytecodeTest COMMAND test_bytecode)
add_test(NAME TraceTest COMMAND test_trace)

# Create test suite
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_simple test_compile test_debug test_bytecode test_trace
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
# Installation
# ============================================================================

# Install the main header
install(FILES include/amaranth/amaranth.h DESTINATION include/amaranth)

# Install examples and benchmark (optional)
install(TARGETS simple_demo amarantine_demo benchmark DESTINATION bin)

# Remove cmake config files to avoid install issues
# Users can include amaranth.h directly in their projects

# Add uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Uninstalling Amarantine..."
    )
endif()

# ============================================================================
# Code Formatting (clang-format)
# ============================================================================

# Find clang-format
find_program(CLANG_FORMAT clang-format)
mark_as_advanced(CLANG_FORMAT)

# Collect all C++ source/header files at configure time
file(GLOB_RECURSE AMARANTINE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h
)
file(GLOB_RECURSE AMARANTINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc
)

# Combine into a single list for formatting
set(FORMAT_FILES ${AMARANTINE_HEADERS} ${AMARANTINE_SOURCES})

if(CLANG_FORMAT)
    message(STATUS "Found clang-format: ${CLANG_FORMAT}")

    # Sort files for consistent behavior
    list(SORT FORMAT_FILES)

    # Format target - runs clang-format on all C++ source files
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${FORMAT_FILES}
        COMMENT "Running clang-format to format source code"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )

    # Check format target - runs clang-format --dry-run to check formatting
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${FORMAT_FILES}
        COMMENT "Checking code formatting with clang-format"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )

    message(STATUS "Added custom targets: 'make format' and 'make check-format'")
else()
    message(STATUS "clang-format not found - formatting targets not available")
    message(STATUS "  Install clang-format to enable: make format / make check-format")
endif()

# ============================================================================
# Fetch Third-Party Libraries
# ============================================================================

# Detect which script to use based on platform
if(WIN32)
    # On Windows, use PowerShell script
    set(FETCH_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fetch_libs.ps1")
    set(FETCH_CMD powershell -ExecutionPolicy Bypass -File)
else()
    # On Unix-like systems, use bash script
    set(FETCH_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fetch_libs.sh")
    # Check if bash is available, otherwise fall back to sh
    find_program(BASH bash)
    if(BASH)
        set(FETCH_CMD bash)
    else()
        set(FETCH_CMD sh)
    endif()
endif()

# Create a target to fetch third-party libraries
if(EXISTS "${FETCH_SCRIPT}")
    add_custom_target(fetch-third-party
        COMMAND ${FETCH_CMD} "${FETCH_SCRIPT}"
        COMMENT "Fetching third-party libraries for benchmarking..."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Create a convenience alias
    add_custom_target(deps DEPENDS fetch-third-party)

    message(STATUS "Added custom targets: 'make fetch-third-party' and 'make deps'")
else()
    message(STATUS "Fetch script not found at: ${FETCH_SCRIPT}")
    message(STATUS "  Download libraries manually to third_party/ directory")
endif()

# Add a message on configure about available helpers
message(STATUS "")
message(STATUS "Available targets (using ninja):")
message(STATUS "  ninja                      - Build all targets")
message(STATUS "  ninja check               - Run all tests")
message(STATUS "  ninja install             - Install amarantine to system")
message(STATUS "  ninja uninstall           - Uninstall amarantine from system")
if(CLANG_FORMAT)
message(STATUS "  ninja format              - Run clang-format on all sources")
message(STATUS "  ninja check-format        - Check code formatting")
endif()
if(EXISTS "${FETCH_SCRIPT}")
message(STATUS "  ninja fetch-third-party   - Download third-party benchmark libraries")
message(STATUS "  ninja deps                - Alias for fetch-third-party")
endif()
message(STATUS "")
