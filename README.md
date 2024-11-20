# Minimalist Memory Allocator in C (`smalloc`)

This project is a proof-of-concept for a minimalist memory allocator (`smalloc`) designed to allocate and deallocate memory dynamically within a fixed-size memory buffer. It uses a compact metadata system to track memory usage and is capable of functioning without an operating system.

For a detailed explanation of the design, implementation, and use cases, check out the [article on smalloc](https://florealrisso.github.io/articles/mini-alloc.html).

## **Getting Started**

### **Building the Project**

Run the following commands to build and test the allocator:

```bash
# Build the project
make

# Run tests
make test
```

### **Visualizing Memory Dumps**

The tools provided can visualize the `status` array from memory dumps:

- **CLI Viewer**: Build and run the `viewer.c` tool:
  ```bash
  ./viewer dumps/<dump_file>
  ```

- **HTML Viewer**: Open `tools/viewer.html` in your browser and load the dump file.

## **License**

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## **More Information**

For an in-depth look into the allocator, its internals, and examples, visit the [accompanying article](https://florealrisso.github.io/articles/mini-alloc.html).
