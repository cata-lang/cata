#include "codegen.h"
#include "fmt.h"

Codegen::Codegen()
    : context_{std::make_unique<LLVMContext>()},
      module_{std::make_unique<Module>("main", *context_)},
      builder_{std::make_unique<IRBuilder<>>(*context_)},
      named_values_{},
      function_prototypes_{} {}

Codegen& Codegen::instance() {
  static Codegen codegen;
  return codegen;
}

std::string Codegen::get_ir() const {
  std::string ir;
  raw_string_ostream os{ir};
  module_->print(os, nullptr);
  return os.str();
}

Value* Codegen::visitNode(ExprAST* node) {
  node->accept(*this);
  return static_cast<Value*>(visit_result_);
}

Function* Codegen::visitNode(PrototypeAST* node) {
  node->accept(*this);
  return static_cast<Function*>(visit_result_);
}

#define VISITOR_RETURN(value) \
  do {                        \
    visit_result_ = value;    \
    return;                   \
  } while (0)

void Codegen::visitLiteralNode(LiteralExprAST* node) {
  VISITOR_RETURN(ConstantInt::get(*context_, APInt(32, node->value())));
}

void Codegen::visitVariableNode(VariableExprAST* node) {
  AllocaInst* alloca = get_variable(node->name());
  if (!alloca) error("use of undeclared variable, %s", node->name().c_str());
  // load the value
  Value* value =
      builder_->CreateLoad(alloca->getAllocatedType(), alloca, node->name());
  VISITOR_RETURN(value);
}

void Codegen::visitPrefixNode(PrefixExprAST* node) {
  Value* operand = visitNode(node->operand().get());
  if (!operand) VISITOR_RETURN(nullptr);
  switch (node->op()) {
    case Token::Kind::Not: {
      Value* negated = builder_->CreateICmpEQ(
          operand, ConstantInt::get(*context_, APInt(32, 0)), "nottmp");
      VISITOR_RETURN(
          builder_->CreateZExt(negated, Type::getInt32Ty(*context_)));
    }
    case Token::Kind::Plus:
      VISITOR_RETURN(operand);
    case Token::Kind::Minus:
      VISITOR_RETURN(builder_->CreateNeg(operand, "negtmp"));
    case Token::Kind::Tilde:
      VISITOR_RETURN(builder_->CreateNot(operand, "nottmp"));
    default:
      error("invalid prefix operator, %s",
            Token(node->op()).as_string().c_str());
  }
}

void Codegen::visitBinaryNode(BinaryExprAST* node) {
  Value *lhs = visitNode(node->lhs().get()),
        *rhs = visitNode(node->rhs().get());
  if (!lhs || !rhs) VISITOR_RETURN(nullptr);
  Value* result = nullptr;
  switch (node->op()) {
    case Token::Kind::Equals: {
      VariableExprAST* lhs_var =
          dynamic_cast<VariableExprAST*>(node->lhs().get());
      if (!lhs_var) error("left hand side of assignment must be a variable");
      AllocaInst* alloca = get_variable(lhs_var->name());
      if (!alloca)
        error("use of undeclared variable, %s", lhs_var->name().c_str());
      builder_->CreateStore(rhs, alloca);
      VISITOR_RETURN(rhs);
    }
    case Token::Kind::Plus:
      VISITOR_RETURN(builder_->CreateAdd(lhs, rhs, "addtmp"));
    case Token::Kind::Minus:
      VISITOR_RETURN(builder_->CreateSub(lhs, rhs, "subtmp"));
    case Token::Kind::Star:
      VISITOR_RETURN(builder_->CreateMul(lhs, rhs, "multmp"));
    case Token::Kind::Slash:
      VISITOR_RETURN(builder_->CreateSDiv(lhs, rhs, "divtmp"));
    case Token::Kind::Remainder:
      VISITOR_RETURN(builder_->CreateSRem(lhs, rhs, "remtmp"));
    // bitwise
    case Token::Kind::Ampersand:
      VISITOR_RETURN(builder_->CreateAnd(lhs, rhs, "andtmp"));
    case Token::Kind::Pipe:
      VISITOR_RETURN(builder_->CreateOr(lhs, rhs, "ortmp"));
    case Token::Kind::Caret:
      VISITOR_RETURN(builder_->CreateXor(lhs, rhs, "xortmp"));
    case Token::Kind::Tilde:
      VISITOR_RETURN(builder_->CreateNot(rhs, "nottmp"));
    case Token::Kind::LeftShift:
      VISITOR_RETURN(builder_->CreateShl(lhs, rhs, "shltmp"));
    case Token::Kind::RightShift:
      VISITOR_RETURN(builder_->CreateAShr(lhs, rhs, "ashrtmp"));
    // logical
    case Token::Kind::And:
    case Token::Kind::Or: {
      lhs = builder_->CreateICmpNE(
          lhs, ConstantInt::get(*context_, APInt(32, 0)), "nelhs");
      rhs = builder_->CreateICmpNE(
          rhs, ConstantInt::get(*context_, APInt(32, 0)), "nerhs");
      if (node->op() == Token::Kind::And) {
        result = builder_->CreateAnd(lhs, rhs, "andtmp");
      } else {
        result = builder_->CreateOr(lhs, rhs, "ortmp");
      }
      break;
    }
    // comparison
    case Token::Kind::Eq:
      result = builder_->CreateICmpEQ(lhs, rhs, "eqtmp");
      break;
    case Token::Kind::Ne:
      result = builder_->CreateICmpNE(lhs, rhs, "netmp");
      break;
    case Token::Kind::Lt:
      result = builder_->CreateICmpSLT(lhs, rhs, "lttmp");
      break;
    case Token::Kind::Le:
      result = builder_->CreateICmpSLE(lhs, rhs, "letmp");
      break;
    case Token::Kind::Gt:
      result = builder_->CreateICmpSGT(lhs, rhs, "gttmp");
      break;
    case Token::Kind::Ge:
      result = builder_->CreateICmpSGE(lhs, rhs, "getmp");
      break;
    default:
      error("invalid binary operator, %s",
            Token(node->op()).as_string().c_str());
  }
  // extend the result to 32 bits
  VISITOR_RETURN(builder_->CreateZExt(result, Type::getInt32Ty(*context_)));
}

