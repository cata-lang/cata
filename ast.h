#pragma once

#include <iosfwd>
#include <memory>
#include <vector>

#include "token.h"

enum class ExprKind {
  Literal,
  Variable,
  Prefix,
  Binary,
  Call,
  Prototype,
  Function,
  If
};

class ASTNodeVisitor;

class ExprAST {
 public:
  ExprAST(ExprKind kind);
  virtual ~ExprAST() = default;

  ExprKind kind() const;
  virtual void accept(ASTNodeVisitor& visitor) = 0;

  friend std::ostream& operator<<(std::ostream& os, const ExprAST& expr);

 private:
  ExprKind kind_;

 protected:
  virtual void print(std::ostream& os) const = 0;
};

class LiteralExprAST : public ExprAST {
 public:
  LiteralExprAST(int value);

  int value() const;

  void accept(ASTNodeVisitor& visitor) override;

 private:
  int value_;

  void print(std::ostream& os) const override;
};

class VariableExprAST : public ExprAST {
 public:
  VariableExprAST(const std::string& name);

  const std::string& name() const;

  void accept(ASTNodeVisitor& visitor) override;

 private:
  std::string name_;

  void print(std::ostream& os) const override;
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

  void print(std::ostream& os) const override;
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

  void print(std::ostream& os) const override;
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

  void print(std::ostream& os) const override;
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

  void print(std::ostream& os) const override;
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

  void print(std::ostream& os) const override;
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

  void print(std::ostream& os) const override;
};

class ASTNodeVisitor {
 public:
  virtual void visitLiteralNode(LiteralExprAST* node) = 0;
  virtual void visitVariableNode(VariableExprAST* node) = 0;
  virtual void visitPrefixNode(PrefixExprAST* node) = 0;
  virtual void visitBinaryNode(BinaryExprAST* node) = 0;
  virtual void visitCallNode(CallExprAST* node) = 0;
  virtual void visitPrototypeNode(PrototypeAST* node) = 0;
  virtual void visitFunctionNode(FunctionAST* node) = 0;
  virtual void visitIfNode(IfExprAST* node) = 0;
};
