#include "ast.hpp"
#include <iostream>
#include <unordered_map>
#include <unordered_set>

std::unordered_map<Operator, int> precedences{
    {Operator::Add, 60},   {Operator::Sub, 60},     {Operator::Mul, 70},
    {Operator::Div, 70},   {Operator::Exp, 80},     {Operator::Assign, 10},
    {Operator::Equal, 40}, {Operator::Greater, 50}, {Operator::GreaterEq, 50},
    {Operator::Less, 50},  {Operator::LessEq, 50},  {Operator::And, 30},
    {Operator::Or, 20},    {Operator::Not, 100},    {Operator::Pos, 90},
    {Operator::Neg, 90},   {Operator::Index, 110},  {Operator::FuncCall, 110},
};

std::unordered_map<TokenType, Operator> infix_ops{
    {TokenType::Plus, Operator::Add},
    {TokenType::Minus, Operator::Sub},
    {TokenType::Asterisk, Operator::Mul},
    {TokenType::Slash, Operator::Div},
    {TokenType::Caret, Operator::Exp},
    {TokenType::Equal, Operator::Assign},
    {TokenType::EqualEqual, Operator::Equal},
    {TokenType::Greater, Operator::Greater},
    {TokenType::GreaterEqual, Operator::GreaterEq},
    {TokenType::Less, Operator::Less},
    {TokenType::LessEqual, Operator::LessEq},
    {TokenType::KwAnd, Operator::And},
    {TokenType::KwOr, Operator::Or},
};

std::unordered_map<TokenType, Operator> prefix_ops{
    {TokenType::Plus, Operator::Pos},
    {TokenType::Minus, Operator::Neg},
    {TokenType::KwNot, Operator::Not},
};

std::unordered_map<TokenType, Operator> suffix_ops{
    {TokenType::SquareOpen, Operator::Index},
    {TokenType::ParenOpen, Operator::FuncCall},
};

void ASTBuilder::trim_comments() {
  for (auto it = tokens.begin(); it != tokens.end();) {
    if (it->type == TokenType::Comment) {
      it = tokens.erase(it);
    } else {
      ++it;
    }
  }
}

bool ASTBuilder::peek(TokenType type) {
  return i < tokens.size() && tokens[i].type == type;
}

bool ASTBuilder::accept(TokenType type) {
  if (peek(type)) {
    i++;
    return true;
  }
  return false;
}

void ASTBuilder::expect(TokenType type, std::string err) {
  if (!accept(type)) {
    if (i >= tokens.size())
      throw ASTError(tokens.back().loc, err);
    throw ASTError(tokens[i].loc, err);
  }
}

std::optional<ASTSingular> ASTBuilder::parse_singular() {
  switch (tokens[i].type) {
  case TokenType::String:
    return ASTString{tokens[i++].lexeme};
  case TokenType::Int:
    return ASTInt{std::stoi(tokens[i++].lexeme)};
  case TokenType::Float:
    return ASTFloat{std::stof(tokens[i++].lexeme)};
  case TokenType::Symbol:
    return ASTSymbol{tokens[i++].lexeme};
  default:
    return std::nullopt;
  }
}

std::unordered_set<TokenType> stoppers{
    TokenType::ParenClose, TokenType::SquareClose, TokenType::Comma,
    TokenType::KwEnd,      TokenType::KwDo,        TokenType::KwElse};

std::optional<Expr> ASTBuilder::parse_expression(int prec) {
  auto optl = left();
  if (!optl.has_value())
    throw ASTError(tokens[i].loc, "Expected valid LHS for operator.");
  Expr lval = std::move(optl.value());
  while (i < tokens.size()) {
    if (stoppers.contains(tokens[i].type))
      break;
    Operator op;
    if (infix_ops.contains(tokens[i].type)) {
      op = infix_ops[tokens[i].type];
    } else if (suffix_ops.contains(tokens[i].type)) {
      op = suffix_ops[tokens[i].type];
    } else {
      break;
    }
    if (precedences[op] <= prec)
      break;
    i++;
    auto optr = right(op, std::move(lval));
    if (!optr.has_value())
      throw ASTError(tokens[i].loc, "Expected valid expression.");
    lval = std::move(optr.value());
  }
  return lval;
}

