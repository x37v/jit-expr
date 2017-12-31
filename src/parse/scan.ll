%{                                                            /* -*- C++ -*- */

#include "parser.hh"
#include "scanner.hh"
#include "driver.hh"

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
"expr~"            { cout << "found: " << yytext << endl; }
"expr"             { cout << "found: " << yytext << endl; }
"fexpr~"           { cout << "found: " << yytext << endl; }

\$f[0-9]+          { cout << "found float var: " << yytext << endl; }
\$i[0-9]+          { cout << "found int var: " << yytext << endl; }
\$s[0-9]+          { cout << "found sym var: " << yytext << endl; }
\$v[0-9]+          { cout << "found vec var: " << yytext << endl; }
\$x[0-9]+          { cout << "found input var: " << yytext << endl; }
\$y[0-9]+          { cout << "found output var: " << yytext << endl; }
\\\$[0-9]+         { cout << "found dollar var: " << yytext << endl; }

-?[0-9]+\.[0-9]*   { cout << "found a floating-point number:" << yytext << endl; return token::FLOAT; }
-?[0-9]+           { cout << "found an integer:" << yytext << endl; }
[a-zA-Z0-9]+       { cout << "found a string: " << yytext << endl; }

"["                { cout << "found open bracket: " << yytext << endl; }
"]"                { cout << "found close bracket: " << yytext << endl; }
"("                { cout << "found open paren: " << yytext << endl; }
")"                { cout << "found close paren: " << yytext << endl; }
"+"                { cout << "found " << yytext << endl; }
"-"                { cout << "found " << yytext << endl; }
"*"                { cout << "found " << yytext << endl; }
"/"                { cout << "found " << yytext << endl; }
"="                { cout << "found " << yytext << endl; }
">>"               { cout << "found " << yytext << endl; }
"<<"               { cout << "found " << yytext << endl; }
">"                { cout << "found " << yytext << endl; }
"<"                { cout << "found " << yytext << endl; }
">="               { cout << "found " << yytext << endl; }
"<="               { cout << "found " << yytext << endl; }
"&"                { cout << "found " << yytext << endl; }
"|"                { cout << "found " << yytext << endl; }
"^"                { cout << "found " << yytext << endl; }
"~"                { cout << "found " << yytext << endl; }
"!"                { cout << "found " << yytext << endl; }
"\""               { cout << "found " << yytext << endl; }
"\\,"              { cout << "found " << yytext << endl; }
"\\;"              { cout << "found " << yytext << endl; }
"_"                { cout << "found " << yytext << endl; }
.                  { throw std::runtime_error("UNKNOWN TOKEN" + std::string(yytext)); }

.             {
                std::cerr << *driver.location_ << " Unexpected token : "
                                              << *yytext << std::endl;
                driver.error_ = (driver.error_ == 127 ? 127
                                : driver.error_ + 1);
                STEP ();
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
