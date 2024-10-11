#include <unordered_map>
#include <unordered_set>

#include "codegen.h"
#include "fmt.h"
#include "parser.h"

Parser::Parser(const std::string& file_name) : tokenizer_{file_name} {
  while (Token token = tokenizer_.next_token()) {
    tokenizer_.putback(token);
    log("now on %s", token.as_string().c_str());
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
    Codegen::instance().visitNode(expr.get());
  }
}

// literal ::= IntLiteral
std::unique_ptr<ExprAST> Parser::literal() {
  Token token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::IntLiteral) {
    error_expected(tokenizer_, token, "integer literal");
  }
  log("literal: %d", token.int_value());
  return std::make_unique<LiteralExprAST>(token.int_value());
}

// paren ::= LeftParen binary RightParen
std::unique_ptr<ExprAST> Parser::paren() {
  expect(Token::Kind::LeftParen, "(");
  auto expr = binary();
  if (!expr) error_expected(tokenizer_, tokenizer_.cur_token(), "expression");
  expect(Token::Kind::RightParen, ")");
  return expr;
}

// identifier ::= Identifier
//            ::= Identifier LeftParen (binary (Comma binary)*)? RightParen
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
  log("getting arguments for call, %s", name.c_str());
  while (true) {
    token = tokenizer_.next_token();
    if (token.kind() == Token::Kind::RightParen) break;
    tokenizer_.putback(token);
    auto arg = binary();
    if (!arg) error_expected(tokenizer_, tokenizer_.cur_token(), "expression");
    args.push_back(std::move(arg));
    log("got argument");
    token = tokenizer_.next_token();
    if (token.kind() == Token::Kind::RightParen) break;
    if (token.kind() != Token::Kind::Comma) {
      error_expected(tokenizer_, token, "comma or right parenthesis");
    }
  }
  log("got all arguments");
  return std::make_unique<CallExprAST>(name, std::move(args));
}

// primary ::= literal
//         ::= identifier
//         ::= paren
std::unique_ptr<ExprAST> Parser::primary() {
  Token token = tokenizer_.next_token();
  tokenizer_.putback(token);
  log("read primary: %s/%s, and put it back", token.lexeme().c_str(),
      token.as_string().c_str());
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
  static std::unordered_set<Token::Kind> prefix_operators = {
      Token::Kind::Not, Token::Kind::Plus, Token::Kind::Minus};
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
  static std::unordered_map<Token::Kind, int> operator_precedence = {
      {Token::Kind::Plus, 10},
      {Token::Kind::Minus, 10},
      {Token::Kind::Star, 20},
      {Token::Kind::Slash, 20}};
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
  log("got primary");
  Token op = tokenizer_.next_token();
  log("got operator: %s", op.as_string().c_str());
  if (is_terminator(op.kind())) {
    tokenizer_.putback(op);
    log("put back terminator");
    return lhs;
  }
  int precedence = get_binary_precedence(tokenizer_, op);
  while (precedence > prev_precedence) {
    auto rhs = binary(precedence);
    if (!rhs) error_expected(tokenizer_, tokenizer_.cur_token(), "expression");
    log("got rhs");
    lhs = std::make_unique<BinaryExprAST>(op.kind(), std::move(lhs),
                                          std::move(rhs));
    op = tokenizer_.cur_token();
    log("cur token: %s", op.as_string().c_str());
    if (is_terminator(op.kind())) {
      log("found terminator");
      return lhs;
    }
    precedence = get_binary_precedence(tokenizer_, op);
  }
  log("returning binary");
  return lhs;
}

// prototype ::= Identifier LeftParen (Identifier (Comma Identifier)*)?
// RightParen
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

// definition ::= Def prototype LeftBrace binary RightBrace
std::unique_ptr<ExprAST> Parser::definition() {
  expect(Token::Kind::Def, "function definition");
  auto proto = prototype();
  if (!proto) error_expected(tokenizer_, tokenizer_.cur_token(), "prototype");
  expect_lbrace();
  auto body = binary();
  if (!body)
    error_expected(tokenizer_, tokenizer_.cur_token(), "body expression");
  expect_semicolon();
  expect_rbrace();
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

// if_stmt ::= If LeftParen binary RightParen LeftBrace binary Semicolon
//             RightBrace (Else LeftBrace binary Semicolon RightBrace)?
std::unique_ptr<ExprAST> Parser::if_stmt() {
  expect(Token::Kind::If, "if");
  expect_lparen();
  auto cond = binary();
  if (!cond) error_expected(tokenizer_, tokenizer_.cur_token(), "condition");
  expect_rparen();
  expect_lbrace();
  auto then = binary();
  if (!then) error_expected(tokenizer_, tokenizer_.cur_token(), "then block");
  expect_semicolon();
  expect_rbrace();
  log("got if block");
  // else block is optional
  Token token = tokenizer_.next_token();
  if (token.kind() != Token::Kind::Else) {
    log("no else block");
    tokenizer_.putback(token);
    return std::make_unique<IfExprAST>(std::move(cond), std::move(then),
                                       nullptr);
  }
  expect_lbrace();
  auto els = binary();
  if (!els) error_expected(tokenizer_, tokenizer_.cur_token(), "else block");
  expect_semicolon();
  expect_rbrace();
  log("got else block");
  return std::make_unique<IfExprAST>(std::move(cond), std::move(then),
                                     std::move(els));
}

std::unique_ptr<ExprAST> Parser::top_level() {
  auto expr = binary();
  if (!expr) return nullptr;
  log("got top level expression");
  expect_semicolon();
  auto proto =
      std::make_unique<PrototypeAST>("main", std::vector<std::string>{});
  return std::make_unique<FunctionAST>(std::move(proto), std::move(expr));
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
