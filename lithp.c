#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


#define ASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_clean_up(args); \
        return err; \
    }

#define ASSERT_ARG_COUNT(func, args, num) \
    ASSERT(args, args->count == num, \
            "'%s' expected %i arguments, but got %i.", \
            func, num, args->count \
    )

#define ASSERT_TYPE(func, args, index, expect) \
    ASSERT(args, args->cell[index]->type == expect, \
            "'%s' expected type %s at %i, but got %s.", \
            func, ltype_to_name(expect), index, \
            ltype_to_name(args->cell[index]->type) \
    )

#define ASSERT_NOT_EMPTY(func, args, index) \
    ASSERT(args, args->cell[index]->count != 0, \
            "'%s' can't work on empty lists", func \
    )


struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


enum {
    LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR,
    LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR,
};


char* ltype_to_name(int type) {
    switch (type) {
        case LVAL_ERR:
            return "Error";
        case LVAL_NUM:
            return "Number";
        case LVAL_SYM:
            return "Symbol";
        case LVAL_STR:
            return "String";
        case LVAL_FUN:
            return "Function";
        case LVAL_SEXPR:
            return "S-Expression";
        case LVAL_QEXPR:
            return "Q-Expression";
        default:
            return "Not my type";
    }
}


struct lenv {
    lenv* parent;  /* top parent is NULL */
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
    char* str;

    /* function */
    lbuiltin builtin;  /* NULL if user defined */
    lenv* env;
    lval* formals;
    lval* body;

    /* expression */
    int count;
    lval** cell;
};


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


lenv* lenv_new(void);


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


void lenv_clean_up(lenv* e);


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


void lval_print(lval* v);


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


lval* lval_copy(lval* v);


lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->parent = e->parent;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);

    for (int i = 0; i < n->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }

    return n;
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


lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->parent = NULL;
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

    if (e->parent)
        return lenv_get(e->parent, k);

    return lval_err("Unbound symbol '%s'!", k->sym);
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


void lenv_put_global(lenv* e, lval* k, lval* v) {
    while (e->parent)
        e = e->parent;

    lenv_put(e, k, v);
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
lval* builtin_lambda(lenv* e, lval* v);
lval* builtin_put(lenv* e, lval* v);

lval* builtin_add(lenv* e, lval* v);
lval* builtin_sub(lenv* e, lval* v);
lval* builtin_mul(lenv* e, lval* v);
lval* builtin_div(lenv* e, lval* v);

lval* builtin_if(lenv* e, lval* v);
lval* builtin_eq(lenv* e, lval* v);
lval* builtin_ne(lenv* e, lval* v);
lval* builtin_gt(lenv* e, lval* v);
lval* builtin_lt(lenv* e, lval* v);
lval* builtin_ge(lenv* e, lval* v);
lval* builtin_le(lenv* e, lval* v);


void lenv_add_builtins(lenv* e) {
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);
    lenv_add_builtin(e, "\\", builtin_lambda);

    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<", builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);
}


lval* lval_take(lval* v, int i);
lval* lval_eval(lenv* e, lval* v);


lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}


lval* builtin_eval(lenv* e, lval* a) {
    ASSERT_ARG_COUNT("eval", a, 1);
    ASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}


lval* builtin_head(lenv* e, lval* a) {
    ASSERT_ARG_COUNT("head", a, 1);
    ASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    ASSERT_NOT_EMPTY("head", a, 0);

    lval* v = lval_take(a, 0);
    while (v->count > 1) lval_clean_up(lval_pop(v, 1));
    return v;
}


lval* builtin_tail(lenv* e, lval* a) {
    ASSERT_ARG_COUNT("tail", a, 1);
    ASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    ASSERT_NOT_EMPTY("tail", a, 0);

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
        ASSERT_TYPE("join", a, i, LVAL_QEXPR);

    lval* x = lval_pop(a, 0);
    while (a->count)
        x = lval_join(x, lval_pop(a, 0));

    lval_clean_up(a);
    return x;
}


lval* builtin_var(lenv* e, lval* a, char* func) {
    ASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    lval* syms = a->cell[0];
    /* ensure valid symbols */
    for (int i = 0; i < syms->count; i++)
        ASSERT(a, syms->cell[i]->type == LVAL_SYM,
                "'%s' cannot define non-symbol", func);

    ASSERT(a, (syms->count == a->count-1),
            "'%s' can only define same number of values and symbols", func);

    for (int i = 0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            lenv_put_global(e, syms->cell[i], a->cell[i+1]);
        } else if (strcmp(func, "=") == 0) {
            lenv_put(e, syms->cell[i], a->cell[i+1]);
        }
    }

    lval_clean_up(a);
    return lval_sexpr();
}


lval* builtin_def(lenv* e, lval* a) {
    return builtin_var(e, a, "def");
}


lval* builtin_put(lenv* e, lval* a) {
    return builtin_var(e, a, "=");
}


lval* builtin_lambda(lenv* e, lval* a) {
    ASSERT_ARG_COUNT("\\", a, 2);
    ASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    ASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    /* Verify that first Q-Expression only contains symbols */
    for (int i = 0; i < a->cell[0]->count; i++)
        ASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, expected %s.",
                ltype_to_name(a->cell[0]->cell[i]->type),
                ltype_to_name(LVAL_SYM)
        );

    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_clean_up(a);

    return lval_lambda(formals, body);
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


