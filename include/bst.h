#ifndef BST_H
#define BST_H

#include "entry.h"

typedef struct BST BST;

BST *bst_create(void);
void bst_free(BST *tree);

void bst_insert(BST *tree, Entry *entry);
void bst_delete(BST *tree, int key);
void bst_range(BST *tree, int a, int b);

#endif
