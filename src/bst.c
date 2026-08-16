#include <stdio.h>
#include <stdlib.h>

#include "bst.h"

typedef struct Node {
  Entry *entry;
  struct Node *left;
  struct Node *right;
} Node;

struct BST {
  Node *root;
};

static Node *create_node(Entry *entry) {

  Node *n = malloc(sizeof(Node));

  n->entry = entry;
  n->left = NULL;
  n->right = NULL;

  return n;
}

static Node *insert_rec(Node *root, Entry *entry) {

  if (!root)
    return create_node(entry);

  if (entry->key < root->entry->key)
    root->left = insert_rec(root->left, entry);
  else if (entry->key > root->entry->key)
    root->right = insert_rec(root->right, entry);
  else
    root->entry = entry;

  return root;
}

static void range_rec(Node *root, int a, int b) {
  if (!root)
    return;

  if (root->entry->key > a)
    range_rec(root->left, a, b);

  if (root->entry->key >= a && root->entry->key <= b)
    printf("%d %d\n", root->entry->key, root->entry->value);

  if (root->entry->key < b)
    range_rec(root->right, a, b);
}

static void free_rec(Node *root) {
  if (!root)
    return;

  free_rec(root->left);
  free_rec(root->right);

  free(root);
}

BST *bst_create(void) {
  BST *t = malloc(sizeof(BST));
  t->root = NULL;
  return t;
}

void bst_free(BST *tree) {
  free_rec(tree->root);
  free(tree);
}

void bst_insert(BST *tree, Entry *entry) {
  tree->root = insert_rec(tree->root, entry);
}

void bst_range(BST *tree, int a, int b) { range_rec(tree->root, a, b); }

static Node *min_node(Node *n) {

  while (n->left)
    n = n->left;

  return n;
}

static Node *delete_rec(Node *root, int key) {

  if (!root)
    return NULL;

  if (key < root->entry->key) {
    root->left = delete_rec(root->left, key);
  }

  else if (key > root->entry->key) {
    root->right = delete_rec(root->right, key);
  }

  else {
    // case 1: no child
    if (!root->left && !root->right) {
      free(root);
      return NULL;
    }

    // case 2: one child
    if (!root->left) {
      Node *tmp = root->right;
      free(root);
      return tmp;
    }

    if (!root->right) {
      Node *tmp = root->left;
      free(root);
      return tmp;
    }

    // case 3: two children
    Node *succ = min_node(root->right);
    root->entry = succ->entry;
    root->right = delete_rec(root->right, succ->entry->key);
  }

  return root;
}

void bst_delete(BST *tree, int key) {
  tree->root = delete_rec(tree->root, key);
}
