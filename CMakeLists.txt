cmake_minimum_required(VERSION 2.8)

project(libzsf)
enable_language(C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(zsf SHARED src/zsf.c)

set_target_properties (zsf PROPERTIES
    DEFINE_SYMBOL "ZSF_EXPORTS"
    OUTPUT_NAME "zsf"
    PREFIX "lib"
)

add_library(zsf-static STATIC src/zsf.c)

set_target_properties(zsf-static PROPERTIES
    COMPILE_DEFINITIONS "ZSF_STATIC"
    OUTPUT_NAME "zsf"
    PREFIX "lib"
)

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
    )
endif()
