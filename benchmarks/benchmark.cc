#include "amaranth/amaranth.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

// Try to include other regex libraries
#define USE_STD_REGEX 1
#ifdef USE_STD_REGEX
#include <regex>
#endif

// RE2 support
#ifdef HAVE_RE2
#include <re2/re2.h>
#endif

// PCRE2 support - must define PCRE2_CODE_UNIT_WIDTH (8 for UTF-8)
#ifdef HAVE_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

// CTRE support (Compile Time Regular Expression)
#ifdef HAVE_CTRE
#include "ctre.hpp"
#endif

using namespace amaranth;

// Timer utility
class Timer {
 public:
  Timer() : start_(std::chrono::high_resolution_clock::now()) {}

  double elapsed_ms() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - start_).count();
  }

  void reset() {
    start_ = std::chrono::high_resolution_clock::now();
  }

 private:
  std::chrono::high_resolution_clock::time_point start_;
};

// Benchmark result for multiple libraries
struct BenchmarkResult {
  std::string name;
  double amarantine_time_ms = 0;
  double std_time_ms = 0;
  double re2_time_ms = 0;
  double pcre2_time_ms = 0;
  double ctre_time_ms = 0;

  void print() const {
    auto format_time = [](double ms) -> std::string {
      if (ms < 0.001) {
        double ns = ms * 1000000.0;
        return std::to_string(static_cast<long long>(ns)) + " ns";
      } else if (ms < 0.01) {
        return std::to_string(static_cast<int>(ms * 1000)) + " us";
      } else if (ms < 1) {
        return std::to_string(ms).substr(0, std::to_string(ms).find('.') + 3) + " ms";
      } else {
        return std::to_string(ms).substr(0, std::to_string(ms).find('.') + 2) + " ms";
      }
    };

    std::cout << "\n  " << std::setw(30) << std::left << name;
    std::cout << " | Amarantine: " << std::setw(12) << format_time(amarantine_time_ms);

#ifdef USE_STD_REGEX
    std::cout << " | std: " << std::setw(12) << format_time(std_time_ms);
    if (std_time_ms > 0 && amarantine_time_ms > 0) {
      double speedup = std_time_ms / amarantine_time_ms;
      std::cout << " (" << std::fixed << std::setprecision(2) << speedup << "x)";
    }
#endif

#ifdef HAVE_RE2
    std::cout << " | RE2: " << std::setw(12) << format_time(re2_time_ms);
    if (re2_time_ms > 0 && amarantine_time_ms > 0) {
      double speedup = amarantine_time_ms / re2_time_ms;
      std::cout << " (" << std::fixed << std::setprecision(2) << speedup << "x)";
    }
#endif

#ifdef HAVE_PCRE2
    std::cout << " | PCRE2: " << std::setw(12) << format_time(pcre2_time_ms);
#endif

#ifdef HAVE_CTRE
    std::cout << " | CTRE: " << std::setw(12) << format_time(ctre_time_ms);
    if (ctre_time_ms > 0 && amarantine_time_ms > 0) {
      double speedup = amarantine_time_ms / ctre_time_ms;
      std::cout << " (" << std::fixed << std::setprecision(2) << speedup << "x)";
    }
#endif

    std::cout << "\n";
  }
};

// Test data generators
std::string generate_email_string(int count) {
  std::string result = "Contact: ";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 25);

  for (int i = 0; i < count; ++i) {
    std::string local;
    int local_len = 5 + (dis(gen) % 5);
    for (int j = 0; j < local_len; ++j) {
      local += static_cast<char>('a' + dis(gen));
    }

    std::string domain;
    int domain_len = 4 + (dis(gen) % 4);
    for (int j = 0; j < domain_len; ++j) {
      domain += static_cast<char>('a' + dis(gen));
    }

    std::string tld = "com";
    if (i % 3 == 0)
      tld = "net";
    else if (i % 3 == 1)
      tld = "org";

    result += local + "@" + domain + "." + tld + " ";
  }
  return result;
}

std::string generate_hex_string() {
  std::string result = "Colors: ";
  const char* hex_colors[] = {"#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF",
                              "#FFA500", "#800080", "#008080", "#FFC0CB", "#FFD700", "#C0C0C0"};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 11);

  for (int i = 0; i < 50; ++i) {
    result.append(hex_colors[dis(gen)]);
    result.push_back(' ');
  }
  return result;
}

std::string generate_ipv4_string(int count) {
  std::string result;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  for (int i = 0; i < count; ++i) {
    result += std::to_string(dis(gen)) + "." + std::to_string(dis(gen)) + "." +
              std::to_string(dis(gen)) + "." + std::to_string(dis(gen)) + " ";
  }
  return result;
}

