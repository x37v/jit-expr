#include <cstdio>
#include <string>
#include "lex.yy.hpp"
#include "parser.tab.hpp"

#include <iostream>
using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
  yyscan_t scanner;

  auto f = fopen(argv[1], "rb");

  try {
    yylex_init(&scanner);
    yyset_in(f, scanner);
    yylex(scanner);
    yyparse(scanner);
    yylex_destroy(scanner);
  } catch (std::runtime_error& e) {
    cout << e.what() << endl;
  }
}
