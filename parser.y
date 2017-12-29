%{
  #include "ast.h"
  #include "lex.yy.h"
%}

%lex-param {void * scanner}
%parse-param {void * scanner}
%define api.pure full

%union {
  //xnor::ASTNode * node;
  int token;
}

%token <symp> NAME
%token <dval> NUMBER

%left '-' '+'
%left '*' '/'

%type <dval> expression

%%
/*** Rules section ***/
statement_list : statement '\n'
  | statement_list statement '\n'

statement: NAME '=' expression { $1->value = $3; }           
  | expression { printf ("= %g\n", $1); }

expression: NUMBER
  | NAME { $$ = $1->value; }

%%
