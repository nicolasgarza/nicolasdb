#include <gtest/gtest.h>
#include "parser.h"
#include "../ast/ast.h"

using namespace parser;
using namespace nicolassql;
using namespace ast;

TEST(ParserTest, InsertStatement) {
    auto [astPtr, err] = Parse("INSERT INTO users VALUES (105, 233)");
    ASSERT_TRUE(err.empty()) << "Parse error: " << err;
    ASSERT_EQ(astPtr->Statements.size(), 1u);

    auto* stmt = astPtr->Statements[0].get();
    EXPECT_EQ(stmt->Kind, AstKind::InsertKind);

    auto* ins = stmt->InsertStatement;
    ASSERT_NE(ins, nullptr);
    EXPECT_EQ(ins->table.value, "users");

    auto& vals = *ins->values;
    ASSERT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0]->literal->value, "105");
    EXPECT_EQ(vals[1]->literal->value, "233");
}

TEST(ParserTest, CreateTableStatement) {
    auto [astPtr, err] = Parse("CREATE TABLE users (id INT, name TEXT)");
    ASSERT_TRUE(err.empty()) << "Parse error: " << err;
    ASSERT_EQ(astPtr->Statements.size(), 1u);

    auto* stmt = astPtr->Statements[0].get();
    EXPECT_EQ(stmt->Kind, AstKind::CreateTableKind);

    auto* crt = stmt->CreateTableStatement;
    ASSERT_NE(crt, nullptr);
    EXPECT_EQ(crt->name.value, "users");

    auto& cols = *crt->cols;
    ASSERT_EQ(cols.size(), 2u);
    EXPECT_EQ(cols[0]->name.value, "id");
    EXPECT_EQ(cols[0]->datatype.value, "int");
    EXPECT_EQ(cols[1]->name.value, "name");
    EXPECT_EQ(cols[1]->datatype.value, "text");
}

TEST(ParserTest, SelectColumnsAndFrom) {
    // NOTE: the C++ parser currently only recognizes bare identifiers in SELECT,
    //       it does not yet handle '*' or 'AS' aliases.
    auto [astPtr, err] = Parse("SELECT id, name FROM users");
    ASSERT_TRUE(err.empty()) << "Parse error: " << err;
    ASSERT_EQ(astPtr->Statements.size(), 1u);

    auto* stmt = astPtr->Statements[0].get();
    EXPECT_EQ(stmt->Kind, AstKind::SelectKind);

    auto* sl = stmt->SelectStatement;
    ASSERT_NE(sl, nullptr);

    // sl->item is a std::vector<std::unique_ptr<expression>>
    auto& items = sl->item;
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0]->literal->value, "id");
    EXPECT_EQ(items[1]->literal->value, "name");

    // sl->from is a token, not a pointer
    EXPECT_EQ(sl->from.value, "users");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

