/*
 * lenv.c
 * ------
 *
 *   A data structure for managing the environment/scoping.
 *
 */


#include <stdlib.h>
#include <string.h>

#include "lithp.h"


struct lenv
{
    lenv *parent;  /* top parent is NULL */
    int count;
    char **syms;
    lval **vals;
};


lenv *
lenv_copy (lenv *e) {
    lenv *n = malloc (sizeof (lenv));
    n->parent = e->parent;
    n->count = e->count;
    n->syms = malloc (sizeof (char*) * n->count);
    n->vals = malloc (sizeof (lval*) * n->count);

    for (int i = 0; i < n->count; i++)
      {
        n->syms[i] = malloc (strlen (e->syms[i]) + 1);
        strcpy (n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy (e->vals[i]);
    }

    return n;
}


lenv *
lenv_new (void)
{
    lenv *e = malloc (sizeof (lenv));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}


void
lenv_clean_up (lenv *e)
{
    for (int i = 0; i < e->count; i++)
      {
        free (e->syms[i]);
        lval_clean_up (e->vals[i]);
    }
    free (e->syms);
    free (e->vals);
    free (e);
}


lval *
lenv_get (lenv *e, lval *k)
{
    for (int i = 0; i < e->count; i++)
        if (strcmp (e->syms[i], k->symbol) == 0)
            return lval_copy (e->vals[i]);

    if (e->parent)
        return lenv_get (e->parent, k);

    return lval_err ("Unbound symbol '%s'!", k->symbol);
}


void
lenv_put (lenv *e, lval *k, lval *v)
{
    /* update existing value */
    for (int i = 0; i < e->count; i++)
      {
        if (strcmp (e->syms[i], k->symbol) == 0)
          {
            lval_clean_up (e->vals[i]);
            e->vals[i] = lval_copy (v);
            return;
        }
    }

    e->count++;
    e->syms = realloc (e->syms, sizeof (char*) * e->count);
    e->vals = realloc (e->vals, sizeof (lval*) * e->count);

    e->syms[e->count-1] = malloc (strlen (k->symbol) + 1);
    strcpy (e->syms[e->count-1], k->symbol);
    e->vals[e->count-1] = lval_copy (v);
}


void
lenv_put_global (lenv *e, lval *k, lval *v)
{
    while (e->parent)
        e = e->parent;

    lenv_put (e, k, v);
}
