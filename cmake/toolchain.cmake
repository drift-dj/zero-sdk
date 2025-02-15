set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Use the system installed clang unless CUSTOM_CLANG_PATH is set
if(CUSTOM_CLANG_PATH)
  set(CLANG_PATH ${CUSTOM_CLANG_PATH})
  set(CLANGXX_PATH ${CUSTOM_CLANG_PATH})
else()
  find_program(CLANG_PATH clang)
  find_program(CLANGXX_PATH clang++)
endif()

if(CLANG_PATH AND CLANGXX_PATH)
  set(CMAKE_C_COMPILER ${CLANG_PATH} CACHE PATH "" FORCE)
  set(CMAKE_CXX_COMPILER ${CLANGXX_PATH} CACHE PATH "" FORCE)
else()
  message(FATAL_ERROR "System clang not found. Please install clang.")
endif()

# Ensure clang --target options is respected
set(CMAKE_C_COMPILER_TARGET aarch64-linux-gnu)
set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-gnu)

# Silence a false negative during the compiler test phase
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
