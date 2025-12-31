# Contributing to Amarantine

Thank you for your interest in contributing to Amaranth!

## Code Style

This project follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

### Naming Conventions

- **Files**: `snake_case.cc`, `snake_case.h`
- **Classes**: `PascalCase`
- **Functions**: `PascalCase`
- **Variables**: `snake_case`
- **Member variables**: `snake_name_`
- **Constants**: `kPascalCase`
- **Macros**: `PASCAL_CASE`

### Formatting

Run `clang-format` before committing:

```bash
clang-format -i --style=Google include/amaranth/*.h tests/*.cc
```

### Include Order

1. Related header
2. C system headers
3. C++ system headers
4. Other library headers
5. Project headers

## Pull Request Process

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Write tests for your changes
4. Ensure all tests pass
5. Format your code
6. Submit a pull request

## Testing

### Using CMake (Recommended)

```bash
# Configure the project
cmake -G Ninja -B build -S .

# Run all tests
cmake --build build --target check

# Or run individual tests
./build/test_simple
./build/test_compile
./build/test_debug
./build/test_bytecode
./build/test_trace

# Run benchmarks
./build/benchmark

# Run examples
./build/simple_demo
./build/amarantine_demo
```

### Manual Build

```bash
# Build tests
clang++ -std=c++17 -O3 -march=native -Iinclude tests/*.cc -o test_runner
./test_runner

# Run benchmarks
clang++ -std=c++17 -O3 -march=native -Iinclude benchmarks/benchmark.cc -o benchmark
./benchmark
```

## Issues

When reporting bugs, please include:
- Minimal reproduction code
- Expected behavior
- Actual behavior
- Compiler and OS version

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
