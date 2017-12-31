#include "parse/driver.hh"
#include <string>

int main() {
  parse::Driver driver;
  std::string s = "$f1";
  driver.parse_string(s);

  s = "1";
  driver.parse_string(s);

  s = "-1.234";
  driver.parse_string(s);

  return 0;
}
