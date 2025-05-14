add_library(nicolassql_parser
    parser.cpp
)
target_include_directories(nicolassql_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nicolassql_parser
    PUBLIC nicolassql_lexer
           nicolassql_ast
)

find_path(GTEST_INCLUDE_DIRS NAMES gtest/gtest.h)
find_library(GTEST_LIB NAMES gtest)
find_library(GTEST_MAIN_LIB NAMES gtest_main)
if (NOT GTEST_LIB OR NOT GTEST_MAIN_LIB OR NOT GTEST_INCLUDE_DIRS)
  message(FATAL_ERROR "Could not find GoogleTest – make sure it's installed")
endif()

include_directories(${GTEST_INCLUDE_DIRS})

add_executable(parser_tests
    parser_tests.cpp
)
target_link_libraries(parser_tests
    PRIVATE nicolassql_parser
            ${GTEST_LIB}
            ${GTEST_MAIN_LIB}
            pthread
)

include(GoogleTest)
gtest_discover_tests(parser_tests)

