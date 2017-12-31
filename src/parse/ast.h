#pragma once

#include <string>
#include <iostream>

namespace xnor {
  namespace ast {
    class Node {
      public:
        virtual ~Node();
    };

    class Variable : public Node {
      public:
        Variable(const std::string& n);
      private:
        std::string mName;
    };

    template <typename T>
      class Value : public Node {
        public:
          Value(const T& v) : mValue(v) { 
            std::cout << "got " << v << std::endl;
          }
          T value() const { return mValue; }
        private:
          T mValue;
      };

    class UnaryOp : public Node {
      public:
        enum class Op {
          BIT_NOT,
          LOGICAL_NOT
        };
        UnaryOp(Op op, Node * node);
      private:
        Op mOp;
        Node * mNode;
    };

    class BinaryOp : public Node {
      public:
        enum class Op {
          ADD,
          SUBTRACT,
          MULTIPLY,
          DIVIDE,
          SHIFT_LEFT,
          SHIFT_RIGHT,
          COMP_EQUAL,
          COMP_NOT_EQUAL,
          COMP_GREATER,
          COMP_LESS,
          COMP_GREATER_OR_EQUAL,
          COMP_LESS_OR_EQUAL,
          LOGICAL_OR,
          LOGICAL_AND,
          BIT_AND,
          BIT_OR,
          BIT_XOR
        };
        BinaryOp(Node * left, Op op, Node * right);
      private:
        Node * mLeft;
        Op mOp;
        Node * mRight;
    };
  }
}
