#include "codegen.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
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

namespace ast = xnor::ast;

namespace {
  const std::string main_function_name = "expralex";
  const std::map<std::string, std::string> function_alias = {
    {"Sum", "xnor_expr_table_sum"},
    {"sum", "xnor_expr_table_sum_all"},
    {"size", "xnor_expr_table_size"},
    {"fact", "xnor_expr_fact"},
    {"max", "xnor_expr_max"},
    {"min", "xnor_expr_min"},
    {"random", "xnor_expr_random"},
    {"imodf", "xnor_expr_imodf"},
    {"modf", "xnor_expr_modf"},
    {"random", "xnor_expr_random"},
    {"isnan", "xnor_expr_isnan"},
    {"isinf", "xnor_expr_isinf"},
    {"finite", "xnor_expr_finite"},
    {"int", "truncf"},
    {"ln", "logf"},
    {"abs", "fabsf"},
  };
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

    mSymbolPtrType = llvm::PointerType::get(llvm::StructType::create(mContext, "t_symbol_ptr"), 0); //opaque
    mFloatType = llvm::Type::getFloatTy(mContext);
    mIntType = llvm::Type::getInt32Ty(mContext);

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
    argTypes.push_back(llvm::PointerType::get(llvm::PointerType::get(mFloatType, 0), 0));

    std::vector<llvm::Type*> unionTypes;
    unionTypes.push_back(llvm::PointerType::get(mFloatType, 0));
    mInputType = llvm::StructType::create(llvm::makeArrayRef(unionTypes), "union.input_arg_t");
    auto sp = llvm::PointerType::get(mInputType, 0);
    argTypes.push_back(sp);

    argTypes.push_back(mIntType);

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

