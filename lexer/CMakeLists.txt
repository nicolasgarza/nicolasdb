# build main lexer codee
add_library(nicolassql_lexer
    lexer.cpp
    lexer.h
)
target_include_directories(nicolassql_lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_path(GTEST_INCLUDE_DIRS
  NAMES gtest/gtest.h
  PATHS "${HOMEBREW_PREFIX}/Cellar/googletest/1.17.0/include"
)

find_library(GTEST_LIB
  NAMES gtest
  PATHS "${HOMEBREW_PREFIX}/Cellar/googletest/1.17.0/lib"
)

find_library(GTEST_MAIN_LIB
  NAMES gtest_main
  PATHS "${HOMEBREW_PREFIX}/Cellar/googletest/1.17.0/lib"
)

if (NOT GTEST_LIB OR NOT GTEST_MAIN_LIB OR NOT GTEST_INCLUDE_DIRS)
  message(FATAL_ERROR "Could not find GoogleTest in Homebrew Cellar – try FetchContent version instead")
endif()

include_directories(${GTEST_INCLUDE_DIRS})

add_executable(lexer_tests lexer_tests.cpp)
target_link_libraries(lexer_tests
  PRIVATE nicolassql_lexer
          ${GTEST_LIB} ${GTEST_MAIN_LIB} pthread
)

include(GoogleTest)
gtest_discover_tests(lexer_tests)
