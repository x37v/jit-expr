#include "ast.h"
#include <iostream>
#include <regex>

using std::cout;
using std::endl;
namespace a = xnor::ast;

namespace {
  const std::regex var_regex("\\$([fisxy])(\\d*)");
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

  UnaryOp::UnaryOp(Op op, Node * node) : mOp(op), mNode(node) {
    cout << "got unary op" << endl;
  }

  BinaryOp::BinaryOp(Node * left, Op op, Node * right) : mLeft(left), mOp(op), mRight(right) {
    cout << "got binary op" << endl;
  }

  FunctionCall::FunctionCall(const std::string& name, const std::vector<Node*>& args) : mName(name), mArgs(args) {
    cout << "got function call: " << name << endl;
  }

  ArrayAccess::ArrayAccess(const std::string& name, Node * accessor) :
    mArrayName(name), mAccessor(accessor)
  {
  }

  ArrayAccess::ArrayAccess(Variable * varNode, Node * accessor) :
    mArrayVar(varNode), mAccessor(accessor)
  {
  }

  ValueAssignment::ValueAssignment(const std::string& name, Node * node) : mValueName(name), mValueNode(node)
  {
    cout << "assigning to value " << name << endl;
  }

  ArrayAssignment::ArrayAssignment(ArrayAccess * array, Node * node) : mArray(array), mValueNode(node)
  {
    cout << "assign to array " << endl;
  }
}
}
