#pragma once

#include <memory>
#include <vector>

#include "token.h"

enum class ExprKind {
  Literal,
  Variable,
  Prefix,
  Binary,
  Block,
  Call,
  Prototype,
  Function,
  Let,
  If
};

class ASTNodeVisitor;

class ExprAST {
 public:
  ExprAST(ExprKind kind);
  virtual ~ExprAST() = default;

  ExprKind kind() const;
  virtual void accept(ASTNodeVisitor& visitor) = 0;

 private:
  ExprKind kind_;
};

class LiteralExprAST : public ExprAST {
 public:
  LiteralExprAST(int value);

  int value() const;

  void accept(ASTNodeVisitor& visitor) override;

 private:
  int value_;
};

class VariableExprAST : public ExprAST {
 public:
  VariableExprAST(const std::string& name);

  const std::string& name() const;

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::string name_;
};

class PrefixExprAST : public ExprAST {
 public:
  PrefixExprAST(Token::Kind op, std::unique_ptr<ExprAST> operand);

  Token::Kind op() const;
  std::unique_ptr<ExprAST>& operand();

  void accept(ASTNodeVisitor& visitor) override;

 private:
  Token::Kind op_;
  std::unique_ptr<ExprAST> operand_;
};

class BinaryExprAST : public ExprAST {
 public:
  BinaryExprAST(Token::Kind op,
                std::unique_ptr<ExprAST> lhs,
                std::unique_ptr<ExprAST> rhs);

  Token::Kind op() const;
  std::unique_ptr<ExprAST>& lhs();
  std::unique_ptr<ExprAST>& rhs();

  void accept(ASTNodeVisitor& visitor) override;

 private:
  Token::Kind op_;
  std::unique_ptr<ExprAST> lhs_, rhs_;
};

class BlockExprAST : public ExprAST {
 public:
  BlockExprAST(std::vector<std::unique_ptr<ExprAST>>&& exprs);

  std::vector<std::unique_ptr<ExprAST>>& exprs();

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::vector<std::unique_ptr<ExprAST>> exprs_;
};

class CallExprAST : public ExprAST {
 public:
  CallExprAST(const std::string& callee,
              std::vector<std::unique_ptr<ExprAST>> args);

  const std::string& callee() const;
  std::vector<std::unique_ptr<ExprAST>>& args();

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::string callee_;
  std::vector<std::unique_ptr<ExprAST>> args_;
};

class PrototypeAST : public ExprAST {
 public:
  PrototypeAST(const std::string& name, std::vector<std::string> args);

  const std::string& name() const;
  const std::vector<std::string>& args() const;

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::string name_;
  std::vector<std::string> args_;
};

class FunctionAST : public ExprAST {
 public:
  FunctionAST(std::unique_ptr<PrototypeAST> prototype,
              std::unique_ptr<ExprAST> body);

  std::unique_ptr<PrototypeAST>& prototype();
  std::unique_ptr<ExprAST>& body();

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::unique_ptr<PrototypeAST> prototype_;
  std::unique_ptr<ExprAST> body_;
};

class LetExprAST : public ExprAST {
 public:
  LetExprAST(const std::string& name, std::unique_ptr<ExprAST> expr);

  const std::string& name() const;
  std::unique_ptr<ExprAST>& expr();

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::string name_;
  std::unique_ptr<ExprAST> expr_;
};

class IfExprAST : public ExprAST {
 public:
  IfExprAST(std::unique_ptr<ExprAST> condition,
            std::unique_ptr<ExprAST> then_expr,
            std::unique_ptr<ExprAST> else_expr);

  std::unique_ptr<ExprAST>& condition();
  std::unique_ptr<ExprAST>& then_expr();
  std::unique_ptr<ExprAST>& else_expr();

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::unique_ptr<ExprAST> condition_, then_expr_, else_expr_;
};

class ASTNodeVisitor {
 public:
  virtual void visitLiteralNode(LiteralExprAST* node) = 0;
  virtual void visitVariableNode(VariableExprAST* node) = 0;
  virtual void visitPrefixNode(PrefixExprAST* node) = 0;
  virtual void visitBinaryNode(BinaryExprAST* node) = 0;
  virtual void visitBlockNode(BlockExprAST* node) = 0;
  virtual void visitCallNode(CallExprAST* node) = 0;
  virtual void visitPrototypeNode(PrototypeAST* node) = 0;
  virtual void visitFunctionNode(FunctionAST* node) = 0;
  virtual void visitLetNode(LetExprAST* node) = 0;
  virtual void visitIfNode(IfExprAST* node) = 0;
};
