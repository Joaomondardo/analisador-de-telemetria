cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0135 NEW)
project(TelemetryAnalyzer VERSION 1.0 LANGUAGES CXX)

# Configurações do Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Configurações do Compilador
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    add_compile_options(/bigobj /W0) # /W0 para silenciar avisos, /bigobj para grandes projetos
endif()

# Diretórios de include globais
include_directories(${PROJECT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------
# Static library: telemetry_core
# ---------------------------------------------------------------------
add_library(telemetry_core STATIC
    src/memory_mapped_file.cpp
    src/telemetry_analyzer.cpp
    src/telemetry_parser.cpp
    src/csv_parser.cpp
    src/math_channel_evaluator.cpp
    # Adicionando headers para permitir que o CMake os rastreie
    include/memory_mapped_file.hpp
    include/telemetry_analyzer.hpp
    include/telemetry_parser.hpp
    include/csv_parser.hpp
    include/math_channel_evaluator.hpp
    include/telemetry_record.hpp
    include/downsampler.hpp
)

target_include_directories(telemetry_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_options(telemetry_core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-mavx2>
)

# ---------------------------------------------------------------------
# Executable: telemetry_analyzer (console)
# ---------------------------------------------------------------------
add_executable(telemetry_analyzer src/main.cpp)
find_package(Threads REQUIRED)
target_link_libraries(telemetry_analyzer PRIVATE telemetry_core Threads::Threads)

# ---------------------------------------------------------------------
# GoogleTest
# ---------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

add_executable(telemetry_test tests/test_telemetry.cpp tests/test_math_channels.cpp tests/test_downsampler.cpp)
target_link_libraries(telemetry_test PRIVATE telemetry_core gtest_main)
target_include_directories(telemetry_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME telemetry_test COMMAND $<TARGET_FILE:telemetry_test>)

# ---------------------------------------------------------------------
# GUI Executable: telemetry_gui
# ---------------------------------------------------------------------
if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/QtSDK/6.7.3/msvc2022_64/lib/cmake" CACHE PATH "Path to Qt6")
endif()

find_package(Qt6 6.0 CONFIG REQUIRED COMPONENTS Core Widgets Charts)

add_executable(telemetry_gui 
    src/gui_main.cpp 
    src/mainwindow.cpp 
    src/telemetry_plot_widget.cpp
    # IMPORTANTÍSSIMO: Adicionar os headers aqui resolve os erros LNK2001
    include/mainwindow.hpp
    include/telemetry_plot_widget.hpp
)

target_link_libraries(telemetry_gui PRIVATE 
    telemetry_core 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Charts 
    Threads::Threads
)

target_include_directories(telemetry_gui PRIVATE ${PROJECT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------
# Utility: generate_data
# ---------------------------------------------------------------------
add_executable(generate_data tools/generate_data.cpp)