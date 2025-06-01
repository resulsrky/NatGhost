#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PORT_SPACE 65536
#define SHM_NAME "/shrmem"
typedef struct {
    int ID;
    int task_count;
    int task_list[]; // Flexible array
} vm_task_block;

int main(int argc, char **argv) {
    if(argc < 3){
        fprintf(stderr, "Usage: %s <vm_count> <vm_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int vm_count = atoi(argv[1]);
    int vm_id = atoi(argv[2]);
    if(vm_count <= 0 || vm_id < 0 || vm_id >= vm_count){
        fprintf(stderr, "Invalid input\n");
        return EXIT_FAILURE;
    }

    int each_taskc = PORT_SPACE / vm_count;
    size_t block_size = sizeof(vm_task_block) + sizeof(int) * each_taskc;
    size_t total_size = block_size * vm_count;

    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if(fd < 0){
        perror("shm_open");
        return EXIT_FAILURE;
    }

    void *shared = mmap(NULL, total_size, PROT_READ, MAP_SHARED, fd, 0);
    if(shared == MAP_FAILED){
        perror("mmap");
        return EXIT_FAILURE;
    }

    vm_task_block *block = (vm_task_block *)((char *)shared + vm_id * block_size);

    printf("VM ID: %d\n", block->ID);
    printf("Task Count: %d\n", block->task_count);
    for(int i = 0; i < block->task_count; i++){
        printf("Task %d: %d\n", i, block->task_list[i]);
    }

    return 0;
} 