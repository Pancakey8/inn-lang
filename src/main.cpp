#include "ast.hpp"
#include "codegen.hpp"
#include "lexer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "USAGE: " << ((argc > 0) ? argv[0] : "inn")
              << " <input-file> <output-file>";
    return 1;
  }
  std::ifstream fstr(argv[1]);
  std::ostringstream ss;
  ss << fstr.rdbuf();
  std::string code = ss.str();
  fstr.close();

  Tokenizer tknizer(std::move(code));
  tknizer.tokenize();

  // for (const auto &t : tknizer.tokens) {
  //   std::cout << static_cast<int>(t.type) << " \"" << t.lexeme << "\" at ("
  //             << t.loc.row << ", " << t.loc.col << ")" << std::endl;
  // }

  // std::cout << std::endl;
  // std::cout << "------" << std::endl;

  ASTBuilder blder(std::move(tknizer.tokens));
  blder.parse();
  std::ofstream out(std::string(argv[2]) + ".c");
  out << begin_file();
  for (auto &para : blder.roots) {
    std::string gen = generate_one(para);
    // std::cout << gen;
    out << gen;
  }
  out.flush();
  out.close();

  std::string comp =
      "cc " + std::string(argv[2]) + ".c" + " -o " + std::string(argv[2]);
  system(comp.c_str());

  return 0;
}
