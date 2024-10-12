#include "ast.h"
#include "fmt.h"

ExprAST::ExprAST(ExprKind kind) : kind_{kind} {}

ExprKind ExprAST::kind() const {
  return kind_;
}

LiteralExprAST::LiteralExprAST(int value)
    : ExprAST{ExprKind::Literal}, value_{value} {}

int LiteralExprAST::value() const {
  return value_;
}

void LiteralExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitLiteralNode(this);
}

VariableExprAST::VariableExprAST(const std::string& name)
    : ExprAST{ExprKind::Variable}, name_{name} {}

const std::string& VariableExprAST::name() const {
  return name_;
}

void VariableExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitVariableNode(this);
}

PrefixExprAST::PrefixExprAST(Token::Kind op, std::unique_ptr<ExprAST> operand)
    : ExprAST{ExprKind::Prefix}, op_{op}, operand_{std::move(operand)} {}

Token::Kind PrefixExprAST::op() const {
  return op_;
}

std::unique_ptr<ExprAST>& PrefixExprAST::operand() {
  return operand_;
}

void PrefixExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitPrefixNode(this);
}

BinaryExprAST::BinaryExprAST(Token::Kind op,
                             std::unique_ptr<ExprAST> lhs,
                             std::unique_ptr<ExprAST> rhs)
    : ExprAST{ExprKind::Binary},
      op_{op},
      lhs_{std::move(lhs)},
      rhs_{std::move(rhs)} {}

Token::Kind BinaryExprAST::op() const {
  return op_;
}

std::unique_ptr<ExprAST>& BinaryExprAST::lhs() {
  return lhs_;
}

std::unique_ptr<ExprAST>& BinaryExprAST::rhs() {
  return rhs_;
}

void BinaryExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitBinaryNode(this);
}

BlockExprAST::BlockExprAST(std::vector<std::unique_ptr<ExprAST>>&& exprs)
    : ExprAST{ExprKind::Block}, exprs_{std::move(exprs)} {}

std::vector<std::unique_ptr<ExprAST>>& BlockExprAST::exprs() {
  return exprs_;
}

void BlockExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitBlockNode(this);
}

CallExprAST::CallExprAST(const std::string& callee,
                         std::vector<std::unique_ptr<ExprAST>> args)
    : ExprAST{ExprKind::Call}, callee_{callee}, args_{std::move(args)} {}

const std::string& CallExprAST::callee() const {
  return callee_;
}

std::vector<std::unique_ptr<ExprAST>>& CallExprAST::args() {
  return args_;
}

void CallExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitCallNode(this);
}

PrototypeAST::PrototypeAST(const std::string& name,
                           std::vector<std::string> args)
    : ExprAST{ExprKind::Prototype}, name_{name}, args_{std::move(args)} {}

const std::string& PrototypeAST::name() const {
  return name_;
}

const std::vector<std::string>& PrototypeAST::args() const {
  return args_;
}

void PrototypeAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitPrototypeNode(this);
}

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> prototype,
                         std::unique_ptr<ExprAST> body)
    : ExprAST{ExprKind::Function},
      prototype_{std::move(prototype)},
      body_{std::move(body)} {}

std::unique_ptr<PrototypeAST>& FunctionAST::prototype() {
  return prototype_;
}

std::unique_ptr<ExprAST>& FunctionAST::body() {
  return body_;
}

void FunctionAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitFunctionNode(this);
}

LetExprAST::LetExprAST(const std::string& name, std::unique_ptr<ExprAST> expr)
    : ExprAST{ExprKind::Let}, name_{name}, expr_{std::move(expr)} {}

const std::string& LetExprAST::name() const {
  return name_;
}

std::unique_ptr<ExprAST>& LetExprAST::expr() {
  return expr_;
}

void LetExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitLetNode(this);
}

IfExprAST::IfExprAST(std::unique_ptr<ExprAST> condition,
                     std::unique_ptr<ExprAST> then_expr,
                     std::unique_ptr<ExprAST> else_expr)
    : ExprAST{ExprKind::If},
      condition_{std::move(condition)},
      then_expr_{std::move(then_expr)},
      else_expr_{std::move(else_expr)} {}

std::unique_ptr<ExprAST>& IfExprAST::condition() {
  return condition_;
}

std::unique_ptr<ExprAST>& IfExprAST::then_expr() {
  return then_expr_;
}

std::unique_ptr<ExprAST>& IfExprAST::else_expr() {
  return else_expr_;
}

void IfExprAST::accept(ASTNodeVisitor& visitor) {
  visitor.visitIfNode(this);
}
