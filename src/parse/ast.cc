#include "ast.h"
#include <iostream>
#include <regex>

using std::cout;
using std::endl;
namespace a = xnor::ast;

namespace {
  const std::regex var_regex("\\$([fisvxy])(\\d*)");
  const std::map<std::string, a::Variable::VarType> var_type_map = {
    {"f", a::Variable::VarType::FLOAT},
    {"i", a::Variable::VarType::INT},
    {"s", a::Variable::VarType::SYMBOL},
    {"v", a::Variable::VarType::VECTOR},
    {"x", a::Variable::VarType::INPUT},
    {"y", a::Variable::VarType::OUTPUT},
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

    cout << "got " << v << endl;
  }

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
    cout << "got unary op" << endl;
  }

  BinaryOp::BinaryOp(NodePtr left, Op op, NodePtr right) : mLeft(left), mOp(op), mRight(right) {
    cout << "got binary op" << endl;
  }

  FunctionCall::FunctionCall(const std::string& name, const std::vector<NodePtr>& args) : mName(name), mArgs(args) {
    cout << "got function call: " << name << endl;
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
    cout << "assigning to value " << name << endl;
  }

  ArrayAssignment::ArrayAssignment(ArrayAccessPtr array, NodePtr node) : mArray(array), mValueNode(node)
  {
    cout << "assign to array " << endl;
  }
}
}
