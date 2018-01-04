#include "ast.h"
#include <iostream>
#include <regex>

namespace a = xnor::ast;

namespace {
  using ot = a::Node::OutputType;
  a::PrintFunc childPrintFunc(a::PrintFunc f) {
    return [f](std::string v, unsigned int depth) {
      f(v, depth + 1);
    };
  }

  const std::regex var_regex("\\$([fisvxy])(\\d*)");
  const std::map<std::string, a::Variable::VarType> var_type_map = {
    {"f", a::Variable::VarType::FLOAT},
    {"i", a::Variable::VarType::INT},
    {"s", a::Variable::VarType::SYMBOL},
    {"v", a::Variable::VarType::VECTOR},
    {"x", a::Variable::VarType::INPUT},
    {"y", a::Variable::VarType::OUTPUT},
  };

  const std::map<std::string, std::vector<a::Node::OutputType> > function_arg_map = {
    {"Sum", {ot::STRING, ot::NUMERIC, ot::NUMERIC}},
    {"abs", {ot::NUMERIC}},
    {"acos", {ot::NUMERIC}},
    {"acosh", {ot::NUMERIC}},
    {"asin", {ot::NUMERIC}},
    {"asinh", {ot::NUMERIC}},
    {"atan2", {ot::NUMERIC, ot::NUMERIC}},
    {"atanh", {ot::NUMERIC}},
    {"cbrt", {ot::NUMERIC}},
    {"ceil", {ot::NUMERIC}},
    {"copysign", {ot::NUMERIC, ot::NUMERIC}},
    {"cos", {ot::NUMERIC}},
    {"erf", {ot::NUMERIC}},
    {"erfc", {ot::NUMERIC}},
    {"exp", {ot::NUMERIC}},
    {"expm1", {ot::NUMERIC}},
    {"fact", {ot::NUMERIC}},
    {"finite", {ot::NUMERIC}},
    {"float", {ot::NUMERIC}},
    {"floor", {ot::NUMERIC}},
    {"fmod", {ot::NUMERIC, ot::NUMERIC}},
    {"if", {ot::NUMERIC, ot::NUMERIC, ot::NUMERIC}},
    {"imodf", {ot::NUMERIC}},
    {"int", {ot::NUMERIC}},
    {"isinf", {ot::NUMERIC}},
    {"isnan", {ot::NUMERIC}},
    {"ldexp", {ot::NUMERIC, ot::NUMERIC}},
    {"ln", {ot::NUMERIC}},
    {"log", {ot::NUMERIC}},
    {"log10", {ot::NUMERIC}},
    {"log1p", {ot::NUMERIC}},
    {"max", {ot::NUMERIC, ot::NUMERIC}},
    {"min", {ot::NUMERIC, ot::NUMERIC}},
    {"modf", {ot::NUMERIC}},
    {"nearbyint", {ot::NUMERIC}},
    {"pow", {ot::NUMERIC, ot::NUMERIC}},
    {"random", {ot::NUMERIC, ot::NUMERIC}},
    {"remainder", {ot::NUMERIC, ot::NUMERIC}},
    {"rint", {ot::NUMERIC}},
    {"round", {ot::NUMERIC}},
    {"sin", {ot::NUMERIC}},
    {"size", {ot::STRING}},
    {"sqrt", {ot::NUMERIC}},
    {"sum", {ot::STRING}},
    {"tan", {ot::NUMERIC}},
    {"trunc", {ot::NUMERIC}},
  };
}

namespace xnor {
namespace ast {
  Node::~Node() {
  }

  //default is numeric output
  Node::OutputType Node::output_type() const { return OutputType::NUMERIC; }

  Variable::Variable(const std::string& v) {
    std::cmatch m;
    if (!std::regex_match(v.c_str(), m, var_regex))
      throw std::runtime_error(v + " is not a valid variable declaration");

    auto it = var_type_map.find(m[1]);
    if (it == var_type_map.end())
      throw std::runtime_error(v + " has unknown type");
    mType = it->second;

    std::string i(m[2]);
    if (i.size())
      mInputIndex = std::stoi(i);
  }

  Variable::~Variable() {
  }


  Variable::VarType Variable::type() const { return mType; }
  void Variable::print(PrintFunc printfuc) const {
    std::string t;
    switch (mType) {
      case VarType::FLOAT:
        t = "f"; break;
      case VarType::INT:
        t = "i"; break;
      case VarType::SYMBOL:
        t = "s"; break;
      case VarType::VECTOR:
        t = "v"; break;
      case VarType::INPUT:
        t = "x"; break;
      case VarType::OUTPUT:
        t = "y"; break;
    }
    printfuc("var $" + t + std::to_string(mInputIndex), 0);
  }

  template <>
    void Value<std::string>::print(PrintFunc printfuc) const {
      printfuc("const: " + mValue, 0);
    }

  Quoted::Quoted(const std::string& value) : mStringValue(value) {
  }