    it++;
    it->setName("veclen");
    mFrameCount = it;
  }

  LLVMCodeGenVisitor::~LLVMCodeGenVisitor() {
  }

  void LLVMCodeGenVisitor::visit(ast::Variable* v){
    //XXX is there a better index?
    auto index = llvm::ConstantInt::get(mIntType, v->input_index());

    llvm::Value * cur = nullptr;

    if (v->type() != ast::Variable::VarType::OUTPUT) {
      cur = mBuilder.CreateLoad(mInput);
      cur = mBuilder.CreateInBoundsGEP(mInputType, cur, index);
    }

    switch(v->type()) {
      case ast::Variable::VarType::FLOAT:
        cur = mBuilder.CreateBitCast(cur, llvm::PointerType::get(mFloatType, 0));
        mValue = mBuilder.CreateLoad(cur, "inputf" + std::to_string(v->input_index()));
        break;
      case ast::Variable::VarType::INT:
        {
          cur = mBuilder.CreateBitCast(cur, llvm::PointerType::get(mFloatType, 0));
          cur = mBuilder.CreateLoad(cur);
          mValue = createFunctionCall("floorf", 
              llvm::FunctionType::get(mFloatType, {mFloatType}, false),
              { cur }, "inputi" + std::to_string(v->input_index()));
        }
        break;
      case ast::Variable::VarType::VECTOR:
        {
          cur = mBuilder.CreateBitCast(cur, llvm::PointerType::get(llvm::PointerType::get(mFloatType, 0), 0));
          cur = mBuilder.CreateLoad(cur);
          cur = mBuilder.CreateInBoundsGEP(mFloatType, cur, mFrameIndex);
          mValue = mBuilder.CreateLoad(cur, "inputv" + std::to_string(v->input_index()));
        }
        break;
      case ast::Variable::VarType::INPUT:
        {
          //returns a float pointer
          cur = mBuilder.CreateBitCast(cur, llvm::PointerType::get(llvm::PointerType::get(mFloatType, 0), 0));
          mValue = mBuilder.CreateLoad(cur, "inputx" + std::to_string(v->input_index()));
        }
        break;
      case ast::Variable::VarType::OUTPUT:
        {
          //returns a float pointer
          cur = mBuilder.CreateLoad(mOutput);
          cur = mBuilder.CreateInBoundsGEP(llvm::PointerType::get(mFloatType, 0), cur, llvm::ConstantInt::get(mIntType, v->input_index()));
          mValue = mBuilder.CreateLoad(cur, "inputy" + std::to_string(v->input_index()));
        }
        break;
      case ast::Variable::VarType::SYMBOL:
        {
          //returns a symbols pointer
          mValue = mBuilder.CreateBitCast(cur, llvm::PointerType::get(mSymbolPtrType, 0));
          mValue = mBuilder.CreateLoad(cur, "inputs" + std::to_string(v->input_index()));
        }
        break;
      default:
        throw std::runtime_error("type not supported yet");
    }
  }

  void LLVMCodeGenVisitor::visit(ast::Value<int>* v){
    mValue = llvm::ConstantFP::get(mFloatType, static_cast<float>(v->value()));
  }

  void LLVMCodeGenVisitor::visit(ast::Value<float>* v){
    mValue = llvm::ConstantFP::get(mFloatType, v->value());
  }

  void LLVMCodeGenVisitor::visit(ast::Value<std::string>* v){
    auto sym = getSymbol(v->value());
    mValue = createFunctionCall("xnor_expr_value_get",
        llvm::FunctionType::get(mFloatType, {mSymbolPtrType}, false),
        {sym}, "tmpvalueget");
  }

  void LLVMCodeGenVisitor::visit(ast::Quoted* v){
    if (v->value().size()) {
      mValue = getSymbol(v->value());
    } else {
      v->variable()->accept(this);
    }
  }

  void LLVMCodeGenVisitor::visit(ast::UnaryOp* v){
    v->node()->accept(this);
    auto right = mValue;

    switch (v->op()) {
      case ast::UnaryOp::Op::BIT_NOT:
        mValue = toFloat(mBuilder.CreateNot(toInt(right), "nottmp"));
        return;
      case ast::UnaryOp::Op::LOGICAL_NOT:
        //logical not is the same as x == 0
        mValue = wrapLogic(mBuilder.CreateFCmpOEQ(right, llvm::ConstantFP::get(mFloatType, 0.0f), "eqtmp"));
        return;
      case ast::UnaryOp::Op::NEGATE: {
        auto zero = llvm::ConstantFP::get(mFloatType, 0.0f);
        mValue = mBuilder.CreateFSub(zero, right, "negtmp");
      }
        return;
    }
    throw std::runtime_error("unimplemented");
  }

  void LLVMCodeGenVisitor::visit(ast::BinaryOp* v){
    v->left()->accept(this);
    auto left = mValue;

    v->right()->accept(this);
    auto right = mValue;

    switch (v->op()) {
      case ast::BinaryOp::Op::ADD:
        mValue = mBuilder.CreateFAdd(left, right, "addtmp");
        return;
      case ast::BinaryOp::Op::SUBTRACT:
        mValue = mBuilder.CreateFSub(left, right, "subtmp");
        return;
      case ast::BinaryOp::Op::MULTIPLY:
        mValue = mBuilder.CreateFMul(left, right, "multmp");
        return;
      case ast::BinaryOp::Op::DIVIDE:
        mValue = mBuilder.CreateFDiv(left, right, "divtmp");
        return;
      case ast::BinaryOp::Op::SHIFT_LEFT:
        mValue = toFloat(mBuilder.CreateShl(toInt(left), toInt(right), "sltmp"));
        return;
      case ast::BinaryOp::Op::SHIFT_RIGHT:
        mValue = toFloat(mBuilder.CreateLShr(toInt(left), toInt(right), "srtmp"));
        return;
      case ast::BinaryOp::Op::COMP_EQUAL:
        mValue = wrapLogic(mBuilder.CreateFCmpOEQ(left, right, "eqtmp"));
        return;
      case ast::BinaryOp::Op::COMP_NOT_EQUAL:
        mValue = wrapLogic(mBuilder.CreateFCmpONE(left, right, "neqtmp"));
        return;
      case ast::BinaryOp::Op::COMP_GREATER:
        mValue = wrapLogic(mBuilder.CreateFCmpOGT(left, right, "gttmp"));
        return;
      case ast::BinaryOp::Op::COMP_LESS:
        mValue = wrapLogic(mBuilder.CreateFCmpOLT(left, right, "lttmp"));
        return;
      case ast::BinaryOp::Op::COMP_GREATER_OR_EQUAL:
        mValue = wrapLogic(mBuilder.CreateNot(mBuilder.CreateFCmpOLT(left, right, "lttmp"), "nottmp"));
        return;
      case ast::BinaryOp::Op::COMP_LESS_OR_EQUAL:
        mValue = wrapLogic(mBuilder.CreateNot(mBuilder.CreateFCmpOGT(left, right, "lttmp"), "nottmp"));
        return;
      case ast::BinaryOp::Op::LOGICAL_OR:
        //just use bitwise then not equal 0
        mValue = wrapLogic(mBuilder.CreateICmpNE(mBuilder.CreateOr(toInt(left), toInt(right), "ortmp"),
              llvm::ConstantInt::get(mIntType, 0), "neqtmp"));
        return;
      case ast::BinaryOp::Op::LOGICAL_AND:
        //just use bitwise then not equal 0
        mValue = wrapLogic(mBuilder.CreateICmpNE(mBuilder.CreateAnd(toInt(left), toInt(right), "andtmp"),
              llvm::ConstantInt::get(mIntType, 0), "neqtmp"));
        return;
      case ast::BinaryOp::Op::BIT_AND:
        mValue = wrapLogic(mBuilder.CreateAnd(toInt(left), toInt(right), "andtmp"));
        return;
      case ast::BinaryOp::Op::BIT_OR:
        mValue = wrapLogic(mBuilder.CreateOr(toInt(left), toInt(right), "ortmp"));
        return;
      case ast::BinaryOp::Op::BIT_XOR:
        mValue = wrapLogic(mBuilder.CreateXor(toInt(left), toInt(right), "xortmp"));
        return;
    }
    throw std::runtime_error("not supported yet");
  }

  void LLVMCodeGenVisitor::visit(ast::FunctionCall* v){
    auto n = v->name();

    //if
    if (n == "if") {
      //translated from kaleidoscope example, chapter 5
      v->args().at(0)->accept(this);

      //true if not equal to zero
      auto cond = mBuilder.CreateFCmpONE(mValue, llvm::ConstantFP::get(mFloatType, 0.0f), "neqtmp");

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
      auto *phi = mBuilder.CreatePHI(mFloatType, 2, "iftmp");
      phi->addIncoming(thenv, thenbb);
      phi->addIncoming(elsev, elsebb);
      mValue = phi;
      return;
    } else if (n == "float") { //this doesn't do anything, all math is float
      v->args().at(0)->accept(this);
      return;
    }

    //table functions
    auto it = function_alias.find(n);
    if (it != function_alias.end()) {
      n = it->second;
    } else {
      n = n + "f"; //we're using the floating point version of these calls
    }

    llvm::Function * f = mModule->getFunction(n);
    if (!f) {
      std::vector<llvm::Type *> args;
      for (auto a: v->args())
        args.push_back(a->output_type() == xnor::ast::Node::OutputType::STRING ? mSymbolPtrType : mFloatType);

      llvm::FunctionType *ft = llvm::FunctionType::get(mFloatType, args, false);
      f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, n, mModule.get());
      if (!f)
        throw std::runtime_error("cannot find function with name: " + v->name());

      //XXX is it a leak if we don't store this somewhere??
    }

    //visit the children, store them in the args
    std::vector<llvm::Value *> args;
    for (auto a: v->args()) {
      a->accept(this);
      args.push_back(mValue);
    }

    mValue = mBuilder.CreateCall(f, args, "calltmp");
  }

  void LLVMCodeGenVisitor::visit(ast::SampleAccess* v) {
    //get the index node
    v->index_node()->accept(this);
    auto index = mValue;

    //get the variable
    v->source()->accept(this);
    auto var = mValue; //this is a pointer to a float

    float clamp_top = (v->source()->type() == ast::Variable::VarType::OUTPUT) ? -1.0f : 0;
    auto top = llvm::ConstantFP::get(mFloatType, clamp_top);
    auto zero = llvm::ConstantFP::get(mFloatType, 0.0f);
    auto bottom = mBuilder.CreateFSub(zero, toFloat(mFrameCount), "bottom");

    //clamp index between -frame_size and clamp_top
    //index < top ? index : top
    auto lt = mBuilder.CreateFCmpOLT(index, top, "lttmp");
    index = mBuilder.CreateSelect(lt, index, top);

    //index < bottom ? bottom : index
    lt = mBuilder.CreateFCmpOLT(index, bottom, "lttmp");
    index = mBuilder.CreateSelect(lt, bottom, index);

    //offset with the current sample index
    index = mBuilder.CreateFAdd(index, toFloat(mFrameIndex), "offset");


    llvm::Value * arrayLength;
    //input buffers are actually 2x as long because you have to be able to previous values as well
    if (v->source()->type() == ast::Variable::VarType::INPUT)
      arrayLength = toFloat(mBuilder.CreateShl(mFrameCount, llvm::ConstantInt::get(mIntType, 1)));
    else
      arrayLength = toFloat(mFrameCount);

    //index < 0 : frame_count + index : index
    lt = mBuilder.CreateFCmpOLT(index, zero, "ltmp");

    //actually just add zero or the array length
    auto offset = mBuilder.CreateSelect(lt, arrayLength, zero);
    index = mBuilder.CreateFAdd(index, offset);

#if 0
    index = toInt(index);
    auto p = mBuilder.CreateInBoundsGEP(mFloatType, var, index);
    mValue = mBuilder.CreateLoad(p);
#else
    mValue = createFunctionCall("xnor_expr_array_read", 
        llvm::FunctionType::get(mFloatType, {llvm::PointerType::get(mFloatType, 0), mFloatType, mIntType}, false),
        { var, index, toInt(arrayLength) }, "tmparrayinterp");
#endif
  }

  void LLVMCodeGenVisitor::visit(ast::ArrayAccess* v){
    if (v->name().size()) {
      auto sym = getSymbol(v->name());
      mValue = mBuilder.CreateBitCast(sym, mSymbolPtrType);
    } else {
      v->name_var()->accept(this);
    }

    auto name = mValue;

    v->index_node()->accept(this);
    auto index = mValue;

    mValue = createFunctionCall("xnor_expr_table_value_ptr", 
        llvm::FunctionType::get(llvm::PointerType::get(mFloatType, 0), {mSymbolPtrType, mFloatType}, false),
        { name, index }, "tmparrayaccess");
  }

  void LLVMCodeGenVisitor::visit(ast::ValueAssignment* v){
    auto sym = getSymbol(v->value_name());
    v->value_node()->accept(this);

    auto value = mValue;
    mValue = createFunctionCall("xnor_expr_value_assign",
        llvm::FunctionType::get(mFloatType, {mSymbolPtrType, mFloatType}, false),
        {sym, value}, "tmpvalueassign");
  }

  void LLVMCodeGenVisitor::visit(ast::ArrayAssignment* v){
    v->array()->accept(this);
    auto aptr = mValue;
    v->value_node()->accept(this);
    auto value = mValue;
    mBuilder.CreateStore(value, aptr);
    mValue = value;
  }

  void LLVMCodeGenVisitor::visit(xnor::ast::Deref* v) {
    v->value_node()->accept(this); //returns a pointer to a float

    //XXX gen code for this, its real simple
    mValue = createFunctionCall("xnor_expr_deref",
        llvm::FunctionType::get(mFloatType, {llvm::PointerType::get(mFloatType, 0)}, false),
        {mValue}, "tmpderef");
  }

  LLVMCodeGenVisitor::function_t LLVMCodeGenVisitor::function(std::vector<ast::NodePtr> statements, bool print) {
    llvm::Value * cur = nullptr;

    auto outargt = llvm::PointerType::get(llvm::PointerType::get(mFloatType, 0), 0);
    llvm::Value * output = mBuilder.CreateAlloca(outargt, (unsigned)0);
    mBuilder.CreateStore(mOutput, output);
    mOutput = output;

    //store
    llvm::Value * fcount = mBuilder.CreateAlloca(mIntType, (unsigned)0);
    mBuilder.CreateStore(mFrameCount, fcount);
    mFrameCount = mBuilder.CreateLoad(fcount, "framecnt");

    //loop start
    llvm::Value * StartVal = llvm::ConstantInt::get(mIntType, 0);
    llvm::Function *TheFunction = mBuilder.GetInsertBlock()->getParent();
    llvm::BasicBlock *PreheaderBB = mBuilder.GetInsertBlock();
    llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(mContext, "loop", TheFunction);

    // Insert an explicit fall through from the current block to the LoopBB.
    mBuilder.CreateBr(LoopBB);
    // Start insertion in LoopBB.
    mBuilder.SetInsertPoint(LoopBB);

    // Start the PHI node with an entry for Start.
    llvm::PHINode *Variable = mBuilder.CreatePHI(mIntType, 2, "loopvar");
    Variable->addIncoming(StartVal, PreheaderBB);

    mFrameIndex = Variable;
    //add statements
    for (unsigned int i = 0; i < statements.size(); i++) {
      auto index = llvm::ConstantInt::get(mIntType, i);
      cur = mBuilder.CreateLoad(mOutput);
      cur = mBuilder.CreateInBoundsGEP(llvm::PointerType::get(mFloatType, 0), cur, index);
      cur = mBuilder.CreateLoad(cur);
      cur = mBuilder.CreateInBoundsGEP(mFloatType, cur, mFrameIndex);

      statements.at(i)->accept(this);
      mBuilder.CreateStore(mValue, cur);
    }

    // Emit the step value.
    llvm::Value *NextVar = mBuilder.CreateAdd(Variable, llvm::ConstantInt::get(mIntType, 1), "nextvar");
    llvm::Value *EndVal = mFrameCount;
    llvm::Value *EndCond = mBuilder.CreateICmpNE(EndVal, NextVar);

    // Create the "after loop" block and insert it.
    llvm::BasicBlock *LoopEndBB = mBuilder.GetInsertBlock();
    llvm::BasicBlock *AfterBB = llvm::BasicBlock::Create(mContext, "afterloop", TheFunction);

    // Insert the conditional branch into the end of LoopEndBB.
    mBuilder.CreateCondBr(EndCond, LoopBB, AfterBB);

    // Any new code will be inserted in AfterBB.
    mBuilder.SetInsertPoint(AfterBB);

    // Add a new entry to the PHI node for the backedge.
    Variable->addIncoming(NextVar, LoopEndBB);

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
        [](const std::string &/*S*/) { return nullptr; });
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
    return mBuilder.CreateUIToFP(v, mFloatType, "cast");
  }

  llvm::Value * LLVMCodeGenVisitor::toInt(llvm::Value * v) {
    return mBuilder.CreateFPToSI(v, mIntType, "cast");
  }

  llvm::Value * LLVMCodeGenVisitor::toFloat(llvm::Value * v) {
    return mBuilder.CreateSIToFP(v, mFloatType, "cast");
  }

  llvm::Value * LLVMCodeGenVisitor::getSymbol(const std::string& name) {
    t_symbol * sym = gensym(name.c_str()); //XXX hold onto symbol and free later?
    if (!sym)
      throw std::runtime_error("couldn't get symbol " + name);
    return llvm::ConstantInt::get(mDataLayout.getIntPtrType(mContext, 0), reinterpret_cast<uintptr_t>(sym));
  }

  //llvm::Value * LLVMCodeGenVisitor::linterpWithWrap(llvm::Value * fptr, llvm::Value * findex, llvm::Value * ilength) {
  //}
  llvm::Value * LLVMCodeGenVisitor::createFunctionCall(
      const std::string& func_name,
      llvm::FunctionType *func_type,
      std::vector<llvm::Value *> args, std::string callname) {

    llvm::Function * f = mModule->getFunction(func_name);
    if (!f) {
      f = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, func_name, mModule.get());
      if (!f)
        throw std::runtime_error("cannot find function with name " + func_name);
      //XXX is it a leak if we don't store this somewhere??
    }
    return mBuilder.CreateCall(f, args, callname);
  }

}
