%{
  #include <string>
  #include "ast.h"
  #include "lex.yy.hpp"
  #include <stdexcept>
	void yyerror(yyscan_t scanner, const char *s) {
    throw std::runtime_error(s);
  }
%}

%pure-parser
%parse-param { yyscan_t scanner }

%union {
  //xnor::ASTNode * node;
  std::string * string;
  int token;
}

%token <string> TIDENTIFIER

%type <string> ident

%%

ident : TIDENTIFIER { $$ = new std::string(*$1); delete $1; }
	  ;

%%
