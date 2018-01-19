#include "codegen.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

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
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

using std::cout;
using std::endl;

namespace {
  const std::vector<std::string> table_functions = {"Sum", "sum", "size"};
  const std::string main_function_name = "expralex";
}

namespace xnor {

  void LLVMCodeGenVisitor::init() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
  }

  LLVMCodeGenVisitor::LLVMCodeGenVisitor() :
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

#if 1
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    mFunctionPassManager->add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    mFunctionPassManager->add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    mFunctionPassManager->add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    mFunctionPassManager->add(llvm::createCFGSimplificationPass());
#endif

    mFunctionPassManager->doInitialization();

    std::vector<llvm::Type*> argTypes;
    argTypes.push_back(llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getFloatTy(mContext), 0), 0));

    std::vector<llvm::Type*> unionTypes;
    unionTypes.push_back(llvm::PointerType::get(llvm::Type::getFloatTy(mContext), 0));
    mInputType = llvm::StructType::create(llvm::makeArrayRef(unionTypes), "union.input_arg_t");
    auto sp = llvm::PointerType::get(mInputType, 0);
    argTypes.push_back(sp);

    //opaque
    mSymbolType = llvm::StructType::create(mContext, "t_symbol_ptr");

    llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(mContext), llvm::makeArrayRef(argTypes), false);
    //XXX should we use internal linkage and figure out how to grab those symbols?
    mMainFunction = llvm::Function::Create(ftype, llvm::GlobalValue::InternalLinkage, main_function_name, mModule.get());
    mBlock = llvm::BasicBlock::Create(mContext, "entry", mMainFunction, 0);
    mBuilder.SetInsertPoint(mBlock);

    //setup arguments 
    auto it = mMainFunction->args().begin();
    it->setName("out");
    mOutput = it;

    it++;
    it->setName("in");

    mInput = it;
    llvm::Value * input = mBuilder.CreateAlloca(sp, (unsigned)0);
    /*llvm::Value * inarg =*/ mBuilder.CreateStore(mInput, input);
    mInput = input;
  }

  LLVMCodeGenVisitor::~LLVMCodeGenVisitor() {
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Variable* v){
    //XXX is there a better index?
    auto index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(mContext), v->input_index());

    llvm::Value * cur = mBuilder.CreateLoad(mInput);
    cur = mBuilder.CreateInBoundsGEP(mInputType, cur, index);
    switch(v->type()) {
      case xnor::ast::Variable::VarType::FLOAT:
        cur = mBuilder.CreateBitCast(cur, llvm::PointerType::get(llvm::Type::getFloatTy(mContext), 0));
        mValue = mBuilder.CreateLoad(cur);
        break;
      case xnor::ast::Variable::VarType::INT:
        {
          cur = mBuilder.CreateBitCast(cur, llvm::PointerType::get(llvm::Type::getFloatTy(mContext), 0));
          mValue = mBuilder.CreateLoad(cur);

          //use floorf function
          auto n = "floorf";
          llvm::Function * f = mModule->getFunction(n);
          if (!f) {
            std::vector<llvm::Type *> args = {llvm::Type::getFloatTy(mContext)};
            llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getFloatTy(mContext), args, false);
            f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, n, mModule.get());
            if (!f)
              throw std::runtime_error("cannot find floorf function");
            //XXX is it a leak if we don't store this somewhere??
          }

          //visit the children, store them in the args
          std::vector<llvm::Value *> args = { mValue };
          mValue = mBuilder.CreateCall(f, args, "calltmp");
        }
        break;
      default:
        throw std::runtime_error("type not supported yet");
    }
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<int>* v){
    mValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(mContext), static_cast<float>(v->value()));
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<float>* v){
    mValue = llvm::ConstantFP::get(llvm::Type::getFloatTy(mContext), v->value());
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Value<std::string>* v){
    /*
    t_symbol * sym = gensym(v->value().c_str());
    auto pt = llvm::PointerType::get(mSymbolType, 0);
    */
    throw std::runtime_error("not implemented");
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Quoted* v){
    throw std::runtime_error("not implemented");
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
    auto n = v->name();

    //if
    if (n == "if") {
      //translated from kaleidoscope example, chapter 5
      v->args().at(0)->accept(this);

      //true if not equal to zero
      auto cond = mBuilder.CreateFCmpONE(mValue, llvm::ConstantFP::get(llvm::Type::getFloatTy(mContext), 0.0f), "neqtmp");

      llvm::Function *f = mBuilder.GetInsertBlock()->getParent();
      auto thenbb = llvm::BasicBlock::Create(mContext, "then", f);
      auto elsebb = llvm::BasicBlock::Create(mContext, "else");
      auto mergebb = llvm::BasicBlock::Create(mContext, "ifcont");

      mBuilder.CreateCondBr(cond, thenbb, elsebb);

      mBuilder.SetInsertPoint(thenbb);
      v->args().at(1)->accept(this);
      auto thenv = mValue;
      if (!thenv)
        throw std::runtime_error("couldn't create true expression");

      mBuilder.CreateBr(mergebb);
      thenbb = mBuilder.GetInsertBlock(); //codegen of then can change current block so update thenbb for phi
      mMainFunction->getBasicBlockList().push_back(elsebb);

      mBuilder.SetInsertPoint(elsebb);
      v->args().at(2)->accept(this);
      auto elsev = mValue;
      if (!elsev)
        throw std::runtime_error("couldn't create else expression");

      mBuilder.CreateBr(mergebb);
      elsebb = mBuilder.GetInsertBlock(); //codegen can change the current block so update elsebb for phi
      mMainFunction->getBasicBlockList().push_back(mergebb);

      //bring everything together
      mBuilder.SetInsertPoint(mergebb);
      auto *phi = mBuilder.CreatePHI(llvm::Type::getFloatTy(mContext), 2, "iftmp");
      phi->addIncoming(thenv, thenbb);
      phi->addIncoming(elsev, elsebb);
      mValue = phi;
      return;
    }

    //table functions
    auto it = std::find(table_functions.begin(), table_functions.end(), n);
    if (it != table_functions.end())
      throw std::runtime_error("table functions not supported yet");

    //all other functions, math taking and returning floats

    //alias
    if (n == "ln")
      n = "log";
    else if (n == "fact")
      n = "xnor_expr_fact";
    n = n + "f"; //we're using the floating point version of these calls

    llvm::Function * f = mModule->getFunction(n);
    if (!f) {
      std::vector<llvm::Type *> args;
      //only table funcs use strings
      for (auto n: v->args())
        args.push_back(llvm::Type::getFloatTy(mContext));

      llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getFloatTy(mContext), args, false);
      f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, n, mModule.get());
      if (!f)
        throw std::runtime_error("cannot find function with name: " + v->name());

      //XXX is it a leak if we don't store this somewhere??
    }

    //visit the children, store them in the args
    std::vector<llvm::Value *> args;
    for (auto n: v->args()) {
      n->accept(this);
      args.push_back(mValue);
    }

    mValue = mBuilder.CreateCall(f, args, "calltmp");
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ArrayAccess* v){
    throw std::runtime_error("not implemented");
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ValueAssignment* v){
    throw std::runtime_error("not implemented");
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::ArrayAssignment* v){
    throw std::runtime_error("not implemented");
  }

  LLVMCodeGenVisitor::function_t LLVMCodeGenVisitor::function(std::vector<xnor::ast::NodePtr> statements, bool print) {
    llvm::Value * cur = nullptr;

    auto outargt = llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getFloatTy(mContext), 0), 0);
    llvm::Value * output = mBuilder.CreateAlloca(outargt, (unsigned)0);
    mBuilder.CreateStore(mOutput, output);

    //XXX is there a better index?
    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(mContext), 0);
    for (unsigned int i = 0; i < statements.size(); i++) {
      auto index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(mContext), i);
      cur = mBuilder.CreateLoad(output);
      cur = mBuilder.CreateInBoundsGEP(llvm::PointerType::get(llvm::Type::getFloatTy(mContext), 0), cur, index);
      cur = mBuilder.CreateLoad(cur);
      cur = mBuilder.CreateInBoundsGEP(llvm::Type::getFloatTy(mContext), cur, zero);

      statements.at(i)->accept(this);
      mBuilder.CreateStore(mValue, cur);
    }

    mBuilder.CreateRet(nullptr);
    llvm::verifyFunction(*mMainFunction);
    mFunctionPassManager->run(*mMainFunction);
    if (print)
      mModule->print(llvm::errs(), nullptr);

    auto Resolver = llvm::orc::createLambdaResolver(
        [&](const std::string &Name) {
          if (auto Sym = findMangledSymbol(Name))
            return Sym;
          return llvm::JITSymbol(nullptr);
        },
        [](const std::string &S) { return nullptr; });
    auto H = llvm::cantFail(mCompileLayer.addModule(std::move(mModule), std::move(Resolver)));
    mModuleHandles.push_back(H);

    auto ExprSymbol = findSymbol(main_function_name);
    if (!ExprSymbol)
      throw std::runtime_error("couldn't find symbol " + main_function_name);

    function_t func = reinterpret_cast<function_t>((uintptr_t)cantFail(ExprSymbol.getAddress()));
    return func;
  }

  llvm::JITSymbol LLVMCodeGenVisitor::findSymbol(const std::string Name) {
    return findMangledSymbol(mangle(Name));
  }

  llvm::JITSymbol LLVMCodeGenVisitor::findMangledSymbol(const std::string &Name) {
    const bool ExportedSymbolsOnly = false;

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
