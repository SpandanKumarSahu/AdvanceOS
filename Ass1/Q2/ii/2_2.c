#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

#define PB2_SET_TYPE    _IOW(0x10, 0x31, int32_t*)
#define PB2_SET_ORDER   _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO    _IOR(0x10, 0x33, int32_t*)
#define PB2_GET_OBJ     _IOR(0x10, 0x34, int32_t*)

MODULE_LICENSE("GPL");

static struct file_operations file_ops;

struct obj_info{
  int32_t deg1cnt;
  int32_t deg2cnt;
  int32_t deg3cnt;
  int32_t maxdepth;
  int32_t mindepth;
};

struct search_obj{
  unsigned char objtype;
  char found;
  int32_t int_obj;
  char str[100];
  int32_t len;
};

struct node{
  int32_t int_obj;
  char str[100];
  int32_t len;
  struct node* left;
  struct node* right;
  struct node* parent;
};

struct bst{
  unsigned char type;
  unsigned char order;
  unsigned char firstread;
  struct node* root;
  struct node* read_ptr;
};

struct queuenode{
  struct node *n;
  int level;
  struct queuenode *next;
};

struct queue{
  struct queuenode *front;
  struct queuenode *rear;
};

// This function traverses in the BST and sets the data to the appropriate pointer
static int insert(struct bst *b, void *data,int len){
  int32_t int_obj;
  char str[100];
  struct node *temp;
  struct node *newobj;

  if(b->type == 0xFF){
    int_obj=*(int32_t *)data;
    temp=b->root;
    if(b->root == NULL){
      newobj=kmalloc(sizeof(struct node),0);
      newobj->int_obj=int_obj;
      newobj->left=NULL;
      newobj->right=NULL;
      newobj->parent = NULL;
      b->root = newobj;
      return 1;
    }
    while(temp!=NULL){
      if(int_obj == temp->int_obj)
        return -EPERM;
      if(int_obj < temp->int_obj){
        if(temp->left == NULL){
          newobj=kmalloc(sizeof(struct node),0);
          newobj->int_obj=int_obj;
          newobj->left=NULL;
          newobj->right=NULL;
          newobj->parent=temp;
          temp->left=newobj;
        }

        else
          temp = temp->left;
      }

      else if(int_obj > temp->int_obj){
        if(temp->right == NULL){
          newobj=kmalloc(sizeof(struct node),0);
          newobj->int_obj=int_obj;
          newobj->left=NULL;
          newobj->right=NULL;
          newobj->parent=temp;
          temp->right=newobj;
        }
        else
          temp = temp->right;
      }
    }
  }
  else if(b->type == 0xF0){
    strcpy(str,(char *)data);
    temp=b->root;
    if(b->root == NULL){
      newobj=kmalloc(sizeof(struct node),0);
      strcpy(newobj->str,str);
      newobj->left=NULL;
      newobj->right=NULL;
      newobj->parent = NULL;
      newobj->len=len;
      b->root = newobj;
      return 1;
    }
    while(temp!=NULL)
    {
      if(strcmp(str,temp->str)==0)
        return -EPERM;

      if(strcmp(str,temp->str) < 0){
        if(temp->left == NULL){
          newobj=kmalloc(sizeof(struct node),0);
          strcpy(newobj->str,str);
          newobj->left=NULL;
          newobj->right=NULL;
          newobj->parent=temp;
          newobj->len=len;
          temp->left=newobj;
        }

        else
          temp = temp->left;
      }

      else{
        if(temp->right == NULL){
          newobj=kmalloc(sizeof(struct node),0);
          strcpy(newobj->str,str);
          newobj->left=NULL;
          newobj->right=NULL;
          newobj->parent=temp;
          newobj->len=len;
          temp->right=newobj;
        }
        else
          temp = temp->right;
      }
    }

  }
  return 1;
}

