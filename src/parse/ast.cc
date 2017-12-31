#include "ast.h"
#include <iostream>
using std::cout;
using std::endl;

namespace xnor {
namespace ast {
  Node::~Node() {
  }

  Variable::Variable(const std::string& v) : mName(v) {
    cout << "got " << mName << endl;
  }

  UnaryOp::UnaryOp(Op op, Node * node) : mOp(op), mNode(node) {
    cout << "got unary op" << endl;
  }

  BinaryOp::BinaryOp(Node * left, Op op, Node * right) : mLeft(left), mOp(op), mRight(right) {
    cout << "got binary op" << endl;
  }
}
}
