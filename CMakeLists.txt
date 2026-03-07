cmake_minimum_required(VERSION 3.16)

project(RomBuilder LANGUAGES C CXX)

# --------------------------------
# Global compiler settings
# --------------------------------

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------
# Warnings (very important)
# --------------------------------

if (MSVC)

    add_compile_options(
        /W4
        /permissive-
    )

else()

    add_compile_options(
        -Wpedantic
        -fdiagnostics-show-line-numbers
        -fdiagnostics-show-option
    )

endif()

# --------------------------------
# Optimization / Debug flags
# --------------------------------

if (MSVC)

    set(CMAKE_C_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")

    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /Zi")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi")

else()

    set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fsanitize=address,undefined")

    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fdiagnostics-color=always)
endif()

# --------------------------------
# Subprojects
# --------------------------------

add_subdirectory(RomBuilder)
