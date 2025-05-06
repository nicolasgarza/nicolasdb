#include <tuple>
#include <vector>
#include "lexer.h"

namespace nicolassql {

std::tuple<std::vector<token*>, std::string> lex(const std::string& source) {
	std::vector<token*> tokens;
	cursor cur{};

	while (cur.pointer < source.length()) {
		std::vector<lexer> lexers = {lexKeyword, lexSymbol, lexString, lexNumeric, lexIdentifier};
		for (lexer l : lexers) {
			if (auto [token, newCursor, ok] = l(source, cur); ok) {
				cur = newCursor;
				if (token != nullptr) {
					tokens.push_back(token.release());
				}

				continue;
			}
		}

		std::string hint = "";
		if (tokens.size() > 0) {
			hint = " after " + tokens[tokens.size()-1]->value;
		}

		std::string err = "Unable to lex token" + hint + " at " + 
							std::to_string(cur.loc.line) + ":" + std::to_string(cur.loc.col);
		return {tokens, err};
	}

}

namespace {
	std::tuple<std::unique_ptr<token>, cursor, bool> lexNumeric(std::string source, cursor ic) {
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
				.value = source.substr(ic.pointer, cur.pointer - ic.pointer),
				.kind = tokenKind::numericKind,
				.loc = ic.loc
			}),
			cur,
			true
		);

	}

	std::tuple<std::unique_ptr<token>, cursor, bool> lexCharacterDelimited(const std::string& source, cursor ic, char delimiter) {
		cursor cur = ic;

		if (source.substr(cur.pointer, source.length()-1).length() == 0) {
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

	std::tuple<std::unique_ptr<token>, cursor, bool> lexString(const std::string& source, cursor ic) {
		return lexCharacterDelimited(source, ic, '\'');
	}


}

}


