#ifndef XNOR_ASTNODE
#define XNOR_ASTNODE

namespace xnor {
  class ASTNode {
    public:
      virtual ~ASTNode();
  };

  template <typename T>
    class ASTNValue : public ASTNode {
      public:
        ASTNValue(T v) : mValue(v) {}
        T value() const { return mValue; }
      private:
        T mValue;
    };
}

#endif
