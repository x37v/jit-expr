#include "parse/driver.hh"
#include <string>

#include <iostream>
#include <fstream>
using std::cout;
using std::endl;

int main(int argc, char * argv[]) {
  if (argc != 2)
    throw std::runtime_error("must provide a file as an argument");

  parse::Driver driver;

  auto test = [](xnor::ast::Node * root) {
    cout << "parse " << (root ? "success" : "fail") << endl;
  };

  std::ifstream infile(argv[1]);

  std::string line;
  while(std::getline(infile, line)) {
    cout << "parsing " << line << endl;
    test(driver.parse_string(line));
    cout << endl;
  }

  return 0;
}
