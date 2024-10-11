#pragma once

#include <memory>

#include "ast.h"
#include "tokenizer.h"

class Parser {
 public:
  Parser(const std::string& file_name);

  std::unique_ptr<ExprAST> literal();
  std::unique_ptr<ExprAST> paren();
  std::unique_ptr<ExprAST> identifier();
  std::unique_ptr<ExprAST> primary();
  std::unique_ptr<ExprAST> prefix();
  std::unique_ptr<ExprAST> binary(int prev_precedence = 0);
  std::unique_ptr<ExprAST> statement();
  std::unique_ptr<ExprAST> block();
  std::unique_ptr<PrototypeAST> prototype();
  std::unique_ptr<ExprAST> definition();
  std::unique_ptr<ExprAST> extern_proto();
  std::unique_ptr<ExprAST> let_stmt();
  std::unique_ptr<ExprAST> if_stmt();
  std::unique_ptr<ExprAST> top_level();

 private:
  Tokenizer tokenizer_;

  void expect(Token::Kind kind, const std::string& what);
  void expect_lparen();
  void expect_rparen();
  void expect_lbrace();
  void expect_rbrace();
  void expect_semicolon();
};

int interpret_expr(std::unique_ptr<ExprAST>& expr);
