#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


#define ASSERT(args, cond, err) \
        if (!(cond)) { lval_clean_up(args); return lval_err(err); }


enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };


typedef struct lval {
        int type;
        long num;

        char* err;
        char* sym;

        int count;
        struct lval** cell;
} lval;


lval* lval_num(long x) {
        lval* v = malloc(sizeof(lval));
        v->type = LVAL_NUM;
        v->num = x;
        return v;
}


lval* lval_err(char* m) {
        lval* v = malloc(sizeof(lval));
        v->type = LVAL_ERR;
        v->err = malloc(strlen(m) + 1);
        strcpy(v->err, m);
        return v;
}


lval* lval_sym(char* s) {
        lval* v = malloc(sizeof(lval));
        v->type = LVAL_SYM;
        v->sym = malloc(strlen(s) + 1);
        strcpy(v->sym, s);
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
                case LVAL_NUM: break;
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


lval* lval_take(lval* v, int i);
lval* lval_eval(lval* v);


lval* builtin_list(lval* a) {
        a->type = LVAL_QEXPR;
        return a;
}


lval* builtin_eval(lval* a) {
        ASSERT(a, a->count == 1, "'eval' passed too many arguments!");
        ASSERT(a, a->cell[0]->type == LVAL_QEXPR,
                        "'eval' passed incorrect type!");

        lval* x = lval_take(a, 0);
        x->type = LVAL_SEXPR;
        return lval_eval(x);
}


lval* builtin_head(lval* a) {
        ASSERT(a, a->count == 1, "'head' passed too many arguments!");
        ASSERT(a, a->cell[0]->type == LVAL_QEXPR,
                        "'head' passed incorrect type!");
        ASSERT(a, a->cell[0]->count != 0, "'head' passed {}!");

        lval* v = lval_take(a, 0);
        while (v->count > 1) lval_clean_up(lval_pop(v, 1));
        return v;
}


lval* builtin_tail(lval* a) {
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


lval* builtin_join(lval* a) {
        for (int i = 0; i < a->count; i++)
                ASSERT(a, a->cell[i]->type == LVAL_QEXPR,
                                "'join' passed incorrect type!");

        lval* x = lval_pop(a, 0);
        while (a->count)
                x = lval_join(x, lval_pop(a, 0));

        lval_clean_up(a);
        return x;
}


lval* builtin_op(lval* a, char* op) {
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


lval* lval_take(lval* v, int i) {
        lval* x = lval_pop(v, i);
        lval_clean_up(v);
        return x;
}


lval* builtin(lval* a, char* symbol) {
        if (strcmp("list", symbol) == 0) return builtin_list(a);
        if (strcmp("head", symbol) == 0) return builtin_head(a);
        if (strcmp("tail", symbol) == 0) return builtin_tail(a);
        if (strcmp("join", symbol) == 0) return builtin_join(a);
        if (strcmp("eval", symbol) == 0) return builtin_eval(a);
        if (strstr("+-/*", symbol)) return builtin_op(a, symbol);
        lval_clean_up(a);
        return lval_err("Unknown symbol");
}


lval* lval_eval(lval* v);


lval* lval_eval_sexpr(lval* v) {
        if (v->count == 0) return v;  /* empty expression */
        for (int i = 0; i < v->count; i++)
                v->cell[i] = lval_eval(v->cell[i]);

        for (int i = 0; i < v->count; i++)
                if (v->cell[i]->type == LVAL_ERR)
                        return lval_take(v, i);

        if (v->count == 1) return lval_take(v, 0);

        lval* f = lval_pop(v, 0);
        if (f->type != LVAL_SYM) {
                lval_clean_up(f);
                lval_clean_up(v);
                return lval_err("S-expression does not start with a symbol!");
        }

        lval* result = builtin(v, f->sym);
        lval_clean_up(f);
        return result;
}


lval* lval_eval(lval* v) {
        if (v->type == LVAL_SEXPR) return lval_eval_sexpr(v);
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
                        symbol   : \"list\" | \"head\" | \"tail\" | \"join\" \
                                 | \"eval\" | '+'      | '-'      | '*' | '/' ;\
                        sexpr    : '(' <expr>* ')' ;\
                        qexpr    : '{' <expr>* '}' ;\
                        expr     : <number> | <symbol> | <sexpr> | <qexpr> ;\
                        program  : /^/ <expr>* /$/ ;\
                ",
                Number, Symbol, Sexpr, Qexpr, Expr, Program);

        puts("Lithp 0.0.4");
        puts("Preth Ctrl+c to Exit\n");

        while (1) {
                char* input = readline("lithp> ");
                add_history(input);

                mpc_result_t r;
                if (mpc_parse("<stdin>", input, Program, &r)) {
                        lval* x = lval_eval(lval_read(r.output));
                        lval_println(x);
                        lval_clean_up(x);
                        mpc_ast_delete(r.output);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(input);
        }

        mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Program);
        return 0;
}
