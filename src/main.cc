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
  
  llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

  std::ifstream infile(argv[1]);

  parse::Driver driver;
  std::string line;

  while(std::getline(infile, line)) {
    try { 
      cout << "parsing: " << line << endl;
      auto t = driver.parse_string(line);

      float value = 0;
      float ** out = new float*[1];
      out[0] = &value;

      xnor::LLVMCodeGenVisitor::input_arg_t * in = new xnor::LLVMCodeGenVisitor::input_arg_t[12];
      in[0].flt = 53.2;

      for (auto c: t) {
        //xnor::AstPrintVisitor v;
        //c->accept(&v);
        xnor::LLVMCodeGenVisitor cv;
        c->accept(&cv);
        auto f = cv.function();
        f(out, in);
        cout << "output " << value << endl;
      }
      cout << endl;
    } catch (std::runtime_error& e) {
      cerr << "fail: " << e.what() << endl;
      return -1;
    }
  }

  return 0;
}
