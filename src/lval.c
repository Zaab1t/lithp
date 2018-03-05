/* lval.c
 * ------
 *
 *   `lval` and related functions such as con- and destructors.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"

#include "lithp.h"


/*****************************************************************************/
/*                       CONSTRUCTORS AND DESTRUCTOR                         */
/*****************************************************************************/

lval *
lval_err(char *fmt, ...)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    /* edit here for longer error messages */
    v->error_msg = malloc(512);
    vsnprintf(v->error_msg, 511, fmt, va);

    v->error_msg = realloc(v->error_msg, strlen(v->error_msg) + 1);
    va_end(va);
    return v;
}


lval *
lval_num(intmax_t x)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->number = x;
    return v;
}


lval *
lval_sym(char *s)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->symbol = malloc(strlen(s) + 1);
    strcpy(v->symbol, s);
    return v;
}


lval *
lval_str(char *s)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}


lval *
lval_fun(lbuiltin func, char *doc)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    v->docstring = malloc(strlen(doc) + 1);
    strcpy(v->docstring, doc);
    return v;
}


lval *
lval_lambda(lval *formals, lval *body)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}


lval *
lval_sexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


lval *
lval_qexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


void
lval_cleanup(lval *v)
{
    switch (v->type) {
    case LVAL_NUM:
        break;

    case LVAL_FUN:
        if (!v->builtin) {
            lenv_clean_up(v->env);
            lval_cleanup(v->formals);
            lval_cleanup(v->body);
        }
        break;

    case LVAL_ERR:
        free(v->error_msg);
        break;

    case LVAL_STR:
        free(v->str);
        break;

    case LVAL_SYM:
        free(v->symbol);
        break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
        for (uint64_t i = 0; i < v->count; i++)
            lval_cleanup(v->cell[i]);
        free(v->cell);
    }
    free(v);
}


/*****************************************************************************/
/*                                UTILITIES                                  */
/*****************************************************************************/

lval *
lval_copy(lval *v)
{
    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
    case LVAL_ERR:
        x->error_msg = malloc(strlen(v->error_msg) + 1);
        strcpy(x->error_msg, v->error_msg);
        break;

    case LVAL_NUM:
        x->number = v->number;
        break;

    case LVAL_SYM:
        x->symbol = malloc(strlen(v->symbol) + 1);
        strcpy(x->symbol, v->symbol);
        break;

    case LVAL_STR:
        x->str = malloc(strlen(v->str) + 1);
        strcpy(x->str, v->str);
        break;

    case LVAL_FUN:
        if (v->builtin) {
            x->builtin = v->builtin;
            x->docstring = malloc(strlen(v->docstring) + 1);
            strcpy(x->docstring, v->docstring);
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
        x->cell = malloc(sizeof(lval *) * x->count);
        for (uint64_t i = 0; i < x->count; i++)
            x->cell[i] = lval_copy(v->cell[i]);
        break;
    }

    return x;
}


/*
 * Function:  ltype_to_name
 * ------------------------
 *   Return a string representation for a given type from the lval
 *   enumeration.
 */
char *
ltype_to_name(int type)
{
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


void
lval_print_str(lval *v)
{
    char *escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("%s", escaped);
    free(escaped);
}


void
lval_print_expr(lval *v, char open, char close)
{
    if (v->count) {
        putchar(open);
        for (uint64_t i = 0; i < v->count; i++) {
            lval_print(v->cell[i]);
            if (i != (v->count - 1))
                putchar(' ');
        }
        putchar(close);
    }
}


void
lval_print(lval *v)
{
    switch (v->type) {
    case LVAL_NUM:
        printf("%li", v->number);
        break;
    case LVAL_ERR:
        printf("Error: %s", v->error_msg);
        break;
    case LVAL_SYM:
        printf("%s", v->symbol);
        break;
    case LVAL_STR:
        lval_print_str(v);
        break;
    case LVAL_FUN:
        if (v->builtin) {
            printf("<builtin>: \33[34m%s\033[0m", v->docstring);
        } else {
            printf("(\\ ");
            lval_print(v->formals);
            putchar(' ');
            lval_print(v->body);
            putchar(')');
        }
        break;
    case LVAL_SEXPR:
        lval_print_expr(v, '(', ')');
        break;
    case LVAL_QEXPR:
        lval_print_expr(v, '{', '}');
        break;
    }
}


void
lval_println(lval *v)
{
    lval_print(v);
    if (!((v->type == LVAL_SEXPR || v->type == LVAL_SEXPR) && v->count == 0))
        putchar('\n');
}


/*****************************************************************************/
/*                                 OPERATIONS                                */
/*****************************************************************************/

/*
 * Function:  lval_equal
 * ---------------------
 *   Compare _any_ two `lval`s and return 1 if they can be considered
 *   equal, otherwise 0.
 *
 *   TODO: return bool?
 */
int
lval_eq(lval *x, lval *y)
{
    if (x->type != y->type)
        return 0;

    switch (x->type) {
    case LVAL_NUM:
        return (x->number == y->number);

    case LVAL_ERR:
        return (strcmp(x->error_msg, y->error_msg) == 0);

    case LVAL_SYM:
        return (strcmp(x->symbol, y->symbol) == 0);

    case LVAL_STR:
        return (strcmp(x->str, y->str) == 0);

    case LVAL_FUN:
        if (x->builtin || y->builtin)
            return (x->builtin == y->builtin);
        return (lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body));

    case LVAL_QEXPR:
    case LVAL_SEXPR:
        if (x->count != y->count)
            return 0;
        for (uint64_t i = 0; i < x->count; i++)
            if (!lval_eq(x->cell[i], y->cell[i]))
                return 0;
        return 1;
    }

    return 0;
}


