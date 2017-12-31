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
        delete location_;
        location_ = new location();
    }

    int Driver::parse()
    {
        scanner_->switch_streams(&std::cin, &std::cerr);
        return parser_->parse();
    }

    int Driver::parse_file (const std::string& path)
    {
        std::ifstream s(path.c_str(), std::ifstream::in);
        scanner_->switch_streams(&s, &std::cerr);

        int ret = parser_->parse();

        s.close();

        return ret;
    }

    int Driver::parse_string(const std::string& value) {
      std::stringstream s;
      s << value;
      scanner_->switch_streams(&s, &std::cerr);
      return parser_->parse();
    }
}
