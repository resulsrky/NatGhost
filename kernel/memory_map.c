#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <string.h>


#define OVERALL_TASK 65536
#define SHARE_NAME "/shrmem"

int main (int arc, char **argv){
	if(arc < 2) {
		fprintf(stderr,"You should declare the VM count "); 
		exit(EXIT_FAILURE);
	}	
typedef struct{
	int ID;
	int task_count;
	int task_list[];
}memory_block;

	int vm_count = atoi(argv[1]);if(vm_count <= 0) return -1;
	int each_task = OVERALL_TASK/vm_count;
	int remain_tasks = OVERALL_TASK % vm_count;
	
	size_t block_size = sizeof(memory_block) + each_task*sizeof(int);
	size_t total_size = block_size*vm_count;
	
	int fd = shm_open(SHARE_NAME, O_CREAT | O_RDWR | O_TRUNC, 0666);

	if(fd == -1){
   		 perror("shm_open");
    		return -1;
		}

	
	int trun = ftruncate(fd, total_size);if(trun == -1)return -1;

	void *shared_mem = mmap(NULL, total_size, 
			PROT_READ |
			PROT_WRITE,
			MAP_SHARED, 
			fd,
			0);
	
	if(shared_mem == MAP_FAILED)exit(EXIT_FAILURE);

	int current = 0;
	for(int i = 0; i < vm_count; i++){
		memory_block *block = (memory_block *)( (char *)shared_mem
							   + i*block_size);
		block->ID  = i;
		block->task_count = each_task;
		for(int j = 0; j < each_task; j++){
			block->task_list[j] = ++current;
			}
		printf("VM[%d] written (%d ports)\n", block->ID, block->task_count);
	
	}
			
    printf("Shared memory written. It will persist in /dev/shm/%s\n", SHARE_NAME + 1);

}