void Codegen::visitBlockNode(BlockExprAST* node) {
  Value* last_value = nullptr;
  for (auto& expr : node->exprs()) {
    last_value = visitNode(expr.get());
    if (!last_value) VISITOR_RETURN(nullptr);
  }
  VISITOR_RETURN(last_value);
}

void Codegen::visitCallNode(CallExprAST* node) {
  std::vector<Value*> args;
  for (size_t i = 0; i < node->args().size(); ++i) {
    args.push_back(visitNode(node->args()[i].get()));
    if (!args.back()) VISITOR_RETURN(nullptr);
  }
  std::vector<Type*> arg_types(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    arg_types[i] = args[i]->getType();
  }
  Function* callee = get_function(node->callee(), arg_types, true);
  if (!callee) error("called undefined function, %s", node->callee().c_str());
  if (callee->arg_size() != node->args().size())
    error("function %s expects %lu arguments, but got %lu",
          node->callee().c_str(), callee->arg_size(), node->args().size());
  VISITOR_RETURN(builder_->CreateCall(callee, args, "calltmp"));
}

// TODO: overwrite? previous prototype if it exists
void Codegen::visitPrototypeNode(PrototypeAST* node) {
  std::vector<Type*> arg_types(node->args().size(),
                               Type::getInt32Ty(*context_));
  FunctionType* function_type =
      FunctionType::get(Type::getInt32Ty(*context_), arg_types, false);
  Function* function = Function::Create(
      function_type, Function::ExternalLinkage, node->name(), module_.get());
  size_t i = 0;
  for (auto& arg : function->args()) {
    arg.setName(node->args()[i++]);
  }
  VISITOR_RETURN(function);
}

void Codegen::visitFunctionNode(FunctionAST* node) {
  auto& prototype = *node->prototype();
  // Assuming int32 for now
  std::vector<Type*> arg_types(prototype.args().size(),
                               Type::getInt32Ty(*context_));
  Function* function =
      get_function(node->prototype()->name(), arg_types, false);
  if (!function) function = visitNode(node->prototype().get());
  if (!function) VISITOR_RETURN(nullptr);
  for (size_t i = 0; i < function->arg_size(); ++i) {
    if (function->getArg(i)->getName() != prototype.args()[i])
      // the prototype is the "header" of this function, so the argument name
      // and prototype name are reversed
      error(
          "argument name, %s, does not match prototype, %s, in function %s "
          "argument %lu",
          prototype.args()[i].c_str(),
          function->getArg(i)->getName().str().c_str(),
          prototype.name().c_str(), i + 1);
  }
  function_prototypes_[prototype.name()] = std::move(node->prototype());
  function = get_function(prototype.name(), arg_types, true);
  if (!function)
    error("failed to create function, %s", prototype.name().c_str());
  BasicBlock* basic_block = BasicBlock::Create(*context_, "entry", function);
  builder_->SetInsertPoint(basic_block);
  begin_scope();
  for (auto& arg : function->args()) {
    arg.setName(prototype.args()[arg.getArgNo()]);
    // store the argument in an alloca at the beginning of the function
    IRBuilder<> tmp_builder(basic_block);
    AllocaInst* alloca = tmp_builder.CreateAlloca(Type::getInt32Ty(*context_),
                                                  nullptr, arg.getName());
    tmp_builder.CreateStore(&arg, alloca);
    set_variable(std::string(arg.getName()), alloca);
  }
  if (Value* ret = visitNode(node->body().get())) {
    builder_->CreateRet(ret);
    verifyFunction(*function);
    // TODO: optimize function
    VISITOR_RETURN(function);
  }
  function->eraseFromParent();
  VISITOR_RETURN(nullptr);
}

