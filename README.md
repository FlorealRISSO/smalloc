# smalloc

smalloc is a proof-of-concept (POC) memory allocator that demonstrates an approach to memory management. The key idea is to reserve the beginning of a memory page for storing the status of allocations within that page, while using the rest of the page for the actual allocations.

## Project Structure

```
smalloc/
├── LICENSE
├── Makefile
├── smalloc.h
└── test.c
```

- `smalloc.h`: Contains the implementation of the allocator
- `test.c`: Main file with tests to demonstrate the allocator's functionality
- `Makefile`: Build configuration
- `LICENSE`: MIT License file

## Key Concepts

### Page Layout

The allocator divides a memory page into two main sections:
1. Status Array: Located at the beginning of the page
2. Data Area: The rest of the page used for actual allocations

### Status Representation

The status of each allocation unit is represented using a 2-bit encoding:

- `00`: FREE
- `01`: USED1
- `10`: USED2
- `11`: USED3

This compact representation allows for efficient status tracking of the page's contents.

### Allocation Strategy

The allocator searches for contiguous free space in the status array to find suitable locations for new allocations. It uses bitwise operations to efficiently manipulate and query the status bits.

### Deallocation

When memory is freed, the allocator updates the status bits accordingly, marking the freed chunks as available for future allocations.

## Key Features

1. Efficient use of memory by storing allocation metadata compactly
2. Bitwise operations for fast status checks and updates
3. Support for variable-sized allocations
4. Designed with a focus on memory efficiency and performance

## Building and Testing

To build and run the tests:

```bash
make
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Limitations and Future Work

- This is a proof-of-concept and may not be suitable for production use without further development and testing.
- The current implementation is limited to a single page. Future versions could extend this to multiple pages.
- Performance benchmarking and optimization could be areas for future improvement.

## Contributing

Contributions to improve smalloc are welcome. Please feel free to submit issues or pull requests on the project repository.
