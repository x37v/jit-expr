#include <cstdio>
#include "lex.yy.hpp"

int main(int argc, char* argv[]) {
  yyscan_t scanner;

  auto f = fopen(argv[1], "rb");

  yylex_init(&scanner);
  yyset_in(f, scanner);
  yylex(scanner);
  yylex_destroy(scanner);
}
