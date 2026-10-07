/* Agustin Blaumann (ab3211) -- CS 214 Project I */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Test failed: %s (%s:%d)\n", \
                #condition, __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void full_heap(void)
{
    unsigned char *p = malloc(4088);
    CHECK(p != NULL);
    memset(p, 0xA5, 4088);
    CHECK(p[0] == 0xA5 && p[4087] == 0xA5);
    free(p);
}

static void alignment_and_data(void)
{
    unsigned char *objects[64];
    size_t sizes[64];
    size_t i, j;
    for (i = 0; i < 64; ++i) {
        sizes[i] = i + 1;
        objects[i] = malloc(sizes[i]);
        CHECK(objects[i] != NULL);
        CHECK((uintptr_t)objects[i] % 8 == 0);
        memset(objects[i], (int)i, sizes[i]);
    }
    for (i = 0; i < 64; ++i)
        for (j = 0; j < sizes[i]; ++j)
            CHECK(objects[i][j] == (unsigned char)i);
    for (i = 0; i < 64; i += 2)
        free(objects[i]);
    for (i = 1; i < 64; i += 2) {
        for (j = 0; j < sizes[i]; ++j)
            CHECK(objects[i][j] == (unsigned char)i);
        free(objects[i]);
    }
    full_heap();
}

static void reuse_and_coalescing(void)
{
    unsigned char *a = malloc(24), *b = malloc(24), *c = malloc(24);
    void *merged;
    CHECK(a && b && c);
    memset(c, 0xFF, 24);
    free(a);
    free(b);
    merged = malloc(56); /* 24 + reclaimed header + 24 */
    CHECK(merged == (void *)a);
    memset(merged, 0, 56);
    CHECK(c[0] == 0xFF && c[23] == 0xFF);
    free(merged);
    free(c);
    full_heap();

    a = malloc(24);
    b = malloc(24);
    c = malloc(24);
    CHECK(a && b && c);
    free(c);
    free(a);
    free(b); /* joins free neighbors on both sides */
    full_heap();

    a = malloc(4080); /* eight bytes left: too small for another chunk */
    CHECK(a != NULL);
    memset(a, 0x5A, 4080);
    free(a);
    full_heap();
}

static void random_data(void)
{
    unsigned char *objects[32] = {NULL};
    size_t sizes[32] = {0};
    unsigned char patterns[32] = {0};
    int operation, i;
    size_t j;
    srand(3211);
    for (operation = 0; operation < 10000; ++operation) {
        int index = rand() % 32;
        for (i = 0; i < 32; ++i)
            if (objects[i] != NULL)
                for (j = 0; j < sizes[i]; ++j)
                    CHECK(objects[i][j] == patterns[i]);
        if (objects[index] != NULL) {
            free(objects[index]);
            objects[index] = NULL;
        } else {
            sizes[index] = (size_t)(rand() % 48 + 1);
            patterns[index] = (unsigned char)rand();
            objects[index] = malloc(sizes[index]);
            CHECK(objects[index] != NULL);
            memset(objects[index], patterns[index], sizes[index]);
        }
    }
    for (i = 0; i < 32; ++i)
        if (objects[i] != NULL)
            free(objects[i]);
    full_heap();
}

/* Diagnostic modes print the expected stderr to stdout for check.sh to compare.
 * __LINE__ + 1 specifies the following macro call's exact source location. */
static void expected_malloc(size_t size, int line)
{
    printf("malloc: Unable to allocate %zu bytes (%s:%d)\n",
           size, __FILE__, line);
}

static void expected_free(int line)
{
    printf("free: Inappropriate pointer (%s:%d)\n", __FILE__, line);
}

static void bounds(void)
{
    void *p;
    expected_malloc(0, __LINE__ + 1);
    p = malloc(0);
    CHECK(p == NULL);
    expected_malloc(4089, __LINE__ + 1);
    p = malloc(4089);
    CHECK(p == NULL);
    expected_malloc(SIZE_MAX, __LINE__ + 1);
    p = malloc(SIZE_MAX);
    CHECK(p == NULL);
    full_heap();
}

static void exhaustion(void)
{
    void *objects[256];
    void *p;
    int i;
    for (i = 0; i < 256; ++i) {
        objects[i] = malloc(1);
        CHECK(objects[i] != NULL);
    }
    expected_malloc(1, __LINE__ + 1);
    p = malloc(1);
    CHECK(p == NULL);
    free(objects[100]);
    p = malloc(1);
    CHECK(p == objects[100]);
    free(p);
    for (i = 0; i < 256; ++i)
        if (i != 100)
            free(objects[i]);
    full_heap();
}

static void fragmentation(void)
{
    void *objects[4], *p;
    int i;
    for (i = 0; i < 4; ++i) {
        objects[i] = malloc(1016);
        CHECK(objects[i] != NULL);
    }
    free(objects[0]);
    free(objects[2]);
    expected_malloc(1500, __LINE__ + 1);
    p = malloc(1500);
    CHECK(p == NULL); /* plenty of total space, but no contiguous fit */
    free(objects[1]);
    p = malloc(3040); /* three chunks plus two reclaimed headers */
    CHECK(p == objects[0]);
    memset(p, 0xCC, 3040);
    free(p);
    free(objects[3]);
    full_heap();
}

int main(int argc, char **argv)
{
    const char *mode = argc == 2 ? argv[1] : "valid";
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [mode]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(mode, "valid") == 0) {
        full_heap();
        alignment_and_data();
        reuse_and_coalescing();
        random_data();
        puts("Correctness tests passed");
    } else if (strcmp(mode, "bounds") == 0) {
        bounds();
    } else if (strcmp(mode, "exhaustion") == 0) {
        exhaustion();
    } else if (strcmp(mode, "fragmentation") == 0) {
        fragmentation();
    } else if (strcmp(mode, "stack") == 0) {
        int x;
        expected_free(__LINE__ + 1);
        free(&x);
        return EXIT_FAILURE;
    } else if (strcmp(mode, "interior") == 0) {
        char *p = malloc(16);
        CHECK(p != NULL);
        expected_free(__LINE__ + 1);
        free(p + 1);
        return EXIT_FAILURE;
    } else if (strcmp(mode, "double") == 0) {
        void *p = malloc(24);
        CHECK(p != NULL);
        free(p);
        expected_free(__LINE__ + 1);
        free(p);
        return EXIT_FAILURE;
    } else if (strcmp(mode, "coalesced-double") == 0) {
        void *a = malloc(24), *b = malloc(24);
        CHECK(a && b);
        free(a);
        free(b);
        expected_free(__LINE__ + 1);
        free(b);
        return EXIT_FAILURE;
    } else if (strcmp(mode, "header") == 0) {
        char *p = malloc(16);
        CHECK(p != NULL);
        expected_free(__LINE__ + 1);
        free(p - 8);
        return EXIT_FAILURE;
    } else if (strcmp(mode, "null") == 0) {
        expected_free(__LINE__ + 1);
        free(NULL);
        return EXIT_FAILURE;
    } else if (strcmp(mode, "leak") == 0) {
        void *a = malloc(1), *b = malloc(9), *c = malloc(24);
        CHECK(a && b && c);
        free(c);
        puts("mymalloc: 24 bytes leaked in 2 objects.");
    } else if (strcmp(mode, "padded-leak") == 0) {
        void *p = malloc(4080);
        CHECK(p != NULL);
        puts("mymalloc: 4088 bytes leaked in 1 objects.");
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
