#include "amaranth/amaranth.h"

#include <cassert>
#include <chrono>
#include <iostream>

using namespace amaranth;

void test_bytecode_compilation() {
  std::cout << "Testing bytecode compilation... ";
  Regex re("abc");
  // If compilation succeeded without throwing, we're good
  assert(re.isCompiled());
  std::cout << "PASS" << std::endl;
}

void test_digit_bytecode() {
  std::cout << "Testing digit bytecode... ";
  Regex re(R"(\d+)");
  MatchResult result;
  assert(re.match("123", result));
  assert(result.matched_text == "123");
  std::cout << "PASS" << std::endl;
}

void test_range_bytecode() {
  std::cout << "Testing range bytecode... ";
  Regex re("[a-z]+");
  MatchResult result;
  assert(re.match("hello", result));
  assert(result.matched_text == "hello");
  std::cout << "PASS" << std::endl;
}

void test_concat_bytecode() {
  std::cout << "Testing concat bytecode... ";
  Regex re("ab");
  MatchResult result;
  assert(re.match("ab", result));
  assert(!re.match("a", result));
  assert(!re.match("b", result));
  std::cout << "PASS" << std::endl;
}

void test_alternate_bytecode() {
  std::cout << "Testing alternate bytecode... ";
  Regex re("a|b");
  MatchResult result;
  assert(re.match("a", result));
  assert(re.match("b", result));
  std::cout << "PASS" << std::endl;
}

void test_capture_bytecode() {
  std::cout << "Testing capture bytecode... ";
  Regex re("(abc)");
  MatchResult result;
  assert(re.match("abc", result));
  assert(result.captures.size() >= 1);
  std::cout << "PASS" << std::endl;
}

void test_repetition_bytecode() {
  std::cout << "Testing repetition bytecode... ";
  Regex re("a{3}");
  MatchResult result;
  assert(re.match("aaa", result));
  assert(!re.match("aa", result));
  std::cout << "PASS" << std::endl;
}

void test_instrument() {
  std::cout << "Testing instrumentation... ";
  // Simple timing test
  Regex re(R"(\d{4}-\d{2}-\d{2})");
  auto start = std::chrono::high_resolution_clock::now();
  MatchResult result;
  for (int i = 0; i < 1000; ++i) {
    re.match("2024-01-15", result);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "PASS (" << duration.count() << " µs for 1000 matches)" << std::endl;
}

int main() {
  std::cout << "=== Amarantine Bytecode Tests ===" << std::endl << std::endl;

  test_bytecode_compilation();
  test_digit_bytecode();
  test_range_bytecode();
  test_concat_bytecode();
  test_alternate_bytecode();
  test_capture_bytecode();
  test_repetition_bytecode();
  test_instrument();

  std::cout << std::endl << "=== All Bytecode Tests Passed! ===" << std::endl;
  return 0;
}
