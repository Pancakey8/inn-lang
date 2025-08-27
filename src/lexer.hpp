#pragma once
#include <string>
#include <vector>

struct Position {
  int row;
  int col;
  Position(int r, int c) : row(r), col(c) {}
};

enum class TokenType {
  Comment,
  Int,
  Float,
  String,
  Symbol,

  CurlyOpen,
  CurlyClose, // Keep for later
  SquareOpen,
  SquareClose,
  ParenOpen,
  ParenClose,

  Plus,
  Minus,
  Asterisk,
  Slash,
  Caret,
  Ampersand,

  Comma,
  Equal,
  EqualEqual,
  Greater,
  GreaterEqual,
  Less,
  LessEqual,

  KwAnd,
  KwNot,
  KwOr,
  KwFunc,
  KwVar,
  KwIf,
  KwElse,
  KwWhile,
  KwDo,
  KwEnd,
  KwReturn,
  KwBreak
};

struct LexerToken {
  TokenType type;
  std::string lexeme;
  Position loc;

  LexerToken(TokenType t, const std::string &l, Position loc)
      : type(t), lexeme(l), loc(loc) {}
};

struct Tokenizer {
  std::vector<LexerToken> tokens;
  int row = 1, col = 1;
  size_t i = 0;
  std::string src;
  Tokenizer(std::string &&src) : src(std::move(src)) {}

  void add_token(TokenType type, const std::string &lexeme);

  void tokenize();

  bool try_number();

  bool try_string();

  bool try_symbolic();

  bool try_operator();
};
