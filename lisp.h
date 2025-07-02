#include <stdio.h>

int ht_init(int size);

void init_env();

struct object *load_file(struct object *args);

struct object *cons(struct object *x, struct object *y);

struct object *make_symbol(char *s);

static struct object *NIL;

#define null(x) ((x) == NULL || (x) == NIL)

void print_exp(char *str, struct object *e);

struct object *eval(struct object *exp, struct object *env);

struct object *read_exp(FILE *in);

static struct object *ENV;