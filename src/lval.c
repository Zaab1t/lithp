#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"

#include "lithp.h"


lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    /* edit here for longer error messages */
    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);

    v->err = realloc(v->err, strlen(v->err) + 1);
    va_end(va);
    return v;
}


lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}


lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}


lval* lval_str(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}


lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}


lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}


lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


void lval_clean_up(lval* v) {
    switch (v->type) {
        case LVAL_NUM:
            break;

        case LVAL_FUN:
            if (!v->builtin) {
                lenv_clean_up(v->env);
                lval_clean_up(v->formals);
                lval_clean_up(v->body);
            }
            break;

        case LVAL_ERR:
            free(v->err);
            break;

        case LVAL_STR:
            free(v->str);
            break;

        case LVAL_SYM:
            free(v->sym);
            break;

        case LVAL_SEXPR:
        case LVAL_QEXPR:
            for (int i = 0; i < v->count; i++)
                lval_clean_up(v->cell[i]);
            free(v->cell);
    }
    free(v);
}


void lval_print_str(lval* v) {
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("'%s'", escaped);
    free(escaped);
}


void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != (v->count-1))
            putchar(' ');
    }
    putchar(close);
}


void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM:
            printf("%li", v->num); break;
        case LVAL_ERR:
            printf("Error: %s", v->err); break;
        case LVAL_SYM:
            printf("%s", v->sym); break;
        case LVAL_STR:
            lval_print_str(v); break;
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(\\ ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}'); break;
    }
}


void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}


int lval_eq(lval* x, lval* y) {
    if (x->type != y->type)
        return 0;

    switch (x->type) {
        case LVAL_NUM:
            return (x->num == y->num);

        case LVAL_ERR:
            return (strcmp(x->err, y->err) == 0);

        case LVAL_SYM:
            return (strcmp(x->sym, y->sym) == 0);

        case LVAL_STR:
            return (strcmp(x->str, y->str) == 0);

        case LVAL_FUN:
            if (x->builtin || y->builtin)
                return (x->builtin == y->builtin);
            return (lval_eq(x->formals, y->formals) &&
                    lval_eq(x->body, y->body));

        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count)
                return 0;
            for (int i = 0; i < x->count; i++)
                if (!lval_eq(x->cell[i], y->cell[i]))
                    return 0;
            return 1;

    }

    return 0;
}

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;

        case LVAL_NUM:
            x->num = v->num;
            break;

        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;

        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;

        case LVAL_FUN:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;

        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++)
                x->cell[i] = lval_copy(v->cell[i]);
            break;
    }

    return x;
}
lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1],
            sizeof(lval*) * (v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}


lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}


lval* lval_join(lval* x, lval* y) {
    while (y->count)
        x = lval_add(x, lval_pop(y, 0));

    lval_clean_up(y);
    return x;
}


lval* lval_call(lenv* e, lval* f, lval* a) {
    if (f->builtin)
        return f->builtin(e, a);

    /* partial evaluation */
    while (a->count) {
        if (f->formals->count == 0) {
            lval_clean_up(a);
            return lval_err("explicit error msg");
        }

        lval* sym = lval_pop(f->formals, 0);

        if (strcmp(sym->sym, ":") == 0) {
            if (f->formals->count != 1) {
                lval_clean_up(a);
                return lval_err("Invalid function format. ':' should be"
                        " followed by a single symbol.");
            }

            /* bind next formal to remaining arguments */
            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_clean_up(sym);
            lval_clean_up(nsym);
            break;
        }

        lval* val = lval_pop(a, 0);

        lenv_put(f->env, sym, val);

        lval_clean_up(sym);
        lval_clean_up(val);
    }

    lval_clean_up(a);  /* arg list has been bound */

    if (f->formals->count == 0) {
        f->env->parent = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(),
                    lval_copy(f->body)));
    }
    
    return lval_copy(f);
}


lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_clean_up(v);
    return x;
}


lval* lval_eval_sexpr(lenv* e, lval* v) {
    if (v->count == 0) return v;  /* empty expression */

    for (int i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(e, v->cell[i]);

    for (int i = 0; i < v->count; i++)
        if (v->cell[i]->type == LVAL_ERR)
            return lval_take(v, i);

    if (v->count == 1) return lval_take(v, 0);

    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval_clean_up(f);
        lval_clean_up(v);
        return lval_err("First element is not a function!");
    }

    lval* result = lval_call(e, f, v);
    lval_clean_up(f);
    return result;
}


lval* lval_eval(lenv* e, lval* v) {
    if (v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_clean_up(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) return lval_eval_sexpr(e, v);
    return v;
}
