#include "parse/driver.hh"
#include <string>

#include <iostream>
#include <fstream>
using std::cout;
using std::endl;

int main(int argc, char * argv[]) {
  if (argc != 2)
    throw std::runtime_error("must provide a file as an argument");

  std::ifstream infile(argv[1]);

  parse::Driver driver;
  std::string line;
  while(std::getline(infile, line)) {
    cout << "parsing: " << line << endl;
    auto c = driver.parse_string(line).size();
    cout << "parse " << (c ? "success" : "fail" ) << " trees: " << c << endl;
    cout << endl;
  }

  return 0;
}
