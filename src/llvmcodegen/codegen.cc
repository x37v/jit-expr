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

  LLVMCodeGenVisitor::LLVMCodeGenVisitor(xnor::ast::VariableVector variables) :
    mVariables(variables),
    mContext(),
    mBuilder(mContext),
    mTargetMachine(llvm::EngineBuilder().selectTarget()),
    mDataLayout(mTargetMachine->createDataLayout()),
    mObjectLayer([]() { return std::make_shared<llvm::SectionMemoryManager>(); }),
    mCompileLayer(mObjectLayer, llvm::orc::SimpleCompiler(*mTargetMachine))
  {
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr); //XXX do we want this?

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
    //XXX should we use internal linkage and figure out how to grab those symbols?
    mMainFunction = llvm::Function::Create(ftype, llvm::GlobalValue::ExternalLinkage, "expralex", mModule.get());
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
    v->node()->accept(this);
    auto right = mValue;

    switch (v->op()) {
      case xnor::ast::UnaryOp::Op::BIT_NOT:
        mValue = toFloat(mBuilder.CreateNot(toInt(right), "nottmp"));
        return;
      case xnor::ast::UnaryOp::Op::LOGICAL_NOT:
        //logical not is the same as x == 0
        mValue = wrapLogic(mBuilder.CreateFCmpOEQ(right, llvm::ConstantFP::get(llvm::Type::getFloatTy(mContext), 0.0f), "eqtmp"));
        return;
      case xnor::ast::UnaryOp::Op::NEGATE:
        mValue = mBuilder.CreateNeg(right, "negtmp");
        return;
    }
    throw std::runtime_error("unimplemented");
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
      case xnor::ast::BinaryOp::Op::DIVIDE:
        mValue = mBuilder.CreateFDiv(left, right, "divtmp");
        return;
      case xnor::ast::BinaryOp::Op::SHIFT_LEFT:
        mValue = toFloat(mBuilder.CreateShl(toInt(left), toInt(right), "sltmp"));
        return;
      case xnor::ast::BinaryOp::Op::SHIFT_RIGHT:
        mValue = toFloat(mBuilder.CreateLShr(toInt(left), toInt(right), "srtmp"));
        return;
      case xnor::ast::BinaryOp::Op::COMP_EQUAL:
        mValue = wrapLogic(mBuilder.CreateFCmpOEQ(left, right, "eqtmp"));
        return;
      case xnor::ast::BinaryOp::Op::COMP_NOT_EQUAL:
        mValue = wrapLogic(mBuilder.CreateFCmpONE(left, right, "neqtmp"));
        return;
      case xnor::ast::BinaryOp::Op::COMP_GREATER:
        mValue = wrapLogic(mBuilder.CreateFCmpOGT(left, right, "gttmp"));
        return;
      case xnor::ast::BinaryOp::Op::COMP_LESS:
        mValue = wrapLogic(mBuilder.CreateFCmpOLT(left, right, "lttmp"));
        return;
      case xnor::ast::BinaryOp::Op::COMP_GREATER_OR_EQUAL:
        mValue = wrapLogic(mBuilder.CreateNot(mBuilder.CreateFCmpOLT(left, right, "lttmp"), "nottmp"));
        return;
      case xnor::ast::BinaryOp::Op::COMP_LESS_OR_EQUAL:
        mValue = wrapLogic(mBuilder.CreateNot(mBuilder.CreateFCmpOGT(left, right, "lttmp"), "nottmp"));
        return;
      case xnor::ast::BinaryOp::Op::LOGICAL_OR:
        //just use bitwise then not equal 0
        mValue = wrapLogic(mBuilder.CreateICmpNE(mBuilder.CreateOr(toInt(left), toInt(right), "ortmp"),
              llvm::ConstantInt::get(llvm::Type::getInt32Ty(mContext), 0), "neqtmp"));
        return;
      case xnor::ast::BinaryOp::Op::LOGICAL_AND:
        //just use bitwise then not equal 0
        mValue = wrapLogic(mBuilder.CreateICmpNE(mBuilder.CreateAnd(toInt(left), toInt(right), "andtmp"),
              llvm::ConstantInt::get(llvm::Type::getInt32Ty(mContext), 0), "neqtmp"));
        return;
      case xnor::ast::BinaryOp::Op::BIT_AND:
        mValue = wrapLogic(mBuilder.CreateAnd(toInt(left), toInt(right), "andtmp"));
        return;
      case xnor::ast::BinaryOp::Op::BIT_OR:
        mValue = wrapLogic(mBuilder.CreateOr(toInt(left), toInt(right), "ortmp"));
        return;
      case xnor::ast::BinaryOp::Op::BIT_XOR:
        mValue = wrapLogic(mBuilder.CreateXor(toInt(left), toInt(right), "xortmp"));
        return;
    }
    throw std::runtime_error("not supported yet");
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
    //mModule->print(llvm::errs(), nullptr);

    auto Resolver = llvm::orc::createLambdaResolver(
        [&](const std::string &Name) {
          if (auto Sym = findMangledSymbol(Name))
            return Sym;
          return llvm::JITSymbol(nullptr);
        },
        [](const std::string &S) { return nullptr; });
    auto H = llvm::cantFail(mCompileLayer.addModule(std::move(mModule), std::move(Resolver)));
    mModuleHandles.push_back(H);


    auto ExprSymbol = findSymbol("expralex");
    if (!ExprSymbol)
      throw std::runtime_error("couldn't find symbol 'expralex'");

    float (*FP)() = (float (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
    cout << "Evaluated to " << FP() << endl;
  }

  llvm::JITSymbol LLVMCodeGenVisitor::findSymbol(const std::string Name) {
    return findMangledSymbol(mangle(Name));
  }

  llvm::JITSymbol LLVMCodeGenVisitor::findMangledSymbol(const std::string &Name) {

#ifdef LLVM_ON_WIN32
    // The symbol lookup of ObjectLinkingLayer uses the SymbolRef::SF_Exported
    // flag to decide whether a symbol will be visible or not, when we call
    // IRCompileLayer::findSymbolIn with ExportedSymbolsOnly set to true.
    //
    // But for Windows COFF objects, this flag is currently never set.
    // For a potential solution see: https://reviews.llvm.org/rL258665
    // For now, we allow non-exported symbols on Windows as a workaround.
    const bool ExportedSymbolsOnly = false;
#else
    const bool ExportedSymbolsOnly = true;
#endif

    // Search modules in reverse order: from last added to first added.
    // This is the opposite of the usual search order for dlsym, but makes more
    // sense in a REPL where we want to bind to the newest available definition.

    for (auto H : llvm::make_range(mModuleHandles.rbegin(), mModuleHandles.rend()))
      if (auto Sym = mCompileLayer.findSymbolIn(H, Name, ExportedSymbolsOnly))
        return Sym;

    // If we can't find the symbol in the JIT, try looking in the host process.
    if (auto SymAddr = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name))
      return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);