/*
 * Function:  lval_pop
 * -------------------
 *   Remove and return the *i*'th element in `v->cell`.
 */
lval *
lval_pop(lval *v, uint64_t i)
{
    lval *x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    return x;
}


/*
 * Function  lval_add
 * ------------------
 *   Append *x* to *v*.
 */
lval *
lval_add(lval *v, lval *x)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}


/*
 * Function:  lval_join
 * --------------------
 *   Merge two `Q-expression`s into one.
 */
lval *
lval_join(lval *x, lval *y)
{
    while (y->count)
        x = lval_add(x, lval_pop(y, 0));

    lval_cleanup(y);
    return x;
}


/*
 * Function: lval_call
 * -------------------
 *   Return the evaluation of a lisp function (`LVAL_FUN`).
 *
 *   If there are too few arguments, the function is just partially
 *   evaluated.
 */
lval *
lval_call(lenv *e, lval *f, lval *a)
{
    if (f->builtin)
        return f->builtin(e, a);

    /* partial evaluation */
    while (a->count) {
        if (f->formals->count == 0) {
            lval_cleanup(a);
            return lval_err("explicit error msg");
        }

        lval *sym = lval_pop(f->formals, 0);

        if (strcmp(sym->symbol, ":") == 0) {
            if (f->formals->count != 1) {
                lval_cleanup(a);
                return lval_err(
                    "Invalid function format. ':' should be"
                    " followed by a single symbol.");
            }

            /* bind next formal to remaining arguments */
            lval *nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_cleanup(sym);
            lval_cleanup(nsym);
            break;
        }

        lval *val = lval_pop(a, 0);

        lenv_put(f->env, sym, val);

        lval_cleanup(sym);
        lval_cleanup(val);
    }

    lval_cleanup(a); /* arg list has been bound */

    if (f->formals->count == 0) {
        f->env->parent = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    }

    return lval_copy(f);
}


/*
 * Function:  lval_take
 * --------------------
 *   Return the *i*th element of *v*, and throw *v* into orbit.
 */
lval *
lval_take(lval *v, uint64_t i)
{
    lval *x = lval_pop(v, i);
    lval_cleanup(v);
    return x;
}


/*****************************************************************************/
/*                                 EVALUATION                                */
/*****************************************************************************/

lval *
lval_eval_sexpr(lenv *e, lval *v)
{
    if (v->count == 0)
        return v; /* empty expression */

    for (uint64_t i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(e, v->cell[i]);

    for (uint64_t i = 0; i < v->count; i++)
        if (v->cell[i]->type == LVAL_ERR)
            return lval_take(v, i);

    if (v->count == 1)
        return lval_take(v, 0);

    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval_cleanup(f);
        lval_cleanup(v);
        return lval_err("First element is not a function!");
    }

    lval *result = lval_call(e, f, v);
    lval_cleanup(f);
    return result;
}


/*
 * Function  lval_eval
 * -------------------
 *   Evaluate a lisp value.
 */
lval *
lval_eval(lenv *e, lval *v)
{
    if (v->type == LVAL_SYM) {
        lval *x = lenv_get(e, v);
        lval_cleanup(v);
        return x;
    }
    if (v->type == LVAL_SEXPR)
        return lval_eval_sexpr(e, v);
    return v;
}
