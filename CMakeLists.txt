cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0135 NEW)
project(TelemetryAnalyzer VERSION 1.0 LANGUAGES CXX)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Header include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------
# Static library: telemetry_core (contains parser and analyzer)
# ---------------------------------------------------------------------
# Add static library with source files
add_library(telemetry_core STATIC
    src/memory_mapped_file.cpp
    src/telemetry_analyzer.cpp
    src/telemetry_parser.cpp
    src/csv_parser.cpp
)
# Export include path for consumers
target_include_directories(telemetry_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------
# Executable: telemetry_analyzer (main program)
# ---------------------------------------------------------------------
add_executable(telemetry_analyzer src/main.cpp)
find_package(Threads REQUIRED)
    target_link_libraries(telemetry_analyzer PRIVATE telemetry_core Threads::Threads)

# ---------------------------------------------------------------------
# GoogleTest via FetchContent
# ---------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Use same CRT on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ---------------------------------------------------------------------
# Test executable: run_tests
# ---------------------------------------------------------------------
# Test executable
add_executable(telemetry_test tests/test_telemetry.cpp)
target_link_libraries(telemetry_test PRIVATE telemetry_core gtest_main)
target_include_directories(telemetry_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_test(NAME telemetry_test COMMAND $<TARGET_FILE:telemetry_test>)

# Adicionar o gerador de dados como um executavel utilitario
add_executable(generate_data tools/generate_data.cpp)