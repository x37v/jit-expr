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
#include <llvm/IR/IRBuilder.h>
#include <llvm/Bitcode/BitstreamReader.h>
#include <llvm/Bitcode/BitstreamWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

using std::cout;
using std::endl;

namespace xnor {

  LLVMCodeGenVisitor::LLVMCodeGenVisitor() {
    mModule = new llvm::Module("xnor/expr", mContext);

    std::vector<llvm::Type*> argTypes;
    llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getFloatTy(mContext), llvm::makeArrayRef(argTypes), false);
    mMainFunction = llvm::Function::Create(ftype, llvm::GlobalValue::InternalLinkage, "main", mModule);
    mBlock = llvm::BasicBlock::Create(mContext, "entry", mMainFunction, 0);
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

    llvm::Instruction::BinaryOps instr;

    switch (v->op()) {
      case xnor::ast::BinaryOp::Op::ADD:
        instr = llvm::Instruction::Add; break;
      case xnor::ast::BinaryOp::Op::SUBTRACT:
        instr = llvm::Instruction::Sub; break;
      case xnor::ast::BinaryOp::Op::MULTIPLY:
        instr = llvm::Instruction::Mul; break;
      case xnor::ast::BinaryOp::Op::DIVIDE:
        instr = llvm::Instruction::SDiv; break;
        /*
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
    mValue = llvm::BinaryOperator::Create(instr, left, right, "", mBlock);
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

    llvm::ReturnInst::Create(mContext, mValue, mBlock);

    llvm::legacy::PassManager pm;
    pm.add(llvm::createPrintModulePass(llvm::outs()));
    pm.run(*mModule);

    std::cout << "Running code...\n";
    llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(mModule)).create();
    if (ee == nullptr)
      throw std::runtime_error("error creating llvm::EngineBuilder");
    ee->finalizeObject();

    std::vector<llvm::GenericValue> noargs;
    llvm::GenericValue v = ee->runFunction(mMainFunction, noargs);
    std::cout << "Code was run.\n";
  }

}
