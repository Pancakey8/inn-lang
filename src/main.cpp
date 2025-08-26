#include "ast.hpp"
#include "lexer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 1;
  std::ifstream fstr(argv[1]);
  std::ostringstream ss;
  ss << fstr.rdbuf();
  std::string code = ss.str();

  Tokenizer tknizer(std::move(code));
  tknizer.tokenize();

  for (const auto &t : tknizer.tokens) {
    std::cout << static_cast<int>(t.type) << " \"" << t.lexeme << "\" at ("
              << t.loc.row << ", " << t.loc.col << ")" << std::endl;
  }

  std::cout << std::endl;
  std::cout << "------" << std::endl;

  ASTBuilder blder(std::move(tknizer.tokens));
  blder.parse();
  for (auto &para : blder.roots) {
    debug_print(para);
  }

  return 0;
}
