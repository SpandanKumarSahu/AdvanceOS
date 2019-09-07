#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

static struct file_operations file_ops;

typedef struct {
	int type;
	int num;
	int received;
	int sent;
	int *integers;
	char **strings;
} data;

// The sorting algorithm: We follow bubble sort
static void sort(data *d) {
	int i, j, tempint;
	char tempstr[100];

	for(i=0; i<d->num; i++){
		for(j=0; j<d->num-i-1; j++){
			if(d->type == 0xFF && d->integers[j]>d->integers[j+1]){
				tempint=d->integers[j];
				d->integers[j]=d->integers[j+1];
				d->integers[j+1]=tempint;
			} else if(d->type == 0xF0 && strcmp(d->strings[j],d->strings[j+1])>0){
				strcpy(tempstr,d->strings[j]);
				strcpy(d->strings[j],d->strings[j+1]);
				strcpy(d->strings[j+1],tempstr);
			}
		}
	}
}

// The customized release function for freeing up memory
static void custom_release(data *d){
	printk(KERN_INFO "Releasing");
	if(d->type == 0xFF)
		kfree(d->integers);
	else{
		int i;
		for(i=0; i<d->num; i++)
			kfree(d->strings[i]);
		kfree(d->strings);
	}
	kfree(d);
	return;
}

// Release the memory even if the file is being closed midway
// Release is called only when all fd's are closed, flush is called after each
static int flush(struct file *file, fl_owner_t id){
	if(file->private_data != NULL){
		data *d = file->private_data;
		file->private_data = NULL;
		custom_release(d);
	}
	return 0;
}

// The write function
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
	data *d;

	// The sorting initialization part (first write)
	if(file->private_data == NULL) {
		printk(KERN_INFO "Restarting sort");
		if(count != 2*sizeof(char)){
			printk(KERN_INFO "Incorrect start");
			return -EINVAL;
		} else {
			unsigned char* init = (unsigned char*)buf;
			if(init[1]>100) return -EINVAL;
			if(init[0] != 0xFF && init[0] != 0xF0) return -EINVAL;

			d=(data *)kmalloc(sizeof(data),0);
			if(init[0] == 0xFF || init[0] == 0xF0)
				d->type = init[0];
			else return -EINVAL;
			d->num = init[1], d->received=0, d->sent=0;
			if(d->type == 0xFF)
				d->integers = (int *) kmalloc(d->num * sizeof(int), 0);
			else
				d->strings = (char **)kmalloc(d->num * sizeof(char*), 0);

			file->private_data=d;
		}
		return 2*sizeof(int);
	} else {		// We are writing to the file
		d = file->private_data;
		if(d->num == d->received) return 0;

		if(d->type == 0xFF){ // integer tupe
			if(count != sizeof(int)) return -EINVAL;
			d->integers[d->received] = *((int *)buf);
		} else { // String type
			if(count>100 && strlen(buf)>100) return -EINVAL;
			count = (count <= strlen(buf)) ? count:strlen(buf);
			d->strings[d->received] = (char *) kmalloc((count+1)*sizeof(int), 0);
			strcpy(d->strings[d->received],buf);
			d->strings[d->received][count]='\0';
		}
		d->received++;
		if(d->num != d->received) return d->num - d->received;

		if(d->num == d->received) // Writing complete
			sort(d);
		return 0;
	}
}

// The read function
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
    data *d = file->private_data;
    int i, val;
    if(d == NULL) return -EACCES;
    else if(d->received != d->num) return -EACCES;
    else if(d->sent == d->num) {
	  strcpy(buf,"\0");
	  return -EACCES;
	} else {
		// Read sorted list one by one
		if(d->type==0xF0){
			count = count<=strlen(d->strings[d->sent])+1 ? count: strlen(d->strings[d->sent])+1;
			for(i=0; i<count; i++)
				buf[i]=d->strings[d->sent][i];
			d->sent++;
		} else {
			if(count!=sizeof(int)) return -EACCES;
			val=d->integers[d->sent];
			strcpy(buf,(char *)&val);
			d->sent++;
		}
		// Free up the space, and clear private_data for next sorting
		if(d->sent ==  d->num){
			file->private_data = NULL;
			custom_release(d);
		}
		return count;
	}
}

static int hello_init(void) {
    struct proc_dir_entry *entry;
    printk(KERN_ALERT "Ready to Sort? Jump into it!\n");
    entry=proc_create("partb_1", 0666, NULL, &file_ops);
    if(!entry) return -ENOENT;
    file_ops.owner = THIS_MODULE;
    file_ops.write = write;
    file_ops.read = read;
    file_ops.flush = flush;
    return 0;
}

static void hello_exit(void){
    printk(KERN_ALERT "Goodbye, I am done sorting!\n");
    remove_proc_entry("partb_1", NULL);
}

module_init(hello_init);
module_exit(hello_exit);
