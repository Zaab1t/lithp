/* builtins.c
 * ----------
 *
 *   All the functions defined in C instead of lithp.
 *
 *   All builtins have the same signature, so they can be passed around
 *   arbitrarily: `lval * f (lenv *e, lval *a)`.
 *
 */


#include "mpc.h"

#include "lithp.h"


#define ASSERT (args, cond, fmt, ...) \
    if (!(cond)) { \
        lval *err = lval_err (fmt, ##__VA_ARGS__); \
        lval_clean_up (args); \
        return err; \
    }

#define ASSERT_ARG_COUNT (func, args, num) \
    ASSERT (args, args->count == num, \
            "'%s' expected %i arguments, but got %i.", \
            func, num, args->count \
    )

#define ASSERT_TYPE (func, args, index, expect) \
    ASSERT (args, args->cell[index]->type == expect, \
            "'%s' expected type %s at %i, but got %s.", \
            func, ltype_to_name (expect), index, \
            ltype_to_name (args->cell[index]->type) \
    )

#define ASSERT_NOT_EMPTY (func, args, index) \
    ASSERT (args, args->cell[index]->count != 0, \
            "'%s' can't work on empty lists", func \
    )


lval *
builtin_list (lenv *e, lval *a)
{
    a->type = LVAL_QEXPR;
    return a;
}


lval *
builtin_eval (lenv *e, lval *a)
{
    ASSERT_ARG_COUNT ("eval", a, 1);
    ASSERT_TYPE ("eval", a, 0, LVAL_QEXPR);

    lval *x = lval_take (a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval (e, x);
}


lval *
builtin_head (lenv *e, lval *a)
{
    ASSERT_ARG_COUNT ("head", a, 1);
    ASSERT_TYPE ("head", a, 0, LVAL_QEXPR);
    ASSERT_NOT_EMPTY ("head", a, 0);

    lval *v = lval_take (a, 0);
    while (v->count > 1) lval_clean_up (lval_pop (v, 1));
    return v;
}


lval *
builtin_tail (lenv *e, lval *a)
{
    ASSERT_ARG_COUNT ("tail", a, 1);
    ASSERT_TYPE ("tail", a, 0, LVAL_QEXPR);
    ASSERT_NOT_EMPTY ("tail", a, 0);

    lval *v = lval_take (a, 0);
    lval_clean_up (lval_pop (v, 0));
    return v;
}

lval *
builtin_join (lenv *e, lval *a)
{
    for (int i = 0; i < a->count; i++)
        ASSERT_TYPE ("join", a, i, LVAL_QEXPR);

    lval *x = lval_pop (a, 0);
    while (a->count)
        x = lval_join (x, lval_pop (a, 0));

    lval_clean_up (a);
    return x;
}


lval *
builtin_var (lenv *e, lval *a, char *func)
{
    ASSERT_TYPE (func, a, 0, LVAL_QEXPR);

    lval *syms = a->cell[0];
    /* ensure valid symbols */
    for (int i = 0; i < syms->count; i++)
        ASSERT (a, syms->cell[i]->type == LVAL_SYM,
                "'%s' cannot define non-symbol", func);

    ASSERT (a, (syms->count == a->count-1),
            "'%s' can only define same number of values and symbols", func);

    for (int i = 0; i < syms->count; i++)
    {
        if (strcmp (func, "def") == 0)
          {
            lenv_put_global (e, syms->cell[i], a->cell[i+1]);
        }
        else if (strcmp (func, "=") == 0)
          {
            lenv_put (e, syms->cell[i], a->cell[i+1]);
        }
    }

    lval_clean_up (a);
    return lval_sexpr ();
}


lval *
builtin_def (lenv *e, lval *a)
{
    return builtin_var (e, a, "def");
}


lval *
builtin_put (lenv *e, lval *a)
{
    return builtin_var (e, a, "=");
}


lval *
builtin_lambda (lenv *e, lval *a)
{
    ASSERT_ARG_COUNT ("\\", a, 2);
    ASSERT_TYPE ("\\", a, 0, LVAL_QEXPR);
    ASSERT_TYPE ("\\", a, 1, LVAL_QEXPR);

    /* Verify that first Q-Expression only contains symbols */
    for (int i = 0; i < a->cell[0]->count; i++)
        ASSERT (a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, expected %s.",
                ltype_to_name (a->cell[0]->cell[i]->type),
                ltype_to_name (LVAL_SYM)
        );

    lval *formals = lval_pop (a, 0);
    lval *body = lval_pop (a, 0);
    lval_clean_up (a);

    return lval_lambda (formals, body);
}


lval *
builtin_print (lenv *e, lval *a)
{
    for (int i = 0; i < a->count; i++)
      {
        lval_print (a->cell[i]);
        putchar (' ');
    }
    putchar ('\n');
    lval_clean_up (a);
    return lval_sexpr ();
}


lval *
builtin_error (lenv *e, lval *a)
{
    ASSERT_ARG_COUNT ("error", a, 1);
    ASSERT_TYPE ("error", a, 0, LVAL_STR);

    lval *error = lval_err (a->cell[0]->str);

    lval_clean_up (a);
    return error;
}


lval *
builtin_op (lenv *e, lval *a, char *op)
{
    /* Ensure that all args are numbers */
    for (int i = 0; i < a->count; i++)
        if (a->cell[i]->type != LVAL_NUM)
          {
            lval_clean_up (a);
            return lval_err ("Can only operate on numbers!");
        }

    lval *x = lval_pop (a, 0);

    if ((strcmp (op, "-") == 0) && a->count == 0)
        x->number = -x->number;

    while (a->count > 0) {
        lval *y = lval_pop (a, 0);

        if (strcmp (op, "+") == 0)
            x->number += y->number;
        if (strcmp (op, "-") == 0)
            x->number -= y->number;
        if (strcmp (op, "*") == 0)
            x->number *= y->number;
        if (strcmp (op, "/") == 0) {
            if (y->number == 0) {
                lval_clean_up (x);
                lval_clean_up (y);
                x = lval_err ("Division by Zero!");
                break;
            }
            x->number /= y->number;
        }

        lval_clean_up (y);
    }

    lval_clean_up (a);
    return x;
}


lval *
builtin_add (lenv *e, lval *a)
{
    return builtin_op (e, a, "+");
}


lval *
builtin_sub (lenv *e, lval *a)
{
    return builtin_op (e, a, "-");
}


lval *
builtin_mul (lenv *e, lval *a)
{
    return builtin_op (e, a, "*");
}


lval *
builtin_div (lenv *e, lval *a)
{
    return builtin_op (e, a, "/");
}


lval *
builtin_if (lenv *e, lval *a)
{
    ASSERT_ARG_COUNT ("if", a, 3);
    ASSERT_TYPE ("if", a, 0, LVAL_NUM);
    ASSERT_TYPE ("if", a, 1, LVAL_QEXPR);
    ASSERT_TYPE ("if", a, 2, LVAL_QEXPR);

    lval *x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if (a->cell[0]->number)
      {
        x = lval_eval (e, lval_pop (a, 1));
    } else
      {
        x = lval_eval (e, lval_pop (a, 2));
    }

    lval_clean_up (a);
    return x;
}


lval *
builtin_ord (lenv *e, lval *a, char *op)
{
    ASSERT_ARG_COUNT (op, a, 2);
    ASSERT_TYPE (op, a, 0, LVAL_NUM);
    ASSERT_TYPE (op, a, 1, LVAL_NUM);

    int r;
    if (strcmp (op, ">") == 0)
      {
        r = (a->cell[0]->number > a->cell[1]->number);
    }
    else if (strcmp (op, "<") == 0)
      {
        r = (a->cell[0]->number < a->cell[1]->number);
    }
    else if (strcmp (op, ">=") == 0)
      {
        r = (a->cell[0]->number >= a->cell[1]->number);
    }
    else  /* (strcmp (op, "<=") == 0) */
      {
        r = (a->cell[0]->number <= a->cell[1]->number);
    }

    lval_clean_up (a);
    return lval_num (r);
}


lval *
builtin_cmp (lenv *e, lval *a, char *op)
{
    ASSERT_ARG_COUNT (op, a, 2);

    int r = lval_eq (a->cell[0], a->cell[1]);
    if (strcmp (op, "!=") == 0)
        r = !r;

    lval_clean_up (a);
    return lval_num (r);
}


lval *
builtin_gt (lenv *e, lval *a)
{
    return builtin_ord (e, a, ">");
}


lval *
builtin_lt (lenv *e, lval *a)
{
    return builtin_ord (e, a, "<");
}


lval *
builtin_ge (lenv *e, lval *a)
{
    return builtin_ord (e, a, ">=");
}


lval *
builtin_le (lenv *e, lval *a)
{
    return builtin_ord (e, a, "<=");
}


lval *
builtin_eq (lenv *e, lval *a)
{
    return builtin_cmp (e, a, "==");
}


lval *
builtin_ne (lenv *e, lval *a)
{
    return builtin_cmp (e, a, "!=");
}


lval *
builtin_import (lenv *e, lval *a)
{
    ASSERT_ARG_COUNT ("import", a, 1);
    ASSERT_TYPE ("import", a, 0, LVAL_STR);

    mpc_result_t r;
    if (mpc_parse_contents (a->cell[0]->str, Program, &r)) {
        lval *expr = lval_read (r.output);
        mpc_ast_delete (r.output);

        while (expr->count) {
            lval *x = lval_eval (e, lval_pop (expr, 0));
            if (x->type == LVAL_ERR)
                lval_println (x);
            lval_clean_up (x);
        }

        lval_clean_up (expr);
        lval_clean_up (a);

        return lval_sexpr ();
    }
    char *error_msg = mpc_err_string (r.error);
    mpc_err_delete (r.error);

    lval *err = lval_err ("Could not load Library %s", error_msg);
    free (error_msg);
    lval_clean_up (a);
    return err;
}
