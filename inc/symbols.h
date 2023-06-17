#ifndef SYMBOLS_H
#define SYMBOLS_H

#define SYMBOL_TABLE_SIZE 100

//TODO: use this for labels, variables, etc.

void symbols_init();

char * symbols_new(char * name, char * value, int s_type);

void symbols_update(char * name, char * value, int s_type);

char * symbols_find(char * name, int s_type);

char * symbols_find_any_type(char * name);

void symbols_done();

#endif