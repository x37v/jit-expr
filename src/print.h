#pragma once

#include "ast.h"

namespace xnor {
  class AstPrintVisitor : public xnor::ast::Visitor {
    public:
      virtual void visit(xnor::ast::Variable* v);
      virtual void visit(xnor::ast::Value<int>* v);
      virtual void visit(xnor::ast::Value<float>* v);
      virtual void visit(xnor::ast::Value<std::string>* v);
      virtual void visit(xnor::ast::Quoted* v);
      virtual void visit(xnor::ast::UnaryOp* v);
      virtual void visit(xnor::ast::BinaryOp* v);
      virtual void visit(xnor::ast::FunctionCall* v);
      virtual void visit(xnor::ast::SampleAccess* v);
      virtual void visit(xnor::ast::ArrayValue* v);
      virtual void visit(xnor::ast::ValueAssignment* v);
      virtual void visit(xnor::ast::ArrayAssignment* v);

    private:
      unsigned int mDepth = 0;
      void print(const std::string& v);
      void print_child(xnor::ast::NodePtr c);
  };
}
