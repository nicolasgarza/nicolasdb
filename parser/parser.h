#pragma once

#include <string>
#include <tuple>
#include <memory>
#include "../ast/ast.h"

namespace parser {

std::tuple<std::unique_ptr<ast::Ast>, std::string> Parse(std::string source);

}
