#include "../global.h"

/* red black tree */
const int RED = 0;
const int BLACK = 1;
int g_count=0;

rb_node* get_node(rb_node* parent,unsigned char key[],unsigned int hash,void *data );
rb_node* clock_wise(rb_node** root,rb_node* node);
rb_node* counter_clock_wise(rb_node** root,rb_node* node);

unsigned int sdbm_hash( unsigned char *key)
{
	unsigned int  h = 0;
	int i=0;
	while(i<KEY_LEN-1) {
		h = *(key++) +  (h<<6) + (h<<16) - h;
		i++;
	}
	return h;
}

rb_node * rb_init(){
	
}

void rb_free(rb_node** root){
     if(*root==NULL)
		return;
    if((*root)->lchild != NULL){
     	rb_free(&((*root)->lchild));
     }
     if((*root)->rchild != NULL){
     rb_free(&((*root)->rchild));
     }
    free((*root));
   (*root)=NULL;
	
}

rb_node* get_node(rb_node* parent, unsigned char key[],unsigned int hash,void *data){
    rb_node *tmp = (rb_node*)malloc(sizeof(rb_node));
    memcpy(tmp->key,key,KEY_LEN);
    tmp->hash=hash;
    tmp->colour = RED;
    tmp->parent = parent;
    tmp->lchild = tmp->rchild = NULL;
    tmp->data=data;
    return tmp;
}
rb_node* clock_wise(rb_node** root,rb_node* node){
    if(node == NULL || node->lchild == NULL)
    return NULL;
    rb_node *rb_1=node, *rb_2=node->lchild, *rb_3=node->lchild->rchild;
    if(rb_1->parent != NULL){
    if(rb_1->parent->lchild == rb_1)
        rb_1->parent->lchild = rb_2;
    else
        rb_1->parent->rchild = rb_2;
    }else if(rb_1 == *root){
	*root = rb_2;
    }
    rb_2->parent = rb_1->parent;
    rb_1->parent = rb_2;
    rb_2->rchild = rb_1;
    rb_1->lchild = rb_3;
    if(rb_3 != NULL)rb_3->parent = rb_1;
    return rb_2;    
}
rb_node* counter_clock_wise(rb_node**root,rb_node* node){
    if(node == NULL || node->rchild == NULL)
    return NULL;
    rb_node *rb_1=node, *rb_2=node->rchild, *rb_3=node->rchild->lchild;
    if(rb_1->parent != NULL){
    if(rb_1->parent->lchild == rb_1)
        rb_1->parent->lchild = rb_2;
    else 
        rb_1->parent->rchild = rb_2;
    }
    else if(rb_1 == *root){
    *root = rb_2;
}
    rb_2->parent = rb_1->parent;

    rb_1->parent = rb_2;
      rb_2->lchild = rb_1;
  
      rb_1->rchild = rb_3;
      if(rb_3 != NULL)rb_3->parent = rb_1;
  
      return rb_2;
  }
  
  rb_node* rb_search(rb_node* root,unsigned char key[]){
      rb_node *p = root;
      unsigned int hash;
      hash=sdbm_hash(key);
	  
      while(p != NULL){
	      if(hash < p->hash)
	        p = p->lchild;
	    else if(hash > p->hash)
	        p = p->rchild;
	    else{

			if(memcmp(key,p->key,KEY_LEN)==0)
				    break;
			else if(memcmp(key,p->key,KEY_LEN)<0)
				     p = p->lchild;
			else 
				p = p->rchild;
		}
      }
      return p;
  }
  
void rb_insert(rb_node **root,unsigned char key[],void *data){
    rb_node *p=*root, *q=NULL;
   int left=1;	
   unsigned int hash;
   hash=sdbm_hash(key);
   if(*root == NULL){
    *root = get_node(NULL, key,hash,data);
    (*root)->colour = BLACK;
    return;
    }
   	

    while(p != NULL){
	    q = p;
	    if(hash < p->hash)
	       { p = p->lchild;left=1;}
	    else if(hash > p->hash)
	        {p = p->rchild;left=0;}
	    else{
		        if(memcmp(key,p->key,KEY_LEN)<0)
				   { p = p->lchild;left=1;}
			else if (memcmp(key,p->key,KEY_LEN)>0)
				     {p = p->rchild;left=0;}
			else 
				{
				printf("%s:%d same key\n",__FILE__,__LINE__);
				return;	   /* if same ,don't insert */
				}
			}
    }
    if(left)
    	q->lchild = get_node(q, key,hash,data);
    else
     	q->rchild = get_node(q, key,hash,data);
   
     while(q != NULL && q->colour == RED){
	     p = q->parent;//p won't null, or BUG.
	 
	     if(p->lchild == q){
	         if(q->rchild != NULL && q->rchild->colour == RED)
	         counter_clock_wise(root,q);        
	         q = clock_wise(root,p);
	         q->lchild->colour = BLACK;
	     }
	     else{
	         if(q->lchild != NULL && q->lchild->colour == RED)
	         clock_wise(root,q);
	         q = counter_clock_wise(root,p);
	         q->rchild->colour = BLACK;
	     }
	 
	     q = q->parent;
     }
     (*root)->colour = BLACK;
 }

 /* LDR */
 void show_rb_tree(rb_node* node){
     if(node == NULL)
     	return;
    
    if(node->lchild != NULL){
   // printf("[-1]\n");
     show_rb_tree(node->lchild);
     }
   // printf("(%u,%d)\t", node->hash, node->colour);
      g_count++;
     if(node->rchild != NULL){
    // printf("[1]\n");
     show_rb_tree(node->rchild);
     }
     //printf("[0]\n");
 }

