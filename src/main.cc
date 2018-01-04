#include "parse/driver.hh"
#include <string>

#include <iostream>
#include <fstream>
using std::cerr;
using std::cout;
using std::endl;

int main(int argc, char * argv[]) {
  if (argc != 2)
    throw std::runtime_error("must provide a file as an argument");

  std::ifstream infile(argv[1]);

  parse::Driver driver;
  std::string line;
  while(std::getline(infile, line)) {
    try { 
      cout << "parsing: " << line << endl;
      auto t = driver.parse_string(line);
      for (auto c: t) {
        c->print([](std::string v, unsigned int d) {
          for (auto i = 0; i < d; i++)
            cout << "  ";
          cout << v << endl;
        });
      }
      cout << endl;
    } catch (std::runtime_error& e) {
      cerr << "fail: " << e.what() << endl;
      return -1;
    }
  }

  return 0;
}
