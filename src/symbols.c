#include "symbols.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct symbol {
    char * name;
    char * value;
    int symbol_type;
    struct symbol * next;
};

struct symbol symbols_table[SYMBOL_TABLE_SIZE];

unsigned long djb2_hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while (c = *str++) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

void symbols_init() {
    int i;
    for (i=0; i<SYMBOL_TABLE_SIZE; i++) symbols_table[i].name = NULL;
}

void symbol_create(struct symbol * s, char * name, char * value, int s_type) {
    int nl = strlen(name);
    int vl = strlen(value);
    if (s->name = malloc(nl)) {
        sprintf(s->name, "%s", name);
        if (s->value = malloc(vl)) {
            sprintf(s->value, "%s", value);
            s->symbol_type = s_type;
        }
        s->next = NULL;
    }
}

char * symbols_new(char * name, char * value, int s_type) {
    int h = djb2_hash(name) % SYMBOL_TABLE_SIZE;
    struct symbol * ptr = NULL;
    if (!(symbols_table[h].name)) {
        symbols_table[h].next = NULL;
        symbol_create(&(symbols_table[h]), name, value, s_type);
        return (symbols_table[h].value);
    }
    else {
        for (ptr = &(symbols_table[h]); ptr->next; ptr=ptr->next);
        ptr->next = malloc(sizeof(struct symbol));
        if (ptr->next) {
            symbol_create(ptr->next, name, value, s_type);
            return (ptr->next->value);
        }
    }
    return NULL;
}

char * symbols_find(char * name, int s_type) {
    int h = djb2_hash(name) % SYMBOL_TABLE_SIZE;
    struct symbol * ptr;
    for (ptr = &(symbols_table[h]); ptr && ptr->name && (strcmp(name, ptr->name) || (ptr->symbol_type != s_type)); ptr = ptr->next);
    if (ptr) return ptr->value;
    else return NULL;
}

void symbols_update(char * name, char * value, int s_type) {
    int vl = strlen(value);
    int h = djb2_hash(name) % SYMBOL_TABLE_SIZE;
    struct symbol * ptr;
    for (ptr = &(symbols_table[h]); ptr && ptr->name && (strcmp(name, ptr->name) || (ptr->symbol_type != s_type)); ptr = ptr->next);
    if (ptr) {
        if (ptr->value) free(ptr->value);
        ptr->value = malloc(vl);
        sprintf(ptr->value, "%s", value);
    }
    else symbols_new(name, value, s_type);
}


char * symbols_find_any_type(char * name) {
    int h = djb2_hash(name) % SYMBOL_TABLE_SIZE;
    struct symbol * ptr;
    for (ptr = &(symbols_table[h]); ptr && ptr->name && strcmp(name, ptr->name); ptr = ptr->next);
    if (ptr) return ptr->value;
    else return NULL;
}

void symbols_done() {
    struct symbol * ptr;
    struct symbol * next;
    for (int i=0; i<SYMBOL_TABLE_SIZE; i++) {
        if (symbols_table[i].name) {
            free(symbols_table[i].name);
            if (symbols_table[i].value) free(symbols_table[i].value);
            for (ptr=symbols_table[i].next; ptr;) {
                next = ptr->next;
                free(ptr);
                ptr = next;
            }
        }
    }
    
}
                   
//#define SYMBOL_TEST

#ifdef SYMBOL_TEST

void main () {
    char * v;
    symbols_init();
    v = symbols_new("myname", "myvalue1", 1); printf("added %s\n", v);
    v = symbols_new("myname", "myvalue2", 2); printf("added %s\n", v);
    if (v=symbols_find("myname", 1)) printf("check: found %s\n", v);
    if (v=symbols_find("myname", 2)) printf("check: found %s\n", v);
    if (!(v=symbols_find("myname", 3))) printf("check: did't find wrong type\n");
    if (!(v=symbols_find("unknown", 3))) printf("check: did't find missing name\n");
    if (v=symbols_find_any_type("myname")) printf("check: found %s\n", v);
    if (!(v=symbols_find_any_type("unknown"))) printf("check: did't find missing name by any type\n");
    symbols_update("myname", "newvalue1", 1);
    if (v=symbols_find("myname", 1)) printf("check: found %s\n", v);
    symbols_update("newsymbol", "newvalue3", 3);
    if (v=symbols_find("newsymbol", 3)) printf("check: found %s\n", v);
    symbols_done();
    
    
}

#endif