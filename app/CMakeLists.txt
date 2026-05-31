# Bangla Phonetic TSF Text Input Processor (COM in-proc DLL).
# Build with: cmake -B build-x64 -DBUILD_TIP=ON   (then also build x86 for
# 32-bit host apps; ship both).
add_library(BanglaPhonetic SHARED
  src/dllmain.cpp
  src/Globals.cpp
  src/ClassFactory.cpp
  src/TextService.cpp
  src/DisplayAttribute.cpp
  src/Register.cpp
  BanglaPhonetic.def
)

target_include_directories(BanglaPhonetic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(BanglaPhonetic PRIVATE
  bnphonetic
  ole32
  advapi32
  user32
)

set_target_properties(BanglaPhonetic PROPERTIES
  OUTPUT_NAME "BanglaPhonetic"
)
