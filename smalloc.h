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

#include <stdint.h>
#include <string.h>

#define PAGE_SZ 4096
#define CHUNK_SZ 8

#define NB_STATUS 124
#define NB_CHUNK 496

// -------------------- Status
#define MAKE_STATUS(_status) \
	_status << 6 | _status << 4 | _status << 2 | _status \

#define ACCESS(_value, _i) \
	((_value) >> ((_i) * 2)) & 0b11

#define SET(_value, _i, _status) \
	_value = ((_value) & status_clear_mask[(_i)]) | ((_status) << (_i * 2))

#define SET_TO(_value, _i, _status) \
	do { \
		int M_mask = (0b11111111 << ((_i) * 2)); \
		_value &= M_mask; \
		uint8_t M_status = MAKE_STATUS(_status); \
		M_status = (~M_mask) & M_status; \
		_value |= M_status; \
	} while(0)

#define SET_FROM(_value, _i, _status) \
	do { \
		int M_mask = (0b11111111 >> (8 - (_i * 2))); \
		_value &= M_mask; \
		uint8_t M_status = MAKE_STATUS(_status); \
		M_status = (~M_mask) & M_status; \
		_value |= M_status; \
	} while(0)


const char* status_str[4] = { "FREE", "USED1", "USED2", "USED3" };
const uint8_t status_mask[4] = { 0b11, 0b11 << 2, 0b11 << 4, 0b11 << 6 };
const uint8_t status_clear_mask[4] = { 0b11111100, 0b11110011, 0b11001111, 0b00111111 };
typedef enum {
	FREE = 0,
	USED1 = 1,
	USED2 = 2,
	USED3 = 3,
} Status;

static void
status_memset(uint8_t* ptr, Status status, size_t istart, size_t jstart, size_t iend, size_t jend)
{
	if (istart == iend) {
		for (int j = jstart; j <= jend; j++) {
			SET(ptr[istart], j, status);
		}
		return;
	}

	SET_FROM(ptr[istart], jstart, status);
	if (istart + 1 <= iend - 1) {
		memset(&ptr[istart + 1], MAKE_STATUS(status), (iend - istart) - 1);
	}
	SET_TO(ptr[iend], (jend + 1), status);
}

// --------------------
typedef struct Page {
	uint8_t status[NB_STATUS];
	uint32_t next;
	uint64_t data[NB_CHUNK];
} Page;

Page page = {0};


void*
alloc(size_t sz)
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
	sub_status = ACCESS(_status, _j); \
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
		uint8_t status = page.status[i];
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

	static int tbl[4][4] = {
		{ USED1, USED2, USED1, USED1 },
		{ USED2, USED2, USED3, USED2 },
		{ USED1, USED3, USED1, USED1 },
		{ USED1, USED2, USED1, USED1 },
	};


	int next = 0;
	if (iend == NB_STATUS) {
		next = page.status[iend + 1];
	}

	int prev = 0;
	if (istart != 0 && jstart != 0) {
		prev = ACCESS(page.status[istart], (jstart - 1));
	} else if (istart != 0) {
		prev = ACCESS(page.status[istart - 1], 3);
	} else {
		prev = ACCESS(page.status[istart], (jstart - 1));
	}

	Status sub_status = tbl[prev][next];
	status_memset(page.status, sub_status, istart, jstart, iend, jend);

	return &page.data[((istart * 4) + jstart)];
}


void
dealloc(void *ptr)
{
	int idx = (uint64_t*)ptr - page.data;
	int istart = idx / 4;
	int jstart = idx % 4;

	Status status = (Status) ACCESS(page.status[istart], jstart);

#define IFSET_CLEAR(_i, _j) \
	x = ACCESS(page.status[_i], _j); \
	if (x != status) return; \
	SET(page.status[_i], _j, FREE); 			 \

	for (int j = jstart; j < 4; j++) {
		int x;
		IFSET_CLEAR(istart, j);
	}

	for (int i = istart + 1; i < NB_STATUS; i++) {
		int x;
		IFSET_CLEAR(i, 0);
		IFSET_CLEAR(i, 1);
		IFSET_CLEAR(i, 2);
		IFSET_CLEAR(i, 3);
	}
#undef IFSET_CLEAR
}

