%x id
%x srcfile
%x keys
%x line
%x type
%x scope
%x end
%x comment

%{
#include <stdio.h>
#include <string.h>

extern char **filters;
extern int n_filters;

static char *scope_name = NULL;

static char *cur_id = NULL;
static char  cur_type = 0;
static int   cur_line = -1;

static char *last_id = NULL;
static char  last_type = 0;
static int   last_line = -1;
static int   last_anon = 0;

#define ECHO put_val(yytext, 1)

%}

%%

<comment>\n { BEGIN(INITIAL); }
<*>\n { BEGIN(INITIAL); put_scope(); eject_line(); set_last(); }

<INITIAL>!  { BEGIN(comment); }
<comment>.* { /* ignore */ }

<INITIAL>[^![:space:]]+ { put_key("id"); ECHO; set_id(); }
<INITIAL>[[:space:]]+   { BEGIN(srcfile); }
<srcfile>[^[:space:]]+  { put_key("file"); echo_filename(); BEGIN(keys); }

<line>[[:digit:]]+ {
	set_line();
	put_key("line");
	put_val(yytext, 0);
	BEGIN(keys);
}
<type>[a-z]             { put_key("type"); ECHO; set_type(); BEGIN(keys); }
<scope>[^[:space:]]+    { set_scope(); BEGIN(keys); }

<keys>line:   { BEGIN(line); }
<keys>kind:   { BEGIN(type); }
<keys>struct: { BEGIN(scope); }
<keys>union:  { BEGIN(scope); }
<keys>enum:   { BEGIN(scope); }

<keys,line,type,scope>.     { /* ignore */ }

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

set_id()
{
	if (cur_id)
		free(cur_id);
	cur_id = strdup(yytext);
}

set_line()
{
	cur_line = atoi(yytext);
}

set_type()
{
	cur_type = yytext[0];
}

set_last()
{
	if (cur_type == 's' || cur_type == 'u' || cur_type == 'g') {
		last_id = cur_id;
		cur_id = NULL;
		last_type = cur_type;
		last_line = cur_line;
	} else {
		if (cur_id) {
			free(cur_id);
			cur_id = NULL;
		}

		if (scope_name) {
			free(scope_name);
			scope_name = NULL;
		}
	}
}

set_scope()
{
	char *end_name = strchr(yytext, ':');
	if (end_name)
		*end_name = 0;
	scope_name = strdup(yytext);
}

put_scope()
{
	if (!scope_name)
		return;

	if (last_id && !strcmp(scope_name, last_id)) {
		if (!last_anon) {
			put_key("scope");
			put_valf("%c %s", 1, last_type, scope_name);
		}
	} else {
		if (last_id)
			free(last_id);
		last_id = strdup(scope_name);
		last_anon = 1;
		last_line = cur_line - 1;
	}

	put_key("scope_line");
	put_valf("%d", 0, last_line);
}
