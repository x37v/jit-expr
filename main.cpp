#include <cstdio>
#include "lex.yy.h"
#include "parser.tab.h"

int main(int argc, char* argv[]) {
  yyscan_t scanner;

  auto f = fopen(argv[1], "rb");

  yylex_init(&scanner);
  yyset_in(f, scanner);
  //yylex(scanner);
  yyparse(&scanner);
  yylex_destroy(scanner);
}
