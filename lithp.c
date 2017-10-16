#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


#define ASSERT(args, cond, err) \
    if (!(cond)) { lval_clean_up(args); return lval_err(err); }


struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


enum {
    LVAL_ERR, LVAL_NUM, LVAL_SYM,
    LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR,
};


struct lenv {
    int count;
    char** syms;
    lval** vals;
};


/* function pointer */
typedef lval*(*lbuiltin)(lenv*, lval*);


struct lval {
    int type;

    long num;
    char* err;
    char* sym;
    lbuiltin fun;

    int count;
    lval** cell;
};


lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
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


lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;
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
        case LVAL_FUN:
            break;

        case LVAL_ERR:
            free(v->err);
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


void lval_print(lval* v);


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
        case LVAL_FUN:
            printf("<function>"); break;
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

        case LVAL_FUN:
            x->fun = v->fun;
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


lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}


void lenv_clean_up(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_clean_up(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}


lval* lenv_get(lenv* e, lval* k) {
    for (int i = 0; i < e->count; i++)
        if (strcmp(e->syms[i], k->sym) == 0)
            return lval_copy(e->vals[i]);
    return lval_err("Unbound symbol!");
}


void lenv_put(lenv* e, lval* k, lval* v) {
    /* update existing value */
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_clean_up(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    e->count++;
    e->syms = realloc(e->syms, sizeof(char*) * e->count);
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);

    e->syms[e->count-1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count-1], k->sym);
    e->vals[e->count-1] = lval_copy(v);
}


void lenv_add_builtin(lenv* e, char* symbol, lbuiltin func) {
    lval* k = lval_sym(symbol);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_clean_up(k);
    lval_clean_up(v);
}


lval* builtin_list(lenv* e, lval* v);
lval* builtin_head(lenv* e, lval* v);
lval* builtin_tail(lenv* e, lval* v);
lval* builtin_eval(lenv* e, lval* v);
lval* builtin_join(lenv* e, lval* v);
lval* builtin_def(lenv* e, lval* v);

lval* builtin_add(lenv* e, lval* v);
lval* builtin_sub(lenv* e, lval* v);
lval* builtin_mul(lenv* e, lval* v);
lval* builtin_div(lenv* e, lval* v);


void lenv_add_builtins(lenv* e) {
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "def", builtin_def);

    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
}


lval* lval_take(lval* v, int i);
lval* lval_eval(lenv* e, lval* v);


lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}


lval* builtin_eval(lenv* e, lval* a) {
    ASSERT(a, a->count == 1, "'eval' passed too many arguments!");
    ASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "'eval' passed incorrect type!");

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}


lval* builtin_head(lenv* e, lval* a) {
    ASSERT(a, a->count == 1, "'head' passed too many arguments!");
    ASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "'head' passed incorrect type!");
    ASSERT(a, a->cell[0]->count != 0, "'head' passed {}!");

    lval* v = lval_take(a, 0);
    while (v->count > 1) lval_clean_up(lval_pop(v, 1));
    return v;
}


lval* builtin_tail(lenv* e, lval* a) {
    ASSERT(a, a->count == 1, "'tail' passed too many arguments!");
    ASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "'tail' passed incorrect type!");
    ASSERT(a, a->cell[0]->count != 0, "'tail' passed {}!");

    lval* v = lval_take(a, 0);
    lval_clean_up(lval_pop(v, 0));
    return v;
}

lval* lval_join(lval* x, lval* y) {
    while (y->count)
        x = lval_add(x, lval_pop(y, 0));

    lval_clean_up(y);
    return x;
}


lval* builtin_join(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++)
        ASSERT(a, a->cell[i]->type == LVAL_QEXPR,
                "'join' passed incorrect type!");

    lval* x = lval_pop(a, 0);
    while (a->count)
        x = lval_join(x, lval_pop(a, 0));

    lval_clean_up(a);
    return x;
}


lval* builtin_def(lenv* e, lval* a) {
    ASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "'def' passed incorrect type!");

    lval* syms = a->cell[0];

    /* ensure valid symbols */
    for (int i = 0; i < syms->count; i++)
        ASSERT(a, syms->cell[i]->type == LVAL_SYM,
                "'def' cannot define non-symbol");

    ASSERT(a, syms->count == a->count-1,
            "'def' can only define same number of values and symbols");

    for (int i = 0; i < syms->count; i++)
        lenv_put(e, syms->cell[i], a->cell[i+1]);

    lval_clean_up(a);
    return lval_sexpr();
}


lval* builtin_op(lenv* e, lval* a, char* op) {
    /* Ensure that all args are numbers */
    for (int i = 0; i < a->count; i++)
        if (a->cell[i]->type != LVAL_NUM) {
            lval_clean_up(a);
            return lval_err("Can only operate on numbers!");
        }

    lval* x = lval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0)
        x->num = -x->num;

    while (a->count > 0) {
        lval* y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0)
            x->num += y->num;
        if (strcmp(op, "-") == 0)
            x->num -= y->num;
        if (strcmp(op, "*") == 0)
            x->num *= y->num;
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_clean_up(x);
                lval_clean_up(y);
                x = lval_err("Division by Zero!");
                break;
            }
            x->num /= y->num;
        }

        lval_clean_up(y);
    }

    lval_clean_up(a);
    return x;
}


lval* builtin_add(lenv* e, lval* a) {
    return builtin_op(e, a, "+");
}


lval* builtin_sub(lenv* e, lval* a) {
    return builtin_op(e, a, "-");
}


lval* builtin_mul(lenv* e, lval* a) {
    return builtin_op(e, a, "*");
}


lval* builtin_div(lenv* e, lval* a) {
    return builtin_op(e, a, "/");
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

    lval* result = f->fun(e, v);
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


lval* lval_read_num(mpc_ast_t* node) {
    errno = 0;
    long x = strtol(node->contents, NULL, 10);
    if (errno == ERANGE) return lval_err("Invalid number");
    return lval_num(x);
}


lval* lval_read(mpc_ast_t* node) {
    if (strstr(node->tag, "number")) return lval_read_num(node);
    if (strstr(node->tag, "symbol")) return lval_sym(node->contents);

    lval* x = NULL;
    if (strcmp(node->tag, ">") == 0) x = lval_sexpr();
    if (strstr(node->tag, "sexpr")) x = lval_sexpr();
    if (strstr(node->tag, "qexpr")) x = lval_qexpr();

    for (int i = 0; i < node->children_num; i++) {
        if (strcmp(node->children[i]->contents, "(") == 0) continue;
        if (strcmp(node->children[i]->contents, ")") == 0) continue;
        if (strcmp(node->children[i]->contents, "}") == 0) continue;
        if (strcmp(node->children[i]->contents, "{") == 0) continue;
        if (strcmp(node->children[i]->tag, "regex") == 0) continue;
        x = lval_add(x, lval_read(node->children[i]));
    }

    return x;
}


int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Program = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT,
        "\
            number   : /-?[0-9]+/ ;\
            symbol   : /[a-zA-Z0-9_+\\-*\\/=<>!&]+/ ;\
            sexpr    : '(' <expr>* ')' ;\
            qexpr    : '{' <expr>* '}' ;\
            expr     : <number> | <symbol> | <sexpr> | <qexpr> ;\
            program  : /^/ <expr>* /$/ ;\
        ",
        Number, Symbol, Sexpr, Qexpr, Expr, Program
    );

    puts("Lithp 0.0.5");
    puts("Preth Ctrl+c to Exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    while (1) {
        char* input = readline("lithp> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Program, &r)) {
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_clean_up(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    lenv_clean_up(e);
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Program);
    return 0;
}
