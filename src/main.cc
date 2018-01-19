#include "parse/driver.hh"
#include "llvmcodegen/codegen.h"
#include <llvm/Support/TargetSelect.h>
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
  
  xnor::LLVMCodeGenVisitor::init();

  std::ifstream infile(argv[1]);

  parse::Driver driver;
  std::string line;

  while(std::getline(infile, line)) {
    try { 
      cout << "parsing: " << line << endl;
      auto t = driver.parse_string(line);

      float value = 0;
      float ** out = new float*[4];
      out[0] = &value;
      out[1] = &value;
      out[2] = &value;
      out[3] = &value;

      xnor::LLVMCodeGenVisitor::input_arg_t * in = new xnor::LLVMCodeGenVisitor::input_arg_t[12];
      in[0].flt = 53.2;

      xnor::LLVMCodeGenVisitor cv;
      auto f = cv.function(t, true);
      f(out, in, 1);
      cout << "output " << value << endl;
      cout << endl;
    } catch (std::runtime_error& e) {
      cerr << "fail: " << e.what() << endl;
      return -1;
    }
  }

  return 0;
}
