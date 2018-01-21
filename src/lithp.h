#include "mpc.h"


struct lenv;
struct lval;
typedef struct lval lval;
typedef struct lenv lenv;

/* function pointer */
typedef lval*(*lbuiltin)(lenv*, lval*);


/* Type:  lval
 * -----------
 *   This is the most important type in the project. It is used to
 *   represent any value in lithp and is essentially used to
 *   achieve duck typing.
*/
struct lval
{
    int type;  /* from the lval enum */

    long number;
    char *error_msg;
    char *symbol;
    char *str;

    /* function */
    lbuiltin builtin;  /* NULL if user defined */
    lenv *env;
    lval *formals;
    lval *body;

    /* expression */
    int count;
    lval **cell;
};


struct lenv
{
    lenv *parent;  /* top parent is NULL */
    int count;
    char **syms;
    lval **vals;
};


enum {
    LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR,
    LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR,
};


lval *lval_err(char *fmt, ...);
lval *lval_num(long x);
lval *lval_sym(char *s);
lval *lval_str(char *s);
lval *lval_fun(lbuiltin func);
lval *lval_lambda(lval *formals, lval *body);
lval *lval_sexpr(void);
lval *lval_qexpr(void);

void lval_cleanup(lval *v);
int lval_eq(lval *x, lval *y);
lval *lval_copy(lval *v);
lval *lval_pop(lval *v, int i);
lval *lval_add(lval *v, lval *x);
lval *lval_join(lval *x, lval *y);
lval *lval_call(lenv *e, lval *f, lval *a);
lval *lval_take(lval *v, int i);
lval *lval_eval(lenv *e, lval *v);
lval *lval_eval_sexpr(lenv *e, lval *v);

void lval_print(lval *v);
void lval_println(lval *v);
void lval_expr_print(lval *v, char open, char close);
char *ltype_to_name(int type);

lval *lval_read(mpc_ast_t *t);


lenv *lenv_new(void);
void lenv_put(lenv *e, lval *k, lval *v);
void lenv_put_global(lenv *e, lval *k, lval *v);
lval *lenv_get(lenv *e, lval *k);
void lenv_clean_up(lenv *e);
lenv *lenv_copy(lenv *e);


/* BUILTINS */
lval *builtin_list(lenv *e, lval *v);
lval *builtin_head(lenv *e, lval *v);
lval *builtin_tail(lenv *e, lval *v);
lval *builtin_eval(lenv *e, lval *v);
lval *builtin_join(lenv *e, lval *v);
lval *builtin_def(lenv *e, lval *v);
lval *builtin_lambda(lenv *e, lval *v);
lval *builtin_put(lenv *e, lval *v);

lval *builtin_add(lenv *e, lval *v);
lval *builtin_sub(lenv *e, lval *v);
lval *builtin_mul(lenv *e, lval *v);
lval *builtin_div(lenv *e, lval *v);

lval *builtin_if(lenv *e, lval *v);
lval *builtin_eq(lenv *e, lval *v);
lval *builtin_ne(lenv *e, lval *v);
lval *builtin_gt(lenv *e, lval *v);
lval *builtin_lt(lenv *e, lval *v);
lval *builtin_ge(lenv *e, lval *v);
lval *builtin_le(lenv *e, lval *v);

lval *builtin_import(lenv *e, lval *v);
lval *builtin_print(lenv *e, lval *v);
lval *builtin_error(lenv *e, lval *v);


/* PARSER */
mpc_parser_t *Number;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Symbol;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Program;
