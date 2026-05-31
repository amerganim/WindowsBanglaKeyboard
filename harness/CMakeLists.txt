# Standalone console harness for exercising the phonetic engine (Phase 2).
add_executable(bnphonetic_harness
  main.cpp
)

target_link_libraries(bnphonetic_harness PRIVATE bnphonetic)
