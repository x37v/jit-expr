#include "driver.hh"
#include "parser.hh"
#include "scanner.hh"
#include <sstream>

namespace parse
{
    Driver::Driver()
        : scanner_ (new Scanner()),
          parser_ (new Parser(*this)),
          location_ (new location())
    {
    }


    Driver::~Driver()
    {
        delete parser_;
        delete scanner_;
        delete location_;
    }


    void Driver::reset()
    {
      mTrees.clear();
      mInputs.clear();

      delete location_;
      location_ = new location();
    }

    TreeVector Driver::parse()
    {
      reset();
      scanner_->switch_streams(&std::cin, &std::cerr);
      parser_->parse();
      return trees();
    }

    TreeVector Driver::parse_file (const std::string& path)
    {
      reset();

      std::ifstream s(path.c_str(), std::ifstream::in);
      scanner_->switch_streams(&s, &std::cerr);

      parser_->parse();

      s.close();

      return trees();
    }

    TreeVector Driver::parse_string(const std::string& value) {
      reset();

      std::stringstream s;
      s << value;
      scanner_->switch_streams(&s, &std::cerr);
      parser_->parse();
      return trees();
    }

    parse::TreeVector Driver::trees() const { return mTrees; }
    parse::InputVector Driver::inputs() const { return mInputs; }

    void Driver::add_tree(xnor::ast::NodePtr v) { mTrees.push_back(v); }
    void Driver::add_input(xnor::ast::VariablePtr v) { mInputs.push_back(v); }
}
