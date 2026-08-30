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

static int range_rec(Node *root, int a, int b, Entry *results, int capacity, int *count) {
  if (!root || *count >= capacity)
    return 1;

  if (root->entry->key > a)
    range_rec(root->left, a, b, results, capacity, count);

  if (root->entry->key >= a && root->entry->key <= b && *count < capacity) {
    results[*count] = *root->entry;
    (*count)++;
  }

  if (root->entry->key < b)
    range_rec(root->right, a, b, results, capacity, count);

  return 1;
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

  if (!t)
    return NULL;

  t->root = NULL;

  return t;
}

void bst_free(BST *tree) {
  if (!tree)
    return;

  free_rec(tree->root);
  free(tree);
}

void bst_insert(BST *tree, Entry *entry) {
  if (!tree || !entry)
    return;

  tree->root = insert_rec(tree->root, entry);
}

int bst_range(BST *tree, int a, int b, Entry *results, int capacity) {
  if (!tree || !results || capacity <= 0 || a > b)
      return 0;

  int count = 0;

  count = range_rec(tree->root, a, b, results, capacity, 0);

  return count;
}

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
  if (!tree)
    return;

  tree->root = delete_rec(tree->root, key);
}

void bst_clear(BST *tree) {
  if (!tree)
    return;

  free_rec(tree->root);
  tree->root = NULL;
}
