#ifndef XNOR_AST_H
#define XNOR_AST_H
#pragma once

#include <string>
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
    typedef std::function<void(std::string v, unsigned int depth)> PrintFunc;

    class Node {
      public:
        virtual ~Node();
        enum class OutputType {
          NUMERIC,
          STRING
        };
        virtual OutputType output_type() const;
        virtual void print(PrintFunc printfuc) const = 0;
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
        virtual void print(PrintFunc printfuc) const;
      private:
        unsigned int mInputIndex = 0;
        VarType mType = VarType::FLOAT;
    };

    template <typename T>
      class Value : public Node {
        public:
          Value(const T& v) : mValue(v) { }
          T value() const { return mValue; }
          virtual void print(PrintFunc printfuc) const;
        private:
          T mValue;
      };

    template <>
      void Value<std::string>::print(PrintFunc printfuc) const;

    template <typename T>
      void Value<T>::print(PrintFunc printfuc) const {
        printfuc("const: " + std::to_string(mValue), 0);
      }

    class Quoted : public Node {
      public:
        Quoted(const std::string& value);
        Quoted(VariablePtr var);
        virtual OutputType output_type() const override;
        virtual void print(PrintFunc printfuc) const;
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
        virtual void print(PrintFunc printfuc) const;
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
        virtual void print(PrintFunc printfuc) const;
      private:
        NodePtr mLeft;
        Op mOp;
        NodePtr mRight;
    };

    class FunctionCall : public Node {
      public:
        FunctionCall(const std::string& name, const std::vector<NodePtr>& args);
        virtual void print(PrintFunc printfuc) const;
      private:
        std::string mName;
        std::vector<NodePtr> mArgs;
    };

    class ArrayAccess : public Node {
      public:
        ArrayAccess(const std::string& name, NodePtr accessor);
        ArrayAccess(VariablePtr varNode, NodePtr accessor);
        virtual void print(PrintFunc printfuc) const;
      private:
        std::string mArrayName;
        VariablePtr mArrayVar = nullptr;
        NodePtr mAccessor;
    };

    class ValueAssignment : public Node {
      public:
        ValueAssignment(const std::string& name, NodePtr node);
        virtual void print(PrintFunc printfuc) const;
      private:
        std::string mValueName;
        NodePtr mValueNode;
    };

    class ArrayAssignment : public Node {
      public:
        ArrayAssignment(ArrayAccessPtr array, NodePtr node);
        virtual void print(PrintFunc printfuc) const;
      private:
        ArrayAccessPtr mArray;
        NodePtr mValueNode;
    };

  }
}
#endif
