#include "smalloc.h"

#include <stdio.h>
#include <assert.h>

#define print_bits(x)                                            \
  do {                                                           \
    unsigned long long a__ = (x);                                \
    size_t bits__ = sizeof(x) * 8;                               \
    printf(#x ": ");                                             \
    while (bits__--) putchar(a__ &(1ULL << bits__) ? '1' : '0'); \
    putchar('\n');                                               \
  } while (0)

void
print_array(uint8_t* ptr, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		print_bits(ptr[i]);
	}
}

void
print_status(uint8_t* arr, size_t arr_size)
{
	for (size_t i = 0; i < arr_size; i++) {
		uint8_t byte = arr[i];

		for (int j = 3; j >= 0; j--) {
			// Extract 2 bits
			uint8_t two_bits = (byte >> (6 - 2 * j)) & 0x03; // Shift and mask to get 2 bits

			// Select the corresponding status string
			const char* status = status_str[two_bits];

			// Print or process the status
			printf("%s ", status);
		}
		printf("\n");
	}
}

int
main(int argc, char *argv[])
{
	uint8_t *start = (uint8_t*) page.data;

	// Test 1: First allocation
	size_t sz1 = 8;
	uint8_t *data1 = alloc(sz1);
	size_t chunk1 = ((sz1 / CHUNK_SZ) + ((sz1 % CHUNK_SZ) ? 1 : 0)) * CHUNK_SZ;

	assert(data1 != NULL && "First allocation failed.");
	assert(start == data1 && "The first alloc should start at the top of the page.");
	printf("First allocation successful: chunk size = %ld\n", chunk1);
	print_status(page.status, 4);

	// Test 2: Second allocation
	size_t sz2 = 70;
	uint8_t *data2 = alloc(sz2);
	size_t chunk2 = ((sz2 / CHUNK_SZ) + ((sz2 % CHUNK_SZ) ? 1 : 0)) * CHUNK_SZ;

	assert(data2 != NULL && "Second allocation failed.");
	assert(&data1[chunk1] == data2 && "The second alloc should start right after the first.");
	printf("Second allocation successful: chunk size = %ld\n", chunk2);
	print_status(page.status, 4);

	// Test 3: Third allocation
	size_t sz3 = 80;
	uint8_t *data3 = alloc(sz3);
	size_t chunk3 = ((sz3 / CHUNK_SZ) + ((sz3 % CHUNK_SZ) ? 1 : 0)) * CHUNK_SZ;

	assert(data3 != NULL && "Third allocation failed.");
	assert(&data2[chunk2] == data3 && "The third alloc should start right after the second.");
	printf("Third allocation successful: chunk size = %ld\n", chunk3);
	print_status(page.status, 4);

	// Test 4: Deallocate and reallocate
	dealloc(data2);
	uint8_t *data4 = alloc(sz2);

	assert(data4 != NULL && "Reallocation after deallocation failed.");
	assert(data2 == data4 && "Reallocation should occur at the same place as the deallocated chunk.");
	printf("Reallocation successful after deallocation.\n");
	print_status(page.status, 4);

	// Test 5: Smaller allocation after deallocation
	dealloc(data4);
	size_t sz5 = 20;
	uint8_t *data5 = alloc(sz5);
	size_t chunk5 = ((sz5 / CHUNK_SZ) + ((sz5 % CHUNK_SZ) ? 1 : 0)) * CHUNK_SZ;

	assert(data5 != NULL && "Allocation of smaller size after deallocation failed.");
	assert(data5 == data2 && "Reallocation should occur at the same place as the deallocated chunk.");
	printf("Smaller allocation successful after deallocation: chunk size = %ld\n", chunk5);
	print_status(page.status, 4);

	// Test 6: Allocating after all previous deallocations
	dealloc(data1);
	dealloc(data3);
	uint8_t *data6 = alloc(sz3);

	assert(data6 != NULL && "Allocation after multiple deallocations failed.");
	printf("Allocation successful after multiple deallocations");

	// Print page status (for visual inspection, if needed)
	print_status(page.status, 4);

	return 0;
}