lval* builtin_if(lenv* e, lval* a) {
    ASSERT_ARG_COUNT("if", a, 3);
    ASSERT_TYPE("if", a, 0, LVAL_NUM);
    ASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    ASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if (a->cell[0]->num) {
        x = lval_eval(e, lval_pop(a, 1));
    } else {
        x = lval_eval(e, lval_pop(a, 2));
    }

    lval_clean_up(a);
    return x;
}


lval* builtin_ord(lenv* e, lval* a, char* op) {
    ASSERT_ARG_COUNT(op, a, 2);
    ASSERT_TYPE(op, a, 0, LVAL_NUM);
    ASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if (strcmp(op, ">") == 0) {
        r = (a->cell[0]->num > a->cell[1]->num);
    } else if (strcmp(op, "<") == 0) {
        r = (a->cell[0]->num < a->cell[1]->num);
    } else if (strcmp(op, ">=") == 0) {
        r = (a->cell[0]->num >= a->cell[1]->num);
    } else {  /* (strcmp(op, "<=") == 0) */
        r = (a->cell[0]->num <= a->cell[1]->num);
    }

    lval_clean_up(a);
    return lval_num(r);
}


lval* builtin_cmp(lenv* e, lval* a, char* op) {
    ASSERT_ARG_COUNT(op, a, 2);

    int r = lval_eq(a->cell[0], a->cell[1]);
    if (strcmp(op, "!=") == 0)
        r = !r;

    lval_clean_up(a);
    return lval_num(r);
}


lval* builtin_gt(lenv* e, lval* a) {
    return builtin_ord(e, a, ">");
}


lval* builtin_lt(lenv* e, lval* a) {
    return builtin_ord(e, a, "<");
}


lval* builtin_ge(lenv* e, lval* a) {
    return builtin_ord(e, a, ">=");
}


lval* builtin_le(lenv* e, lval* a) {
    return builtin_ord(e, a, "<=");
}


lval* builtin_eq(lenv* e, lval* a) {
    return builtin_cmp(e, a, "==");
}


lval* builtin_ne(lenv* e, lval* a) {
    return builtin_cmp(e, a, "!=");
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


lval* lval_read(mpc_ast_t* t);


mpc_parser_t* Number;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Symbol;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Program;


lval* builtin_import(lenv* e, lval* a) {
    ASSERT_ARG_COUNT("import", a, 1);
    ASSERT_TYPE("import", a, 0, LVAL_STR);

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Program, &r)) {
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while (expr->count) {
            lval* x = lval_eval(e, lval_pop(expr, 0));
            if (x->type == LVAL_ERR)
                lval_println(x);
            lval_clean_up(x);
        }

        lval_clean_up(expr);
        lval_clean_up(a);

        return lval_sexpr();
    }
    char* error_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);

    lval* err = lval_err("Could not load Library %s", error_msg);
    free(error_msg);
    lval_clean_up(a);
    return err;
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


lval* lval_read_num(mpc_ast_t* node) {
    errno = 0;
    long x = strtol(node->contents, NULL, 10);
    if (errno == ERANGE) return lval_err("Invalid number");
    return lval_num(x);
}


lval* lval_read_str(mpc_ast_t* node) {
    /* unescape and remove quotation chars */
    node->contents[strlen(node->contents) - 1] = '\0';
    char* unescaped = malloc(strlen(node->contents+1) + 1);
    strcpy(unescaped, node->contents+1);
    unescaped = mpcf_unescape(unescaped);

    lval* str = lval_str(unescaped);
    free(unescaped);
    return str;
}


lval* lval_read(mpc_ast_t* node) {
    if (strstr(node->tag, "number")) return lval_read_num(node);
    if (strstr(node->tag, "string")) return lval_read_str(node);

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
        if (strstr(node->children[i]->tag, "comment")) continue;
        x = lval_add(x, lval_read(node->children[i]));
    }

    return x;
}


int main(int argc, char** argv) {
    Number = mpc_new("number");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Symbol = mpc_new("symbol");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Program = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT,
        "\
            number   : /-?[0-9]+/ ;\
            string   : /'(\\\\.|[^'])*'/ ;\
            comment  : /;[^\\r\\n]*/ ;\
            symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!:,&]+/ ;\
            sexpr    : '(' <expr>* ')' ;\
            qexpr    : '{' <expr>* '}' ;\
            expr     : <number> | <string> | <comment> \
                     | <symbol> | <sexpr> | <qexpr> ;\
            program  : /^/ <expr>* /$/ ;\
        ",
        Number, String, Comment, Symbol, Sexpr, Qexpr, Expr, Program
    );

    puts("Lithp 0.0.11");
    puts("Preth Ctrl+c to Exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
            lval* x = builtin_import(e, args);
            if (x->type == LVAL_ERR)
                lval_println(x);
            lval_clean_up(x);
        }
    } else {  /* REPL */
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
    }

    lenv_clean_up(e);
    mpc_cleanup(8,
            Number, String, Comment, Symbol,
            Sexpr, Qexpr, Expr, Program
    );
    return 0;
}
