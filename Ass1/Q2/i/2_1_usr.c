#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
	int fd;
	unsigned number;
	fd = open("/proc/partb_1", O_RDWR);
	if(fd < 0) return 1;

	// Sorting string arrays
	unsigned char init[2];
	init[0] = 0xF0;
	init[1] = 5;
	write(fd, init, 2*sizeof(char));

	char value[10];
	char a[5][10]={"Hello", "bye", "Lovish", "Spandan", "Sandip"};
	int r;
	for(int i = 0; i<5; i++)
		r = write(fd, a[i], 10);
	for(int i = 0; i<5; i++){
		r = read(fd, value, 10);
		printf("%s ",value);
	}
	printf("\n");

	// Sorting int arrays
	init[0]  = 0xFF;
	init[1] = 5;
	write(fd, init, 2*sizeof(unsigned char));
	int value1;
	int a1[5]={17, 22, 7, 36, 2};
	for(int i = 0; i<5; i++)
		r = write(fd, &a1[i], sizeof(int));
	for(int i = 0; i<5; i++){
		r = read(fd, &value1, sizeof(int));
		printf("%d ", value1);
	}
	printf("\n");

  close(fd);
  return 0;
}
