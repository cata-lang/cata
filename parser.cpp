#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "astprinter.h"
#include "codegen.h"
#include "fmt.h"
#include "parser.h"

Parser::Parser(const std::string& file_name) : tokenizer_{file_name} {
  ASTPrinter printer;
  while (Token token = tokenizer_.next_token()) {
    printer.clear();
    tokenizer_.putback(token);
    log("Parsing %s", token.as_string().c_str());
    std::unique_ptr<ExprAST> expr;
    switch (token.kind()) {
      case Token::Kind::Def:
        expr = definition();
        break;
      case Token::Kind::Extern:
        expr = extern_proto();
        break;
      default:
        expr = top_level();
        break;
    }
    printer.visitNode(expr.get());
    // std::cout << printer << "\n";
    Codegen::instance().visitNode(expr.get());
  }
}

// literal ::= IntLiteral
std::unique_ptr<ExprAST> Parser::literal() {
  Token token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::IntLiteral) {
    error_expected(tokenizer_, token, "integer literal");
  }
  return std::make_unique<LiteralExprAST>(token.int_value());
}

// paren ::= '(' binary ')'
std::unique_ptr<ExprAST> Parser::paren() {
  expect(Token::Kind::LeftParen, "(");
  auto expr = binary();
  if (!expr) error_expected(tokenizer_, tokenizer_.cur_token(), "expression");
  expect(Token::Kind::RightParen, ")");
  return expr;
}

// identifier ::= Identifier
//            ::= Identifier '(' (binary (',' binary)*)? ')'
std::unique_ptr<ExprAST> Parser::identifier() {
  Token token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::Identifier) {
    error_expected(tokenizer_, token, "identifier");
  }
  std::string name = token.lexeme();
  token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::LeftParen) {
    tokenizer_.putback(token);
    return std::make_unique<VariableExprAST>(name);
  }
  std::vector<std::unique_ptr<ExprAST>> args;
  while (true) {
    token = tokenizer_.next_token();
    if (token.kind() == Token::Kind::RightParen) break;
    tokenizer_.putback(token);
    auto arg = binary();
    if (!arg) error_expected(tokenizer_, tokenizer_.cur_token(), "expression");
    args.push_back(std::move(arg));
    token = tokenizer_.next_token();
    if (token.kind() == Token::Kind::RightParen) break;
    if (token.kind() != Token::Kind::Comma) {
      error_expected(tokenizer_, token, "comma or right parenthesis");
    }
  }
  return std::make_unique<CallExprAST>(name, std::move(args));
}

// primary ::= literal
//         ::= identifier
//         ::= paren
std::unique_ptr<ExprAST> Parser::primary() {
  Token token = tokenizer_.next_token();
  tokenizer_.putback(token);
  switch (token.kind()) {
    case Token::Kind::Eof:
      return nullptr;
    case Token::Kind::Identifier:
      return identifier();
    case Token::Kind::IntLiteral:
      return literal();
    case Token::Kind::LeftParen:
      return paren();
    case Token::Kind::If:
      return if_stmt();
    default:
      error_expected(tokenizer_, token, "primary expression");
  }
}

// prefix ::= primary
//        ::= op prefix
std::unique_ptr<ExprAST> Parser::prefix() {
  static const std::unordered_set<Token::Kind> prefix_operators = {
      Token::Kind::Not, Token::Kind::Plus, Token::Kind::Minus,
      Token::Kind::Tilde};
  Token op = tokenizer_.next_token();
  if (prefix_operators.find(op.kind()) == prefix_operators.end()) {
    tokenizer_.putback(op);
    return primary();
  }
  auto operand = prefix();
  if (!operand) error_expected(tokenizer_, tokenizer_.cur_token(), "operand");
  return std::make_unique<PrefixExprAST>(op.kind(), std::move(operand));
}

