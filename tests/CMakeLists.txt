# Self-contained unit tests for the phonetic engine. No external test framework
# is fetched (works fully offline); a tiny assertion harness lives in the test.
add_executable(engine_tests
  test_transliterator.cpp
)

target_link_libraries(engine_tests PRIVATE bnphonetic)

add_test(NAME engine_tests COMMAND engine_tests)
