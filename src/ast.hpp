#pragma once
#include "lexer.hpp"
#include <memory>
#include <optional>
#include <variant>
#include <vector>

struct ASTInt {
  int value;
};

struct ASTFloat {
  float value;
};

struct ASTString {
  std::string value;
};

struct ASTSymbol {
  std::string value;
};

using ASTSingular = std::variant<ASTInt, ASTFloat, ASTString, ASTSymbol>;

enum class Operator {
  // Infix
  Add,
  Sub,
  Mul,
  Div,
  Exp,
  Assign,
  Equal,
  Greater,
  GreaterEq,
  Less,
  LessEq,
  And,
  Or,
  // Prefix
  Not,
  Pos,
  Neg,
  // Suffix
  Index,
  FuncCall // Cheaty bcus not held in ASTOperation
};

struct Expr;

struct ASTOperation {
  Operator op;
  std::unique_ptr<Expr> left;  // !! nullable
  std::unique_ptr<Expr> right; // !! nullable
};

struct ASTArray {
  std::vector<Expr> values;
};

struct ASTFuncCall {
  std::unique_ptr<Expr> callee;
  std::vector<Expr> args;
};

struct Expr {
  std::variant<ASTOperation, ASTArray, ASTSingular, ASTFuncCall> value;
};

struct ASTType {
  std::string name;
  size_t count; // 0 if not an array.
};

struct ASTVarDeclare {
  std::string name;
  ASTType type;
  std::optional<Expr> value;
};

struct Statement;

struct ASTFuncDeclare {
  std::string name;
  ASTType ret;
  std::vector<std::pair<std::string, ASTType>> args;
  std::vector<Statement> body;
};

struct ASTWhile {
  Expr condition;
  std::vector<Statement> body;
};

struct ASTIf {
  using Branch = std::pair<Expr, std::vector<Statement>>;
  std::vector<Branch> branches;
  std::vector<Statement> otherwise;
};

struct Statement {
  std::variant<ASTVarDeclare, Expr, ASTWhile, ASTIf> value;
};

using Paragraph = std::variant<Statement, ASTFuncDeclare>;

struct ASTError : public std::runtime_error {
  Position pos;
  ASTError(Position pos, std::string what)
      : pos(pos), std::runtime_error(what) {}
};

void debug_print(Paragraph &p);

struct ASTBuilder {
  std::vector<LexerToken> tokens;
  std::vector<Paragraph> roots;
  size_t i = 0;

  bool peek(TokenType type);

  bool accept(TokenType type);

  void expect(TokenType type, std::string err);

  template <typename T>
  void expect_value(const std::optional<T> &opt, std::string err) {
    if (!opt.has_value()) {
      if (i >= tokens.size())
        throw ASTError(tokens.back().loc, err);
      throw ASTError(tokens[i].loc, err);
    }
  }

  ASTBuilder(std::vector<LexerToken> &&toks) : tokens(std::move(toks)) {
    trim_comments();
  }

  void trim_comments();

  std::optional<ASTSingular> parse_singular();

  std::optional<ASTType> parse_type();

  std::optional<ASTVarDeclare> parse_vardecl();

  std::optional<ASTWhile> parse_while();

  std::optional<ASTIf> parse_if();

  std::optional<ASTFuncDeclare> parse_funcdecl();

  std::optional<Statement> parse_statement();

  std::optional<Expr> parse_expression(int prec = 0);

  std::optional<Expr> right(Operator op, Expr left);

  std::optional<Expr> left();

  void parse();
};
