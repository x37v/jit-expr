%{     /* PARSER */

#include "parser.hh"
#include "scanner.hh"

#define yylex driver.scanner_->yylex
%}

%code requires
{
  #include <iostream>
  #include "driver.hh"
  #include "location.hh"
  #include "position.hh"
  #include "ast.h"
  #include <string>
}

%code provides
{
  namespace parse
  {
    // Forward declaration of the Driver class
    class Driver;

    inline void
    yyerror (const char* msg)
    {
      std::cerr << msg << std::endl;
    }
  }
}



%require "2.4"
%language "C++"
%locations
%defines
%debug

%define api.namespace {parse}
%define parser_class_name {Parser}

%define api.value.type variant

%parse-param { Driver &driver }
%lex-param { Driver &driver }
%error-verbose

%token <float> FLOAT;
%token <int> INT;
%token <std::string> STRING;
%token <std::string> VAR VAR_DOLLAR;
%token <std::string> OPEN_PAREN CLOSE_PAREN

%type <xnor::ast::Node *> var constant;

/* Tokens */
%token TOK_EOF 0
	PLUS "+"
	MINUS "-"
	MULTIPLY  "*"
	SHIFT_LEFT "<<"
	SHIFT_RIGHT ">>"
;

/* Entry point of grammar */
%start start

%%

start: 
	  var | constant
	  ;

var : VAR  { $$ = new xnor::ast::Variable($1); }
	 ;

constant : INT { $$ = new xnor::ast::Value<int>($1); }
         | FLOAT { $$ = new xnor::ast::Value<float>($1); }
         ;

%%

namespace parse
{
    void Parser::error(const location&, const std::string& m)
    {
        std::cerr << *driver.location_ << ": " << m << std::endl;
        driver.error_ = (driver.error_ == 127 ? 127 : driver.error_ + 1);
    }
}
