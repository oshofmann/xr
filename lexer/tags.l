%x id
%x srcfile
%x line
%x type
%x end
%x comment

%{
#include <stdio.h>
#include <string.h>

extern char **filters;
extern int n_filters;

#define ECHO put_val(yytext, 1)

%}

%%

<comment>\n { BEGIN(INITIAL); }
<INITIAL,id,srcfile,line,type,end>\n         { BEGIN(INITIAL); eject_line(); }

<INITIAL>! { BEGIN(comment); }
<comment>.* { /* ignore */ }

<INITIAL>[^![:space:]]+ { put_key("id"); ECHO; }
<INITIAL>[[:space:]]+   { BEGIN(srcfile); }
<srcfile>[^[:space:]]+  { put_key("file"); echo_filename(); }
<srcfile>[[:space:]]+   { BEGIN(line); }
<line>[[:digit:]]+      { put_key("line"); put_val(yytext, 0); }
<line>[[:space:]]+      { BEGIN(type); }
<type>.                 { put_key("type"); ECHO; BEGIN(end); }
<srcfile,line,end>.     { /* ignore */ }

%%

yywrap() { return 1; }

echo_filename()
{
	char *name = yytext;

	int i;
	for (i = 0; i < n_filters; i++) {
		int len = strlen(filters[i]);
		if (!strncmp(yytext, filters[i], len)) {
			name = &yytext[len];
			break;
		}
	}

	put_val(name, 1);
}
