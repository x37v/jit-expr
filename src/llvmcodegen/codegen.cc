#include "codegen.h"

#include <vector>
#include <iostream>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitstreamReader.h>
#include <llvm/Bitcode/BitstreamWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

using std::cout;
using std::endl;

namespace xnor {

  LLVMCodeGenVisitor::LLVMCodeGenVisitor() :
    mContext(),
    mBuilder(mContext),
    mTargetMachine(llvm::EngineBuilder().selectTarget()),
    mDataLayout(mTargetMachine->createDataLayout()),
    mObjectLayer([]() { return std::make_shared<llvm::SectionMemoryManager>(); }),
    mCompileLayer(mObjectLayer, llvm::orc::SimpleCompiler(*mTargetMachine))
  {
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

    mModule = llvm::make_unique<llvm::Module>("xnor/expr", mContext);
    mModule->setDataLayout(mDataLayout);

    mFunctionPassManager = llvm::make_unique<llvm::legacy::FunctionPassManager>(mModule.get());

    // Do simple "peephole" optimizations and bit-twiddling optzns.
    mFunctionPassManager->add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    mFunctionPassManager->add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    mFunctionPassManager->add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    mFunctionPassManager->add(llvm::createCFGSimplificationPass());
    mFunctionPassManager->doInitialization();


    std::vector<llvm::Type*> argTypes;
    llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getFloatTy(mContext), llvm::makeArrayRef(argTypes), false);

    mMainFunction = llvm::Function::Create(ftype, llvm::GlobalValue::InternalLinkage, "main", mModule.get());
    mBlock = llvm::BasicBlock::Create(mContext, "entry", mMainFunction, 0);

    mBuilder.SetInsertPoint(mBlock);
  }

  LLVMCodeGenVisitor::~LLVMCodeGenVisitor() {
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Variable* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<int>* v){
    mValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(mContext), static_cast<float>(v->value()));
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<float>* v){
    mValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(mContext), v->value());
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<std::string>* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Quoted* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::UnaryOp* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::BinaryOp* v){
    v->left()->accept(this);
    auto left = mValue;

    v->right()->accept(this);
    auto right = mValue;

    switch (v->op()) {
      case xnor::ast::BinaryOp::Op::ADD:
        mValue = mBuilder.CreateFAdd(left, right, "addtmp");
        return;
      case xnor::ast::BinaryOp::Op::SUBTRACT:
        mValue = mBuilder.CreateFSub(left, right, "subtmp");
        return;
      case xnor::ast::BinaryOp::Op::MULTIPLY:
        mValue = mBuilder.CreateFMul(left, right, "multmp");
        return;
        /*
      case xnor::ast::BinaryOp::Op::DIVIDE:
      case xnor::ast::BinaryOp::Op::SHIFT_LEFT:
        op = "<<"; break;
      case xnor::ast::BinaryOp::Op::SHIFT_RIGHT:
        op = ">>"; break;
      case xnor::ast::BinaryOp::Op::COMP_EQUAL:
        op = "=="; break;
      case xnor::ast::BinaryOp::Op::COMP_NOT_EQUAL:
        op = "!="; break;
      case xnor::ast::BinaryOp::Op::COMP_GREATER:
        op = ">"; break;
      case xnor::ast::BinaryOp::Op::COMP_LESS:
        op = "<"; break;
      case xnor::ast::BinaryOp::Op::COMP_GREATER_OR_EQUAL:
        op = ">="; break;
      case xnor::ast::BinaryOp::Op::COMP_LESS_OR_EQUAL:
        op = "<="; break;
      case xnor::ast::BinaryOp::Op::LOGICAL_OR:
        op = "||"; break;
      case xnor::ast::BinaryOp::Op::LOGICAL_AND:
        op = "&&"; break;
      case xnor::ast::BinaryOp::Op::BIT_AND:
        op = "~"; break;
      case xnor::ast::BinaryOp::Op::BIT_OR:
        op = "|"; break;
      case xnor::ast::BinaryOp::Op::BIT_XOR:
        op = "^"; break;
        */
      default:
        throw std::runtime_error("not supported yet");
    }
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::FunctionCall* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ArrayAccess* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ValueAssignment* v){
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ArrayAssignment* v){
  }

  void LLVMCodeGenVisitor::run() {
    if (mValue == nullptr)
      throw std::runtime_error("null return value");

    mBuilder.CreateRet(mValue);
    llvm::verifyFunction(*mMainFunction);
    mFunctionPassManager->run(*mMainFunction);

    mModule->print(llvm::errs(), nullptr);
  }

}
