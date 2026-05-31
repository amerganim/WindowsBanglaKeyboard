cmake_minimum_required(VERSION 3.20)
project(WindowsBanglaKeyboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # /utf-8 is essential: our source contains Bangla UTF-8 string literals.
  add_compile_options(/utf-8 /W4 /permissive-)
endif()

enable_testing()

add_subdirectory(engine)
add_subdirectory(tests)

# The TSF Text Input Processor (Phase 0/3). It is a Windows-only COM DLL and is
# off by default so the portable engine + tests build everywhere. Enable with:
#   cmake -B build -DBUILD_TIP=ON
option(BUILD_TIP "Build the TSF Text Input Processor DLL (Windows only)" OFF)
if(WIN32 AND BUILD_TIP)
  add_subdirectory(app)
endif()
