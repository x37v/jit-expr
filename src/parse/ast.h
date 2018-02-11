//Copyright (c) Alex Norman, 2018, see LICENSE.xnor

#ifndef XNOR_AST_H
#define XNOR_AST_H
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace xnor {
  namespace ast {
    class Node;
    class Variable;
    template <typename T>
      class Value;
    class Quoted;
    class UnaryOp;
    class BinaryOp;
    class FunctionCall;
    class SampleAccess;
    class ArrayAccess;
    class ValueAssignment;
    class ArrayAssignment;
    class Deref;
    typedef std::shared_ptr<Node> NodePtr;
    typedef std::shared_ptr<Variable> VariablePtr;
    typedef std::shared_ptr<SampleAccess> SampleAccessPtr;
    typedef std::shared_ptr<ArrayAccess> ArrayAccessPtr;
    typedef std::shared_ptr<Deref> DerefPtr;
    typedef std::function<void(std::string v, unsigned int depth)> PrintFunc;

    typedef std::vector<xnor::ast::VariablePtr> VariableVector;

    class Visitor {
      public:
        virtual void visit(Variable* v) = 0;
        virtual void visit(Value<int>* v) = 0;
        virtual void visit(Value<float>* v) = 0;
        virtual void visit(Value<std::string>* v) = 0;
        virtual void visit(Quoted* v) = 0;
        virtual void visit(UnaryOp* v) = 0;
        virtual void visit(BinaryOp* v) = 0;
        virtual void visit(FunctionCall* v) = 0;
        virtual void visit(SampleAccess* v) = 0;
        virtual void visit(ArrayAccess* v) = 0;
        virtual void visit(ValueAssignment* v) = 0;
        virtual void visit(ArrayAssignment* v) = 0;
        virtual void visit(Deref* v) = 0;
    };

    class Node {
      public:
        virtual ~Node();
        enum class OutputType {
          FLOAT,
          INT,
          STRING
        };
        virtual OutputType output_type() const;
        virtual void accept(Visitor* v) = 0;
    };

    //templatized to with CRTP to auto gen accept method
    template <typename T>
      class VNode : public Node {
        public:
          virtual void accept(Visitor* v) override {
            v->visit(static_cast<T *>(this));
          };
      };


    class Variable : public VNode<Variable> {
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
        virtual ~Variable();
        virtual OutputType output_type() const override;

        unsigned int input_index() const;
        VarType type() const;
      private:
        unsigned int mInputIndex = 0;
        VarType mType = VarType::FLOAT;
    };

    template <typename T>
      class Value : public VNode<Value<T>> {
        public:
          Value(const T& v) : mValue(v) { }
          T value() const { return mValue; }
          void output_type(Node::OutputType v) { mOutputType = v; }
          virtual Node::OutputType output_type() const override { return mOutputType; }
        private:
          T mValue;
          Node::OutputType mOutputType = Node::OutputType::FLOAT;
      };

    class Quoted : public VNode<Quoted> {
      public:
        Quoted(const std::string& value);
        Quoted(VariablePtr var);
        virtual OutputType output_type() const override;

        std::string value() const { return mStringValue; }
        VariablePtr variable() const { return mQuotedVar; }
      private:
        std::string mStringValue;
        VariablePtr mQuotedVar = nullptr;
    };

    class UnaryOp : public VNode<UnaryOp> {
      public:
        enum class Op {
          BIT_NOT,
          LOGICAL_NOT,
          NEGATE
        };
        UnaryOp(Op op, NodePtr node);
        virtual OutputType output_type() const override;
        Op op() const { return mOp; }
        NodePtr node() const { return mNode; }
      private:
        Op mOp;
        NodePtr mNode;
    };

    class BinaryOp : public VNode<BinaryOp> {
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
        virtual OutputType output_type() const override;

        Op op() const { return mOp; }
        NodePtr left() const { return mLeft; }
        NodePtr right() const { return mRight; }
      private:
        NodePtr mLeft;
        Op mOp;
        NodePtr mRight;
    };

    class FunctionCall : public VNode<FunctionCall> {
      public:
        FunctionCall(const std::string& name, const std::vector<NodePtr>& args);
        virtual OutputType output_type() const override;
        std::string name() const { return mName; }
        const std::vector<NodePtr>& args() const { return mArgs; }
      private:
        std::string mName;
        std::vector<NodePtr> mArgs;
    };

    class SampleAccess : public VNode<SampleAccess> {
      public:
        SampleAccess(VariablePtr varNode, NodePtr accessor);

        VariablePtr source() const { return mSource; }
        NodePtr index_node() const { return mAccessor; }
      private:
        VariablePtr mSource;
        NodePtr mAccessor;
    };

    class ArrayAccess : public VNode<ArrayAccess> {
      public:
        ArrayAccess(const std::string& name, NodePtr accessor);
        ArrayAccess(VariablePtr varNode, NodePtr accessor);

        std::string name() const { return mArrayName; }
        VariablePtr name_var() const { return mArrayVar; }
        NodePtr index_node() const { return mAccessor; }
      private:
        std::string mArrayName;
        VariablePtr mArrayVar = nullptr;
        NodePtr mAccessor;
    };

    class ValueAssignment : public VNode<ValueAssignment> {
      public:
        ValueAssignment(const std::string& name, NodePtr node);
        virtual OutputType output_type() const override;

        std::string value_name() const { return mValueName; }
        NodePtr value_node() const { return mValueNode; }
      private:
        std::string mValueName;
        NodePtr mValueNode;
    };

    class ArrayAssignment : public VNode<ArrayAssignment> {
      public:
        ArrayAssignment(ArrayAccessPtr array, NodePtr node);
        virtual OutputType output_type() const override;

        ArrayAccessPtr array() const { return mArray; }
        NodePtr value_node() const { return mValueNode; }
      private:
        ArrayAccessPtr mArray;
        NodePtr mValueNode;
    };

    class Deref : public VNode<Deref> {
      public:
        Deref(ArrayAccessPtr v);
        NodePtr value_node() const { return mValue; }
        virtual OutputType output_type() const override;
      private:
        ArrayAccessPtr mValue;
    };
  }
}
#endif
