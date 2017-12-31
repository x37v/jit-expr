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
%token <std::string> VAR_FLOAT VAR_INTEGER VAR_SYMBOL VAR_VECTOR VAR_INPUT VAR_OUTPUT VAR_DOLLAR;
%token <std::string> OPEN_PAREN CLOSE_PAREN

%type <xnor::ast::Variable *> var

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
	  var
	  ;

var : VAR_FLOAT  { $$ = "asdf"; }
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
