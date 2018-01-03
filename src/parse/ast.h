#pragma once

#include <string>
#include <iostream>
#include <vector>

namespace xnor {
  namespace ast {
    class Node {
      public:
        virtual ~Node();
    };

    class Variable : public Node {
      public:
        enum class VarType {
          FLOAT,
          INT,
          SYMBOL,
          VECTOR,
          INPUT,
          OUTPUT
        };
        Variable(const std::string& n);
        unsigned int input_index() const;
        VarType type() const;
      private:
        unsigned int mInputIndex = 0;
        VarType mType = VarType::FLOAT;
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

    class Quoted : public Node {
      public:
        Quoted(const std::string& value);
        Quoted(Variable * var);
      private:
        std::string mStringValue;
        Variable * mQuotedVar = nullptr;
    };

    class UnaryOp : public Node {
      public:
        enum class Op {
          BIT_NOT,
          LOGICAL_NOT,
          NEGATE
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

    class FunctionCall : public Node {
      public:
        FunctionCall(const std::string& name, const std::vector<Node *>& args);
      private:
        std::string mName;
        std::vector<Node *> mArgs;
    };

    class ArrayAccess : public Node {
      public:
        ArrayAccess(const std::string& name, Node * accessor);
        ArrayAccess(Variable * varNode, Node * accessor);
      private:
        std::string mArrayName;
        Variable * mArrayVar = nullptr;
        Node * mAccessor;
    };

    class ValueAssignment : public Node {
      public:
        ValueAssignment(const std::string& name, Node * node);
      private:
        std::string mValueName;
        Node * mValueNode;
    };

    class ArrayAssignment : public Node {
      public:
        ArrayAssignment(ArrayAccess * array, Node * node);
      private:
        ArrayAccess * mArray;
        Node * mValueNode;
    };
  }
}
