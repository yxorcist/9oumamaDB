#ifndef BST_H
#define BST_H

#include "entry.h"

typedef struct BST BST;

BST *bst_create(void);
void bst_free(BST *tree);

void bst_insert(BST *tree, Entry *entry);
void bst_delete(BST *tree, int key);
int bst_range(BST *tree, int a, int b, Entry *results, int capacity);
void bst_clear(BST *tree);

#endif
