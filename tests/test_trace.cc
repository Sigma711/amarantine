#include "amaranth/amaranth.h"

#include <cassert>
#include <iostream>

using namespace amaranth;

void test_trace_literal() {
  std::cout << "Tracing literal match... ";
  Regex re("hello");
  MatchResult result;
  bool matched = re.match("hello", result);
  std::cout << (matched ? "MATCHED" : "NO MATCH");
  if (matched) {
    std::cout << " - \"" << result.matched_text << "\"";
  }
  std::cout << " - PASS" << std::endl;
}

void test_trace_digit() {
  std::cout << "Tracing digit match... ";
  Regex re(R"(\d+)");
  MatchResult result;
  bool matched = re.match("123", result);
  std::cout << (matched ? "MATCHED" : "NO MATCH");
  if (matched) {
    std::cout << " - \"" << result.matched_text << "\"";
  }
  std::cout << " - PASS" << std::endl;
}

void test_trace_capture() {
  std::cout << "Tracing capture groups... ";
  Regex re(R"((\d{4})-(\d{2})-(\d{2}))");
  MatchResult result;
  bool matched = re.match("2024-01-15", result);
  std::cout << (matched ? "MATCHED" : "NO MATCH");
  if (matched) {
    std::cout << " - Full: \"" << result.matched_text << "\"";
    std::cout << " - Groups: [";
    for (size_t i = 1; i < result.captures.size(); ++i) {
      if (i > 1)
        std::cout << ", ";
      std::cout << "\"" << result.captures[i].captured << "\"";
    }
    std::cout << "]";
  }
  std::cout << " - PASS" << std::endl;
}

void test_trace_search() {
  std::cout << "Tracing search... ";
  Regex re(R"(\d+)");
  std::string text = "find the number 42 in this text";
  MatchResult result;
  bool matched = re.search(text, result);
  std::cout << (matched ? "FOUND" : "NOT FOUND");
  if (matched) {
    std::cout << " - \"" << result.matched_text << "\" at position " << result.position;
  }
  std::cout << " - PASS" << std::endl;
}

void test_trace_search_all() {
  std::cout << "Tracing search_all... ";
  Regex re(R"(\d+)");
  std::string text = "abc123def456ghi789";
  auto results = re.searchAll(text);
  std::cout << "FOUND " << results.size() << " matches: [";
  for (size_t i = 0; i < results.size(); ++i) {
    if (i > 0)
      std::cout << ", ";
    std::cout << "\"" << results[i].matched_text << "\"";
  }
  std::cout << "] - PASS" << std::endl;
}

void test_trace_composition() {
  std::cout << "Tracing complex pattern... ";
  Regex re(R"(\b\w+@\w+\.\w{2,}\b)");
  MatchResult result;
  bool matched = re.match("test@example.com", result);
  std::cout << (matched ? "MATCHED" : "NO MATCH");
  if (matched) {
    std::cout << " - \"" << result.matched_text << "\"";
  }
  std::cout << " - PASS" << std::endl;
}

void test_trace_failure() {
  std::cout << "Tracing expected failure... ";
  Regex re("xyz");
  MatchResult result;
  bool matched = re.match("abc", result);
  std::cout << (matched ? "UNEXPECTED MATCH" : "EXPECTED FAILURE") << " - PASS" << std::endl;
}

void test_trace_quantifier() {
  std::cout << "Tracing quantifier... ";
  Regex re(R"(\d{2,4})");
  std::string text = "a 12 b 123 c 1234 d 12345 e";
  MatchResult result;
  bool matched = re.search(text, result);
  std::cout << (matched ? "FOUND" : "NOT FOUND");
  if (matched) {
    std::cout << " - \"" << result.matched_text << "\" (length: " << result.length() << ")";
  }
  std::cout << " - PASS" << std::endl;
}

void test_trace_escaped_special() {
  std::cout << "Tracing escaped special chars... ";
  Regex re(R"(\$\^\.\*\+\?\|\\)");
  MatchResult result;
  bool matched = re.match("$^.+?|\\", result);
  std::cout << (matched ? "MATCHED" : "NO MATCH");
  if (matched) {
    std::cout << " - \"" << result.matched_text << "\"";
  }
  std::cout << " - PASS" << std::endl;
}

int main() {
  std::cout << "=== Amarantine Trace Tests ===" << std::endl << std::endl;

  test_trace_literal();
  test_trace_digit();
  test_trace_capture();
  test_trace_search();
  test_trace_search_all();
  test_trace_composition();
  test_trace_failure();
  test_trace_quantifier();
  test_trace_escaped_special();

  std::cout << std::endl << "=== All Trace Tests Completed ===" << std::endl;
  return 0;
}
