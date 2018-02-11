//Copyright (c) 2017 Rémi Berson, modified for jit-expr 2018 by Alex Norman

#include "driver.hh"
#include "parser.hh"
#include "scanner.hh"
#include <sstream>
#include <map>
#include <algorithm>

namespace parse
{
    Driver::Driver()
        : scanner_ (new Scanner()),
          parser_ (new Parser(*this)),
          location_ (new location())
    {
    }


    Driver::~Driver() {
        delete parser_;
        delete scanner_;
        delete location_;
    }


    void Driver::reset() {
      mTrees.clear();
      mInputs.clear();

      delete location_;
      location_ = new location();
    }

    TreeVector Driver::parse() {
      reset();
      scanner_->switch_streams(&std::cin, &std::cerr);
      parser_->parse();

      validate();
      return trees();
    }

    TreeVector Driver::parse_file(const std::string& path) {
      reset();

      std::ifstream s(path.c_str(), std::ifstream::in);
      scanner_->switch_streams(&s, &std::cerr);

      parser_->parse();

      s.close();

      validate();
      return trees();
    }

    TreeVector Driver::parse_string(const std::string& value) {
      reset();

      std::stringstream s;
      s << value;
      scanner_->switch_streams(&s, &std::cerr);
      parser_->parse();

      validate();
      return trees();
    }

    parse::TreeVector Driver::trees() const { return mTrees; }
    xnor::ast::VariableVector Driver::inputs() const {
      xnor::ast::VariableVector i;
      //filter out outputs
      std::copy_if(mInputs.begin(), mInputs.end(), std::back_inserter(i), [](xnor::ast::VariablePtr v) { return v->type() != xnor::ast::Variable::VarType::OUTPUT; });
      std::sort(i.begin(), i.end(), [](xnor::ast::VariablePtr a, xnor::ast::VariablePtr b) { return a->input_index() < b->input_index(); });
      return i;
    }

    void Driver::add_tree(xnor::ast::NodePtr v) { mTrees.push_back(v); }
    xnor::ast::VariablePtr Driver::add_input(xnor::ast::VariablePtr v) {
      for (auto i: mInputs) {
        if (i->input_index() == v->input_index() && i->type() == v->type())
          return i;
      }
      mInputs.push_back(v);
      return v;
    }
    void Driver::validate() {
      //make sure we have consecutive inputs
      std::map<unsigned int, xnor::ast::Variable::VarType> inputs;
      for (auto i: mInputs) {
        if (i->type() == xnor::ast::Variable::VarType::OUTPUT) {
          if (i->input_index() >= mTrees.size())
            throw std::runtime_error("output var index is greater than number of outputs");
          continue;
        }

        unsigned int idx = i->input_index();
        auto it = inputs.find(idx);
        if (it != inputs.end()) {
          if (it->second != i->type())
            throw std::runtime_error("mismatched input types at index: " + std::to_string(idx + 1));
          continue;
        }
        inputs[idx] = i->type();
      }

      for (unsigned int i = 0; i < inputs.size(); i++) {
        if (inputs.find(i) == inputs.end())
          throw std::runtime_error("missing an input variable at index " + std::to_string(i + 1));
      }
    }
}
