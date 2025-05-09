#pragma once
#include "../lexer/lexer.h"
#include "../ast/ast.h"
#include <initializer_list>
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
	if (cursor > tokens.size()) {
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

	ast::Ast a{};
	uint64_t cursor = 0;
	while (cursor < tokens.size()) {
		auto [stmt, newCursor, ok] = parseStatement(tokens, cursor, tokenFromSymbol(semicolonSymbol));
		if (ok) {
			helpMessage(tokens, cursor, "Expected statement");
			return {nullptr, "Failed to parse, expected statement"};
		}
		cursor = newCursor;

		a.Statements.push_back(stmt);

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
				.CreateTableStatement = std::move(crtTbl),
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



}
