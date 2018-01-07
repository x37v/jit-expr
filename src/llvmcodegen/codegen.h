#pragma once 

#include "ast.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>

#include <llvm/ADT/iterator_range.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

namespace llvm {
  class Module;
  class Function; 
  class Value;
  class BasicBlock;
  class TargetMachine;
}


namespace xnor {
  class LLVMCodeGenVisitor : public xnor::ast::Visitor {
    public:

      using ObjLayerT = llvm::orc::RTDyldObjectLinkingLayer;
      using CompileLayerT = llvm::orc::IRCompileLayer<ObjLayerT, llvm::orc::SimpleCompiler>;
      using ModuleHandleT = CompileLayerT::ModuleHandleT;

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
      llvm::IRBuilder<> mBuilder;

      std::unique_ptr<llvm::TargetMachine> mTargetMachine;
      std::unique_ptr<llvm::Module> mModule;
      std::unique_ptr<llvm::legacy::FunctionPassManager> mFunctionPassManager;

      llvm::Function * mMainFunction;
      llvm::Value * mValue;
      llvm::BasicBlock * mBlock;

      const llvm::DataLayout mDataLayout;
      ObjLayerT mObjectLayer;
      CompileLayerT mCompileLayer;

      llvm::JITSymbol findMangledSymbol(const std::string& name);
  };
}
