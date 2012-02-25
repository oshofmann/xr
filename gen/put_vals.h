#ifndef PUT_VALS_H
#define PUT_VALS_H

void start_val(int str);
void end_val(int str);
void add_val(const char *val, int str);
void add_valf(const char *fmt, int str, ...);
void put_val(const char *val, int str);
void put_valf(const char *fmt, int str, ...);
void start_key();
void end_key();
void put_key(const char *key);
void eject_line();

#endif
