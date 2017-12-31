#include <cstdio>
#include <string>

#include "ast.h"
#include "parser.tab.hpp"
#include "lex.yy.hpp"

#include <iostream>
using std::cout;
using std::endl;

int yyparse(xnor::ASTNode ** ast, void *scanner);

xnor::ASTNode *parse_file(FILE * fp) {
  xnor::ASTNode *ast;
    yyscan_t scanner;
    YY_BUFFER_STATE state;

    if (yylex_init(&scanner)) {
        // couldn't initialize
        return NULL;
    }

    if (!fp)
        fp = stdin;
    state = yy_create_buffer(fp, YY_BUF_SIZE, scanner);
    yy_switch_to_buffer(state, scanner);

    if (yyparse(&ast, scanner)) {
        // error parsing
        return NULL;
    }

    yy_delete_buffer(state, scanner);

    yylex_destroy(scanner);

    return ast;
}

int main(int argc, char* argv[]) {
  yyscan_t scanner;

  auto f = fopen(argv[1], "rb");

  try {
    parse_file(f);
    /*
    yylex_init(&scanner);
    yyset_in(f, scanner);
    //yylex(scanner);
    //yyparse(scanner);
    yylex_destroy(scanner);
    */
  } catch (std::runtime_error& e) {
    cout << e.what() << endl;
  }
}
