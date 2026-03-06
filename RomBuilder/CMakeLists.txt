# CMakeList.txt : CMake project for RomBuilder, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (RomBuilder "RomBuilder.cpp" "RomBuilder.h" "bytecode.h" "register.h" "bytecode.cpp" "vm.h" "runtime.cpp" "runtime.h" "system_call.h" "assembler.cpp" "assembler.h" "interpreter.cpp" "interpreter.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RomBuilder PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
