#include "ast.h"
#include <iostream>
#include <regex>
#include <iostream>

using std::cout;
using std::endl;

namespace a = xnor::ast;

namespace {
  using ot = a::Node::OutputType;

  const std::regex var_regex("\\$([fisvxyFISVXY])(\\d*)");
  const std::map<std::string, a::Variable::VarType> var_type_map = {
    {"f", a::Variable::VarType::FLOAT},
    {"F", a::Variable::VarType::FLOAT},
    {"i", a::Variable::VarType::INT},
    {"I", a::Variable::VarType::INT},
    {"s", a::Variable::VarType::SYMBOL},
    {"S", a::Variable::VarType::SYMBOL},
    {"v", a::Variable::VarType::VECTOR},
    {"V", a::Variable::VarType::VECTOR},
    {"x", a::Variable::VarType::INPUT},
    {"X", a::Variable::VarType::INPUT},
    {"y", a::Variable::VarType::OUTPUT},
    {"Y", a::Variable::VarType::OUTPUT},
  };

  const std::map<std::string, std::vector<a::Node::OutputType> > function_arg_map = {
    {"Sum", {ot::STRING, ot::FLOAT, ot::FLOAT}},
    {"abs", {ot::FLOAT}},
    {"acos", {ot::FLOAT}},
    {"acosh", {ot::FLOAT}},
    {"asin", {ot::FLOAT}},
    {"asinh", {ot::FLOAT}},
    {"atan2", {ot::FLOAT, ot::FLOAT}},
    {"atanh", {ot::FLOAT}},
    {"cbrt", {ot::FLOAT}},
    {"ceil", {ot::FLOAT}},
    {"copysign", {ot::FLOAT, ot::FLOAT}},
    {"cos", {ot::FLOAT}},
    {"erf", {ot::FLOAT}},
    {"erfc", {ot::FLOAT}},
    {"exp", {ot::FLOAT}},
    {"expm1", {ot::FLOAT}},
    {"fact", {ot::FLOAT}},
    {"finite", {ot::FLOAT}},
    {"float", {ot::FLOAT}},
    {"floor", {ot::FLOAT}},
    {"fmod", {ot::FLOAT, ot::FLOAT}},
    {"if", {ot::FLOAT, ot::FLOAT, ot::FLOAT}},
    {"imodf", {ot::FLOAT}},
    {"int", {ot::FLOAT}},
    {"isinf", {ot::FLOAT}},
    {"isnan", {ot::FLOAT}},
    {"ldexp", {ot::FLOAT, ot::FLOAT}},
    {"ln", {ot::FLOAT}},
    {"log", {ot::FLOAT}},
    {"log10", {ot::FLOAT}},
    {"log1p", {ot::FLOAT}},
    {"max", {ot::FLOAT, ot::FLOAT}},
    {"min", {ot::FLOAT, ot::FLOAT}},
    {"modf", {ot::FLOAT}},
    {"nearbyint", {ot::FLOAT}},
    {"pow", {ot::FLOAT, ot::FLOAT}},
    {"random", {ot::FLOAT, ot::FLOAT}},
    {"remainder", {ot::FLOAT, ot::FLOAT}},
    {"rint", {ot::FLOAT}},
    {"round", {ot::FLOAT}},
    {"sin", {ot::FLOAT}},
    {"size", {ot::STRING}},
    {"sqrt", {ot::FLOAT}},
    {"sum", {ot::STRING}},
    {"tan", {ot::FLOAT}},
    {"trunc", {ot::FLOAT}},
  };
}

namespace xnor {
namespace ast {
  Node::~Node() {
  }

  //default is numeric output
  Node::OutputType Node::output_type() const { return OutputType::FLOAT; }

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

  
  Node::OutputType Variable::output_type() const {
    return mType  == VarType::INT ? OutputType::INT : OutputType::FLOAT;
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

  Node::OutputType UnaryOp::output_type() const { return mNode->output_type(); }


  BinaryOp::BinaryOp(NodePtr left, Op op, NodePtr right) : mLeft(left), mOp(op), mRight(right) {
  }

  Node::OutputType BinaryOp::output_type() const {
    if (mRight->output_type() == OutputType::INT && mLeft->output_type() == OutputType::INT)
      return OutputType::INT;
    return OutputType::FLOAT;
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
      if ((args.at(i)->output_type() == ot::STRING && arg_types.at(i) != ot::STRING) || (args.at(i)->output_type() != ot::STRING && arg_types.at(i) == ot::STRING)) {
        throw std::runtime_error("function " + name + " arg " + std::to_string(i) + " type mismatch" + (args.at(i)->output_type() == ot::STRING ? "IS STRING" : "NOT"));
      }
    }
  }

  Node::OutputType FunctionCall::output_type() const {
    if (mName == "int")
      return OutputType::INT;
    else if (mName == "float")
      return OutputType::FLOAT;
    for (auto a: mArgs) {
      if (a->output_type() == OutputType::FLOAT)
        return a->output_type();
    }
    return OutputType::INT;
  }

  SampleAccess::SampleAccess(VariablePtr var, NodePtr accessor) :
    mSource(var), mAccessor(accessor)
  {
    if (var->type() != a::Variable::VarType::INPUT && var->type() != a::Variable::VarType::OUTPUT)
      throw std::runtime_error("only input and output variables can be accessed at the sample level");
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

  Node::OutputType ValueAssignment::output_type() const {
    return mValueNode->output_type();
  }

  ArrayAssignment::ArrayAssignment(ArrayAccessPtr array, NodePtr node) : mArray(array), mValueNode(node)
  {
  }

  Node::OutputType ArrayAssignment::output_type() const {
    return mValueNode->output_type();
  }

  Deref::Deref(ArrayAccessPtr array) : mValue(array) {}

  Node::OutputType Deref::output_type() const {
    return mValue->output_type();
  }
}
}
