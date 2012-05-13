D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%x comment
%x new_comment
%x ml_comment
%x include
%x in_line
%x ml_string

%{
#include <stdio.h>
#include "put_vals.h"

#define GT add_val("&gt;", 1)
#define LT add_val("&lt;", 1)
#define AMP add_val("&amp;", 1)
#define NBSP add_val("&nbsp;", 1)
#define TS add_val("<span class=\"ts\"/>", 1)

#define ECHO add_val(yytext, 1)

%}

%%

<in_line>\n             { newline(); BEGIN(0); }

<INITIAL>\n { begin_line(); newline(); }
<INITIAL>.  { begin_line(); unput(yytext[0]); BEGIN(in_line); }

<in_line>{
   \t             { TS; }

   {L}({L}|{D})*		{ identifier(); }

   0[xX]{H}+{IS}?		{ ECHO; }
   0{D}+{IS}?		{ ECHO; }
   {D}+{IS}?		{ ECHO; }
   L?'(\\.|[^\\'])+'	{ ECHO; }

   {D}+{E}{FS}?		{ ECHO; }
   {D}*"."{D}+({E})?{FS}?	{ ECHO; }
   {D}+"."{D}*({E})?{FS}?	{ ECHO; }

   "..."			{ ECHO; }
   ">>="			{ ECHO; }
   "<<="			{ ECHO; }
   "+="			{ ECHO; }
   "-="			{ ECHO; }
   "*="			{ ECHO; }
   "/="			{ ECHO; }
   "%="			{ ECHO; }
   "&="			{ ECHO; }
   "^="			{ ECHO; }
   "|="			{ ECHO; }
   ">>"			{ ECHO; }
   "<<"			{ ECHO; }
   "++"			{ ECHO; }
   "--"			{ ECHO; }
   "->"			{ ECHO; }
   "&&"			{ ECHO; }
   "||"			{ ECHO; }
   "<="			{ ECHO; }
   ">="			{ ECHO; }
   "=="			{ ECHO; }
   "!="			{ ECHO; }
   ";"			{ ECHO; }
   ("{"|"<%")		{ ECHO; }
   ("}"|"%>")		{ ECHO; }
   ","			{ ECHO; }
   ":"			{ ECHO; }
   "="			{ ECHO; }
   "("			{ ECHO; }
   ")"			{ ECHO; }
   ("["|"<:")		{ ECHO; }
   ("]"|":>")		{ ECHO; }
   "."			{ ECHO; }
   "&"			{ AMP; }
   "!"			{ ECHO; }
   "~"			{ ECHO; }
   "-"			{ ECHO; }
   "+"			{ ECHO; }
   "*"			{ ECHO; }
   "/"			{ ECHO; }
   "%"			{ ECHO; }
   "<"			{ LT; }
   ">"			{ GT; }
   "^"			{ ECHO; }
   "|"			{ ECHO; }
   "?"			{ ECHO; }
}

<in_line>L?\"(\\.|[^\\"])*\"		{ ECHO; }
<in_line>L?\"(\\.|[^\\"])*\\\n	{ ml_string_continue(); BEGIN(ml_string); }
<ml_string>(\\.|[^\\"])*\\\n	{ begin_line(); ml_string_continue(); }
<ml_string>(\\.|[^\\"])*\"		{ begin_line(); ECHO; BEGIN(in_line); }


<comment,new_comment,include><  { LT; }
<comment,new_comment,include>>  { GT; }
<comment,new_comment,include>&  { AMP; }
<comment,new_comment,include>\t { TS; }

<in_line>"/*"              { begin_comment(); ECHO; }
<comment>"*/"     { ECHO; end_comment(); }
<comment>\n       { newline_comment(); }
<comment>.        { ECHO; }

<in_line>"//"     { begin_new_comment(); ECHO; }
<new_comment>\n   { end_new_comment(); }

<ml_comment>^\*\/  { begin_line(); begin_comment(); ECHO; end_comment(); }
<ml_comment>"*/"  { end_comment(); }
<ml_comment>.     { begin_line(); begin_comment(); unput(yytext[0]); }
<ml_comment>^\n    { begin_line(); begin_comment(); newline_comment(); }
<ml_comment>\n    { newline_comment(); }

%%

yywrap()
{
	return(1);
}

newline()
{
   end_val(1);
   eject_line();
}

begin_line()
{
   start_val(1);
}

begin_comment()
{
   BEGIN(comment);
   add_val("<span class=\"comment\">", 1);
}

end_comment()
{
   BEGIN(in_line);
   add_val("</span>", 1);
}

newline_comment()
{
   end_comment();
   BEGIN(ml_comment);
   newline();
}

begin_new_comment()
{
   BEGIN(new_comment);
   add_val("<span class=\"comment\">", 1);
}

end_new_comment()
{
   BEGIN(0);
   add_val("</span>", 1);
   newline();
}

ml_string_continue()
{
	yytext[yyleng - 1] = 0;
	add_valf("%s", 1, yytext);
	newline();
}

identifier()
{
   add_valf("<a class=\"id\" href=\"#%s\">", 1, yytext);
   add_val(yytext, 1);
   add_val("</a>", 1);
}
