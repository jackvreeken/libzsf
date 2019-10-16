cmake_minimum_required(VERSION 2.8)

project(libzsf)
enable_language(C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

option(USE_FAST_MATH "Enable fast math optimizations" OFF)
if(USE_FAST_MATH)
    if (MSVC)
        add_compile_options(/fp:fast)
    elseif((CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_C_COMPILER_ID MATCHES "GNU"))
        add_compile_options(-ffast-math)
    endif()
else()
    if (MSVC)
        add_compile_options(/fp:precise)
    endif()
endif()

option(USE_FAST_TANH "Enable fast tanh approximation" OFF)
if(USE_FAST_TANH)
    add_definitions(-DZSF_USE_FAST_TANH)
endif()

add_library(zsf SHARED src/zsf.c)

set_target_properties (zsf PROPERTIES
    DEFINE_SYMBOL "ZSF_EXPORTS"
    OUTPUT_NAME "zsf"
    PREFIX "lib"
    PUBLIC_HEADER "include/zsf.h"
)

add_library(zsf-static STATIC src/zsf.c)

set_target_properties(zsf-static PROPERTIES
    COMPILE_DEFINITIONS "ZSF_STATIC"
    OUTPUT_NAME "zsf-static"
    PREFIX "lib"
    PUBLIC_HEADER "include/zsf.h"
)

set(INSTALL_TARGETS zsf zsf-static)

# We also generate a 32-bits stdcall version for VBA
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits - do nothing
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_library(zsf-stdcall SHARED src/zsf.c)

    set_target_properties (zsf-stdcall PROPERTIES
        DEFINE_SYMBOL "ZSF_EXPORTS"
        COMPILE_DEFINITIONS "ZSF_USE_STDCALL"
        OUTPUT_NAME "zsf-stdcall"
        PREFIX "lib"
        PUBLIC_HEADER "include/zsf.h"
    )

    set(INSTALL_TARGETS ${INSTALL_TARGETS} zsf-stdcall)
endif()

install(
    TARGETS
    ${INSTALL_TARGETS})
