/* Agustin Blaumann (ab3211) -- CS 214 Project I */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mymalloc.h"

#ifndef MEMLENGTH
#define MEMLENGTH 4096
#endif
#define HEADERSIZE 8
#define ALIGNMENT 8

static union {
    char bytes[MEMLENGTH];
    double not_used;
} heap;
static int initialized;

_Static_assert(sizeof(uint64_t) == HEADERSIZE, "Headers must occupy eight bytes");
_Static_assert(MEMLENGTH >= 16 && MEMLENGTH % ALIGNMENT == 0,
               "Heap length must be a multiple of eight and at least sixteen");

/* Payload size occupies the upper bits; bit zero records allocation status.
 * memcpy avoids imposing an incompatible effective type on the char array. */
static uint64_t read_header(size_t offset)
{
    uint64_t header;
    memcpy(&header, heap.bytes + offset, HEADERSIZE);
    return header;
}

static size_t payload_size(uint64_t header)
{
    return (size_t)(header & ~UINT64_C(7));
}

static void write_header(size_t offset, size_t size, int allocated)
{
    uint64_t header = (uint64_t)size | (uint64_t)allocated;
    memcpy(heap.bytes + offset, &header, HEADERSIZE);
}

static void report_leaks(void)
{
    size_t offset = 0, bytes = 0, objects = 0;
    while (offset < MEMLENGTH) {
        uint64_t header = read_header(offset);
        size_t size = payload_size(header);
        if (header & UINT64_C(1)) {
            bytes += size;
            ++objects;
        }
        offset += HEADERSIZE + size;
    }
    if (objects != 0)
        fprintf(stderr, "mymalloc: %zu bytes leaked in %zu objects.\n",
                bytes, objects);
}

static void initialize(void)
{
    if (!initialized) {
        write_header(0, MEMLENGTH - HEADERSIZE, 0);
        if (atexit(report_leaks) != 0) {
            fprintf(stderr, "mymalloc: Unable to register leak detector\n");
            exit(EXIT_FAILURE);
        }
        initialized = 1;
    }
}

void *mymalloc(size_t size, char *file, int line)
{
    size_t offset = 0, rounded;
    initialize();
    /* Check before rounding, including requests near SIZE_MAX. */
    if (size == 0 || size > MEMLENGTH - HEADERSIZE) {
        fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n",
                size, file, line);
        return NULL;
    }
    rounded = (size + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1);
    while (offset < MEMLENGTH) {
        uint64_t header = read_header(offset);
        size_t available = payload_size(header);
        if (!(header & UINT64_C(1)) && available >= rounded) {
            /* A remainder must hold a header and at least eight data bytes. */
            if (available - rounded >= HEADERSIZE + ALIGNMENT) {
                write_header(offset + HEADERSIZE + rounded,
                             available - rounded - HEADERSIZE, 0);
                available = rounded;
            }
            write_header(offset, available, 1);
            return heap.bytes + offset + HEADERSIZE;
        }
        offset += HEADERSIZE + available;
    }
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n",
            size, file, line);
    return NULL;
}

/* Repeatedly merge neighboring free chunks, reclaiming the intervening header.
 * Allocated headers and payloads are never changed by this traversal. */
static void coalesce(void)
{
    size_t offset = 0;
    while (offset < MEMLENGTH) {
        uint64_t header = read_header(offset);
        size_t size = payload_size(header);
        size_t next = offset + HEADERSIZE + size;
        if (!(header & UINT64_C(1)) && next < MEMLENGTH) {
            uint64_t neighbor = read_header(next);
            if (!(neighbor & UINT64_C(1))) {
                write_header(offset, size + HEADERSIZE + payload_size(neighbor), 0);
                continue;
            }
        }
        offset = next;
    }
}

void myfree(void *ptr, char *file, int line)
{
    size_t offset = 0;
    initialize();
    /* Only exact live payload addresses are accepted. We never dereference,
     * subtract, or order an untrusted pointer, including NULL. */
    while (offset < MEMLENGTH) {
        uint64_t header = read_header(offset);
        size_t size = payload_size(header);
        if (ptr == (void *)(heap.bytes + offset + HEADERSIZE)) {
            if (header & UINT64_C(1)) {
                write_header(offset, size, 0);
                coalesce();
                return;
            }
            break;
        }
        offset += HEADERSIZE + size;
    }
    fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
    exit(2);
}
