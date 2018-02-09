#pragma once 

#include "ast.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>

#include <m_pd.h>

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
      static void init();

      typedef union {
        t_float flt;
        t_symbol * sym;
        t_sample * vec;
      } input_arg_t;

      typedef void(*function_t)(float **, input_arg_t *, int nframes);

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
      virtual void visit(xnor::ast::SampleAccess* v);
      virtual void visit(xnor::ast::ArrayAccess* v);
      virtual void visit(xnor::ast::ValueAssignment* v);
      virtual void visit(xnor::ast::ArrayAssignment* v);
      virtual void visit(xnor::ast::Deref* v);

      function_t function(std::vector<xnor::ast::NodePtr> statements, bool print = false);
    private:
      llvm::LLVMContext mContext;
      llvm::IRBuilder<> mBuilder;

      std::unique_ptr<llvm::TargetMachine> mTargetMachine;
      std::unique_ptr<llvm::Module> mModule;
      std::unique_ptr<llvm::legacy::FunctionPassManager> mFunctionPassManager;

      llvm::Function * mMainFunction;
      llvm::Value * mValue;
      llvm::Value * mOutput;
      llvm::Value * mInput;
      llvm::Value * mFrameIndex;
      llvm::Value * mFrameCount;
      llvm::BasicBlock * mBlock;

      llvm::Type * mFloatType;
      llvm::Type * mIntType;
      llvm::Type * mInputType;
      llvm::Type * mSymbolPtrType;

      const llvm::DataLayout mDataLayout;
      ObjLayerT mObjectLayer;
      CompileLayerT mCompileLayer;

      std::vector<ModuleHandleT> mModuleHandles;

      llvm::JITSymbol findMangledSymbol(const std::string& name);
      llvm::JITSymbol findSymbol(const std::string name);
      std::string mangle(const std::string& name);
      llvm::Value * wrapLogic(llvm::Value * v);
      llvm::Value * toInt(llvm::Value * v);
      llvm::Value * toFloat(llvm::Value * v);
      llvm::Value * getSymbol(const std::string& name);
      void wrapIntIfNeeded(xnor::ast::Node * n);

      //returns float
      //llvm::Value * linterpWithWrap(llvm::Value * fptr, llvm::Value * findex, llvm::Value * ilength);
      
      llvm::Value * createFunctionCall(
          const std::string& func_name,
          llvm::FunctionType *ft,
          std::vector<llvm::Value *> args, std::string callname = "tmpfcall");
  };
}
