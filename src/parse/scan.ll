%{                                                            /* -*- C++ -*- */

#include "parser.hh"
#include "scanner.hh"
#include "driver.hh"

#include <string>
#include <iostream>
using std::cout;
using std::endl;

/*  Defines some macros to update locations */


#define STEP()                                      \
  do {                                              \
    driver.location_->step ();                      \
  } while (0)


#define COL(Col)				                    \
  driver.location_->columns (Col)

#define LINE(Line)				                    \
  do{						                        \
    driver.location_->lines (Line);		            \
 } while (0)

#define YY_USER_ACTION				                \
  COL(yyleng);


typedef parse::Parser::token token;
typedef parse::Parser::token_type token_type;

#define yyterminate() return token::TOK_EOF

%}

%option debug
%option c++
%option noyywrap
%option never-interactive
%option yylineno
%option nounput
%option batch
%option prefix="parse"

/*
%option stack
*/

/* Abbreviations.  */

blank   [ \t]+
eol     [\n\r]+

%%

 /* The rules.  */
%{
  STEP();
%}

[ \t\n]            ;
eol                ;
"expr~"            { cout << "found: " << yytext << endl; }
"expr"             { cout << "found: " << yytext << endl; }
"fexpr~"           { cout << "found: " << yytext << endl; }

\$[fiv][0-9]+      { yylval->build<std::string>() = std::string(yytext); return token::VAR; }
\$s[0-9]+          { yylval->build<std::string>() = std::string(yytext); return token::VAR_INDEXED; }
\$[xy][0-9]*       { yylval->build<std::string>() = std::string(yytext); return token::VAR_INDEXED; }
\\\$[0-9]+         { yylval->build<std::string>() = std::string(yytext); return token::VAR_DOLLAR; }

-?[0-9]+\.[0-9]*      { yylval->build<float>() = std::stof(yytext); return token::FLOAT; }
-?[0-9]+              { yylval->build<int>() = std::stoi(yytext); return token::INT; }
[a-zA-Z]([_a-zA-Z0-9]|\\\$[0-9]+)* { yylval->build<std::string>() = std::string(yytext); return token::STRING; }

"["                { return token::OPEN_BRACKET; }
"]"                { return token::CLOSE_BRACKET; }
"("                { return token::OPEN_PAREN; }
")"                { return token::CLOSE_PAREN; }
"="                { return token::ASSIGN; }
"\+"               { return token::ADD; }
"-"                { return token::NEG; }
"\*"               { return token::MULTIPLY; }
"\/"               { return token::DIVIDE; }
"=="               { return token::COMP_EQUAL; }
"!="               { return token::COMP_NOT_EQUAL; }
">"                { return token::COMP_GREATER; }
"<"                { return token::COMP_LESS; }
">="               { return token::COMP_GREATER_OR_EQUAL; }
"<="               { return token::COMP_LESS_OR_EQUAL; }
"||"               { return token::LOGICAL_OR; }
"&&"               { return token::LOGICAL_AND; }
">>"               { return token::SHIFT_RIGHT; }
"<<"               { return token::SHIFT_LEFT; }

"&"                { return token::BIT_AND; }
"|"                { return token::BIT_OR; }
"^"                { return token::BIT_XOR; }

"~"                { return token::UNOP_BIT_NOT; }
"!"                { return token::UNOP_LOGICAL_NOT; }

"\""               { return token::QUOTE; }
"\\,"              { return token::COMMA; }
"\\;"              { return token::SEMICOLON; }

.             {
                std::cerr << *driver.location_ << " Unexpected token : " << *yytext << std::endl;
                driver.error_ = (driver.error_ == 127 ? 127 : driver.error_ + 1);
                STEP ();
                throw std::runtime_error("UNKNOWN TOKEN: " + std::string(yytext));
              }

%%


/*

   CUSTOM C++ CODE

*/

namespace parse
{

    Scanner::Scanner()
    : parseFlexLexer()
    {
    }

    Scanner::~Scanner()
    {
    }

    void Scanner::set_debug(bool b)
    {
        yy_flex_debug = b;
    }
}

#ifdef yylex
# undef yylex
#endif

int parseFlexLexer::yylex()
{
  std::cerr << "call parsepitFlexLexer::yylex()!" << std::endl;
  return 0;
}