void Codegen::visitLetNode(LetExprAST* node) {
  Value* value = visitNode(node->expr().get());
  if (!value) VISITOR_RETURN(nullptr);
  AllocaInst* alloca = builder_->CreateAlloca(Type::getInt32Ty(*context_),
                                              nullptr, node->name());
  builder_->CreateStore(value, alloca);
  set_variable(node->name(), alloca);
  VISITOR_RETURN(value);
}

void Codegen::visitIfNode(IfExprAST* node) {
  Value* cond = visitNode(node->condition().get());
  if (!cond) VISITOR_RETURN(nullptr);
  cond = builder_->CreateICmpNE(cond, ConstantInt::get(*context_, APInt(32, 0)),
                                "ifcond");
  Function* function = builder_->GetInsertBlock()->getParent();
  BasicBlock *then_block = BasicBlock::Create(*context_, "then", function),
             *else_block = BasicBlock::Create(*context_, "else"),
             *merge_block = BasicBlock::Create(*context_, "ifcont");
  builder_->CreateCondBr(cond, then_block, else_block);
  builder_->SetInsertPoint(then_block);
  begin_scope();
  Value* then_value = visitNode(node->then_expr().get());
  end_scope();
  if (!then_value) VISITOR_RETURN(nullptr);
  builder_->CreateBr(merge_block);
  then_block = builder_->GetInsertBlock();
  function->insert(function->end(), else_block);
  builder_->SetInsertPoint(else_block);
  Value* else_value = nullptr;
  if (node->else_expr()) {
    begin_scope();
    else_value = visitNode(node->else_expr().get());
    end_scope();
    if (!else_value) VISITOR_RETURN(nullptr);
  }
  builder_->CreateBr(merge_block);
  else_block = builder_->GetInsertBlock();
  function->insert(function->end(), merge_block);
  builder_->SetInsertPoint(merge_block);
  PHINode* phi_node =
      builder_->CreatePHI(Type::getInt32Ty(*context_), 2, "iftmp");
  phi_node->addIncoming(then_value, then_block);
  if (else_value) {
    phi_node->addIncoming(else_value, else_block);
  } else {
    phi_node->addIncoming(ConstantInt::get(*context_, APInt(32, 0)),
                          else_block);
  }
  VISITOR_RETURN(phi_node);
}

void Codegen::begin_scope() {
  named_values_.push_back({});
}

void Codegen::end_scope() {
  named_values_.pop_back();
}

AllocaInst* Codegen::get_variable(const std::string& name) {
  // variables can be shadowed, so we start searching from the top
  for (auto it = named_values_.rbegin(); it != named_values_.rend(); ++it) {
    if (auto value = it->find(name); value != it->end()) return value->second;
  }
  return nullptr;
}

void Codegen::set_variable(const std::string& name, AllocaInst* alloca) {
  named_values_.back()[name] = alloca;
}

Function* Codegen::get_function(const std::string& name,
                                const std::vector<Type*>& arg_types,
                                bool expect_declared) {
  if (Function* function = module_->getFunction(name)) {
    if (!expect_declared && !function->empty())
      error("redefinition of function, %s", name.c_str());
    if (function->arg_size() != arg_types.size())
      error("function %s expects %lu arguments, but got %lu", name.c_str(),
            function->arg_size(), arg_types.size());
    for (size_t i = 0; i < arg_types.size(); ++i) {
      if (function->getArg(i)->getType() != arg_types[i])
        error("function %s argument %lu type mismatch", name.c_str(), i + 1);
    }
    return function;
  }
  if (std::unique_ptr<PrototypeAST>& prototype = function_prototypes_[name]) {
    return visitNode(prototype.get());
  }
  return nullptr;
}

#undef VISITOR_RETURN
