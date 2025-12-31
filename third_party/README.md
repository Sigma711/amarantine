# Third Party Libraries for Benchmarking

This directory contains third-party regex libraries used for performance benchmarking.

## Supported Libraries

| Library | Description | Build System | Notes |
|---------|-------------|--------------|-------|
| **RE2** | Google's fast, safe regex engine | CMake + Ninja | DFA-based, no backtracking |
| **PCRE2** | Perl-Compatible Regular Expressions | CMake + Ninja | Full Perl regex support |
| **CTRE** | Compile-Time Regular Expressions | Header-only | Compile-time pattern matching |
| **Hyperscan** | Intel SIMD-accelerated regex | CMake + Ninja | Multi-pattern matching (optional) |

## Quick Setup

Run the fetch script from the project root:

```bash
./scripts/fetch_libs.sh
```

Or use the CMake target:

```bash
cmake -S . -B build -G Ninja
ninja -C build fetch-third-party
```

## Manual Setup

### RE2
```bash
cd third_party
git clone --depth 1 --branch 2023-11-01 https://github.com/google/re2.git
cd re2/obj
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -G Ninja
ninja
```

### PCRE2
```bash
cd third_party
git clone --depth 1 --branch pcre2-10.42 https://github.com/PCRE2Project/pcre2.git
cd pcre2/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -G Ninja
ninja
```

### CTRE
```bash
cd third_party
git clone --depth 1 https://github.com/hanickadot/compile-time-regular-expressions.git ctre
# CTRE is header-only, no build required
```

## Usage with CMake

The main CMakeLists.txt will automatically detect these libraries in the third_party directory.
To force using system-installed versions instead:

```cmake
cmake -DUSE_SYSTEM_RE2=ON -DUSE_SYSTEM_PCRE2=ON -S . -B build -G Ninja
```

## Building the Benchmark

```bash
cmake -S . -B build -G Ninja
ninja -C build benchmark
./build/benchmark
```

Available regex engines in benchmark:
- **Amarantine** (built-in)
- **std::regex** (C++ Standard Library)
- **RE2** (if present in third_party)
- **PCRE2** (if present in third_party)
- **CTRE** (if present in third_party)