static ssize_t copy_current_node(struct bst *b,char *buf, size_t count, struct node *curr){
  int i;
  if(b->type==0xFF){
    if(count<sizeof(int32_t)){
      buf[0]=0;
      return -1;
    }
    count=sizeof(int32_t);
    strcpy(buf,(char *)&curr->int_obj);
  }
  else if(b->type==0xF0){
    count = count < curr->len+1 ? count:curr->len+1;
    for(i=0;i<count;i++) buf[i]=curr->str[i];
    b->read_ptr=curr->parent;
  }
  return count;
}
/* This `read_next` function does the following:
 1. Sets the value at the address `read_ptr` in buf
 2. Updates read_ptr to the next node to be read (as per inorder/preorder/postorder)
    specified in b->order.
 3. Return 1, if successful, 0 if no nodes are there to be read.
 (See pt 4 of assignment part 2)
*/
static ssize_t read_next(struct bst *b, char *buf, size_t count){
  struct node *curr,*temp;

  curr=b->read_ptr;
  if(curr==NULL) return -EACCES;

  if(b->order==0x00){
  if(curr->left==NULL&&curr->right==NULL){
    count=copy_current_node(b,buf,count,curr);
    if(count==-1) return -EACCES;

    if(curr->parent->left==curr)
      b->read_ptr=curr->parent;
    else{
      temp=curr;
      while(temp!=b->root && temp->parent->right==temp){
        temp=temp->parent;
      }
      if(temp==b->root)
        b->read_ptr=NULL;
      else
        b->read_ptr=temp->parent;
    }
  }
  else if(curr->right!=NULL){
    count=copy_current_node(b,buf,count,curr);
    if(count==-1) return -EACCES;
    b->read_ptr=curr->right;
  }
  else{
    count=copy_current_node(b,buf,count,curr);
    if(count==-1) return -EACCES;
    temp=curr;
    while(temp!=b->root && temp->parent->right==temp){
      temp=temp->parent;
    }
    if(temp==b->root)
      b->read_ptr=NULL;
    else
      b->read_ptr=temp->parent;
  }
  return count;
  }

  else if(b->order == 0x02){
    count=copy_current_node(b,buf,count,curr);
    if(count==-1) return -EACCES;

    if(curr==b->root)
      b->read_ptr=NULL;
    else if(curr->parent->left == curr){
      if(curr->parent->right!=NULL)
        b->read_ptr=curr->parent->right;
      else
        b->read_ptr=curr->parent;
    }
    else{
      b->read_ptr=curr->parent;
    }
    return count;
  }

  else if(b->order == 0x01){
    if(curr->left==NULL && curr->right==NULL){
      count=copy_current_node(b,buf,count,curr);
    if(count==-1) return -EACCES;

    if(curr->parent->left==curr){
      if(curr->parent->right!=NULL)
        b->read_ptr=curr->parent->right;

      else{
        temp=curr;
        while(temp!=b->root && temp->parent->right!=NULL && temp->parent!=temp)
          temp=temp->parent;

        if(temp==b->root) b->read_ptr=NULL;
        else b->read_ptr=temp->right;
      }
    }
    return count;
    }

    else{
      count=copy_current_node(b,buf,count,curr);
    if(count==-1) return -EACCES;

      if(curr->left!=NULL)
        b->read_ptr=curr->left;

      else
        b->read_ptr=curr->right;
    }
  }

  return -EINVAL;
}

/*
* Set the fields of struct obj_info and return the size of the bst
*/
static long get_info(struct bst *b, struct obj_info *info){
  int size = 0;
  int degree;
  struct queue bfsqueue;
  struct queuenode *curr,*temp;

  info->deg1cnt = info->deg2cnt = info->deg3cnt = info->maxdepth = 0;
  info->mindepth = INT_MAX;

  bfsqueue.front = kmalloc(sizeof(struct queuenode),0);
  bfsqueue.front->n = b->root;
  bfsqueue.front->level=0;
  bfsqueue.front->next = NULL;
  bfsqueue.rear = bfsqueue.front;


  while(bfsqueue.front != NULL){
    curr=bfsqueue.front;
    size++;
    degree=0;

    if(curr->n->parent!=NULL) degree++;
    if(curr->n->left!=NULL){
      degree++;
      temp=kmalloc(sizeof(struct queuenode),0);
      temp->n=curr->n->left;
      temp->level=curr->level+1;
      temp->next=NULL;
      bfsqueue.rear->next=temp;
      bfsqueue.rear=temp;
    }
    if(curr->n->right!=NULL){
      degree++;
      temp=kmalloc(sizeof(struct queuenode),0);
      temp->n=curr->n->right;
      temp->level=curr->level+1;
      temp->next=NULL;
      bfsqueue.rear->next=temp;
      bfsqueue.rear=temp;
    }

    if(degree==3) info->deg3cnt++;
    if(degree==2) info->deg2cnt++;
    if(degree==1) info->deg1cnt++;

    if(curr->n->left==NULL&&curr->n->right==NULL){
      if(curr->level < info->mindepth)
        info->mindepth=curr->level;

      if(curr->level > info->maxdepth)
        info->maxdepth=curr->level;
    }

    bfsqueue.front=bfsqueue.front->next;
    kfree(curr);
  }

  return size;
}

