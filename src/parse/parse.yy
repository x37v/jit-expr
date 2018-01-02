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
%token COMMA SEMICOLON OPEN_PAREN CLOSE_PAREN OPEN_BRACKET CLOSE_BRACKET ASSIGN QUOTE

%token NEG
%token ADD
%token MULTIPLY
%token DIVIDE
%token COMP_EQUAL
%token COMP_NOT_EQUAL
%token COMP_GREATER
%token COMP_LESS
%token COMP_GREATER_OR_EQUAL
%token COMP_LESS_OR_EQUAL
%token LOGICAL_OR
%token LOGICAL_AND
%token SHIFT_RIGHT
%token SHIFT_LEFT
%token BIT_AND
%token BIT_OR
%token BIT_XOR

%token UNOP_BIT_NOT
%token UNOP_LOGICAL_NOT

%token <std::string> STRING
%token <std::string> VAR VAR_DOLLAR VAR_INDEXED

%type <xnor::ast::Variable *> var
%type <xnor::ast::ArrayAccess *> array_op
%type <xnor::ast::Node *> constant binary_op unary_op statement function_call assign
%type <std::vector<xnor::ast::Node *>*> call_args

/* Tokens */
%token TOK_EOF 0;

%destructor {} <std::string>
%destructor {} <float>
%destructor {} <int>

%left ADD NEG
%left MULTIPLY DIVIDE
%right OPEN_PAREN
%right ASSIGN
%right UMINUS

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
    | NEG statement %prec UMINUS { $$ = new xnor::ast::UnaryOp(xnor::ast::UnaryOp::Op::NEGATE, $2); }
    ;

assign :
       STRING ASSIGN statement { $$ = new xnor::ast::ValueAssignment($1, $3); }
       | array_op ASSIGN statement { $$ = new xnor::ast::ArrayAssignment($1, $3); }
       ;

var : VAR  { $$ = new xnor::ast::Variable($1); }
    | VAR_INDEXED { $$ = new xnor::ast::Variable($1); }
    ;

constant : INT { $$ = new xnor::ast::Value<int>($1); }
         | FLOAT { $$ = new xnor::ast::Value<float>($1); }
         | STRING { $$ = new xnor::ast::Value<std::string>($1); }
         | VAR_DOLLAR { $$ = new xnor::ast::Value<std::string>($1); }
         | QUOTE STRING QUOTE { $$ = new xnor::ast::Value<std::string>("\"" + std::string($2) + "\""); }
         ;

binary_op : 
          statement ADD statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::ADD, $3); }
        | statement MULTIPLY statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::MULTIPLY, $3); }
        | statement DIVIDE statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::DIVIDE, $3); }
        | statement COMP_EQUAL statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::COMP_EQUAL, $3); }
        | statement COMP_NOT_EQUAL statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::COMP_NOT_EQUAL, $3); }
        | statement COMP_GREATER statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::COMP_GREATER, $3); }
        | statement COMP_LESS statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::COMP_LESS, $3); }
        | statement COMP_GREATER_OR_EQUAL statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::COMP_GREATER_OR_EQUAL, $3); }
        | statement COMP_LESS_OR_EQUAL statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::COMP_LESS_OR_EQUAL, $3); }
        | statement LOGICAL_OR statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::LOGICAL_OR, $3); }
        | statement LOGICAL_AND statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::LOGICAL_AND, $3); }
        | statement SHIFT_RIGHT statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::SHIFT_RIGHT, $3); }
        | statement SHIFT_LEFT statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::SHIFT_LEFT, $3); }
        | statement BIT_AND statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::BIT_AND, $3); }
        | statement BIT_OR statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::BIT_OR, $3); }
        | statement BIT_XOR statement { $$ = new::xnor::ast::BinaryOp($1, xnor::ast::BinaryOp::Op::BIT_XOR, $3); }
        ;

unary_op : UNOP_LOGICAL_NOT statement { $$ = new xnor::ast::UnaryOp(xnor::ast::UnaryOp::Op::LOGICAL_NOT, $2); }
         | UNOP_BIT_NOT statement { $$ = new xnor::ast::UnaryOp(xnor::ast::UnaryOp::Op::BIT_NOT, $2); }
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
