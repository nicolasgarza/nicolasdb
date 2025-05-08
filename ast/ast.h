#include <vector>
#include "../lexer/lexer.h"

namespace ast {


enum class AstKind : uint64_t {
	SelectKind = 0,
	CreateTableKind,
	InsertKind,
};

enum class expressionKind : uint64_t {
	literalKind = 0
};

struct expression {
	nicolassql::token* literal;
	expressionKind kind;
};

struct columnDefinition {
	nicolassql::token name;
	nicolassql::token datatype;
};

struct CreateTableStatement {
	nicolassql::token name;
	std::vector<columnDefinition*>* cols;
};

struct SelectStatement {
	std::vector<expression*> item;
	nicolassql::token from;
};

struct InsertStatement {
	nicolassql::token table;
	std::vector<expression*>* values;
};

struct Statement {
	SelectStatement* SelectStatement;
	CreateTableStatement* CreateTableStatement;
	InsertStatement* InsertStatement;
	AstKind Kind;
};

struct Ast {
	std::vector<Statement*> Statements;
};

}
