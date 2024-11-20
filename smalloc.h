// MIT License
//
// Copyright (c) 2024 Floréal Risso
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef SMALLOC_H
#define SMALLOC_H

#ifndef MEMORY_SZ
#error "You must define MEMORY_SZ, e.g. #define MEMORY_SZ 4096"
#endif

#define ALIGN_SZ 8
#define STATUS_SZ 1
#define CHUNK_SZ 8
#define NB_STATUS_OCTET 4

#define COUPLE_STATUS_CHUNK (STATUS_SZ + (CHUNK_SZ * NB_STATUS_OCTET))
#define PADDING_MAX (MEMORY_SZ % COUPLE_STATUS_CHUNK)
#define USEFUL_SZ (MEMORY_SZ - PADDING_MAX)

#define NB_STATUS (USEFUL_SZ / COUPLE_STATUS_CHUNK)
#define NB_CHUNK (NB_STATUS * NB_STATUS_OCTET)
#define PADDING_SZ (PADDING_MAX - (NB_STATUS % ALIGN_SZ))

typedef struct Memory {
	uint8_t status[NB_STATUS]; // equivalent uint2_t[NB_CHUNK]
	#if PADDING_SZ > 0
        uint8_t padding[PADDING_SZ];
    #endif
	uint64_t data[NB_CHUNK];
} Memory;


void* alloc(Memory *mem, size_t sz);
void dealloc(Memory *mem, void *ptr);

#endif // SMALLOC_H

#ifdef SMALLOC_IMPLEMENTATION

#ifndef SMALLOC_MEMSET
#error "You must define SMALLOC_MEMSET, e.g. #define SMALLOC_MEMSET memset"
#endif

#ifndef INLINE
#define INLINE inline __attribute__((always_inline))
#endif

static_assert(sizeof(Memory) == MEMORY_SZ, "Memory size must be MEMORY_SZ");
static_assert(NB_STATUS * NB_STATUS_OCTET == NB_CHUNK, "NB_STATUS * NB_STATUS_OCTET must be equal to NB_CHUNK");
static_assert(NB_CHUNK >= 0, "NB_CHUNK must be >= 0");

typedef enum {
	FREE  = 0b00,
	USED1 = 0b01,
	USED2 = 0b10,
	USED3 = 0b11,
} Status;

const char* status_str[4] = { "FREE", "USED1", "USED2", "USED3" };
const uint8_t status_mask[4] = { 0b11, 0b11 << 2, 0b11 << 4, 0b11 << 6 };
const uint8_t status_clear_mask[4] = { 0b11111100, 0b11110011, 0b11001111, 0b00111111 };
const int tbl[4][4] = {
	{ USED1, USED2, USED1, USED1 },
	{ USED2, USED2, USED3, USED2 },
	{ USED1, USED3, USED1, USED1 },
	{ USED1, USED2, USED1, USED1 },
};

INLINE
uint8_t
make_status(uint8_t status)
{
    return status << 6 | status << 4 | status << 2 | status;
}

INLINE
uint8_t
st_access(uint8_t status, int i)
{
    return (status >> (i * 2)) & 0b11;
}

INLINE
uint8_t
set(uint8_t value, uint8_t status, int i)
{
    return ((value) & status_clear_mask[(i)]) | ((status) << (i * 2));
}

INLINE
uint8_t
set_to(uint8_t value, uint8_t status, int i)
{
    int M_mask = (0b11111111 << ((i) * 2));
	value &= M_mask;
	uint8_t M_status = make_status(status);
	M_status = (~M_mask) & M_status;
	value |= M_status;

	return value;
}

INLINE
uint8_t
set_from(uint8_t value, uint8_t status, int i)
{
    int M_mask = (0b11111111 >> (8 - (i * 2)));
    value &= M_mask;
    uint8_t M_status = make_status(status);
    M_status = (~M_mask) & M_status;
    value |= M_status;

    return value;
}

INLINE
uint8_t
set_part(uint8_t value, uint8_t status, int start, int end)
{
    int M_mask = (0b11111111 << (end * 2)) | (0b11111111 >> (8 - (start * 2)));
    value &= M_mask;
    uint8_t M_status = make_status(status);
    M_status = (~M_mask) & M_status;
    value |= M_status;

    return value;
}

static void
status_memset(uint8_t* ptr, Status status, size_t istart, size_t jstart, size_t iend, size_t jend)
{
	if (istart == iend) {
    	ptr[istart] = set_part(ptr[istart], status, jstart, jend + 1);
        return;
	}

	ptr[istart] = set_from(ptr[istart], status, jstart);
	if (istart + 1 <= iend - 1) {
		SMALLOC_MEMSET(&ptr[istart + 1], make_status(status), (iend - istart) - 1);
	}
	ptr[iend] = set_to(ptr[iend], status, jend + 1);
}

// --------------------

void*
alloc(Memory *mem, size_t sz)
{
	if (!sz) {
		return 0;
	}

	size_t nb_chunk = (sz / CHUNK_SZ) + ((sz % CHUNK_SZ) && 1);
	int istart = -1, iend = -1;
	int jstart = -1, jend = -1;

	// search empty space
	int count = 0, i = 0, subidx = 0;

#define VERIFY(_status, _j) \
	sub_status = st_access(_status, _j); \
	if (sub_status == FREE) { \
		if (count == 0) { \
			istart = i; \
			jstart = _j; \
		} \
		count++; \
		if (count == nb_chunk) { \
			iend = i; \
			jend = _j; \
			break; \
		} \
	} else { \
		count = 0; \
	}

	while (i < NB_STATUS) {
		uint8_t status = mem->status[i];
		Status sub_status;
		VERIFY(status, 0);
		VERIFY(status, 1);
		VERIFY(status, 2);
		VERIFY(status, 3);
		i++;
	}
#undef VERIFY

	if (iend == -1 || nb_chunk != count) {
		return 0;
	}

	if (iend > NB_STATUS) {
	    return 0;
	}

	int next = 0;
	if (iend == NB_STATUS) {
		next = mem->status[iend + 1];
	}

	int prev = 0;
	if (istart != 0 && jstart != 0) {
		prev = st_access(mem->status[istart], (jstart - 1));
	} else if (istart != 0) {
		prev = st_access(mem->status[istart - 1], 3);
	} else {
		prev = st_access(mem->status[istart], (jstart - 1));
	}

	Status sub_status = tbl[prev][next];
	status_memset(mem->status, sub_status, istart, jstart, iend, jend);

	return &mem->data[((istart * 4) + jstart)];
}


void
dealloc(Memory *mem, void *ptr)
{
	int idx = (uint64_t*)ptr - mem->data;
	int istart = idx / 4;
	int jstart = idx % 4;

	Status status = (Status) st_access(mem->status[istart], jstart);

#define IFSET_CLEAR(_i, _j)                             \
do {                                                    \
    int x = st_access(mem->status[_i], _j);               \
	if (x != status) return;                            \
	mem->status[_i] = set(mem->status[_i], FREE, _j); \
} while (0)


	for (int j = jstart; j < 4; j++) {
		int x;
		IFSET_CLEAR(istart, j);
	}

	for (int i = istart + 1; i < NB_STATUS; i++) {
		IFSET_CLEAR(i, 0);
		IFSET_CLEAR(i, 1);
		IFSET_CLEAR(i, 2);
		IFSET_CLEAR(i, 3);
	}
#undef IFSET_CLEAR
}

#endif // SMALLOC_IMPLEMENTATION