// Amarantine benchmark
double benchmark_amarantine_match(const std::string& pattern, const std::string& text,
                                  int iterations) {
  try {
    Regex re(pattern);
    MatchResult result;

    // Warmup
    for (int i = 0; i < 10; ++i) {
      re.match(text, result);
    }

    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      re.match(text, result);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_amarantine_search(const std::string& pattern, const std::string& text,
                                   int iterations) {
  try {
    Regex re(pattern);
    MatchResult result;

    // Warmup
    for (int i = 0; i < 10; ++i) {
      re.search(text, result);
    }

    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      re.search(text, result);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

#ifdef USE_STD_REGEX
double benchmark_std_match(const std::string& pattern, const std::string& text, int iterations) {
  try {
    std::regex re(pattern);
    std::smatch smatch;

    // Warmup
    for (int i = 0; i < 10; ++i) {
      std::regex_match(text, smatch, re);
    }

    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      std::regex_match(text, smatch, re);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_std_search(const std::string& pattern, const std::string& text, int iterations) {
  try {
    std::regex re(pattern);
    std::smatch smatch;

    // Warmup
    for (int i = 0; i < 10; ++i) {
      std::regex_search(text, smatch, re);
    }

    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      std::regex_search(text, smatch, re);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}
#endif

#ifdef HAVE_RE2
double benchmark_re2_match(const std::string& pattern, const std::string& text, int iterations) {
  try {
    RE2 re(pattern, RE2::Quiet);
    if (!re.ok())
      return -1;

    // Warmup
    for (int i = 0; i < 10; ++i) {
      RE2::FullMatch(text, re);
    }

    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      RE2::FullMatch(text, re);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_re2_search(const std::string& pattern, const std::string& text, int iterations) {
  try {
    RE2 re(pattern, RE2::Quiet);
    if (!re.ok())
      return -1;
    re2::StringPiece match;

    // Warmup
    for (int i = 0; i < 10; ++i) {
      RE2::PartialMatch(text, re, &match);
    }

    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      RE2::PartialMatch(text, re, &match);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}
#endif

#ifdef HAVE_PCRE2
double benchmark_pcre2_match(const std::string& pattern, const std::string& text, int iterations) {
  try {
    int errorcode;
    PCRE2_SIZE erroroffset;
    pcre2_code* re = pcre2_compile(reinterpret_cast<PCRE2_SPTR>(pattern.c_str()),
                                   PCRE2_ZERO_TERMINATED, 0, &errorcode, &erroroffset, NULL);
    if (!re)
      return -1;

    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, NULL);

    // Warmup
    for (int i = 0; i < 10; ++i) {
      pcre2_match(re, reinterpret_cast<PCRE2_SPTR>(text.c_str()), text.length(), 0, 0, match_data,
                  NULL);
    }

    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      pcre2_match(re, reinterpret_cast<PCRE2_SPTR>(text.c_str()), text.length(), 0, 0, match_data,
                  NULL);
    }
    double elapsed = timer.elapsed_ms() / iterations;

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return elapsed;
  } catch (...) {
    return -1;
  }
}
#endif

#ifdef HAVE_CTRE
// CTRE benchmark - uses constexpr patterns at compile time
// Since patterns must be compile-time, we have specific functions for each test
double benchmark_ctre_literal_match(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{"hello"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_digit_match(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{R"((\d+))"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_word_match(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{R"(\w+)"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_char_class(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{"[aeiou]+"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_negated_class(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{"[^0-9]+"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_email_search(const std::string& text, int iterations) {
  try {
    // Use \S+ instead of [\w.+] to simplify
    static constexpr auto pattern = ctll::fixed_string{R"(\S+@\S+\.[a-zA-Z]+)"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_hex_color(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{R"(#[0-9A-Fa-f]+)"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_ipv4(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{R"([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}

double benchmark_ctre_date_format(const std::string& text, int iterations) {
  try {
    static constexpr auto pattern = ctll::fixed_string{R"([0-9]+-[0-9]+-[0-9]+)"};
    for (int i = 0; i < 10; ++i) {
      ctre::search<pattern>(text);
    }
    Timer timer;
    for (int i = 0; i < iterations; ++i) {
      ctre::search<pattern>(text);
    }
    return timer.elapsed_ms() / iterations;
  } catch (...) {
    return -1;
  }
}
#endif

// Print available libraries
void print_available_libs() {
  std::cout << "Available regex libraries:\n";
  std::cout << "  [ox] Amarantine\n";
#ifdef USE_STD_REGEX
  std::cout << "  [ox] std::regex\n";
#else
  std::cout << "  [--] std::regex (disabled)\n";
#endif
#ifdef HAVE_RE2
  std::cout << "  [ox] RE2\n";
#else
  std::cout << "  [--] RE2 (not found)\n";
  std::cout << "      Run: ./scripts/fetch_libs.sh\n";
#endif
#ifdef HAVE_PCRE2
  std::cout << "  [ox] PCRE2\n";
#else
  std::cout << "  [--] PCRE2 (not found)\n";
  std::cout << "      Run: ./scripts/fetch_libs.sh\n";
#endif
#ifdef HAVE_CTRE
  std::cout << "  [ox] CTRE\n";
#else
  std::cout << "  [--] CTRE (not found)\n";
  std::cout << "      Run: ./scripts/fetch_libs.sh\n";
#endif
}

// Main benchmark
int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  std::cout << "========================================\n";
  std::cout << "  Amarantine Performance Benchmark\n";
  std::cout << "========================================\n\n";

  print_available_libs();
  std::cout << "\n";

  struct TestCase {
    std::string name;
    std::string pattern;
    std::function<std::string()> text_generator;
    int iterations;
    bool search;
  };

  std::vector<TestCase> tests = {
      {"Literal match", "(hello)", []() -> std::string { return "hello world"; }, 10000, false},
      {"Digit match", R"((\d+))", []() -> std::string { return "test 12345"; }, 10000, false},
      {"Word match", R"(\w+)", []() -> std::string { return "hello123"; }, 10000, false},
      {"Character class", R"([aeiou]+)", []() -> std::string { return "aeiou"; }, 10000, false},
      {"Negated class", R"([^0-9]+)", []() -> std::string { return "abc"; }, 10000, false},
      {"Email search", R"([\w.+-]+@[\w.-]+\.[a-zA-Z]{2,})",
       []() -> std::string { return generate_email_string(50); }, 100, true},
      {"Hex color", R"(#[0-9A-Fa-f]{6})", generate_hex_string, 1000, false},
      {"IPv4", R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})",
       []() -> std::string { return "192.168.1.1"; }, 10000, false},
      {"IPv4 search", R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})",
       []() -> std::string { return generate_ipv4_string(100); }, 100, true},
      {"Date format", R"((\d{4})-(\d{2})-(\d{2}))", []() -> std::string { return "2024-01-15"; },
       10000, false},
  };

  std::cout << "=== Pattern Matching Benchmarks ===";
#ifdef USE_STD_REGEX
  std::cout << "                              Amarantine       std";
#else
  std::cout << "                              Amarantine";
#endif
#ifdef HAVE_RE2
  std::cout << "         RE2";
#endif
#ifdef HAVE_PCRE2
  std::cout << "       PCRE2";
#endif
#ifdef HAVE_CTRE
  std::cout << "        CTRE";
#endif
  std::cout << "\n";

  for (const auto& test : tests) {
    BenchmarkResult res;
    res.name = test.name;
    std::string text = test.text_generator();

    if (test.search) {
      res.amarantine_time_ms = benchmark_amarantine_search(test.pattern, text, test.iterations);
#ifdef USE_STD_REGEX
      res.std_time_ms = benchmark_std_search(test.pattern, text, test.iterations);
#endif
#ifdef HAVE_RE2
      res.re2_time_ms = benchmark_re2_search(test.pattern, text, test.iterations);
#endif
#ifdef HAVE_PCRE2
      res.pcre2_time_ms = benchmark_pcre2_match(test.pattern, text, test.iterations);
#endif
#ifdef HAVE_CTRE
      // CTRE for search operations
      if (test.name == "Email search") {
        res.ctre_time_ms = benchmark_ctre_email_search(text, test.iterations);
      } else if (test.name == "IPv4 search") {
        res.ctre_time_ms = benchmark_ctre_ipv4(text, test.iterations);
      }
#endif
    } else {
      res.amarantine_time_ms = benchmark_amarantine_match(test.pattern, text, test.iterations);
#ifdef USE_STD_REGEX
      res.std_time_ms = benchmark_std_match(test.pattern, text, test.iterations);
#endif
#ifdef HAVE_RE2
      res.re2_time_ms = benchmark_re2_match(test.pattern, text, test.iterations);
#endif
#ifdef HAVE_PCRE2
      res.pcre2_time_ms = benchmark_pcre2_match(test.pattern, text, test.iterations);
#endif
#ifdef HAVE_CTRE
      // CTRE for match operations - select based on test name
      if (test.name == "Literal match") {
        res.ctre_time_ms = benchmark_ctre_literal_match(text, test.iterations);
      } else if (test.name == "Digit match") {
        res.ctre_time_ms = benchmark_ctre_digit_match(text, test.iterations);
      } else if (test.name == "Word match") {
        res.ctre_time_ms = benchmark_ctre_word_match(text, test.iterations);
      } else if (test.name == "Character class") {
        res.ctre_time_ms = benchmark_ctre_char_class(text, test.iterations);
      } else if (test.name == "Negated class") {
        res.ctre_time_ms = benchmark_ctre_negated_class(text, test.iterations);
      } else if (test.name == "Hex color") {
        res.ctre_time_ms = benchmark_ctre_hex_color(text, test.iterations);
      } else if (test.name == "IPv4") {
        res.ctre_time_ms = benchmark_ctre_ipv4(text, test.iterations);
      } else if (test.name == "Date format") {
        res.ctre_time_ms = benchmark_ctre_date_format(text, test.iterations);
      }
#endif
    }

    if (res.amarantine_time_ms >= 0) {
      res.print();
    }
  }

  std::cout << "\n========================================\n";
  std::cout << "Benchmark completed!\n";
  std::cout << "========================================\n";

  return 0;
}
