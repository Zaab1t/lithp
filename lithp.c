#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


long eval_op(long x, char* op, long y) {
        if (strcmp(op, "+") == 0) return x + y;
        if (strcmp(op, "-") == 0) return x - y;
        if (strcmp(op, "*") == 0) return x * y;
        if (strcmp(op, "/") == 0) return x / y;
        return 0;
}


long eval(mpc_ast_t* node) {
        if (strstr(node->tag, "number"))
                return atoi(node->contents);
        
        char* op = node->children[1]->contents;
        long x = eval(node->children[2]);

        for (int i = 3; strstr(node->children[i]->tag, "expr"); i++)
                x = eval_op(x, op, eval(node->children[i]));
        
        return x;
}


int main(int argc, char** argv) {
        mpc_parser_t* Number = mpc_new("number");
        mpc_parser_t* Operator = mpc_new("operator");
        mpc_parser_t* Expr = mpc_new("expr");
        mpc_parser_t* Program = mpc_new("program");

        mpca_lang(MPCA_LANG_DEFAULT,
                "\
                        number   : /-?[0-9]+/ ;\
                        operator : '+' | '-' | '*' | '/' ;\
                        expr     : <number> | '(' <operator> <expr>+ ')' ;\
                        program  : /^/ <operator> <expr>+ /$/ ;\
                ",
                Number, Operator, Expr, Program);

        puts("Lithp 0.0.1");
        puts("Preth Ctrl+c to Exit\n");

        while (1) {
                char* input = readline("lithp> ");
                add_history(input);

                mpc_result_t r;
                if (mpc_parse("<stdin>", input, Program, &r)) {
                        printf("%li\n", eval(r.output));
                        mpc_ast_delete(r.output);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(input);
        }

        mpc_cleanup(4, Number, Operator, Expr, Program);
        return 0;
}
