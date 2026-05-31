# Portable phonetic transliteration engine. No Windows/COM dependencies so it
# can be unit-tested on its own and reused by the TIP, the test harness, etc.
add_library(bnphonetic STATIC
  src/Transliterator.cpp
)

target_include_directories(bnphonetic PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(bnphonetic PUBLIC cxx_std_17)