  Quoted::Quoted(VariablePtr var) : mQuotedVar(var) {
    if (var == nullptr)
      throw std::runtime_error("variable cannot be null");
    if (var->type() != a::Variable::VarType::SYMBOL)
      throw std::runtime_error("quoted values can only be strings or symbol variables");
  }

  Node::OutputType Quoted::output_type() const { return Node::OutputType::STRING; }

  void Quoted::print(PrintFunc printfuc) const {
    if (mStringValue.size()) {
      printfuc("quoted: " + mStringValue, 0);
      return;
    }
    printfuc("quoted with children:", 0);
    mQuotedVar->print(childPrintFunc(printfuc));
  }

  UnaryOp::UnaryOp(Op op, NodePtr node) : mOp(op), mNode(node) {
  }

  void UnaryOp::print(PrintFunc printfuc) const {
    std::string op;
    switch (mOp) {
      case Op::BIT_NOT:
        op = "~"; break;
      case Op::LOGICAL_NOT:
        op = "!"; break;
      case Op::NEGATE:
        op = "-"; break;
    }

    printfuc("UnaryOp: " + op + " child:", 0);
    mNode->print(childPrintFunc(printfuc));
  }

  BinaryOp::BinaryOp(NodePtr left, Op op, NodePtr right) : mLeft(left), mOp(op), mRight(right) {
  }

  void BinaryOp::print(PrintFunc printfuc) const {
    std::string op;
    switch (mOp) {
      case Op::ADD:
        op = "+"; break;
      case Op::SUBTRACT:
        op = "-"; break;
      case Op::MULTIPLY:
        op = "*"; break;
      case Op::DIVIDE:
        op = "/"; break;
      case Op::SHIFT_LEFT:
        op = "<<"; break;
      case Op::SHIFT_RIGHT:
        op = ">>"; break;
      case Op::COMP_EQUAL:
        op = "=="; break;
      case Op::COMP_NOT_EQUAL:
        op = "!="; break;
      case Op::COMP_GREATER:
        op = ">"; break;
      case Op::COMP_LESS:
        op = "<"; break;
      case Op::COMP_GREATER_OR_EQUAL:
        op = ">="; break;
      case Op::COMP_LESS_OR_EQUAL:
        op = "<="; break;
      case Op::LOGICAL_OR:
        op = "||"; break;
      case Op::LOGICAL_AND:
        op = "&&"; break;
      case Op::BIT_AND:
        op = "~"; break;
      case Op::BIT_OR:
        op = "|"; break;
      case Op::BIT_XOR:
        op = "^"; break;
    }
    printfuc("BinaryOp: " + op, 0);
    mLeft->print(childPrintFunc(printfuc));
    mRight->print(childPrintFunc(printfuc));
  }

  FunctionCall::FunctionCall(const std::string& name, const std::vector<NodePtr>& args) : mName(name), mArgs(args) {
    auto it = function_arg_map.find(name);
    if (it == function_arg_map.end())
      throw std::runtime_error("function not found with name: " + name);
    const auto& arg_types = it->second;
    if (args.size() != arg_types.size()) {
      throw std::runtime_error("function " + name + " expects " + std::to_string(arg_types.size()) + " arguments, got: " + std::to_string(args.size()));
    }
    for (unsigned int i = 0; i < args.size(); i++) {
      if (args.at(i)->output_type() != arg_types.at(i))
        throw std::runtime_error("function " + name + " arg " + std::to_string(i) + " type mismatch");
    }
  }

  void FunctionCall::print(PrintFunc printfuc) const {
    printfuc("FunctionCall: " + mName, 0);
    for (auto c: mArgs)
      c->print(childPrintFunc(printfuc));
  }

  ArrayAccess::ArrayAccess(const std::string& name, NodePtr accessor) :
    mArrayName(name), mAccessor(accessor)
  {
  }

  ArrayAccess::ArrayAccess(VariablePtr varNode, NodePtr accessor) :
    mArrayVar(varNode), mAccessor(accessor)
  {
  }

  void ArrayAccess::print(PrintFunc printfuc) const {
    if (mArrayName.size()) {
      printfuc("ArrayAccess: " + mArrayName + " value: ", 0);
    } else {
      printfuc("ArrayAccess name: ", 0);
      mArrayVar->print(childPrintFunc(printfuc));
      printfuc("value: ", 0);
    }
    mAccessor->print(childPrintFunc(printfuc));
  }

  ValueAssignment::ValueAssignment(const std::string& name, NodePtr node) : mValueName(name), mValueNode(node)
  {
  }

  void ValueAssignment::print(PrintFunc printfuc) const {
    printfuc("ValueAssignment " + mValueName, 0);
    mValueNode->print(childPrintFunc(printfuc));
  }

  ArrayAssignment::ArrayAssignment(ArrayAccessPtr array, NodePtr node) : mArray(array), mValueNode(node)
  {
  }

  void ArrayAssignment::print(PrintFunc printfuc) const {
    printfuc("ArrayAssignment :", 0);
    mArray->print(childPrintFunc(printfuc));
    mValueNode->print(childPrintFunc(printfuc));
  }
}
}
