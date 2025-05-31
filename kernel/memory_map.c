#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define TASK_COUNT 65
#define MAPPING    1000

typedef struct {
    int ID;
    int task_ports[TASK_COUNT];
} memory_block_t;

int main() {
    size_t mem_size = sizeof(memory_block_t) * MAPPING;

    memory_block_t *shared_mem = mmap(NULL, mem_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    int current = 0;

    for (int i = 0; i < MAPPING; i++) {
        shared_mem[i].ID = i;
        for (int j = 0; j < TASK_COUNT; j++) {
            shared_mem[i].task_ports[j] = current++;
        }
        current = shared_mem[i].task_ports[TASK_COUNT - 1] + 65;
    }

    for (int i = 0; i < 3; i++) {
        printf("ID: %d -> ", shared_mem[i].ID);
        for (int j = 0; j < TASK_COUNT; j++) {
            printf("%d ", shared_mem[i].task_ports[j]);
        }
        printf("\n");
    }


    return 0;
}

