#include "../lexer/lexer.h"
#include "../ast/ast.h"
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <vector>

namespace parser {

using namespace nicolassql;

std::tuple<std::unique_ptr<ast::Statement>, uint64_t, bool> parseStatement(
    const std::vector<token*>& tokens, 
    uint64_t initialCursor, 
    token delimiter);

std::tuple<std::unique_ptr<ast::SelectStatement>, uint64_t, bool> parseSelectStatement(
    const std::vector<token*>& tokens, 
    uint64_t initialCursor, 
    token delimiter);

std::tuple<std::unique_ptr<std::vector<std::unique_ptr<ast::expression>>>, uint64_t, bool> parseExpressions(
    const std::vector<token*>& tokens, 
    uint64_t initialCursor, 
    const std::vector<token>& delimiters);

std::tuple<std::unique_ptr<ast::InsertStatement>, uint64_t, bool> parseInsertStatement(
	const std::vector<token*>& tokens,
	uint64_t initialCursor,
	token delimiter);

std::tuple<std::unique_ptr<ast::CreateTableStatement>, uint64_t, bool> parseCreateTableStatement(
	const std::vector<token*>& tokens,
	uint64_t initialCursor,
	token delimiter);

token tokenFromKeyword(keyword k) {
	return token{
		.kind = tokenKind::keywordKind,
		.value = std::string(k),
	};
}

token tokenFromSymbol(symbol s) {
	return token{
		.kind = tokenKind::symbolKind,
		.value = std::string(s),
	};
}

bool expectToken(const std::vector<token*>& tokens, uint64_t cursor, const token& t) {
	if (cursor >= tokens.size()) {
		return false;
	}

	return t.equals(*tokens[cursor]);
}

void helpMessage(const std::vector<token*>& tokens, uint64_t cursor, std::string msg) {
	token* c;
	if (cursor < tokens.size()) {
		c = tokens[cursor];
	} else {
		c = tokens[cursor - 1];
	}

	std::printf("[%llu,%llu]: %s, got: %s\n", c->loc.line, c->loc.col, msg.c_str(), c->value.c_str());
}

std::tuple<std::unique_ptr<ast::Ast>, std::string> Parse(std::string source) {
    auto [tokens, err] = lex(source);
	if (err != "") {
		return {nullptr, err};
	}

	if (!tokens.empty()) {
		token semiTok = tokenFromSymbol(semicolonSymbol);
		token* lastTok = tokens.back();
		if (!semiTok.equals(*lastTok)) {
			tokens.push_back(new token {semiTok});
		}
	}

	ast::Ast a{};
	uint64_t cursor = 0;
	while (cursor < tokens.size()) {
		auto [stmt, newCursor, ok] = parseStatement(tokens, cursor, tokenFromSymbol(semicolonSymbol));
		if (!ok) {
			helpMessage(tokens, cursor, "Expected statement");
			return {nullptr, "Failed to parse, expected statement"};
		}
		cursor = newCursor;

		a.Statements.push_back(std::move(stmt));

		bool atLeastOneSemicolon = false;
		while (expectToken(tokens, cursor, tokenFromSymbol(semicolonSymbol)) == true) {
			cursor++;
			atLeastOneSemicolon = true;
		}

		if (!atLeastOneSemicolon) {
			helpMessage(tokens, cursor, "Expected semi-colon delimiter between statemetns");
			return {nullptr, "Missing semi-colon between statements"};
		}
	}

	return std::make_tuple(
		std::make_unique<ast::Ast>(std::move(a)),
		""
	);
}

std::tuple<std::unique_ptr<ast::Statement>, uint64_t, bool> parseStatement(
				const std::vector<token*>& tokens, 
				uint64_t initialCursor, 
				token delimiter) {
	uint64_t cursor = initialCursor;

	token semicolonToken = tokenFromSymbol(semicolonSymbol);

	// look for SELECT statement
	auto [slct, newCursor, ok] = parseSelectStatement(tokens, cursor, semicolonToken);
	if (ok) {
		return std::make_tuple(
			std::make_unique<ast::Statement>(ast::Statement{
				.Kind = ast::AstKind::SelectKind,
				.SelectStatement = slct.release(),
			}), 
			newCursor, 
			true
		);
	}

	// look for INSERT statement
	auto [inst, newCursor1, ok1] = parseInsertStatement(tokens, cursor, semicolonToken);
	if (ok1) {
		return std::make_tuple(
			std::make_unique<ast::Statement>(ast::Statement{
				.Kind = ast::AstKind::InsertKind,
				.InsertStatement = inst.release(),
			}), 
			newCursor1, 
			true
		);
	}

	// look for CREATE statement
	auto [crtTbl, newCursor2, ok2] = parseCreateTableStatement(tokens, cursor, semicolonToken);
	if (ok2) {
		return std::make_tuple(
			std::make_unique<ast::Statement>(ast::Statement{
				.Kind = ast::AstKind::CreateTableKind,
				.CreateTableStatement = crtTbl.release(),
			}), 
			newCursor2, 
			true
		);
	}

	return {nullptr, initialCursor, false};
}

std::tuple<token*, uint64_t, bool> parseToken(
		const std::vector<token*>& tokens, 
		uint64_t initialCursor, 
		tokenKind kind) {
	uint64_t cursor = initialCursor;

	if (cursor >= tokens.size()) {
		return {nullptr, initialCursor, false};
	}

	token* current = tokens[cursor];
	if (current->kind == kind) {
		return {current, cursor + 1, true};
		
	}

	return {nullptr, initialCursor, false};
}

std::tuple<std::unique_ptr<ast::SelectStatement>, uint64_t, bool> parseSelectStatement(
								const std::vector<token*>& tokens,
								uint64_t initialCursor,
								token delimiter) {
	uint64_t cursor = initialCursor;
	if (!expectToken(tokens, cursor, tokenFromKeyword(selectKeyword))) {
		return {nullptr, initialCursor, false};
	}
	cursor++;

	ast::SelectStatement slct{};

	std::vector<token> endDelimiters = { tokenFromKeyword(fromKeyword), delimiter };
	auto [exps, newCursor, ok] = parseExpressions(tokens, cursor, endDelimiters);
	if (!ok) {
		return {nullptr, initialCursor, false};
	}

	slct.item = std::move(*exps);
	cursor = newCursor;

	if (expectToken(tokens, cursor, tokenFromKeyword(fromKeyword))) {
		cursor++;

		auto [from, newCursor1, ok1] = parseToken(tokens, cursor, tokenKind::identifierKind);
		if (!ok1) {
			helpMessage(tokens, cursor, "Expected FROM token");
			return {nullptr, initialCursor, false};
		}

		slct.from = *from;
		cursor = newCursor1;
	}

	return std::make_tuple(
		std::make_unique<ast::SelectStatement>(std::move(slct)),
		cursor,
		true
	);

}



std::tuple<std::unique_ptr<ast::expression>, uint64_t, bool> parseExpression(
		const std::vector<token*>& tokens, 
		uint64_t initialCursor,
		token _token
		) {
	uint64_t cursor = initialCursor;

	std::vector<tokenKind> kinds = {tokenKind::identifierKind, tokenKind::numericKind, tokenKind::stringKind};
	for (tokenKind kind : kinds) {
		auto [t, newCursor, ok] = parseToken(tokens, cursor, kind);
		if (ok) {
			return std::make_tuple(
				std::make_unique<ast::expression>(ast::expression{
					.literal = t,
					.kind = ast::expressionKind::literalKind,
				}),
				newCursor,
				true
			);
		}
	}

	return {nullptr, initialCursor, false};
}

std::tuple<std::unique_ptr<std::vector<std::unique_ptr<ast::expression>>>, uint64_t, bool> parseExpressions(
		const std::vector<token*>& tokens, 
		uint64_t initialCursor, 
		const std::vector<token>& delimiters) {
	size_t cursor = initialCursor;
    auto exps = std::make_unique<std::vector<std::unique_ptr<ast::expression>>>();

	while (true) {
		if (cursor >= tokens.size()) {
			return {nullptr, initialCursor, false};
		}

		// look for delimiter
		bool isDelimiter = false;
		for (const auto& delim : delimiters) {
			if (delim.equals(*tokens[cursor])) {
				isDelimiter = true;
				break;
			}
		}

		if (isDelimiter) {
			break;
		}
	
		// look for comma
		if (!exps->empty()) {
			auto commaTok = tokenFromSymbol(commaSymbol);
			if (!expectToken(tokens, cursor, commaTok)) {
				helpMessage(tokens, cursor, "Expected comma");
				return {nullptr, initialCursor, false};
			}
			++cursor;
		}

		// parse next expression
		auto commaForExpr = tokenFromSymbol(commaSymbol);
		auto [exprPtr, newCursor, okExpr] = parseExpression(tokens, cursor, commaForExpr);
		if (!okExpr) {
			helpMessage(tokens, cursor, "Expected expression");
			return {nullptr, initialCursor, false};
		}

		cursor = newCursor;
		exps->push_back(std::move(exprPtr));
	}

	return { std::move(exps), cursor, true};


}

std::tuple<std::unique_ptr<ast::InsertStatement>, uint64_t, bool> parseInsertStatement(
	const std::vector<token*>& tokens,
	uint64_t initialCursor,
	token delimiter) {

	uint64_t cursor = initialCursor;

	if (!expectToken(tokens, cursor, tokenFromKeyword(insertKeyword))) {
			return {nullptr, initialCursor, false};
	}
	cursor++;

	// look for INTO
	if (!expectToken(tokens, cursor, tokenFromKeyword(intoKeyword))) {
		helpMessage(tokens, cursor, "Expected into");
		return {nullptr, initialCursor, false};
	}
	cursor++;

	// look for table name
	auto [table, newCursor, ok] = parseToken(tokens, cursor, tokenKind::identifierKind);
	if (!ok) {
		helpMessage(tokens, cursor, "Expected table name");
		return {nullptr, initialCursor, false};
	}
	cursor = newCursor;

	// look for VALUES
	if (!expectToken(tokens, cursor, tokenFromKeyword(valuesKeyword))) {
		helpMessage(tokens, cursor, "Expected VALUES");
		return {nullptr, initialCursor, false};
	}
	cursor++;

	// Look for left paren
	if (!expectToken(tokens, cursor, tokenFromSymbol(leftparenSymbol))) {
		helpMessage(tokens, cursor, "Expected left paren");
		return {nullptr, initialCursor, false};
	}
	cursor++;

	// look for expression list
	auto [values, newCursor2, ok2] = parseExpressions(tokens, cursor, std::vector<token>{tokenFromSymbol(rightparenSymbol)});
	if (!ok2) {
		return {nullptr, initialCursor, false};
	}
	cursor = newCursor2;

	// look for right paren
	if (!expectToken(tokens, cursor, tokenFromSymbol(rightparenSymbol))) {
		helpMessage(tokens, cursor, "Expected right paren");
		return {nullptr, initialCursor, false};
	}
	cursor++;

	return std::make_tuple(
			std::make_unique<ast::InsertStatement>(ast::InsertStatement{
				.table = *table,
				.values = std::move(values),
			}), 
			cursor, 
			true
		);

}

std::tuple<std::unique_ptr<std::vector<std::unique_ptr<ast::columnDefinition>>>, uint64_t, bool> parseColumnDefinitions(
		const std::vector<token*>& tokens,
		uint64_t initialCursor,
		token delimiter) {

	uint64_t cursor = initialCursor;

	std::vector<std::unique_ptr<ast::columnDefinition>> cds;
	while (true) {
		if (cursor >= tokens.size()) {
			return {nullptr, initialCursor, false};
		}

		// look for a delimiter
		auto current = tokens[cursor];
		if (delimiter.equals(*current)) {
			break;
		}

		if (cds.size() > 0) {
			if (!expectToken(tokens, cursor, tokenFromSymbol(commaSymbol))) {
				helpMessage(tokens, cursor, "Expected comma");
				return {nullptr, initialCursor, false};
			}

			cursor++;
		}

		// look for a column name
		auto [id, newCursor, ok] = parseToken(tokens, cursor, tokenKind::identifierKind);
		if (!ok) {
			helpMessage(tokens, cursor, "Expected column name");
			return {nullptr, initialCursor, false};
		}
		cursor = newCursor;

		// look for a column type
		auto [ty, newCursor2, ok2] = parseToken(tokens, cursor, tokenKind::keywordKind);
		if (!ok2) {
			helpMessage(tokens, cursor, "Expected column type");
			return {nullptr, initialCursor, false};
		}
		cursor = newCursor2;

		cds.push_back(std::make_unique<ast::columnDefinition>(ast::columnDefinition{
			.name = *id,
			.datatype = *ty,
		}));
	}

	return {std::make_unique<std::vector<std::unique_ptr<ast::columnDefinition>>>(std::move(cds)), cursor, true};
}

std::tuple<std::unique_ptr<ast::CreateTableStatement>, uint64_t, bool> parseCreateTableStatement(
	const std::vector<token*>& tokens,
	uint64_t initialCursor,
	token delimiter) {

	uint64_t cursor = initialCursor;

	if (!expectToken(tokens, cursor, tokenFromKeyword(createKeyword))) {
		return {nullptr, initialCursor, false};
	}
	cursor++;

	if (!expectToken(tokens, cursor, tokenFromKeyword(tableKeyword))) {
		return {nullptr, initialCursor, false};
	}
	cursor++;

	auto [name, newCursor, ok] = parseToken(tokens, cursor, tokenKind::identifierKind);
	if (!ok) {
		helpMessage(tokens, cursor, "Expected table name");
		return {nullptr, initialCursor, false};
	}
	cursor = newCursor;

	if (!expectToken(tokens, cursor, tokenFromSymbol(leftparenSymbol))) {
		helpMessage(tokens, cursor, "Expected left parenthesis");
		return {nullptr, initialCursor, false};
	}
	cursor++;

	auto [cols, newCursor2, ok2] = parseColumnDefinitions(tokens, cursor, tokenFromSymbol(rightparenSymbol));
	if (!ok2) {
		return {nullptr, initialCursor, false};
	}
	cursor = newCursor2;

	if (!expectToken(tokens, cursor, tokenFromSymbol(rightparenSymbol))) {
		helpMessage(tokens, cursor, "Expected right parenthesis");
		return {nullptr, initialCursor, false};
	}
	cursor++;

	return std::make_tuple(
			std::make_unique<ast::CreateTableStatement>(ast::CreateTableStatement{
				.name = *name,
				.cols = std::move(cols),				
			}),
			cursor,
			true
		);
}

}
