#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"


enum { LVAL_NUMBER, LVAL_ERR };
enum { ERR_ZERO_DIVISION, ERR_BAD_OP, ERR_BAD_NUMBER };


typedef struct {
        int type;
        long num;
        int err;
} lval;


lval lval_number(long x) {
        lval v;
        v.type = LVAL_NUMBER;
        v.num = x;
        return v;
}


lval lval_error(long x) {
        lval v;
        v.type = LVAL_ERR;
        v.err = x;
        return v;
}


void lval_print(lval v) {
        if (v.type == LVAL_NUMBER)
                printf("%li", v.num);
        else if (v.type == LVAL_ERR) {
                printf("Error: ");
                if (v.err == ERR_ZERO_DIVISION)
                        printf("Division by 0");
                if (v.err == ERR_BAD_OP)
                        printf("Invalid Operator");
                if (v.err == ERR_BAD_NUMBER)
                        printf("Invalid Number");
        }
}


void lval_println(lval v) {
        lval_print(v);
        putchar('\n');
}


lval eval_op(lval x, char* op, lval y) {
        if (x.type == LVAL_ERR) return x;
        if (y.type == LVAL_ERR) return y;

        if (strcmp(op, "+") == 0) return lval_number(x.num + y.num);
        if (strcmp(op, "-") == 0) return lval_number(x.num - y.num);
        if (strcmp(op, "*") == 0) return lval_number(x.num * y.num);
        if (strcmp(op, "/") == 0) {
                if (y.num == 0) return lval_error(ERR_ZERO_DIVISION);
                return lval_number(x.num / y.num);
        }

        return lval_error(ERR_BAD_OP);
}


lval eval(mpc_ast_t* node) {
        if (strstr(node->tag, "number")) {
                /* Check for error in conversion */
                errno = 0;
                long x = strtol(node->contents, NULL, 10);
                if (errno == ERANGE) return lval_error(ERR_BAD_NUMBER);
                return lval_number(x); 
        }
        
        char* op = node->children[1]->contents;
        lval x = eval(node->children[2]);

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

        puts("Lithp 0.0.2");
        puts("Preth Ctrl+c to Exit\n");

        while (1) {
                char* input = readline("lithp> ");
                add_history(input);

                mpc_result_t r;
                if (mpc_parse("<stdin>", input, Program, &r)) {
                        lval result = eval(r.output);
                        lval_println(result);
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
