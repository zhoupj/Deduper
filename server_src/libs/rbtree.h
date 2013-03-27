#ifndef BTREE_H_
#define BTREE_H

#define KEY_LEN 20

typedef struct rb_node_t{
      unsigned char key[KEY_LEN];
      unsigned int hash;
      int colour;	
      struct rb_node_t * lchild, *rchild, *parent;
      void * data;	  
}rb_node;

rb_node * rb_init();
void rb_free(rb_node** root);
void rb_insert(rb_node** root, unsigned char key[],void *data);
rb_node* rb_search(rb_node*root,unsigned char key[]);
void* rb_delete(rb_node** root,unsigned char key[]);
void show_rb_tree(rb_node* node);



#endif
