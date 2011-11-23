#include <stdio.h>
#include <stdarg.h>
#include <alloca.h>
#include "put_vals.h"

void eject_line()
{
   putchar('\n');
}

void start_val(int str)
{
   if (str)
      putchar('"');
}

void end_val(int str)
{
   if (str)
      putchar('"');
   fputs(",\t", stdout);
}

void add_val(const char *val, int str)
{
   /* Output the line as a valid string literal */
   const char *ipos = val;

   while (*ipos) {
      if (str) {
         switch (*ipos) {
            case '"':
            case '\\':
               putchar('\\');
         }
      }
      putchar(*ipos++);
   }
}

void _add_valf(const char *fmt, int str, va_list ap)
{
   static char val_buf[48];
   char *buf;
   int n;
   va_list ap_copy;

   va_copy(ap_copy, ap);
   n = vsnprintf(val_buf, sizeof(val_buf), fmt, ap);
   if (n < sizeof(val_buf)) {
      buf = val_buf;
   } else {
      buf = alloca(n+1);
      vsnprintf(buf, n+1, fmt, ap_copy);
   }
   va_end(ap_copy);

   add_val(buf, str);
}

void add_valf(const char *fmt, int str, ...)
{
   va_list ap;
   va_start(ap, str);
   _add_valf(fmt, str, ap);
   va_end(ap);
}

void put_val(const char *val, int str)
{
   start_val(str);
   add_val(val, str);
   end_val(str);
}

void put_valf(const char *fmt, int str, ...)
{
   va_list ap;
   va_start(ap, str);
   start_val(str);
   _add_valf(fmt, str, ap);
   end_val(str);
}

void start_key()
{
   start_val(1);
}

void end_key()
{
   putchar('"');
   putchar(':');
}

void put_key(const char *key)
{
   start_key();
   add_val(key, 1);
   end_key();
}
