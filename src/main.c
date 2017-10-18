#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

#include "lithp.h"


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


void lenv_add_builtin(lenv* e, char* symbol, lbuiltin func) {
    lval* k = lval_sym(symbol);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_clean_up(k);
    lval_clean_up(v);
}


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

    lenv_add_builtin(e, "import", builtin_import);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);
}


lval* lval_read(mpc_ast_t* t);


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
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* String = mpc_new("string");
    mpc_parser_t* Comment = mpc_new("comment");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Program = mpc_new("program");

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
       puts("Lithp 0.0.12");
       puts("Preth Ctrl+c to Exit\n");

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