#include "parse/driver.hh"
#include <string>

#include <iostream>
using std::cout;
using std::endl;

int main() {
  parse::Driver driver;
  std::string s = "$f1";

  auto test = [](xnor::ast::Node * root) {
    cout << "parse " << (root ? "success" : "fail") << endl;
  };

  test(driver.parse_string(s));

  s = "1";
  test(driver.parse_string(s));

  s = "01.234";
  test(driver.parse_string(s));

  s = "01.234 * $f1";
  test(driver.parse_string(s));

  s = "~$f1";
  test(driver.parse_string(s));

  s = "log10(234\\, 42.0 + $f1)";
  test(driver.parse_string(s));

  s = "v34_asdf[$f1]";
  test(driver.parse_string(s));

  return 0;
}
