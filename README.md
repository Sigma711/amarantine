# Amarantine

> "The flower that never fades, etched in eternal bloom."

!["amarantine" logo](./images/amarantine.jpg)

Amarantine is a high-performance, lightweight C++ regex engine built with a virtual machine-based execution model. Named after the mythical flower that never fades, this library promises enduring performance and elegant design.

## Features

- **Header-only**: Zero external dependencies, just include and compile
- **VM-based execution**: Compiled to bytecode for efficient matching
- **Sub-microsecond response time**: 92-95ns for capture groups
- **Low memory footprint**: <10KB overhead per compiled pattern
- **Standard regex syntax**: Compatible with common regex patterns
- **Safe execution**: No catastrophic backtracking

## Quick Start

```cpp
#include "amaranth/amaranth.h"
#include <iostream>

int main() {
    using namespace amaranth;

    // Compile a pattern
    Regex re(R"(\d{4}-\d{2}-\d{2})");

    // Match
    MatchResult result;
    if (re.match("Date: 2024-01-15", result)) {
        std::cout << "Found: " << result.matched_text << std::endl;
    }

    // Use with capture groups
    Regex date_re(R"((\d{4})-(\d{2})-(\d{2}))");
    if (date_re.match("2024-01-15", result)) {
        std::cout << "Year: " << result.group(1) << std::endl;
        std::cout << "Month: " << result.group(2) << std::endl;
        std::cout << "Day: " << result.group(3) << std::endl;
    }

    // Search all occurrences
    for (auto& match : re.searchAll("2024-01-15, 2023-12-31")) {
        std::cout << match.matched_text << std::endl;
    }

    return 0;
}
```

## Build

### Using CMake + Ninja (Recommended)

```bash
# Configure with Ninja generator (auto-fetches third-party libs)
cmake -S . -B build -G Ninja

# Build all targets
ninja -C build

# Run tests
ninja -C build check

# Run benchmarks (compares against RE2, PCRE2, CTRE, std::regex)
ninja -C build benchmark
./build/benchmark

# Format code
ninja -C build format
ninja -C build check-format

# Fetch third-party benchmark libraries
ninja -C build fetch-third-party
```

### Using your own project

Add to your `CMakeLists.txt`:

```cmake
# Add include directory
include_directories(path/to/amarantine/include)

# Your executable
add_executable(your_program your_file.cc)
```

## Architecture

```
┌─────────────────────────────────────────────────────┐
│              Amarantine Architecture                 │
├─────────────────────────────────────────────────────┤
│  Pattern  →  Lexer  →  Parser  →  Compiler  → Code  │
│                                                 │    │
│  Text  ────────────────────────────────────→  VM    │
│                                                 │    │
│                                              Result   │
└─────────────────────────────────────────────────────┘
```

## Supported Syntax

| Syntax | Description | Example |
|--------|-------------|---------|
| `.` | Any character | `a.b` |
| `\d`, `\D` | Digit / Non-digit | `\d+` |
| `\w`, `\W` | Word / Non-word | `\w+` |
| `\s`, `\S` | Space / Non-space | `\s+` |
| `[abc]` | Character set | `[a-z]` |
| `[^abc]` | Negated set | `[^0-9]` |
| `*`, `+`, `?` | Quantifiers | `a*`, `b+`, `c?` |
| `{n,m}` | Range quantifier | `\d{2,5}` |
| `(abc)` | Capturing group | `(\d+)` |
| `(?:abc)` | Non-capturing group | `(?:abc)` |
| `a\|b` | Alternation | `cat\|dog` |
| `^`, `$` | Anchors | `^start$` |
| `\digit>` | Escape sequences | `\t`, `\n`, `\x41` |

## Performance

Compared to `std::regex` on a variety of workloads:

| Metric | Amarantine | std::regex | Speedup |
|--------|----------|------------|---------|
| Capture groups | 92 ns | 4 µs | 53x |
| Digit matching | 48 ns | 873 ns | 18x |
| Complex patterns | 2 µs | 4-5 µs | 2x |
| Large text (278KB) | 32 ms | 348 ms | 10x |

See `docs/PERFORMANCE_REPORT.md` for detailed benchmarks.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Project Structure

```
amaranth/
├── include/amaranth/    # Public headers (amaranth.h)
├── tests/               # Unit tests
├── benchmarks/          # Performance tests
├── examples/            # Example code
├── docs/                # Documentation
├── third_party/         # Benchmark libraries (RE2, PCRE2, CTRE)
└── scripts/             # Build helper scripts
```

## Inspiration

Named after the mythical **Amaranth** flower, said to bloom eternally and never fade—a symbol of unfading beauty and enduring resilience. Just as this flower defies time, Amarantine delivers performance that stands the test of endless pattern matching.

> "Like the amaranth, my code shall never fade."