/* 释放节点,返回data 的指针(data 未释放)*/
 void* rb_delete(rb_node** root,unsigned char key[]){
     rb_node *v, *u, *p, *c, *b;
     void *data=NULL;
      unsigned char tmp_key[KEY_LEN];
      unsigned int tmp_hash;
      void * tmp_data;	  
	  
     v = rb_search(*root,key);
     if(v == NULL) return data;
 
     u = v;
     if(v->lchild != NULL && v->rchild != NULL){
	     u = v->rchild;
	     while(u->lchild != NULL){
	         u = u->lchild;
	     }
	    memcpy(tmp_key,u->key,KEY_LEN);
	    tmp_hash=u->hash;
	    tmp_data=u->data;

	    memcpy(u->key,v->key,KEY_LEN);
	    u->hash=v->hash;
	    u->data=v->data;

	     memcpy(v->key,tmp_key,KEY_LEN);
	    v->hash= tmp_hash;
	    v->data=tmp_data;
     }
 
     //u is the node to free.
     if(u->lchild != NULL)
     c = u->lchild;
     else 
     c = u->rchild;
 
     p = u->parent;
     if(p != NULL){
     //remove u from rb_tree.
     if(p->lchild == u)
         p->lchild = c;
    else
         p->rchild = c;
    }
     else{
     //u is root.
    *root = c;
     data=u->data;
     free((void*)u);
     return data;
    }
    
     //u is not root and u is RED, this will not unbalance.
     if(u->colour == RED){
      data=u->data;
     free((void*)u);
     return data;
     }
     data=u->data;
     free((void*)u);
     u = c;
 
     //u is the first node to balance.
     while(u !=* root){
     if(u != NULL && u->colour == RED){
         //if u is RED, change it to BLACK can finsh.
         u->colour = BLACK;
         return data;
     }
 
     if(u == p->lchild)
         b = p->rchild;
     else 
         b = p->lchild;
 
     //printf("deleted: %d\n", b->key);
 
     //b is borther of u. b can't be null, or the rb_tree is must not balance.
     if(b->colour == BLACK){
         //If b's son is RED, rotate the node.
         if(b->lchild != NULL && b->lchild->colour == RED){
         if(u == p->lchild){
             b = clock_wise(root,b);
             b->colour = BLACK;
             b->rchild->colour = RED;
 
             p = counter_clock_wise(root,p);
             p->colour = p->lchild->colour;
             p->lchild->colour = BLACK;
             p->rchild->colour = BLACK;
         }
         else{
             p = clock_wise(root,p);
             p->colour = p->rchild->colour;
            p->rchild->colour = BLACK;
            p->lchild->colour = BLACK;
        }

        return data;
        }
        else if(b->rchild != NULL && b->rchild->colour == RED){
        if(u == p->rchild){
            b = counter_clock_wise(root,b);
            b->colour = BLACK;
            b->lchild->colour = RED;
            p = clock_wise(root,p);
            p->colour = p->rchild->colour;
            p->rchild->colour = BLACK;
            p->lchild->colour = BLACK;
        }
        else{
            p = counter_clock_wise(root,p);
            p->colour = p->lchild->colour;
            p->lchild->colour = BLACK;
            p->rchild->colour = BLACK;
        }        
        return data;
        }
        else{//if b's sons are BLACK, make b RED and move up u.
        b->colour = RED;
        u = p;
       p = u->parent;
        continue;
      }
    }
   else{
        if(u == p->lchild){
        p = counter_clock_wise(root,p);
        p->colour = BLACK;
        p->lchild->colour = RED;
        p = p->lchild;
        }
        else{
        p = clock_wise(root,p);
        p->colour = BLACK;
        p->rchild->colour = RED;
        p = p->rchild;
        }
    }
    }
    (*root)->colour = BLACK;
}
/*
void finger_chunk(unsigned char *buf, unsigned int len,unsigned char hash[])
{
   SHA_CTX context;
   unsigned char digest[20]={0};
   SHA1_Init(&context);
   SHA1_Update(&context, buf, (int)len);
   SHA1_Final(digest,&context);
   memcpy(hash,digest,20);
}
typedef struct item_t {
	unsigned char hash[20];
	 char data[30];
}ITEM;


int g_item_count=10000000;
int main(){
    int i; 
   ITEM* item=NULL,*p=NULL;
   rb_node* root = NULL;
     item=(ITEM*)malloc(sizeof(ITEM)*g_item_count);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
    for(i = 0; i <g_item_count; i++){    
	snprintf(item[i].data,30, "number%d",i+i*134);
	finger_chunk((unsigned char *)item[i].data,strlen(item[i].data),item[i].hash);
   	rb_insert(&root,item[i].hash,(void*)&item[i]);
	//printf("%d \t",i);
    }
    p=(ITEM*)rb_delete(&root,item[g_item_count/2].hash);
   if(p)
   	printf("delete %.20s \t %.30s \n",p->hash,p->data);
   p=(ITEM*) rb_delete(&root,item[g_item_count/3].hash);
    if(p)
   	printf("delete %.20s \t %.30s \n",p->hash,p->data);
   rb_node * rb=rb_search(root,item[g_item_count/2+2].hash);
   if(rb){
   	printf("found: %s \t %d  \n",rb->key,rb->hash);
   }
   else
   	printf("not found\n");
   
    rb=rb_search(root,item[g_item_count/2].hash);
   if(rb){
   	printf("found:%s \t %d \n",rb->key,rb->hash);
   }
   else
   	printf("not found\n");
   
    show_rb_tree(root);
      printf("\n");
    rb_free(&root);
    printf("count=%d \n",g_count);
    show_rb_tree(root);
	  printf("\n");
    printf("count=%d \n",g_count);
    free(item);
    return 0;
}
*/