#ifdef LLVM_ON_WIN32
    // For Windows retry without "_" at beginning, as RTDyldMemoryManager uses
    // GetProcAddress and standard libraries like msvcrt.dll use names
    // with and without "_" (for example "_itoa" but "sin").
    if (Name.length() > 2 && Name[0] == '_')
      if (auto SymAddr =
              llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name.substr(1)))
        return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);
#endif

    return nullptr;
  }

  std::string LLVMCodeGenVisitor::mangle(const std::string &Name) {
    std::string MangledName;
    {
      llvm::raw_string_ostream MangledNameStream(MangledName);
      llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, mDataLayout);
    }
    return MangledName;
  }

  llvm::Value * LLVMCodeGenVisitor::wrapLogic(llvm::Value * v) {
    return mBuilder.CreateUIToFP(v, llvm::Type::getFloatTy(mContext), "cast");
  }

  llvm::Value * LLVMCodeGenVisitor::toInt(llvm::Value * v) {
    return mBuilder.CreateFPToSI(v, llvm::Type::getInt32Ty(mContext), "cast");
  }

  llvm::Value * LLVMCodeGenVisitor::toFloat(llvm::Value * v) {
    return mBuilder.CreateSIToFP(v, llvm::Type::getFloatTy(mContext), "cast");
  }

}
