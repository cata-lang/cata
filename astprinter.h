#pragma once

#include <sstream>

#include "ast.h"

class ASTPrinter : public ASTNodeVisitor {
 public:
  ASTPrinter();

  void visitNode(ExprAST* node);

  void visitLiteralNode(LiteralExprAST* node) override;
  void visitVariableNode(VariableExprAST* node) override;
  void visitPrefixNode(PrefixExprAST* node) override;
  void visitBinaryNode(BinaryExprAST* node) override;
  void visitBlockNode(BlockExprAST* node) override;
  void visitCallNode(CallExprAST* node) override;
  void visitPrototypeNode(PrototypeAST* node) override;
  void visitFunctionNode(FunctionAST* node) override;
  void visitLetNode(LetExprAST* node) override;
  void visitIfNode(IfExprAST* node) override;

  std::string result() const;
  void clear();

  friend std::ostream& operator<<(std::ostream& os, const ASTPrinter& printer);

 private:
  struct IndentStream {
    std::stringstream ss;
    int indent = 0;

    template <typename T>
    IndentStream& operator<<(const T& t) {
      ss << t;
      return *this;
    }

    IndentStream& operator<<(std::ostream& (*f)(std::ostream&)) {
      f(ss);
      if (f == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)) {
        ss << std::string(indent, ' ');
      }
      return *this;
    }

  } os_;

  void indent();
  void dedent();
};
