#include "parse/driver.hh"
#include "print.h"
#include <string>

#include <m_pd.h>

#include <iostream>
#include <fstream>
using std::cerr;
using std::cout;
using std::endl;

#include "print.h"

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
      for (auto n : t) {
        xnor::AstPrintVisitor v;
        n->accept(&v);
      }

      /*
      std::array<float, 4> value {{ 0, 0, 0, 0 }};
      std::array<float *, 4> out {{
        &value.front(),
        &value.front(),
        &value.front(),
        &value.front(),
      }};

      //std::array<float, 4> invec {{ 0, 0, 0, 0 }};
      xnor::LLVMCodeGenVisitor::input_arg_t * in = new xnor::LLVMCodeGenVisitor::input_arg_t[12];
      in[0].flt = 53.2;

      xnor::LLVMCodeGenVisitor cv;
      std::string s;
      auto f = cv.function(t, s);
      f(&out.front(), in, 2);
      cout << "output " << value[0] << endl;
      cout << endl;
      */
    } catch (std::runtime_error& e) {
      cerr << "fail: " << e.what() << endl;
      return -1;
    }
  }

  return 0;
}
