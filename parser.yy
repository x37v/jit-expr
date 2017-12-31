%{
  #include <string>
  #include "ast.h"
  #include "parser.hpp"
  #include "scanner.h"
  #include <stdexcept>

%}

%skeleton "lalr1.cc"
%language "c++"
%define api.namespace { xnor }
%define parser_class_name { Parser }

%parse-param { xnor::ASTNode& root }
/*
%parse-param { xnor::ASTNode ** root }
%parse-param { yyscan_t scanner }
*/

%code requires {
namespace { class ASTNode; }
}

%union {
  xnor::ASTNode * node;
  std::string * string;
	int value;
  int token;
}

%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_PLUS
%token TOKEN_MULTIPLY
%token <value> TOKEN_NUMBER

%type <node> additive_expression
%type <node> multiplicative_expression
%type <node> primary_expression

%%

input
    : additive_expression { /**root = $1;*/ }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression
    {
        $$ = new xnor::ASTNode(); //new_binexp(scAdd, $1, $3);
    }
    | additive_expression '-' multiplicative_expression
    {
        //$$ = new_binexp(scSub, $1, $3);
        $$ = new xnor::ASTNode();
    }
    ;

multiplicative_expression
    : primary_expression { $$ = $1; }
    | multiplicative_expression '*' primary_expression
    {
        //$$ = new_binexp(scMul, $1, $3);
        $$ = new xnor::ASTNode();
    }
    | multiplicative_expression '/' primary_expression
    {
        //$$ = new_binexp(scDiv, $1, $3);
        $$ = new xnor::ASTNode();
    }
    ;

primary_expression
    : '(' additive_expression ')' { $$ = $2; }
    | TOKEN_NUMBER {
			//$$ = new_number($1);
			$$ = new xnor::ASTNode();
		}
    ;

%%
