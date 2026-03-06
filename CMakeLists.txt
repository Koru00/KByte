cmake_minimum_required(VERSION 3.16)

project(RomBuilder LANGUAGES C CXX)

# ---------------------------
# Compiler specific settings
# ---------------------------

if (MSVC)

    # MSVC Debug flags
    add_compile_options(
        $<$<CONFIG:Debug>:/Od>     # Disable optimizations
        $<$<CONFIG:Debug>:/Zi>     # Debug symbols
    )

    # MSVC Release flags
    add_compile_options(
        $<$<CONFIG:Release>:/Ox>   # Full optimization
        $<$<CONFIG:Release>:/Ob3>  # Aggressive inlining
        $<$<CONFIG:Release>:/GL>   # Link time optimization
        $<$<CONFIG:Release>:/DNDEBUG>
    )

else()

    # GCC / Clang flags (Linux, MacOS, etc)

    add_compile_options(
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>

        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>
    )

endif()

# ---------------------------
# Subprojects
# ---------------------------

add_subdirectory(RomBuilder)