#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#define SMALLOC_IMPLEMENTATION
#define SMALLOC_MEMSET memset
#define MEMORY_SZ 4096
#include "smalloc.h"

int dump_status(int fd, Memory *mem)
{
    return write(fd, mem->status, NB_STATUS);
}

void
test_alloc_1b()
{
    Memory mem = {0};
    int fd = open("dumps/test_alloc_1b.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = 8;
    uint64_t *data = alloc(&mem, sz);

    assert(data != NULL &&  "First allocation failed.");
    assert(mem.data == data &&  "The first alloc should start at the top of the mem.");
    dump_status(fd, &mem);

    close(fd);
}

void
test_alloc_1row()
{
   Memory mem = {0};
    int fd = open("dumps/test_alloc_1row.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = 64;
    uint64_t *data = alloc(&mem, sz);

    assert(data != NULL &&  "First allocation failed.");
    assert(mem.data == data &&  "The first alloc should start at the top of the mem.");
    dump_status(fd, &mem);

    close(fd);
}

void
test_alloc_all()
{
    Memory mem = {0};
    int fd = open("dumps/test_alloc_all.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = NB_STATUS * CHUNK_SZ * 4;
    uint64_t *data = alloc(&mem, sz);

    assert(data != NULL &&  "First allocation failed.");
    assert(mem.data == data &&  "The first alloc should start at the top of the mem.");
    dump_status(fd, &mem);

    close(fd);
}

void
test_alloc_too_big()
{
    Memory mem = {0};
    int fd = open("dumps/test_alloc_too_big.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = NB_STATUS * CHUNK_SZ * 4 + 1;
    uint64_t *data = alloc(&mem, sz);

    assert(data == NULL &&  "Allocation should fail.");
    dump_status(fd, &mem);

    close(fd);
}

void
test_alloc_free()
{
    Memory mem = {0};
    int fd = open("dumps/test_alloc_free.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = 8;
    uint64_t *data = alloc(&mem, sz);

    assert(data != NULL &&  "First allocation failed.");
    assert(mem.data == data &&  "The first alloc should start at the top of the mem.");
    dump_status(fd, &mem);

    dealloc(&mem, data);
    assert(mem.status[0] == 0x00 &&  "The status should be 0x00 after dealloc.");
    dump_status(fd, &mem);

    close(fd);
}

void
test_alloc_free_alloc()
{
    Memory mem = {0};
    int fd = open("dumps/test_alloc_free_alloc.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = 8;
    uint64_t *data = alloc(&mem, sz);

    assert(data != NULL &&  "First allocation failed.");
    assert(mem.data == data &&  "The first alloc should start at the top of the mem.");
    dump_status(fd, &mem);

    dealloc(&mem, data);
    assert(mem.status[0] == 0x00 &&  "The status should be 0x00 after dealloc.");
    dump_status(fd, &mem);

    uint64_t *data2 = alloc(&mem, sz);
    assert(data2 == data &&  "The second alloc should reuse the first allocation.");
    dump_status(fd, &mem);

    close(fd);
}

void
test_alloc_alloc()
{
    Memory mem = {0};
    int fd = open("dumps/test_alloc_alloc.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = 8;
    uint64_t *data = alloc(&mem, sz);

    assert(data != NULL &&  "First allocation failed.");
    assert(mem.data == data &&  "The first alloc should start at the top of the mem.");
    dump_status(fd, &mem);

    size_t sz2 = 8;
    uint64_t *data2 = alloc(&mem, sz2);

    assert(data2 != NULL &&  "Second allocation failed.");
    assert(data + 1 == data2 &&  "The second alloc should start after the first one.");
    dump_status(fd, &mem);

    close(fd);
}

void
test_alloc_alloc_alloc_free_middle()
{
    Memory mem = {0};
    int fd = open("dumps/test_alloc_alloc_alloc_free_middle.bin", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    size_t sz = 8;
    uint64_t *data = alloc(&mem, sz);

    assert(data != NULL &&  "First allocation failed.");
    assert(mem.data == data &&  "The first alloc should start at the top of the mem.");
    dump_status(fd, &mem);

    size_t sz2 = 16;
    uint64_t *data2 = alloc(&mem, sz2);

    assert(data2 != NULL &&  "Second allocation failed.");
    assert(data + 1 == data2 &&  "The second alloc should start after the first one.");
    dump_status(fd, &mem);

    size_t sz3 = 32;
    uint64_t *data3 = alloc(&mem, sz3);
    size_t chunk3 = ((sz3 / CHUNK_SZ) + ((sz3 % CHUNK_SZ) ? 1 : 0)) * CHUNK_SZ;

    assert(data3 != NULL &&  "Third allocation failed.");
    assert(data2 + 2 == data3 &&  "The third alloc should start after the second one.");
    dump_status(fd, &mem);

    dealloc(&mem, data2);
    dump_status(fd, &mem);

    uint64_t * redata2 = alloc(&mem, sz2);
    assert(redata2 != NULL &&  "Second allocation failed.");
    assert(redata2 == data2 &&  "The second alloc should reuse the first allocation.");
    dump_status(fd, &mem);

    close(fd);
}

int
main(int argc, char *argv[])
{
    printf("Running tests...\n");
    test_alloc_1b();
    printf("test_alloc_1b passed\n");
    test_alloc_1row();
    printf("test_alloc_1row passed\n");
    test_alloc_all();
    printf("test_alloc_all passed\n");
    test_alloc_too_big();
    printf("test_alloc_too_big passed\n");
    test_alloc_free();
    printf("test_alloc_free passed\n");
    test_alloc_free_alloc();
    printf("test_alloc_free_alloc passed\n");
    test_alloc_alloc();
    printf("test_alloc_alloc passed\n");
    test_alloc_alloc_alloc_free_middle();
    printf("test_alloc_alloc_alloc_free_middle passed\n");

    printf("All tests passed.\n");
    printf("See the dumps in './dumps', NB_STATUS=%d\n", NB_STATUS);
    return 0;
}
