#pragma once

namespace xnor {
  namespace ast {
    class Node {
      public:
        virtual ~Node();
    };

    class Variable : public Node {
      public:
        Variable(std::string n) : mName(n) { }
      private:
        std::string mName;
    };
  }
}
