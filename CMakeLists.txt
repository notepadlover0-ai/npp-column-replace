cmake_minimum_required(VERSION 3.20)

project(ColumnReplace LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ColumnReplace SHARED
    src/ColumnReplace.cpp
    src/ColumnReplaceDialog.cpp
    src/dllmain.cpp
    src/plugin.rc
    src/ColumnReplace.def
)

target_include_directories(ColumnReplace PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_definitions(ColumnReplace PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(ColumnReplace PRIVATE user32 gdi32 comctl32)

set_target_properties(ColumnReplace PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ColumnReplace"
)

if(MSVC)
    target_compile_options(ColumnReplace PRIVATE /W4 /utf-8 /EHsc /permissive-)
    target_link_options(ColumnReplace PRIVATE /INCREMENTAL:NO)
elseif(MINGW)
    target_compile_options(ColumnReplace PRIVATE -Wall -Wextra -Wpedantic -O2)
    target_link_options(ColumnReplace PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        -Wl,--subsystem,windows
        -Wl,--no-undefined
    )
    string(APPEND CMAKE_RC_FLAGS " --codepage=65001")
endif()