std::optional<Expr> ASTBuilder::right(Operator op, Expr left) {
  if (op == Operator::Index) {
    auto opt = parse_expression();
    expect_value(opt, "Expected index.");
    expect(TokenType::SquareClose, "Expected closing bracket.");
    return std::optional<Expr>(
        ASTOperation{Operator::Index, std::make_unique<Expr>(std::move(left)),
                     std::make_unique<Expr>(std::move(opt.value()))});
  } else if (op == Operator::FuncCall) {
    std::vector<Expr> args{};
    if (!accept(TokenType::ParenClose)) {
      do {
        auto opt = parse_expression();
        expect_value(opt, "Expected valid expression.");
        args.push_back(std::move(opt.value()));
        if (!accept(TokenType::Comma))
          break;
      } while (true);
      expect(TokenType::ParenClose, "Expected closing parenthesis.");
    }
    return std::optional<Expr>(
        ASTFuncCall{std::make_unique<Expr>(std::move(left)), std::move(args)});
  }

  int prec = precedences[op];
  if (op == Operator::Exp)
    prec--; // Right assoc.
  auto opt = parse_expression(prec);
  expect_value(opt, "Expected valid expression.");
  return std::optional<Expr>(
      ASTOperation{op, std::make_unique<Expr>(std::move(left)),
                   std::make_unique<Expr>(std::move(opt.value()))});
}

std::optional<Expr> ASTBuilder::left() {
  if (accept(TokenType::SquareOpen)) { // Array literal
    std::vector<Expr> args{};
    if (!accept(TokenType::SquareClose)) {
      do {
        auto opt = parse_expression();
        expect_value(opt, "Expected valid expression.");
        args.push_back(std::move(opt.value()));
        if (!accept(TokenType::Comma))
          break;
      } while (true);
      expect(TokenType::SquareClose, "Expected closing bracket.");
    }
    return std::optional<Expr>(ASTArray{std::move(args)});
  }
  if (accept(TokenType::ParenOpen)) {
    return parse_expression();
  }
  auto sing = parse_singular();
  if (sing.has_value())
    return std::optional<Expr>({sing.value()}); // {{ ... }} yikes
  if (!prefix_ops.contains(tokens[i].type)) {
    return std::nullopt;
  }
  Operator op = prefix_ops[tokens[i].type];
  i++;
  auto right = parse_expression(precedences[op]);
  expect_value(right, "Expected valid expression following prefix operator.");
  return std::optional<Expr>(ASTOperation{
      op, nullptr, std::make_unique<Expr>(std::move(right.value()))});
}

std::optional<ASTType> ASTBuilder::parse_type() {
  if (accept(TokenType::SquareOpen)) {
    expect(TokenType::Int, "Expected positive integer array size.");
    size_t arr_size = std::stoi(tokens[i - 1].lexeme);
    if (arr_size <= 0)
      throw ASTError(tokens[i].loc, "Expected positive integer array size.");
    expect(TokenType::SquareClose, "Expected closing bracket.");
    expect(TokenType::Symbol, "Expected typename.");
    std::string type = tokens[i - 1].lexeme;
    return ASTType{type, arr_size};
  }
  if (accept(TokenType::Symbol)) {
    std::string type = tokens[i - 1].lexeme;
    return ASTType{type, 0};
  }
  return std::nullopt;
}

std::optional<ASTVarDeclare> ASTBuilder::parse_vardecl() {
  if (!accept(TokenType::KwVar))
    return std::nullopt;
  expect(TokenType::Symbol, "Expected variable name.");
  std::string name = tokens[i - 1].lexeme;
  auto opt = parse_type();
  expect_value(opt, "Expected type.");
  ASTType type = opt.value();
  if (accept(TokenType::Equal)) {
    auto opt = parse_expression();
    expect_value(opt, "Invalid expression on RHS.");
    return ASTVarDeclare{name, std::move(type), std::move(opt.value())};
  } else {
    return ASTVarDeclare{name, std::move(type), std::nullopt};
  }
}

