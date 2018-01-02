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
  #include <vector>
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

%token <float> FLOAT
%token <int> INT
%token COMMA SEMICOLON OPEN_PAREN CLOSE_PAREN OPEN_BRACKET CLOSE_BRACKET ASSIGN NEG
%token <std::string> STRING
%token <std::string> VAR VAR_DOLLAR VAR_INDEXED
%token <xnor::ast::BinaryOp::Op> BINARY_OP
%token <xnor::ast::UnaryOp::Op> UNARY_OP

%type <xnor::ast::Variable *> var
%type <xnor::ast::ArrayAccess *> array_op
%type <xnor::ast::Node *> constant binary_op unary_op statement function_call assign
%type <std::vector<xnor::ast::Node *>*> call_args

/* Tokens */
%token TOK_EOF 0;

%destructor {} <std::string>
%destructor {} <float>
%destructor {} <int>

%right ASSIGN

/* Entry point of grammar */
%start statements

%%

statements: statement { driver.add_tree($1); }
          | statements SEMICOLON statement { driver.add_tree($3); }
          | statements SEMICOLON { }
          ;

statement: 
	  var { $$ = $1; }
    | constant { $$ = $1; }
    | binary_op { $$ = $1; }
    | unary_op { $$ = $1; }
    | function_call { $$ = $1; }
    | array_op { $$ = $1; }
    | OPEN_PAREN statement CLOSE_PAREN { $$ = $2; }
    | assign { $$ = $1; }
    | statement NEG statement { $$ = new xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::SUBTRACT, $3); }
    | NEG statement { $$ = new xnor::ast::UnaryOp(xnor::ast::UnaryOp::Op::NEGATE, $2); }
	  ;

assign :
       STRING ASSIGN statement { $$ = new xnor::ast::ValueAssignment($1, $3); }
       | array_op ASSIGN statement { $$ = new xnor::ast::ArrayAssignment($1, $3); }
       ;

var : VAR  { $$ = new xnor::ast::Variable($1); }
	 ;

constant : INT { $$ = new xnor::ast::Value<int>($1); }
         | FLOAT { $$ = new xnor::ast::Value<float>($1); }
         | STRING { $$ = new xnor::ast::Value<std::string>($1); }
         ;

binary_op : statement BINARY_OP statement { $$ = new xnor::ast::BinaryOp($1, $2, $3); }
          ;

unary_op : UNARY_OP statement { $$ = new xnor::ast::UnaryOp($1, $2); }
          ;

array_op : STRING OPEN_BRACKET statement CLOSE_BRACKET { $$ = new xnor::ast::ArrayAccess($1, $3); }
         | VAR_INDEXED OPEN_BRACKET statement CLOSE_BRACKET { $$ = new xnor::ast::ArrayAccess(new xnor::ast::Variable($1), $3); }
         ;

function_call : STRING OPEN_PAREN call_args CLOSE_PAREN { $$ = new xnor::ast::FunctionCall($1, *$3); }
         ;

call_args :  { $$ = new std::vector<xnor::ast::Node *>(); } 
          | statement { $$ = new std::vector<xnor::ast::Node *>(); $$->push_back($1); }
          | call_args COMMA statement { $$ = $1; $1->push_back($3); }
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
