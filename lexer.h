#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <functional>

namespace nicolassql {

struct location {
	uint64_t line;
	uint64_t col;
};

// string_view avoids heap allocations for constant strings
typedef std::string_view keyword;

constexpr keyword selectKeyword = "select";
constexpr keyword fromKeyword = "from";
constexpr keyword asKeyword = "as";
constexpr keyword tableKeyword = "table";
constexpr keyword createKeyword = "create";
constexpr keyword insertKeyword = "insert";
constexpr keyword intoKeyword = "into";
constexpr keyword valuesKeyword = "values";
constexpr keyword intKeyword = "int";
constexpr keyword textKeyword = "text";

typedef std::string_view symbol;

constexpr symbol semicolonSymbol = ";";
constexpr symbol asteriskSymbol = "*";
constexpr symbol commaSymbol = ",";
constexpr symbol leftparenSymbol = "(";
constexpr symbol rightparenSymbol = ")";

enum class tokenKind : unsigned int {
	keywordKind = 0,
	symbolKind,
	identifierKind,
	stringKind,
	numericKind,
};

struct token {
	std::string value;
	tokenKind kind;
	location loc;

	bool equals(const token& other) const {
		return value == other.value && kind == other.kind;
	}
};

struct cursor {
	uint64_t pointer;
	location loc;
};

using lexer = std::function<std::tuple<std::unique_ptr<token>, cursor, bool>(const std::string&, const cursor&)>;

}
