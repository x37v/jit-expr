#include "ast.h"
#include <iostream>
#include <regex>

namespace a = xnor::ast;

namespace {
  using ot = a::Node::OutputType;

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
      mInputIndex = std::stoi(i) - 1;
  }

  Variable::~Variable() {
  }


  unsigned int Variable::input_index() const { return mInputIndex; }
  Variable::VarType Variable::type() const { return mType; }

  Quoted::Quoted(const std::string& value) : mStringValue(value) {
  }

  Quoted::Quoted(VariablePtr var) : mQuotedVar(var) {
    if (var == nullptr)
      throw std::runtime_error("variable cannot be null");
    if (var->type() != a::Variable::VarType::SYMBOL)
      throw std::runtime_error("quoted values can only be strings or symbol variables");
  }

  Node::OutputType Quoted::output_type() const { return Node::OutputType::STRING; }

  UnaryOp::UnaryOp(Op op, NodePtr node) : mOp(op), mNode(node) {
  }

  BinaryOp::BinaryOp(NodePtr left, Op op, NodePtr right) : mLeft(left), mOp(op), mRight(right) {
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

  ArrayAccess::ArrayAccess(const std::string& name, NodePtr accessor) :
    mArrayName(name), mAccessor(accessor)
  {
  }

  ArrayAccess::ArrayAccess(VariablePtr varNode, NodePtr accessor) :
    mArrayVar(varNode), mAccessor(accessor)
  {
  }

  ValueAssignment::ValueAssignment(const std::string& name, NodePtr node) : mValueName(name), mValueNode(node)
  {
  }

  ArrayAssignment::ArrayAssignment(ArrayAccessPtr array, NodePtr node) : mArray(array), mValueNode(node)
  {
  }
}
}
