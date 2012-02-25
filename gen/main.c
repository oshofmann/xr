#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/mman.h>

extern int yylex();
extern FILE *yyin;

char **filters = NULL;
int n_filters;

int main(int argc, char **argv)
{
   FILE *src_in;

   if (argc > 2) {
      filters = &argv[2];
      n_filters = argc - 2;
   }

   src_in = fopen(argv[1], "r");

   yyin = src_in;

   yylex();

   return 0;
}
