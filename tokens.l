%{
#include "parser.tab.hpp"
#include <iostream>
using std::cout;
using std::endl;
%}

%option reentrant noyywrap header-file="./lex.yy.h" bison-bridge

%%
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

-?[0-9]+\.[0-9]*   { cout << "found a floating-point number:" << yytext << endl; }
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

%%

