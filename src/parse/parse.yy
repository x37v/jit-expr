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
%token <xnor::ast::BinaryOp::Op> BINARY_OP
%token <xnor::ast::UnaryOp::Op> UNARY_OP

%type <xnor::ast::Node *> var constant binary_op unary_op statement

/* Tokens */
%token TOK_EOF 0;

/* Entry point of grammar */
%start statement

%%

statement: 
	  var { driver.ast_ = $1; }
    | constant { driver.ast_ = $1; }
    | binary_op { driver.ast_ = $1; }
    | unary_op { driver.ast_ = $1; }
	  ;

var : VAR  { $$ = new xnor::ast::Variable($1); }
	 ;

constant : INT { $$ = new xnor::ast::Value<int>($1); }
         | FLOAT { $$ = new xnor::ast::Value<float>($1); }
         ;

binary_op : statement BINARY_OP statement { $$ = new xnor::ast::BinaryOp($1, $2, $3); }
          ;

unary_op : UNARY_OP statement { $$ = new xnor::ast::UnaryOp($1, $2); }
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
