#include "amaranth/amaranth.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace amaranth;

void test_simple_compile() {
  std::cout << "Testing simple compile... ";
  Regex re("abc");
  assert(re.isCompiled());
  std::cout << "PASS" << std::endl;
}

void test_complex_compile() {
  std::cout << "Testing complex compile... ";
  Regex re(R"(\d{4}-\d{2}-\d{2})");
  assert(re.isCompiled());
  std::cout << "PASS" << std::endl;
}

void test_pattern_property() {
  std::cout << "Testing pattern property... ";
  std::string pattern = R"(\w+)";
  Regex re(pattern);
  assert(re.pattern() == pattern);
  std::cout << "PASS" << std::endl;
}

void test_copy_construct() {
  std::cout << "Testing copy constructor... ";
  Regex original("test\\d+");
  Regex copy(original);
  MatchResult result;
  assert(copy.match("test123", result));
  std::cout << "PASS" << std::endl;
}

void test_copy_assign() {
  std::cout << "Testing copy assignment... ";
  Regex original("abc");
  Regex target("xyz");
  target = original;
  MatchResult result;
  assert(target.match("abc", result));
  std::cout << "PASS" << std::endl;
}

void test_move_construct() {
  std::cout << "Testing move constructor... ";
  Regex original("hello");
  Regex moved(std::move(original));
  MatchResult result;
  assert(moved.match("hello", result));
  std::cout << "PASS" << std::endl;
}

void test_move_assign() {
  std::cout << "Testing move assignment... ";
  Regex original("world");
  Regex target("test");
  target = std::move(original);
  MatchResult result;
  assert(target.match("world", result));
  std::cout << "PASS" << std::endl;
}

void test_compile_error() {
  std::cout << "Testing compile error handling... ";
  try {
    Regex re("[invalid");  // Unclosed bracket
    std::cout << "FAIL - should have thrown" << std::endl;
  } catch (const RegexError& e) {
    std::cout << "PASS (caught: " << e.what() << ")" << std::endl;
  }
}

void test_multiple_patterns() {
  std::cout << "Testing multiple patterns... ";
  std::vector<Regex> regexes;
  regexes.emplace_back("\\d+");
  regexes.emplace_back("[a-z]+");
  regexes.emplace_back("\\w+");
  assert(regexes[0].isCompiled());
  assert(regexes[1].isCompiled());
  assert(regexes[2].isCompiled());
  std::cout << "PASS" << std::endl;
}

int main() {
  std::cout << "=== Amarantine Compile Tests ===" << std::endl << std::endl;

  test_simple_compile();
  test_complex_compile();
  test_pattern_property();
  test_copy_construct();
  test_copy_assign();
  test_move_construct();
  test_move_assign();
  test_compile_error();
  test_multiple_patterns();

  std::cout << std::endl << "=== All Compile Tests Passed! ===" << std::endl;
  return 0;
}
