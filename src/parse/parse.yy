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
%token <std::string> VAR VAR_DOLLAR VAR_INDEXED VAR_SYMBOL

%type <xnor::ast::VariablePtr> var
%type <xnor::ast::ArrayAccessPtr> array_op
%type <xnor::ast::SampleAccessPtr> sample_op
%type <xnor::ast::NodePtr> constant binary_op unary_op statement function_call assign quoted call_arg
%type <std::vector<xnor::ast::NodePtr>> call_args

/* Tokens */
%token TOK_EOF 0;


/* XXX NOT SURE ABOUT PRECEDENCE */
%left COMP_EQUAL COMP_NOT_EQUAL COMP_GREATER COMP_LESS COMP_GREATER_OR_EQUAL COMP_LESS_OR_EQUAL
%left LOGICAL_OR LOGICAL_AND
%left ADD NEG
%left MULTIPLY DIVIDE
%left BIT_AND BIT_OR BIT_XOR
%left SHIFT_RIGHT SHIFT_LEFT
%right UNOP_BIT_NOT UNOP_LOGICAL_NOT
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
    | array_op { $$ = std::make_shared<xnor::ast::Deref>($1); }
    | sample_op { $$ = $1; }
    | OPEN_PAREN statement CLOSE_PAREN { $$ = $2; }
    | assign { $$ = $1; }
    | statement NEG statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::SUBTRACT, $3); }
    | NEG statement %prec UMINUS { $$ = std::make_shared<xnor::ast::UnaryOp>(xnor::ast::UnaryOp::Op::NEGATE, $2); }
    ;

assign :
       STRING ASSIGN statement { $$ = std::make_shared<xnor::ast::ValueAssignment>($1, $3); }
       | array_op ASSIGN statement { $$ = std::make_shared<xnor::ast::ArrayAssignment>($1, $3); }
       ;

var : VAR  {
      xnor::ast::VariablePtr var = std::make_shared<xnor::ast::Variable>($1);
      var = driver.add_input(var);
      $$ = var;
    }
    ;

constant : INT {
          auto v = std::make_shared<xnor::ast::Value<int>>($1);
          v->output_type(xnor::ast::Node::OutputType::INT);
          $$ = v;
          }
         | FLOAT { $$ = std::make_shared<xnor::ast::Value<float>>($1); }
         | STRING { $$ = std::make_shared<xnor::ast::Value<std::string>>($1); }
         | VAR_DOLLAR { $$ = std::make_shared<xnor::ast::Value<std::string>>($1); }
         ;

quoted : QUOTE STRING QUOTE { $$ = std::make_shared<xnor::ast::Quoted>($2); }
       | QUOTE VAR_SYMBOL QUOTE {
            xnor::ast::VariablePtr var = std::make_shared<xnor::ast::Variable>($2);
            var = driver.add_input(var);
            $$ = std::make_shared<xnor::ast::Quoted>(var);
          }
       ;

binary_op : 
          statement ADD statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::ADD, $3); }
        | statement MULTIPLY statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::MULTIPLY, $3); }
        | statement DIVIDE statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::DIVIDE, $3); }
        | statement COMP_EQUAL statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::COMP_EQUAL, $3); }
        | statement COMP_NOT_EQUAL statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::COMP_NOT_EQUAL, $3); }
        | statement COMP_GREATER statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::COMP_GREATER, $3); }
        | statement COMP_LESS statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::COMP_LESS, $3); }
        | statement COMP_GREATER_OR_EQUAL statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::COMP_GREATER_OR_EQUAL, $3); }
        | statement COMP_LESS_OR_EQUAL statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::COMP_LESS_OR_EQUAL, $3); }
        | statement LOGICAL_OR statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::LOGICAL_OR, $3); }
        | statement LOGICAL_AND statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::LOGICAL_AND, $3); }
        | statement SHIFT_RIGHT statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::SHIFT_RIGHT, $3); }
        | statement SHIFT_LEFT statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::SHIFT_LEFT, $3); }
        | statement BIT_AND statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::BIT_AND, $3); }
        | statement BIT_OR statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::BIT_OR, $3); }
        | statement BIT_XOR statement { $$ = std::make_shared<xnor::ast::BinaryOp>($1, xnor::ast::BinaryOp::Op::BIT_XOR, $3); }
        ;

unary_op : UNOP_LOGICAL_NOT statement { $$ = std::make_shared<xnor::ast::UnaryOp>(xnor::ast::UnaryOp::Op::LOGICAL_NOT, $2); }
         | UNOP_BIT_NOT statement { $$ = std::make_shared<xnor::ast::UnaryOp>(xnor::ast::UnaryOp::Op::BIT_NOT, $2); }
         ;

array_op : STRING OPEN_BRACKET statement CLOSE_BRACKET { $$ = std::make_shared<xnor::ast::ArrayAccess>($1, $3); }
         | VAR_SYMBOL OPEN_BRACKET statement CLOSE_BRACKET {
              xnor::ast::VariablePtr var = std::make_shared<xnor::ast::Variable>($1);
              var = driver.add_input(var);
              $$ = std::make_shared<xnor::ast::ArrayAccess>(var, $3);
            }
         ;

sample_op : VAR_INDEXED OPEN_BRACKET statement CLOSE_BRACKET {
            xnor::ast::VariablePtr var = std::make_shared<xnor::ast::Variable>($1);
            var = driver.add_input(var);
            $$ = std::make_shared<xnor::ast::SampleAccess>(var, $3);
         }
         | VAR_INDEXED {
           //$x# -> $x#[0]
           //$y# -> $y#[-1]
           xnor::ast::VariablePtr var = std::make_shared<xnor::ast::Variable>($1);
           var = driver.add_input(var);
           auto val = std::make_shared<xnor::ast::Value<int>>(var->type() == xnor::ast::Variable::VarType::OUTPUT ? -1 : 0);
           $$ = std::make_shared<xnor::ast::SampleAccess>(var, val);
         }

function_call : STRING OPEN_PAREN call_args CLOSE_PAREN { $$ = std::make_shared<xnor::ast::FunctionCall>($1, $3); }
         ;

call_arg : statement { $$ = $1; }
         | quoted { $$ = $1; }
         ;

call_args :  {  } 
          | call_arg { $$.push_back($1); }
          | call_args COMMA call_arg {
            $1.push_back($3);
            $$ = $1;
          }
          ;

%%

#include <sstream>

namespace parse
{
  void Parser::error(const location&, const std::string& m) {
    driver.error_ = (driver.error_ == 127 ? 127 : driver.error_ + 1);
    std::stringstream s;
    s << *driver.location_;
    throw std::runtime_error(s.str() + ": " + m);
  }
}