std::optional<ASTFuncDeclare> ASTBuilder::parse_funcdecl() {
  if (!accept(TokenType::KwFunc))
    return std::nullopt;
  expect(TokenType::Symbol, "Expected function name.");
  std::string name = tokens[i - 1].lexeme;
  expect(TokenType::ParenOpen, "Expected params list.");
  std::vector<std::pair<std::string, ASTType>> args{};
  if (!accept(TokenType::ParenClose)) {
    do {
      expect(TokenType::Symbol, "Expected argument name.");
      std::string name = tokens[i - 1].lexeme;
      auto opt = parse_type();
      expect_value(opt, "Expected argument type.");
      ASTType type = opt.value();
      args.push_back({name, type});
      if (!accept(TokenType::Comma))
        break;
    } while (true);
    expect(TokenType::ParenClose, "Expected closing parenthesis.");
  }
  auto opt = parse_type();
  expect_value(opt, "Expected return type.");
  ASTType ret = opt.value();
  expect(TokenType::KwDo, "Expected 'do'.");
  std::vector<Statement> body;
  while (i < tokens.size() && tokens[i].type != TokenType::KwEnd) {
    auto opt = parse_statement();
    expect_value(opt, "Expected proper statement.");
    body.push_back(std::move(opt.value()));
  }
  expect(TokenType::KwEnd, "Expected block close.");
  return ASTFuncDeclare{name, ret, std::move(args), std::move(body)};
}

std::optional<ASTWhile> ASTBuilder::parse_while() {
  if (!accept(TokenType::KwWhile))
    return std::nullopt;
  auto opt = parse_expression();
  expect_value(opt, "Expected condition.");
  Expr cond = std::move(opt.value());
  expect(TokenType::KwDo, "Expected do block.");
  std::vector<Statement> body;
  if (!accept(TokenType::KwEnd)) {
    do {
      auto opt = parse_statement();
      expect_value(opt, "Expected valid statement.");
      body.push_back(std::move(opt.value()));
    } while (!accept(TokenType::KwEnd));
  }
  return ASTWhile{std::move(cond), std::move(body)};
}

std::optional<ASTIf> ASTBuilder::parse_if() {
  if (!accept(TokenType::KwIf))
    return std::nullopt;

  auto optCond = parse_expression();
  expect_value(optCond, "Expected condition.");
  Expr cond = std::move(optCond.value());

  expect(TokenType::KwDo, "Expected do block.");

  std::vector<Statement> body;
  std::vector<ASTIf::Branch> branches{};
  std::vector<Statement> elseBody;

  while (!peek(TokenType::KwElse) && !peek(TokenType::KwEnd)) {
    auto optStmt = parse_statement();
    expect_value(optStmt, "Expected valid statement.");
    body.push_back(std::move(optStmt.value()));
  }
  branches.push_back({std::move(cond), std::move(body)});

  while (accept(TokenType::KwElse)) {
    if (accept(TokenType::KwIf)) {
      auto optElseIfCond = parse_expression();
      expect_value(optElseIfCond, "Expected condition after else if.");
      Expr elseifCond = std::move(optElseIfCond.value());

      expect(TokenType::KwDo, "Expected do block.");

      std::vector<Statement> elseifBody;
      while (!peek(TokenType::KwElse) && !peek(TokenType::KwEnd)) {
        auto optStmt = parse_statement();
        expect_value(optStmt, "Expected valid statement.");
        elseifBody.push_back(std::move(optStmt.value()));
      }
      branches.push_back({std::move(elseifCond), std::move(elseifBody)});
    } else {
      expect(TokenType::KwDo, "Expected do block.");
      while (!peek(TokenType::KwEnd)) {
        auto optStmt = parse_statement();
        expect_value(optStmt, "Expected valid statement.");
        elseBody.push_back(std::move(optStmt.value()));
      }
      break;
    }
  }

  expect(TokenType::KwEnd, "Expected end to close if.");

  return ASTIf{std::move(branches), std::move(elseBody)};
}

