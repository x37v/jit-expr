#include "driver.hh"
#include "parser.hh"
#include "scanner.hh"
#include <sstream>
#include <map>

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
    xnor::ast::VariableVector Driver::inputs() const { return mInputs; }

    void Driver::add_tree(xnor::ast::NodePtr v) { mTrees.push_back(v); }
    void Driver::add_input(xnor::ast::VariablePtr v) { mInputs.push_back(v); }
    void Driver::validate() {
      //make sure we have consecutive inputs
      std::map<unsigned int, xnor::ast::Variable::VarType> inputs;
      for (auto i: mInputs) {
        if (i->type() == xnor::ast::Variable::VarType::OUTPUT)
          continue; //ignore output vars

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
