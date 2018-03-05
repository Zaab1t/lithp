#include "mpc.h"


typedef struct lval lval;
typedef struct lenv lenv;

/* function pointer */
typedef lval *(*lbuiltin)(lenv *, lval *);


typedef enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_STR,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR,
} lval_type;


/* Type:  lval
 * -----------
 *   This is the most important type in the project. It is used to
 *   represent any value in lithp and is essentially a poor mans
 *   duck typing.
*/
struct lval
{
    lval_type type;

    /* could we use a union here? */
    intmax_t number;
    char *error_msg;
    char *symbol;
    char *str;

    /* builtin function */
    lbuiltin builtin; /* NULL if user defined */
    char *docstring;

    /* user function */
    // TODO: should get docstrings too!
    lenv *env;
    lval *formals;
    lval *body;

    /* expression */
    uint64_t count;
    lval **cell;
};


struct lenv
{
    lenv *parent; /* top parent is NULL */
    uint64_t count;
    char **syms;
    lval **vals;
};


/* LVAL */
lval *
lval_err(char *fmt, ...);
lval *
lval_num(intmax_t);
lval *
lval_sym(char *);
lval *
lval_str(char *);
lval *
lval_fun(lbuiltin func, char *doc);
lval *
lval_lambda(lval *formals, lval *body);
lval *
lval_sexpr(void);
lval *
lval_qexpr(void);

void
lval_cleanup(lval *);
int
lval_eq(lval *, lval *);
lval *
lval_copy(lval *);
lval *
lval_pop(lval *, uint64_t);
lval *
lval_add(lval *, lval *);
lval *
lval_join(lval *, lval *);
lval *
lval_call(lenv *, lval *, lval *);
lval *
lval_take(lval *, uint64_t );
lval *
lval_eval(lenv *, lval *);
lval *
lval_eval_sexpr(lenv *, lval *);

void
lval_print(lval *);
void
lval_println(lval *);
void
lval_expr_print(lval *, char open, char close);
char *
ltype_to_name(int type);

lval *
lval_read(mpc_ast_t *);


/* LENV */
lenv *
lenv_new(void);
void
lenv_put(lenv *, lval *, lval *);
void
lenv_put_global(lenv *, lval *, lval *);
lval *
lenv_get(lenv *, lval *);
void
lenv_clean_up(lenv *);
lenv *
lenv_copy(lenv *);


/* BUILTINS */
lval *
builtin_list(lenv *, lval *);
lval *
builtin_head(lenv *, lval *);
lval *
builtin_tail(lenv *, lval *);
lval *
builtin_eval(lenv *, lval *);
lval *
builtin_join(lenv *, lval *);
lval *
builtin_def(lenv *, lval *);
lval *
builtin_lambda(lenv *, lval *);
lval *
builtin_put(lenv *, lval *);

lval *
builtin_add(lenv *, lval *);
lval *
builtin_sub(lenv *, lval *);
lval *
builtin_mul(lenv *, lval *);
lval *
builtin_div(lenv *, lval *);

lval *
builtin_if(lenv *, lval *);
lval *
builtin_eq(lenv *, lval *);
lval *
builtin_ne(lenv *, lval *);
lval *
builtin_gt(lenv *, lval *);
lval *
builtin_lt(lenv *, lval *);
lval *
builtin_ge(lenv *, lval *);
lval *
builtin_le(lenv *, lval *);

lval *
builtin_import(lenv *, lval *);
lval *
builtin_print(lenv *, lval *);
lval *
builtin_error(lenv *, lval *);


/* PARSER */
mpc_parser_t *Number;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Symbol;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Program;
