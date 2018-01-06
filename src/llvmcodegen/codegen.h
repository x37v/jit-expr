#pragma once 

#include "ast.h"
#include <llvm/IR/LLVMContext.h>

namespace llvm {
  class Module;
  class Function; 
  class Value;
  class BasicBlock;
}

namespace xnor {
  class LLVMCodeGenVisitor : public xnor::ast::Visitor {
    public:
      LLVMCodeGenVisitor();
      virtual ~LLVMCodeGenVisitor();
      virtual void visit(xnor::ast::Variable* v);
      virtual void visit(xnor::ast::Value<int>* v);
      virtual void visit(xnor::ast::Value<float>* v);
      virtual void visit(xnor::ast::Value<std::string>* v);
      virtual void visit(xnor::ast::Quoted* v);
      virtual void visit(xnor::ast::UnaryOp* v);
      virtual void visit(xnor::ast::BinaryOp* v);
      virtual void visit(xnor::ast::FunctionCall* v);
      virtual void visit(xnor::ast::ArrayAccess* v);
      virtual void visit(xnor::ast::ValueAssignment* v);
      virtual void visit(xnor::ast::ArrayAssignment* v);

      void run();
    private:
      llvm::LLVMContext mContext;
      llvm::Module * mModule = nullptr;
      llvm::Function * mMainFunction;
      llvm::Value * mValue;
      llvm::BasicBlock * mBlock;
  };
}
