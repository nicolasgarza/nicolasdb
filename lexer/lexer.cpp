#include <iterator>
#include <string>
#include <tuple>
#include <vector>
#include "lexer.h"

namespace nicolassql {

	std::tuple<std::unique_ptr<token>, cursor, bool> lexKeyword(std::string_view source, cursor ic);
	std::tuple<std::unique_ptr<token>, cursor, bool> lexSymbol(std::string_view source, cursor ic);
	std::tuple<std::unique_ptr<token>, cursor, bool> lexString(std::string_view source, cursor ic);
	std::tuple<std::unique_ptr<token>, cursor, bool> lexNumeric(std::string_view source, cursor ic);
	std::tuple<std::unique_ptr<token>, cursor, bool> lexIdentifier(std::string_view source, cursor ic);

std::tuple<std::vector<token*>, std::string> lex(std::string_view source) {
	std::vector<token*> tokens;
	cursor cur{};

	while (cur.pointer < source.length()) {
		bool matched = false;
		using LexerFn = std::tuple<std::unique_ptr<token>, cursor, bool>(*)(std::string_view, cursor);
		static constexpr std::array<LexerFn, 5> lexers = {
			&lexKeyword, &lexSymbol, &lexString, &lexNumeric, &lexIdentifier
		};
		for (lexer l : lexers) {
			if (auto [token, newCursor, ok] = l(source, cur); ok) {
				cur = newCursor;
				if (token != nullptr) {
					tokens.push_back(token.release());
				}

				matched = true;
				break;
			}
		}

		if (matched) {
			continue;
		}

		std::string hint = "";
		if (tokens.size() > 0) {
			hint = " after " + std::string(tokens[tokens.size()-1]->value);
		}

		std::string err = "Unable to lex token" + hint + " at " + 
							std::to_string(cur.loc.line) + ":" + std::to_string(cur.loc.col);
		return {tokens, err};
	}
	
	return {tokens, ""};
}

std::tuple<std::unique_ptr<token>, cursor, bool> lexNumeric(std::string_view source, cursor ic) {
	cursor cur = ic;	

	bool periodFound = false;
	bool expMarkerFound = false;

	for(; cur.pointer < source.length(); cur.pointer++) {
		char c = source[cur.pointer];
		cur.loc.col++;

		bool isDigit = c >= '0' && c <= '9';
		bool isPeriod = c == '.';
		bool isExpMarker = c == 'e';

		// we have to start with a digit or period
		if (cur.pointer == ic.pointer) {
			if (!isDigit && !isPeriod) {
				return {nullptr, ic, false};
			}

			periodFound = isPeriod;
			continue;
		}

		if (isPeriod) {
			if (periodFound) {
				return {nullptr, ic, false};
			}

			periodFound = true;
			continue;
		}

		if (isExpMarker) {
			if (expMarkerFound) {
				return {nullptr, ic, false};
			}

			// can't have a period after an expMarker
			periodFound = true;
			expMarkerFound = true;

			// expMarker must have digits after it
			if (cur.pointer == source.length()-1) {
				return {nullptr, ic, false};
			}

			char cNext = source[cur.pointer+1];
			if (cNext == '-' || cNext == '+') {
				cur.pointer++;
				cur.loc.col++;
			}

			continue;
		}

		if (!isDigit) {
			break;
		}
	}

	// no characters accumulated
	if (cur.pointer == ic.pointer) {
		return {nullptr, ic, false};
	}

	return std::make_tuple(
		std::make_unique<token>(token{
			.value = std::string(source.substr(ic.pointer, cur.pointer - ic.pointer)),
			.kind = tokenKind::numericKind,
			.loc = ic.loc
		}),
		cur,
		true
	);

}

std::tuple<std::unique_ptr<token>, cursor, bool> lexCharacterDelimited(std::string_view source, cursor ic, char delimiter) {
	cursor cur = ic;

	if (source.substr(cur.pointer).empty()) {
		return {nullptr, ic, false};
	}

	if (source[cur.pointer] != delimiter) {
		return {nullptr, ic, false};
	}

	cur.loc.col++;
	cur.pointer++;

	std::vector<char> value;
	for(; cur.pointer < source.length(); cur.pointer++) {
		char c = source[cur.pointer];

		if (c == delimiter) {
			// SQL escapes are via double characters, not backslash
			if (cur.pointer+1 >= source.length() || source[cur.pointer+1] != delimiter) {
				return std::make_tuple(
					std::make_unique<token>(token{
						.value = std::string(value.begin(), value.end()),
						.loc = ic.loc,
						.kind = tokenKind::stringKind,
					}), 
					cur, 
					true			
				);
			} else {
				value.push_back(delimiter);
				cur.pointer++;
				cur.loc.col++;
			}
		}

		value.push_back(c);
		cur.loc.col++;
	}

	return {nullptr, ic, false};
}

std::tuple<std::unique_ptr<token>, cursor, bool> lexString(std::string_view source, cursor ic) {
	return lexCharacterDelimited(source, ic, '\'');
}

// longestMatch iterates through a source string, starting at the given
// cursor, to find the longest matching substring among the provided
// options
std::string longestMatch(std::string_view source, cursor ic, std::vector<std::string_view>& options) {
	std::vector<char> value;
	std::vector<int> skipList;
	std::string match;

	cursor cur = ic;

	while (cur.pointer < source.length()) {
		value.push_back(std::tolower(source[cur.pointer]));
		cur.pointer++;

		for (int i = 0; i < options.size(); i++) {
			bool isSkipped = false;
			for (int skip : skipList) {
				if (i == skip) {
					isSkipped = true;
					break;
				}
			}
			if (isSkipped) {
				continue;
			}

			// deal with cases like INT vs INTO
			if (options[i] == std::string(value.begin(), value.end())) {
				skipList.push_back(i);
				if (options[i].length() > match.length()) {
					match = options[i];
				}

				continue;
			}

			bool sharesPrefix = std::string(value.begin(), value.end()) == options[i].substr(0, cur.pointer - ic.pointer);  
			bool tooLong = value.size() > options[i].length();
			if (tooLong || !sharesPrefix) {
				skipList.push_back(i);
			}
		}

		if (skipList.size() == options.size()) {
			break;
		}
	}

	return match;
}

std::tuple<std::unique_ptr<token>, cursor, bool> lexSymbol(std::string_view source, cursor ic) {
	char c = source[ic.pointer];
	cursor cur = ic;

	// will get overwritten later, if not an ignored syntax
	cur.pointer++;
	cur.loc.col++;

	switch (c) {
	// syntax that should be thrown away
	case '\n':
		cur.loc.line++;
		cur.loc.col = 0;
	case '\t':
	case ' ':
		return {nullptr, cur, true};

	case ',':
	case '(':
	case ')':
	case ';':
	case '*':
		break;
	default:
		return {nullptr, ic, false};
	}

	return std::make_tuple(
		std::make_unique<token>(token{
			.value = std::string(1, c),
			.loc = ic.loc,
			.kind = tokenKind::symbolKind,
		}), 
		cur, 
		true			
	);

}

std::tuple<std::unique_ptr<token>, cursor, bool> lexKeyword(std::string_view source, cursor ic) {
	cursor cur = ic;
	std::vector<keyword> keywords = {
		selectKeyword,
		insertKeyword,
		valuesKeyword,
		tableKeyword,
		createKeyword,
		fromKeyword,
		intoKeyword,
		textKeyword,
		intKeyword,
		asKeyword,
	};
	
	std::vector<char> value;
	std::vector<int> skipList;
	std::string match;

	while (cur.pointer < source.size()) {
		char c = source[cur.pointer++];
		cur.loc.col++;

		value.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		std::string prefix(value.begin(), value.end());

		for (int i = 0; i < keywords.size(); ++i) {
			if (std::find(skipList.begin(), skipList.end(), i) != skipList.end()) {
				continue;
			}	

			const auto kw = keywords[i];
			const auto len_kw = kw.size();
			const auto len_val = prefix.size();

			if (prefix == kw) {
				skipList.push_back(i);
				if (len_kw > match.size()) {
					match = std::string(kw);
				}
				continue;
			}

			bool sharesPrefix = (len_val <= len_kw
								&& prefix == std::string(kw.substr(0, len_val)));
			bool tooLong = (len_val > len_kw);

			if (tooLong || !sharesPrefix) {
				skipList.push_back(i);
			}
		}

		if (skipList.size() == keywords.size()) {
			break;
		}
	}
	if (match.empty()) {
		return {nullptr, ic, false};
	}

	cur.pointer = ic.pointer + match.size();
	cur.loc.col = ic.loc.col + static_cast<uint64_t>(match.size());

	auto tok = std::make_unique<token>(token{
			.value = match,
			.kind = tokenKind::keywordKind,
			.loc = ic.loc
			});
	return {std::move(tok), cur, true};
}

std::tuple<std::unique_ptr<token>, cursor, bool> lexIdentifier(std::string_view source, cursor ic) {
	// handle seperately if it is a double-quoted identifier
	if (auto [tok, newCursor, ok] = lexCharacterDelimited(source, ic, '"'); ok) {
		return {std::move(tok), newCursor, true};
	}	

	cursor cur = ic;

	char c = source[cur.pointer];
	// other characters count alos, ignoring non-ascii for now
	bool isAlphabetical = (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z'));
	if (!isAlphabetical) {
		return {nullptr, ic, false};
	}
	cur.pointer++;
	cur.loc.col++;

	std::vector<char> value = {c};
	for(; cur.pointer < source.length(); cur.pointer++) {
		c = source[cur.pointer];

		isAlphabetical = (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z'));
		bool isNumeric = (c >= '0' && c <= '9');
		if (isAlphabetical || isNumeric || c == '$' || c == '_') {
			value.push_back(c);
			cur.loc.col++;
			continue;
		}

		break;
	}

	if (value.size() == 0) {
		return {nullptr, ic, false};
	}


	std::string rawIdentifier(value.begin(), value.end());
	std::transform(rawIdentifier.begin(), rawIdentifier.end(), rawIdentifier.begin(),
               [](unsigned char c) { return std::tolower(c); });

	return std::make_tuple(
		std::make_unique<token>(token{
			.value = std::move(rawIdentifier),
			.loc = ic.loc,
			.kind = tokenKind::identifierKind,
		}), 
		cur, 
		true			
	); 

}

std::tuple<std::unique_ptr<token>, cursor, bool> lexString(std::string source, cursor ic) {
	return lexCharacterDelimited(source, ic, '\'');
}

}

