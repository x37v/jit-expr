#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <memory>

namespace xnor {
  namespace ast {
    class Node;
    class Variable;
    class ArrayAccess;
    typedef std::shared_ptr<Node> NodePtr;
    typedef std::shared_ptr<Variable> VariablePtr;
    typedef std::shared_ptr<ArrayAccess> ArrayAccessPtr;

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
        Quoted(VariablePtr var);
      private:
        std::string mStringValue;
        VariablePtr mQuotedVar = nullptr;
    };

    class UnaryOp : public Node {
      public:
        enum class Op {
          BIT_NOT,
          LOGICAL_NOT,
          NEGATE
        };
        UnaryOp(Op op, NodePtr node);
      private:
        Op mOp;
        NodePtr mNode;
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
        BinaryOp(NodePtr left, Op op, NodePtr right);
      private:
        NodePtr mLeft;
        Op mOp;
        NodePtr mRight;
    };

    class FunctionCall : public Node {
      public:
        FunctionCall(const std::string& name, const std::vector<NodePtr>& args);
      private:
        std::string mName;
        std::vector<NodePtr> mArgs;
    };

    class ArrayAccess : public Node {
      public:
        ArrayAccess(const std::string& name, NodePtr accessor);
        ArrayAccess(VariablePtr varNode, NodePtr accessor);
      private:
        std::string mArrayName;
        VariablePtr mArrayVar = nullptr;
        NodePtr mAccessor;
    };

    class ValueAssignment : public Node {
      public:
        ValueAssignment(const std::string& name, NodePtr node);
      private:
        std::string mValueName;
        NodePtr mValueNode;
    };

    class ArrayAssignment : public Node {
      public:
        ArrayAssignment(ArrayAccessPtr array, NodePtr node);
      private:
        ArrayAccessPtr mArray;
        NodePtr mValueNode;
    };

  }
}