static int get_binary_precedence(const Tokenizer& tokenizer, const Token& op) {
  static const std::unordered_map<Token::Kind, int> operator_precedence = {
      // ! ~
      {Token::Kind::Not, 100},
      {Token::Kind::Tilde, 100},
      // * / %
      {Token::Kind::Star, 90},
      {Token::Kind::Slash, 90},
      {Token::Kind::Remainder, 90},
      // + -
      {Token::Kind::Plus, 80},
      {Token::Kind::Minus, 80},
      // << >>
      {Token::Kind::LeftShift, 70},
      {Token::Kind::RightShift, 70},
      // < <= > >=
      {Token::Kind::Lt, 60},
      {Token::Kind::Le, 60},
      {Token::Kind::Gt, 60},
      {Token::Kind::Ge, 60},
      // == !=
      {Token::Kind::Eq, 50},
      {Token::Kind::Ne, 50},
      // &
      {Token::Kind::Ampersand, 35},
      // ^
      {Token::Kind::Caret, 30},
      // |
      {Token::Kind::Pipe, 25},
      // &&
      {Token::Kind::And, 20},
      // ||
      {Token::Kind::Or, 15},
      // =
      {Token::Kind::Equals, 10},
  };
  auto it = operator_precedence.find(op.kind());
  if (it == operator_precedence.end()) {
    error_expected(tokenizer, op, "operator");
  }
  return it->second;
}

// binary ::= prefix
//        ::= binary op binary
std::unique_ptr<ExprAST> Parser::binary(int prev_precedence) {
  static auto is_terminator = [](Token::Kind kind) {
    return kind == Token::Kind::RightParen || kind == Token::Kind::RightBrace ||
           kind == Token::Kind::Comma || kind == Token::Kind::Semicolon;
  };
  auto lhs = prefix();
  if (!lhs) return nullptr;
  Token op = tokenizer_.next_token();
  if (is_terminator(op.kind())) {
    tokenizer_.putback(op);
    return lhs;
  }
  int precedence = get_binary_precedence(tokenizer_, op);
  while (precedence > prev_precedence) {
    auto rhs = binary(precedence);
    if (!rhs) error_expected(tokenizer_, tokenizer_.cur_token(), "expression");
    lhs = std::make_unique<BinaryExprAST>(op.kind(), std::move(lhs),
                                          std::move(rhs));
    op = tokenizer_.cur_token();
    if (is_terminator(op.kind())) {
      return lhs;
    }
    precedence = get_binary_precedence(tokenizer_, op);
  }
  return lhs;
}

// statement ::= if_stmt
//           ::= let_stmt ';'
//           ::= binary ';'
std::unique_ptr<ExprAST> Parser::statement() {
  Token token = tokenizer_.next_token();
  tokenizer_.putback(token);
  std::unique_ptr<ExprAST> stmt;
  switch (token.kind()) {
    case Token::Kind::If:
      return if_stmt();
    case Token::Kind::Let:
      stmt = let_stmt();
      break;
    default:
      stmt = binary();
      break;
  }
  expect_semicolon();
  return stmt;
}

// block ::= '{' statement* '}'
std::unique_ptr<ExprAST> Parser::block() {
  expect_lbrace();
  std::vector<std::unique_ptr<ExprAST>> statements;
  while (Token token = tokenizer_.next_token()) {
    tokenizer_.putback(token);
    if (token.kind() == Token::Kind::RightBrace) break;
    statements.push_back(statement());
  }
  expect_rbrace();
  return std::make_unique<BlockExprAST>(std::move(statements));
}

// prototype ::= Identifier '(' (Identifier (',' Identifier)*)? ')'
std::unique_ptr<PrototypeAST> Parser::prototype() {
  Token token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::Identifier) {
    error_expected(tokenizer_, token, "function name");
  }
  std::string name = token.lexeme();
  expect(Token::Kind::LeftParen, "(");
  std::vector<std::string> args;
  while (true) {
    // get arg name or ')'
    token = tokenizer_.next_token();
    if (token.kind() == Token::Kind::RightParen) break;
    if (token.kind() != Token::Kind::Identifier) {
      error_expected(tokenizer_, token, "argument name");
    }
    args.push_back(token.lexeme());
    // get ',' or ')'
    token = tokenizer_.next_token();
    if (token.kind() == Token::Kind::RightParen) break;
    if (token.kind() != Token::Kind::Comma) {
      error_expected(tokenizer_, token, "comma or right parenthesis");
    }
  }
  return std::make_unique<PrototypeAST>(name, std::move(args));
}

// definition ::= Def prototype block
std::unique_ptr<ExprAST> Parser::definition() {
  expect(Token::Kind::Def, "function definition");
  auto proto = prototype();
  if (!proto) error_expected(tokenizer_, tokenizer_.cur_token(), "prototype");
  auto body = block();
  if (!body)
    error_expected(tokenizer_, tokenizer_.cur_token(), "body expression");
  return std::make_unique<FunctionAST>(std::move(proto), std::move(body));
}

