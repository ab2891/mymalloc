/* Agustin Blaumann (ab3211) -- CS 214 Project I */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "mymalloc.h"

#define RUNS 50
#define OBJECTS 120

static void *allocate(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "memgrind: workload allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void task1(void)
{
    const size_t sizes[] = {8, 16, 32, 64, 128, 512, 1024};
    void *objects[7];
    int i;
    for (i = 0; i < 7; ++i)
        objects[i] = allocate(sizes[i]);
    for (i = 6; i >= 0; --i)
        free(objects[i]);
}

static void task2(void)
{
    void *objects[OBJECTS];
    int i;
    for (i = 0; i < OBJECTS; ++i)
        objects[i] = allocate(1);
    for (i = 0; i < OBJECTS; ++i)
        free(objects[i]);
}

static void task3(void)
{
    void *objects[OBJECTS];
    int allocations = 0, live = 0;
    while (allocations < OBJECTS) {
        if (rand() % 2 == 0) {
            objects[live++] = allocate(1);
            ++allocations;
        } else if (live > 0) {
            int index = rand() % live;
            free(objects[index]);
            objects[index] = objects[--live];
        }
    }
    while (live > 0)
        free(objects[--live]);
}

/* Make alternating holes, refill them, then free each pair out of order. */
static void task4(void)
{
    void *objects[80];
    int i;
    for (i = 0; i < 80; ++i)
        objects[i] = allocate(24);
    for (i = 0; i < 80; i += 2)
        free(objects[i]);
    for (i = 0; i < 80; i += 2)
        objects[i] = allocate(16);
    for (i = 79; i >= 1; i -= 2) {
        free(objects[i]);
        free(objects[i - 1]);
    }
}

/* Random slot replacement with mixed sizes and a final shuffled release.
 * At most 32 * (48 + 8) = 1792 bytes are live; fragmentation also fits. */
static void task5(void)
{
    void *objects[32] = {NULL};
    int order[32];
    int i;
    for (i = 0; i < 600; ++i) {
        int index = rand() % 32;
        if (objects[index] != NULL)
            free(objects[index]);
        objects[index] = allocate((size_t)(rand() % 48 + 1));
        memset(objects[index], index, 1);
    }
    for (i = 0; i < 32; ++i)
        order[i] = i;
    for (i = 31; i > 0; --i) {
        int j = rand() % (i + 1);
        int saved = order[i];
        order[i] = order[j];
        order[j] = saved;
    }
    for (i = 0; i < 32; ++i)
        if (objects[order[i]] != NULL)
            free(objects[order[i]]);
}

static void workload(void)
{
    task1();
    task2();
    task3();
    task4();
    task5();
}

int main(void)
{
    struct timeval start, end;
    double elapsed;
    int run;
    srand(3211);
    if (gettimeofday(&start, NULL) != 0) {
        perror("gettimeofday");
        return EXIT_FAILURE;
    }
    for (run = 0; run < RUNS; ++run)
        workload();
    if (gettimeofday(&end, NULL) != 0) {
        perror("gettimeofday");
        return EXIT_FAILURE;
    }
    elapsed = (double)(end.tv_sec - start.tv_sec) * 1000000.0
            + (double)(end.tv_usec - start.tv_usec);
    printf("Average workload time over %d runs: %.2f microseconds\n",
           RUNS, elapsed / RUNS);
    return EXIT_SUCCESS;
}