/*
* Set the fields of struct search_obj and return 1, if not found, 0 if found
*/
static long get_obj(struct bst *b, struct search_obj *obj){
  struct node *curr;

  curr=b->root;
  obj->found=1;

  if(b->type!=obj->objtype)
    return 0;

  if(b->type == 0xFF){
    while(curr != NULL){
      if(curr->int_obj == obj->int_obj){
        obj->found = 0;
        return 0;
      }
      else if(curr->int_obj < obj->int_obj)
        curr = curr->left;
      else
        curr = curr->right;
    }
  } else {
    while(curr != NULL){
      if(strcmp(curr->str, obj->str)==0){
        obj->found = 0;
        return 0;
      }
      else if(strcmp(curr->str, obj->str)>0)
        curr = curr->left;
      else
        curr = curr->right;
    }
  }
  return 0;
}

static void free_bst(struct node* root){
  if(root->left != NULL){
    free_bst(root->left);
  }
  if(root->right != NULL){
    free_bst(root->right);
  }
  kfree(root);
  return;
}

// Release the memory even if the file is being closed midway
// Release is called only when all fd's are closed, flush is called after each
static int flush(struct file *file, fl_owner_t id){
  struct bst *b;
  if(file->private_data != NULL){
    b = file->private_data;
    file->private_data = NULL;
    free_bst(b->root);
    kfree(b);
  }
  return 0;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg){
  struct bst* b;
  unsigned char type;
  unsigned char otype;
  struct search_obj *obj;
  struct obj_info *info;

  switch(cmd) {
      case PB2_SET_TYPE:
        if(file->private_data != NULL){
          b = file->private_data;
          free_bst(b->root);
          kfree(b);
          file->private_data = NULL;
        }
        b = (struct bst *) kmalloc(sizeof(struct bst), 0);
        b->root = NULL;
        b->read_ptr = NULL;
        b->order = 0x00;
        b->firstread = 0x01;
        type = *((unsigned char*)arg);
        if(type == 0xF0 || type == 0xFF)
          b->type = type;
        else return -EACCES;
        file->private_data = b;
        return 0;

      case PB2_SET_ORDER:
        if(file->private_data == NULL)
          return -EACCES;
        else{
          b = file->private_data;
          otype = *((unsigned char*)arg);
          if(otype == 0x00 || otype == 0x01 || otype == 0x02){
            b->order = otype;
            b->read_ptr = b->root;
            b->firstread = 0x01;
          } else return -EINVAL;
        }
        return 0;

      case PB2_GET_INFO:
        if(file->private_data == NULL)
          return -EACCES;
        info = (struct obj_info*) arg;
        b = file->private_data;
        return get_info(b, info);

      case PB2_GET_OBJ:
        if(file->private_data == NULL)
          return -EACCES;
        obj = (struct search_obj*) arg;
        b = file->private_data;
        return get_obj(b, obj);

      default:
        return -EACCES;
  }
  return 0;
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
  struct bst *b;
  struct node* temp;
  int num;
  char *str;

  if(file->private_data == NULL)
    return -EACCES;

  b = file->private_data;
  temp = b->root;

  if(b->type == 0xFF){
    if(count != sizeof(int)) return -EINVAL;
    num = *((int*) buf);
    if(b->root == NULL){
      insert(b, &num, 4);
      b->read_ptr = b->root;
    } else {
      insert(b, &num, 4);
    }
  } else { // String type
    if(count>100 && strlen(buf)>100) return -EINVAL;
    count = (count <= strlen(buf)) ? count:strlen(buf);
    str = (char *) kmalloc((count+1)*sizeof(int), 0);
    strcpy(str, buf);
    str[count]='\0';
    if(b->root == NULL){
      insert(b,str,count);
      b->read_ptr = b->root;
    } else {
      insert(b,str,count);
    }
  }
  return 0;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
  struct bst *b;
  if(file->private_data == NULL)
    return -EACCES;
  b = file->private_data;
  if(b->root == NULL)
    return 0;
  else {
    if(b->firstread == 0x01){
      b->firstread = 0x00;
      if(b->order == 0x00 || b->order == 0x02){
        // Inorder and Postorder (move to the leftmost child)
        while(b->read_ptr->left != NULL)
          b->read_ptr = b->read_ptr->left;
      } else {
        // Preorder (the root is the first node)
        b->read_ptr = b->root;
      }
    }
    return read_next(b, buf, count);
  }
}

static int hello_init(void) {
    struct proc_dir_entry *entry;
    printk(KERN_ALERT "Ready for BST? EX de do plox\n");
    entry=proc_create("partb_2", 0666, NULL, &file_ops);
    if(!entry) return -ENOENT;
    file_ops.owner = THIS_MODULE;
    file_ops.write = write;
    file_ops.read = read;
    file_ops.unlocked_ioctl = ioctl;
    file_ops.flush = flush;
    return 0;
}

static void hello_exit(void){
    printk(KERN_ALERT "Goodbye! I am done handling BST\n");
    remove_proc_entry("partb_2", NULL);
}

module_init(hello_init);
module_exit(hello_exit);
