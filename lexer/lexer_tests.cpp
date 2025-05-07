#include <gtest/gtest.h>
#include "lexer.h"
#include <algorithm>
#include <cctype>
#include <tuple>
#include <vector>
#include <string>
#include <memory>

using namespace nicolassql;

// simple trim both‐ends helper
static std::string trim(std::string s) {
  auto not_space = [](unsigned char c){ return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

TEST(TokenLexNumeric, ValidAndInvalidNumbers) {
    struct Test { bool ok; std::string src; };
    std::vector<Test> tests = {
        {true,  "105"},
        {true,  "105 "},
        {true,  "123."},
        {true,  "123.145"},
        {true,  "1e5"},
        {true,  "1.e21"},
        {true,  "1.1e2"},
        {true,  "1.1e-2"},
        {true,  "1.1e+2"},
        {true,  "1e-1"},
        {true,  ".1"},
        {true,  "4."},
        {false, "e4"},
        {false, "1.."},
        {false, "1ee4"},
        {false, " 1"},
    };

    for (auto& t : tests) {
        auto [tok, cur, ok] = lexNumeric(t.src, cursor{});
        EXPECT_EQ(t.ok, ok) << "input=" << t.src;
        if (ok) {
            // compare trimmed input to token->value
            EXPECT_EQ(trim(t.src), std::string(tok->value))
              << "input=" << t.src;
        }
    }
}

TEST(TokenLexString, ValidAndInvalidStrings) {
    struct Test { bool ok; std::string src; };
    std::vector<Test> tests = {
        {false, "a"},
        {true,  "'abc'"},
        {true,  "'a b'"},
        {true,  "'a' "},
        {true,  "'a '' b'"},
        {false, "'"},
        {false, ""},
        {false, " 'foo'"},
    };

    for (auto& t : tests) {
        auto [tok, cur, ok] = lexString(t.src, cursor{});
        EXPECT_EQ(t.ok, ok) << "input=" << t.src;
        if (ok) {
            auto s = trim(t.src);
            // strip leading and trailing quote
            auto inner = s.substr(1, s.size() - 2);
            EXPECT_EQ(inner, std::string(tok->value))
              << "input=" << t.src;
        }
    }
}

TEST(TokenLexSymbol, ValidAndInvalidSymbols) {
    struct Test { bool ok; std::string src; };
    std::vector<Test> tests = {
        {true,  "= "},
        {true,  "||"},
        {false, "@"},
        {false, ""},
    };

    for (auto& t : tests) {
        auto [tok, cur, ok] = lexSymbol(t.src, cursor{});
        EXPECT_EQ(t.ok, ok) << "input=" << t.src;
        if (ok) {
            EXPECT_EQ(trim(t.src), std::string(tok->value))
              << "input=" << t.src;
        }
    }
}

TEST(TokenLexIdentifier, ValidAndInvalidIdentifiers) {
    struct Test { bool ok; std::string src, expected; };
    std::vector<Test> tests = {
        {true,  "a",          "a"},
        {true,  "abc",        "abc"},
        {true,  "abc ",       "abc"},
        {true,  "\" abc \"",  " abc "},
        {true,  "a9$",        "a9$"},
        {true,  "userName",   "username"},
        {true,  "\"userName\"", "userName"},
        {false, "\"",         ""},
        {false, "_sadsfa",    ""},
        {false, "9sadsfa",    ""},
        {false, " abc",       ""},
    };

    for (auto& t : tests) {
        auto [tok, cur, ok] = lexIdentifier(t.src, cursor{});
        EXPECT_EQ(t.ok, ok) << "input=" << t.src;
        if (ok) {
            EXPECT_EQ(t.expected, std::string(tok->value))
              << "input=" << t.src;
        }
    }
}

TEST(TokenLexKeyword, ValidAndInvalidKeywords) {
    struct Test { bool ok; std::string src, expected; };
    std::vector<Test> tests = {
        {true,  "select ",  "select"},
        {true,  "from",     "from"},
        {true,  "as",       "as"},
        {true,  "SELECT",   "select"},
        {true,  "into",     "into"},
        {false, " into",    ""},
        {false, "flubbrety",""},
    };

    for (auto& t : tests) {
        auto [tok, cur, ok] = lexKeyword(t.src, cursor{});
        EXPECT_EQ(t.ok, ok) << "input=" << t.src;
        if (ok) {
            EXPECT_EQ(t.expected, std::string(tok->value))
              << "input=" << t.src;
        }
    }
}

TEST(Lex, FullSequences) {
    struct Expected {
        std::string input;
        struct Tok { tokenKind kind; std::string val; uint64_t line, col; };
        std::vector<Tok> toks;
    };

    std::vector<Expected> tests = {
      { "select a",
        {{ tokenKind::keywordKind,    "select", 0, 0 },
         { tokenKind::identifierKind, "a",      0, 7 }} },
      { "select true",
        {{ tokenKind::keywordKind,    "select", 0, 0 },
         { tokenKind::identifierKind, "true",   0, 7 }} },
      { "select 1",
        {{ tokenKind::keywordKind,    "select", 0, 0 },
         { tokenKind::numericKind,    "1",      0, 7 }} },
      { "select 'foo' || 'bar';",
        {{ tokenKind::keywordKind,    "select",    0, 0 },
         { tokenKind::stringKind,     "foo",       0, 7 },
         { tokenKind::symbolKind,     "||",        0,13 },
         { tokenKind::stringKind,     "bar",       0,16 },
         { tokenKind::symbolKind,     ";",         0,21 }} },
      { "CREATE TABLE u (id INT, name TEXT)",
        {{ tokenKind::keywordKind,    "create",    0, 0 },
         { tokenKind::keywordKind,    "table",     0, 7 },
         { tokenKind::identifierKind, "u",         0,13 },
         { tokenKind::symbolKind,     "(",         0,15 },
         { tokenKind::identifierKind, "id",        0,16 },
         { tokenKind::keywordKind,    "int",       0,19 },
         { tokenKind::symbolKind,     ",",         0,22 },
         { tokenKind::identifierKind, "name",      0,24 },
         { tokenKind::keywordKind,    "text",      0,29 },
         { tokenKind::symbolKind,     ")",         0,33 }} },
      { "insert into users Values (105, 233)",
        {{ tokenKind::keywordKind,    "insert",    0, 0 },
         { tokenKind::keywordKind,    "into",      0, 7 },
         { tokenKind::identifierKind, "users",     0,12 },
         { tokenKind::keywordKind,    "values",    0,18 },
         { tokenKind::symbolKind,     "(",         0,25 },
         { tokenKind::numericKind,    "105",       0,26 },
         { tokenKind::symbolKind,     ",",         0,29 },
         { tokenKind::numericKind,    "233",       0,31 },
         { tokenKind::symbolKind,     ")",         0,34 }} },
      { "SELECT id FROM users;",
        {{ tokenKind::keywordKind,    "select",    0, 0 },
         { tokenKind::identifierKind, "id",        0, 7 },
         { tokenKind::keywordKind,    "from",      0,10 },
         { tokenKind::identifierKind, "users",     0,15 },
         { tokenKind::symbolKind,     ";",         0,20 }} },
    };

    for (auto& tc : tests) {
        auto [tokens, err] = lex(tc.input);
        EXPECT_TRUE(err.empty()) << "input=" << tc.input;
        ASSERT_EQ(tc.toks.size(), tokens.size()) << "input=" << tc.input;
        for (size_t i = 0; i < tokens.size(); ++i) {
            EXPECT_EQ(tc.toks[i].kind, tokens[i]->kind)
              << "input="<<tc.input<<" idx="<<i;
            EXPECT_EQ(tc.toks[i].val, std::string(tokens[i]->value))
              << "input="<<tc.input<<" idx="<<i;
            EXPECT_EQ(tc.toks[i].line, tokens[i]->loc.line)
              << "input="<<tc.input<<" idx="<<i;
            EXPECT_EQ(tc.toks[i].col,  tokens[i]->loc.col)
              << "input="<<tc.input<<" idx="<<i;
        }
        // clean up
        for (auto* p : tokens) delete p;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

