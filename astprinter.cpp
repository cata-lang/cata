#include "astprinter.h"

ASTPrinter::ASTPrinter() {}

void ASTPrinter::visitNode(ExprAST* node) {
  node->accept(*this);
}

void ASTPrinter::visitLiteralNode(LiteralExprAST* node) {
  os_ << node->value();
}

void ASTPrinter::visitVariableNode(VariableExprAST* node) {
  os_ << node->name();
}

void ASTPrinter::visitPrefixNode(PrefixExprAST* node) {
  os_ << Token(node->op());
  visitNode(node->operand().get());
}

void ASTPrinter::visitBinaryNode(BinaryExprAST* node) {
  os_ << "(";
  visitNode(node->lhs().get());
  os_ << " " << Token(node->op()) << " ";
  visitNode(node->rhs().get());
  os_ << ")";
}

void ASTPrinter::visitBlockNode(BlockExprAST* node) {
  indent();
  os_ << "{" << std::endl;
  for (size_t i = 0; i < node->exprs().size(); ++i) {
    visitNode(node->exprs()[i].get());
    if (i + 1 == node->exprs().size()) dedent();
    os_ << std::endl;
  }
  os_ << "}";
}

void ASTPrinter::visitCallNode(CallExprAST* node) {
  os_ << node->callee() << "(";
  for (size_t i = 0; i < node->args().size(); ++i) {
    if (i > 0) os_ << ", ";
    visitNode(node->args()[i].get());
  }
  os_ << ")";
}

void ASTPrinter::visitPrototypeNode(PrototypeAST* node) {
  os_ << node->name() << "(";
  for (size_t i = 0; i < node->args().size(); ++i) {
    if (i > 0) os_ << ", ";
    os_ << node->args()[i];
  }
  os_ << ")";
}

void ASTPrinter::visitFunctionNode(FunctionAST* node) {
  os_ << Token(Token::Kind::Def) << " ";
  visitNode(node->prototype().get());
  os_ << " ";
  visitNode(node->body().get());
}

void ASTPrinter::visitLetNode(LetExprAST* node) {
  os_ << Token(Token::Kind::Let) << " " << node->name() << " = ";
  visitNode(node->expr().get());
}

void ASTPrinter::visitIfNode(IfExprAST* node) {
  os_ << Token(Token::Kind::If) << " (";
  visitNode(node->condition().get());
  os_ << ") ";
  visitNode(node->then_expr().get());
  if (node->else_expr()) {
    os_ << " " << Token(Token::Kind::Else) << " ";
    visitNode(node->else_expr().get());
  }
}

std::string ASTPrinter::result() const {
  return os_.ss.str();
}

void ASTPrinter::clear() {
  os_.indent = 0;
  os_.ss.str(std::string());
}

std::ostream& operator<<(std::ostream& os, const ASTPrinter& printer) {
  return os << printer.result();
}

void ASTPrinter::indent() {
  os_.indent += 2;
}

void ASTPrinter::dedent() {
  os_.indent -= 2;
}
