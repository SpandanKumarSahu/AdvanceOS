#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define PB2_SET_TYPE    _IOW(0x10, 0x31, int32_t*)
#define PB2_SET_ORDER   _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO    _IOR(0x10, 0x33, int32_t*)
#define PB2_GET_OBJ     _IOR(0x10, 0x34, int32_t*)

typedef struct {
  int32_t deg1cnt;
  int32_t deg2cnt;
  int32_t deg3cnt;
  int32_t maxdepth;
  int32_t mindepth;
} obj_info;

typedef struct {
  char objtype;
  char found;
  int32_t int_obj;
  char str[100];
  int32_t len;
} search_obj;

int main(){
  int fd, r;
  unsigned long value;
  fd = open("/proc/partb_2", O_RDWR);
  if(fd < 0) return 1;

  // Initialize Integer BST
  unsigned char init[1];
  init[0] = 0xFF;
  r = ioctl(fd, PB2_SET_TYPE, init);

  int a1[9]={17, 22, 7, 36, 2, 13, 21, 7, 24};
	for(int i = 0; i<9; i++)
		r = write(fd, &a1[i], sizeof(int));

  // Get Info
  obj_info info;
  r = ioctl(fd, PB2_GET_INFO, &info);
  printf("In the BST, there are: \n");
  printf("%d nodes with degree 1\n", info.deg1cnt);
  printf("%d nodes with degree 2\n", info.deg2cnt);
  printf("%d nodes with degree 3\n", info.deg3cnt);
  printf("%d is the max depth\n", info.maxdepth);
  printf("%d is the min depth\n", info.mindepth);
  printf("\n");

  // Get Obj
  search_obj obj;
  obj.int_obj = 7;
  r = ioctl(fd, PB2_GET_OBJ, &obj);
  if(obj.found == 0x1){
    printf("ObjectType: %c\n", obj.objtype);
    printf("ObjectValue: %d\n", obj.int_obj);
    printf("Object Found!\n");
  } else {
    printf("Object not found\n");
  }

  // Read the BST node by node
  int32_t number;
  int temp;
  value = 0x02;
  r = ioctl(fd, PB2_SET_ORDER, &value);
  do {
    r = read(fd, &number, sizeof(int32_t));
    temp = (int32_t) number;
    printf("Value is %d\n", temp);
  } while(r>0);

  close(fd);
  return 0;
}
