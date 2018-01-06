#include <vector>
#include "codegen.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Bitcode/BitstreamReader.h>
#include <llvm/Bitcode/BitstreamWriter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

namespace xnor {

  LLVMCodeGenVisitor::LLVMCodeGenVisitor() {
    mModule = new llvm::Module("xnor/expr", mContext);

    std::vector<llvm::Type*> argTypes;
    llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(mContext), llvm::makeArrayRef(argTypes), false);
    mMainFunction = llvm::Function::Create(ftype, llvm::GlobalValue::InternalLinkage, "main", mModule);
    //BasicBlock *bblock = BasicBlock::Create(MyContext, "entry", mainFunction, 0);
  }

  LLVMCodeGenVisitor::~LLVMCodeGenVisitor() {
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Variable* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<int>* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<float>* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<std::string>* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Quoted* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::UnaryOp* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::BinaryOp* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::FunctionCall* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ArrayAccess* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ValueAssignment* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ArrayAssignment* v){
  }

}
