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
}
}
