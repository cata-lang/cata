#pragma once

#include <map>
#include <memory>
#include <vector>

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

#include "ast.h"

using namespace llvm;

class Codegen : ASTNodeVisitor {
 public:
  Codegen(Codegen const&) = delete;
  Codegen& operator=(Codegen const&) = delete;

  static Codegen& instance();

  std::string get_ir() const;

  Value* visitNode(ExprAST* node);
  Function* visitNode(PrototypeAST* node);

 private:
  void* visit_result_;
  std::unique_ptr<LLVMContext> context_;
  std::unique_ptr<Module> module_;
  std::unique_ptr<IRBuilder<>> builder_;
  std::vector<std::map<std::string, AllocaInst*>> named_values_;
  std::map<std::string, std::unique_ptr<PrototypeAST>> function_prototypes_;

  Codegen();

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

  void begin_scope();
  void end_scope();

  AllocaInst* get_variable(const std::string& name);
  void set_variable(const std::string& name, AllocaInst* alloca);

  Function* get_function(const std::string& name,
                         const std::vector<Type*>& arg_types,
                         bool expect_declared);
};
