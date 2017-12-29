#include <cstdio>
#include "lex.yy.hpp"

extern int yylex (yyscan_t yyscanner);

int main(int argc, char* argv[]) {
  yyscan_t scanner;
  yylex_init(&scanner);
  yyset_in(fopen(argv[1], "rb"), scanner);
  yylex(scanner);
  yylex_destroy(scanner);
}