std::optional<Statement> ASTBuilder::parse_statement() {
  if (auto opt = parse_vardecl(); opt.has_value()) {
    return Statement{std::move(opt.value())};
  }
  if (auto opt = parse_while(); opt.has_value()) {
    return Statement{std::move(opt.value())};
  }
  if (auto opt = parse_if(); opt.has_value()) {
    return Statement{std::move(opt.value())};
  }
  if (auto opt = parse_expression(); opt.has_value()) {
    return Statement{std::move(opt.value())};
  }
  return std::nullopt;
}

void ASTBuilder::parse() {
  while (i < tokens.size()) {
    if (auto opt = parse_funcdecl(); opt.has_value()) {
      roots.push_back(std::move(opt.value()));
      continue;
    }
    if (auto opt = parse_statement(); opt.has_value()) {
      roots.push_back(Statement{std::move(opt.value())});
      continue;
    }
  }
}

void debug_print(Expr &ex);

void debug_print(ASTSingular &sing) {
  std::visit(
      [](auto &arg) {
        using T = std::decay_t<decltype(arg)>;
        if (std::is_same_v<T, ASTString>)
          std::cout << "\"";
        std::cout << arg.value;
        if (std::is_same_v<T, ASTString>)
          std::cout << "\"";
      },
      sing);
}

void debug_print(ASTArray &sing) {
  std::cout << "[";
  for (auto &elem : sing.values) {
    debug_print(elem);
    std::cout << ", ";
  }
  std::cout << "]";
}

void debug_print(ASTFuncCall &call) {
  std::cout << "(";
  debug_print(*call.callee);
  std::cout << " ";
  for (auto &arg : call.args) {
    debug_print(arg);
    std::cout << " ";
  }
  std::cout << ")";
}

void debug_print(ASTOperation &op) {
  std::cout << "(";
  if (op.left != nullptr) {
    debug_print(*op.left);
    std::cout << " ";
  }
  std::cout << static_cast<int>(op.op);
  if (op.right != nullptr) {
    std::cout << " ";
    debug_print(*op.right);
  }
  std::cout << ")";
}

void debug_print(Expr &ex) {
  std::visit([](auto &arg) { debug_print(arg); }, ex.value);
}

void debug_print(ASTType &type) {
  std::cout << "[" << type.count << "]" << type.name;
}

void debug_print(Statement &p);

void debug_print(ASTIf &stm) {
  size_t i = 0;
  for (auto &branch : stm.branches) {
    if (i != 0)
      std::cout << "else ";
    std::cout << "if ";
    debug_print(branch.first);
    std::cout << " do " << std::endl;
    for (auto &stmt : branch.second) {
      debug_print(stmt);
      std::cout << std::endl;
    }
    ++i;
  }
  std::cout << "else do" << std::endl;
  for (auto &stmt : stm.otherwise) {
    debug_print(stmt);
    std::cout << std::endl;
  }
  std::cout << "end" << std::endl;
}

void debug_print(ASTVarDeclare &var) {
  std::cout << "var " << var.name << " ";
  debug_print(var.type);
  if (var.value.has_value()) {
    std::cout << " = ";
    debug_print(var.value.value());
  }
  std::cout << std::endl;
}

void debug_print(ASTWhile &whl) {
  std::cout << "while ";
  debug_print(whl.condition);
  std::cout << " do " << std::endl;
  for (auto &stmt : whl.body) {
    debug_print(stmt);
    std::cout << std::endl;
  }
  std::cout << "end" << std::endl;
}

void debug_print(Statement &p) {
  std::visit([](auto &arg) { debug_print(arg); }, p.value);
}

void debug_print(ASTFuncDeclare &fdecl) {
  std::cout << "func " << fdecl.name << "(";
  for (auto &param : fdecl.args) {
    std::cout << param.first << " ";
    debug_print(param.second);
    std::cout << ", ";
  }
  std::cout << ") ";
  debug_print(fdecl.ret);
  std::cout << " do" << std::endl;
  for (auto &stmt : fdecl.body) {
    debug_print(stmt);
    std::cout << std::endl;
  }
  std::cout << "end" << std::endl;
}

void debug_print(Paragraph &p) {
  std::visit([](auto &arg) { debug_print(arg); }, p);
}
