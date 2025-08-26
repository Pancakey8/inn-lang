#include "lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

static std::unordered_map<std::string, TokenType> keywords{
    {"and", TokenType::KwAnd},     {"not", TokenType::KwNot},
    {"or", TokenType::KwOr},       {"func", TokenType::KwFunc},
    {"var", TokenType::KwVar},     {"if", TokenType::KwIf},
    {"while", TokenType::KwWhile}, {"do", TokenType::KwDo},
    {"end", TokenType::KwEnd},     {"else", TokenType::KwElse}};

bool is_symbol(char c) {
  return isalnum(c) || c == '_' || c == '!' || c == '?';
}

void Tokenizer::add_token(TokenType type, const std::string &lexeme) {
  // Not actually accurate :)
  tokens.emplace_back(type, lexeme, Position(row, col - (int)lexeme.size()));
};

void Tokenizer::tokenize() {
  while (i < src.size()) {
    char c = src[i];

    if (c == '\n') {
      row++;
      col = 1;
      i++;
      continue;
    }

    if (isspace(c)) {
      i++;
      col++;
      continue;
    }

    if (c == '#') {
      std::string comment;
      while (i < src.size() && src[i] != '\n') {
        comment += src[i++];
      }
      add_token(TokenType::Comment, comment);
      continue;
    }

    if (try_operator()) {
    } else if (try_number()) {
    } else if (try_symbolic()) {
    } else if (try_string()) {
    } else {
      throw std::runtime_error("Unknown character");
    }

    col++;
  }
}

bool Tokenizer::try_number() {
  if (!isdigit(src[i]))
    return false;
  std::string num;
  bool has_dot = false;
  while (i < src.size() && (isdigit(src[i]) || src[i] == '.')) {
    if (src[i] == '.') {
      if (has_dot)
        throw std::runtime_error("Unexpected dot.");
      has_dot = true;
    }
    num += src[i++];
    col++;
  }
  if (has_dot) {
    add_token(TokenType::Float, num);
  } else {
    add_token(TokenType::Int, num);
  }
  return true;
}

bool Tokenizer::try_string() {
  if (src[i] != '"')
    return false;
  std::string str;
  i++;
  col++;

  while (i < src.size() && src[i] != '"') {
    if (src[i] == '\\') {
      if (i + 1 >= src.size())
        throw std::runtime_error("Dangling backslash");
      char esc = src[i + 1];
      switch (esc) {
      case '"':
        str += '"';
        break;
      case 'n':
        str += '\n';
        break;
      case 't':
        str += '\t';
        break;
      case '\\':
        str += '\\';
        break;
      default:
        throw std::runtime_error(
            "Unrecognized escape sequence: \\" + std::string(1, esc) +
            " at row " + std::to_string(row) + ", col " + std::to_string(col));
      }
      i += 2;
      col += 2;
    } else {
      str += src[i++];
      col++;
    }
  }

  if (i < src.size() && src[i] == '"') {
    i++;
    col++;
    add_token(TokenType::String, str);
    return true;
  } else {
    throw std::runtime_error("Unterminated string");
  }
}

bool Tokenizer::try_symbolic() {
  if (!is_symbol(src[i]))
    return false;
  std::string sym;
  sym += src[i++];
  col++;
  while (i < src.size() && is_symbol(src[i])) {
    sym += src[i++];
    col++;
  }
  auto it = keywords.find(sym);
  if (it != keywords.end()) {
    add_token(it->second, sym);
  } else {
    add_token(TokenType::Symbol, sym);
  }
  return true;
}

bool Tokenizer::try_operator() {
  switch (src[i]) {
  case '{':
    add_token(TokenType::CurlyOpen, "{");
    break;
  case '}':
    add_token(TokenType::CurlyClose, "}");
    break;
  case '[':
    add_token(TokenType::SquareOpen, "[");
    break;
  case ']':
    add_token(TokenType::SquareClose, "]");
    break;
  case '(':
    add_token(TokenType::ParenOpen, "(");
    break;
  case ')':
    add_token(TokenType::ParenClose, ")");
    break;
  case '+':
    add_token(TokenType::Plus, "+");
    break;
  case '-':
    add_token(TokenType::Minus, "-");
    break;
  case '*':
    add_token(TokenType::Asterisk, "*");
    break;
  case '/':
    add_token(TokenType::Slash, "/");
    break;
  case '^':
    add_token(TokenType::Caret, "^");
    break;
  case ',':
    add_token(TokenType::Comma, ",");
    break;
  case '=':
    if (i + 1 < src.size() && src[i + 1] == '=') {
      add_token(TokenType::EqualEqual, "==");
      i++;
      col++;
    } else {
      add_token(TokenType::Equal, "=");
    }
    break;
  case '>':
    if (i + 1 < src.size() && src[i + 1] == '=') {
      add_token(TokenType::GreaterEqual, ">=");
      i++;
      col++;
    } else {
      add_token(TokenType::Greater, ">");
    }
    break;
  case '<':
    if (i + 1 < src.size() && src[i + 1] == '=') {
      add_token(TokenType::LessEqual, "<=");
      i++;
      col++;
    } else {
      add_token(TokenType::Less, "<");
    }
    break;
  default:
    return false;
  }
  i++;
  return true;
}
