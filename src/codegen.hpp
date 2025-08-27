#pragma once

#include "ast.hpp"
#include <unordered_map>
#include <vector>

std::string quote(const std::string &str) {
  std::string result = "\"";
  for (char c : str) {
    switch (c) {
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\t':
      result += "\\t";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '\"':
      result += "\\\"";
      break;
    default:
      result += c;
      break;
    }
  }
  result += "\""; // closing quote
  return result;
}

std::string generate_one(const ASTSingular &sing) {
  return std::visit(
      [](auto &arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ASTInt>)
          return std::to_string(arg.value);
        if constexpr (std::is_same_v<T, ASTFloat>)
          return std::to_string(arg.value);
        if constexpr (std::is_same_v<T, ASTString>)
          return quote(arg.value);
        if constexpr (std::is_same_v<T, ASTSymbol>)
          return arg.value;
      },
      sing);
}

std::string generate_one(const Expr &ex);

std::string generate_one(const ASTFuncCall &fcall) {
  std::string res = "(" + generate_one(*fcall.callee) + "(";
  for (size_t i = 0; i < fcall.args.size(); ++i) {
    res += generate_one(fcall.args[i]);
    if (i != fcall.args.size() - 1)
      res += ",";
  }
  res += "))";
  return res;
}

std::string generate_one(const ASTArray &arr) {
  std::string res = "{";
  for (size_t i = 0; i < arr.values.size(); ++i) {
    res += generate_one(arr.values[i]);
    if (i != arr.values.size() - 1)
      res += ",";
  }
  res += "}";
  return res;
}

std::string generate_one(const ASTOperation &op) {
  std::string left{}, right{};
  if (op.left != nullptr)
    left = generate_one(*op.left);
  if (op.right != nullptr)
    right = generate_one(*op.right);
  switch (op.op) {
  case Operator::Add:
    return "(" + left + "+" + right + ")";
  case Operator::Sub:
    return "(" + left + "-" + right + ")";
  case Operator::Mul:
    return "(" + left + "*" + right + ")";
  case Operator::Div:
    return "(" + left + "/" + right + ")";
  case Operator::Exp:
    return "(exp(" + left + "," + right + "))";
  case Operator::Assign:
    return "(" + left + "=" + right + ")";
  case Operator::Equal:
    return "(" + left + "==" + right + ")";
  case Operator::Greater:
    return "(" + left + ">" + right + ")";
  case Operator::GreaterEq:
    return "(" + left + ">=" + right + ")";
  case Operator::Less:
    return "(" + left + "<" + right + ")";
  case Operator::LessEq:
    return "(" + left + "<=" + right + ")";
  case Operator::And:
    return "(" + left + "&&" + right + ")";
  case Operator::Or:
    return "(" + left + "||" + right + ")";
  case Operator::Not:
    return "(!" + right + ")";
  case Operator::Pos:
    return "(" + right + ")";
  case Operator::Neg:
    return "(-" + right + ")";
  case Operator::Ref:
    return "(&" + right + ")";
  case Operator::Index:
    if (op.right == nullptr) {
      return "(*" + left + ")";
    }
    return "(" + left + "[" + right + "]" + ")";
  default:
    break;
  }
  throw "unreachable";
}

std::string generate_one(const Expr &ex) {
  return std::visit([](auto &arg) -> std::string { return generate_one(arg); },
                    ex.value);
}

std::string generate_one(const Statement &stmt);

std::unordered_map<std::string, std::string> types{
    {"int", "int"}, {"float", "float"}, {"string", "char*"}};

std::string generate_type(const ASTType &var, std::string identifier) {
  std::string res{};
  res += types[var.name] + " "; // Assume exists I guess
  if (var.count == -1)
    res += "*";
  res += identifier;
  if (var.count > 0) {
    res += "[" + std::to_string(var.count) + "]";
  }
  return res;
}

std::string generate_one(const ASTVarDeclare &decl) {
  std::string res{};
  res += generate_type(decl.type, decl.name);
  if (decl.value.has_value()) {
    res += "=" + generate_one(decl.value.value()) + ";\n";
  }
  return res;
}

std::string generate_one(const ASTIf &stmt) {
  std::string res{};
  res += "if (" + generate_one(stmt.branches[0].first) + ") {\n";
  for (auto &s : stmt.branches[0].second) {
    res += generate_one(s);
  }
  res += "}";
  for (size_t i = 1; i < stmt.branches.size(); ++i) {
    res += "else if (" + generate_one(stmt.branches[i].first) + ") {\n";
    for (auto &s : stmt.branches[i].second) {
      res += generate_one(s);
    }
    res += "}";
  }
  if (stmt.otherwise.size() > 0) {
    res += "else {\n";
    for (auto &s : stmt.otherwise) {
      res += generate_one(s);
    }
    res += "}";
  }
  res += "\n";
  return res;
}

std::string generate_one(const ASTWhile &stmt) {
  std::string res{};
  res += "while (" + generate_one(stmt.condition) + ") {\n";
  for (auto &s : stmt.body) {
    res += generate_one(s);
  }
  res += "}\n";
  return res;
}

std::string generate_one(const ASTFuncDeclare &stmt) {
  std::string res{};
  res += stmt.ret.name;
  if (stmt.ret.count != 0)
    res += "*";
  res += " " + stmt.name + "(";
  for (size_t i = 0; i < stmt.args.size(); ++i) {
    res += generate_type(stmt.args[i].second, stmt.args[i].first);
    if (i != stmt.args.size() - 1)
      res += ",";
  }
  res += ") {\n";
  for (auto &s : stmt.body) {
    res += generate_one(s);
  }
  res += "}\n";
  return res;
}

std::string generate_one(const ASTBreak &) { return "break;"; }

std::string generate_one(const ASTReturn &ret) {
  std::string res{"return "};
  if (ret.what.has_value())
    res += generate_one(ret.what.value());
  res += ";";
  return res;
}

std::string generate_one(const Statement &stmt) {
  if (std::holds_alternative<Expr>(stmt.value)) {
    return generate_one(std::get<Expr>(stmt.value)) + ";\n";
  }

  return std::visit([](auto &arg) -> std::string { return generate_one(arg); },
                    stmt.value);
}

// Why? No clue, need this explicitly.
struct GenerateOneVisitor {
  std::string operator()(const Statement &stmt) const {
    return generate_one(stmt);
  }
  std::string operator()(const ASTFuncDeclare &f) const {
    return generate_one(f);
  }
};

std::string generate_one(const Paragraph &para) {
  return std::visit(GenerateOneVisitor{}, para);
}

std::string begin_file() {
  return "#include <math.h>\n#include<stdio.h>\n#include<stdlib.h>\n";
}