// extern_proto ::= Extern prototype
std::unique_ptr<ExprAST> Parser::extern_proto() {
  expect(Token::Kind::Extern, "extern");
  auto proto = prototype();
  if (!proto) error_expected(tokenizer_, tokenizer_.cur_token(), "prototype");
  expect_semicolon();
  return proto;
}

// let_stmt ::= let Identifier '=' binary
std::unique_ptr<ExprAST> Parser::let_stmt() {
  expect(Token::Kind::Let, "let");
  Token token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::Identifier) {
    error_expected(tokenizer_, token, "variable name");
  }
  std::string name = token.lexeme();
  token = tokenizer_.next_token();
  tokenizer_.putback(token);
  if (token.kind() == Token::Kind::Semicolon) {
    return std::make_unique<LetExprAST>(name,
                                        std::make_unique<LiteralExprAST>(0));
  }
  expect(Token::Kind::Equals, "=");
  auto expr = binary();
  if (!expr) error_expected(tokenizer_, tokenizer_.cur_token(), "expression");
  return std::make_unique<LetExprAST>(name, std::move(expr));
}

// if_stmt ::= If '(' binary ')' block ('else' (block | if_stmt))?
std::unique_ptr<ExprAST> Parser::if_stmt() {
  expect(Token::Kind::If, "if");
  expect_lparen();
  auto cond = binary();
  if (!cond) error_expected(tokenizer_, tokenizer_.cur_token(), "condition");
  expect_rparen();
  auto then = block();
  if (!then) error_expected(tokenizer_, tokenizer_.cur_token(), "then block");
  // else block is optional
  Token token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::Else) {
    tokenizer_.putback(token);
    return std::make_unique<IfExprAST>(std::move(cond), std::move(then),
                                       nullptr);
  }
  token = tokenizer_.next_token();
  tokenizer_.putback(token);
  if (token.kind() == Token::Kind::If) {
    return std::make_unique<IfExprAST>(std::move(cond), std::move(then),
                                       if_stmt());
  }
  auto els = block();
  if (!els) error_expected(tokenizer_, tokenizer_.cur_token(), "else block");
  return std::make_unique<IfExprAST>(std::move(cond), std::move(then),
                                     std::move(els));
}

std::unique_ptr<ExprAST> Parser::top_level() {
  error("top level expressions are not supported yet");
  // auto expr = binary();
  // if (!expr) return nullptr;
  // expect_semicolon();
  // auto proto =
  //     std::make_unique<PrototypeAST>("main", std::vector<std::string>{});
  // return std::make_unique<FunctionAST>(std::move(proto), std::move(expr));
}

void Parser::expect(Token::Kind kind, const std::string& what) {
  Token token = tokenizer_.next_token();
  if (token.kind() != kind) {
    error_expected(tokenizer_, token, "%s", what.c_str());
  }
}

void Parser::expect_lparen() {
  expect(Token::Kind::LeftParen, "left parenthesis");
}

void Parser::expect_rparen() {
  expect(Token::Kind::RightParen, "right parenthesis");
}

void Parser::expect_lbrace() {
  expect(Token::Kind::LeftBrace, "opening brace");
}

void Parser::expect_rbrace() {
  expect(Token::Kind::RightBrace, "closing brace");
}

void Parser::expect_semicolon() {
  expect(Token::Kind::Semicolon, "semicolon");
}

int interpret_expr(std::unique_ptr<ExprAST>& expr) {
  switch (expr->kind()) {
    case ExprKind::Binary: {
      auto binary_expr = static_cast<BinaryExprAST*>(expr.get());
      int lhs = interpret_expr(binary_expr->lhs());
      int rhs = interpret_expr(binary_expr->rhs());
      // std::cout << lhs << " " << Token(binary_expr->op()) << " " << rhs
      //           << std::endl;
      switch (binary_expr->op()) {
        case Token::Kind::Plus:
          return lhs + rhs;
        case Token::Kind::Minus:
          return lhs - rhs;
        case Token::Kind::Star:
          return lhs * rhs;
        case Token::Kind::Slash:
          return lhs / rhs;
        default:
          error("unexpected binary operator");
      }
    }
    case ExprKind::Literal: {
      auto literal_expr = static_cast<LiteralExprAST*>(expr.get());
      return literal_expr->value();
    }
  }
  error("unexpected expression kind");
}
