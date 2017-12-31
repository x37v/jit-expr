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
  }
}
