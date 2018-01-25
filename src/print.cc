#include "print.h"
#include "ast.h"
#include <iostream>

using std::cout;
using std::endl;

namespace xnor {
  using namespace xnor::ast;

  void AstPrintVisitor::visit(xnor::ast::Variable* v){
    std::string t;
    switch (v->type()) {
      case Variable::VarType::FLOAT:
        t = "f"; break;
      case Variable::VarType::INT:
        t = "i"; break;
      case Variable::VarType::SYMBOL:
        t = "s"; break;
      case Variable::VarType::VECTOR:
        t = "v"; break;
      case Variable::VarType::INPUT:
        t = "x"; break;
      case Variable::VarType::OUTPUT:
        t = "y"; break;
    }
    print("var $" + t + std::to_string(v->input_index()));
  }

  void AstPrintVisitor::visit(xnor::ast::Value<int>* v){
    print("const: " + std::to_string(v->value()));
  }

  void AstPrintVisitor::visit(xnor::ast::Value<float>* v){
    print("const: " + std::to_string(v->value()));
  }

  void AstPrintVisitor::visit(xnor::ast::Value<std::string>* v){
    print("const: " + v->value());
  }

  void AstPrintVisitor::visit(xnor::ast::Quoted* v){
    if (v->value().size()) {
      print("quoted: " + v->value());
      return;
    }
    print("quoted with children:");
    print_child(v->variable());
  }

  void AstPrintVisitor::visit(xnor::ast::UnaryOp* v){
    std::string op;
    switch (v->op()) {
      case xnor::ast::UnaryOp::Op::BIT_NOT:
        op = "~"; break;
      case xnor::ast::UnaryOp::Op::LOGICAL_NOT:
        op = "!"; break;
      case xnor::ast::UnaryOp::Op::NEGATE:
        op = "-"; break;
    }

    print("UnaryOp: " + op + " child:");
    print_child(v->node());
  }

  void AstPrintVisitor::visit(xnor::ast::BinaryOp* v){
    std::string op;
    switch (v->op()) {
      case xnor::ast::BinaryOp::Op::ADD:
        op = "+"; break;
      case xnor::ast::BinaryOp::Op::SUBTRACT:
        op = "-"; break;
      case xnor::ast::BinaryOp::Op::MULTIPLY:
        op = "*"; break;
      case xnor::ast::BinaryOp::Op::DIVIDE:
        op = "/"; break;
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
    }
    print("BinaryOp: " + op);
    print_child(v->left());
    print_child(v->right());
  }

  void AstPrintVisitor::visit(xnor::ast::FunctionCall* v){
    print("FunctionCall: " + v->name());
    for (auto c: v->args())
      print_child(c);
  }

  void AstPrintVisitor::visit(xnor::ast::ArrayValue* v){
    if (v->name().size()) {
      print("ArrayValue: " + v->name() + " value: ");
    } else {
      print("ArrayValue name: ");
      print_child(v->name_var());
      print("value: ");
    }
    print_child(v->index_node());
  }

  void AstPrintVisitor::visit(xnor::ast::ValueAssignment* v){
    print("ValueAssignment " + v->value_name());
    print_child(v->value_node());
  }

  void AstPrintVisitor::visit(xnor::ast::ArrayAssignment* v){
    print("ArrayAssignment :");
    print_child(v->array());
    print_child(v->value_node());
  }
  
  void AstPrintVisitor::print(const std::string& v) {
    for (unsigned int i = 0; i < mDepth; i++)
      cout << "  ";
    cout << v << endl;
  }

  void AstPrintVisitor::print_child(xnor::ast::NodePtr c) {
    mDepth++;
    c->accept(this);
    mDepth--;
  }
}
