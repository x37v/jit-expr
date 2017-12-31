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
      if (ast_)
        delete ast_;
      ast_ = nullptr;

      delete location_;
      location_ = new location();
    }

    xnor::ast::Node* Driver::parse()
    {
      reset();
      scanner_->switch_streams(&std::cin, &std::cerr);
      parser_->parse();
      return ast_;
    }

    xnor::ast::Node* Driver::parse_file (const std::string& path)
    {
      reset();

      std::ifstream s(path.c_str(), std::ifstream::in);
      scanner_->switch_streams(&s, &std::cerr);

      parser_->parse();

      s.close();

      return ast_;
    }

    xnor::ast::Node* Driver::parse_string(const std::string& value) {
      reset();

      std::stringstream s;
      s << value;
      scanner_->switch_streams(&s, &std::cerr);
      parser_->parse();
      return ast_;
    }
}
