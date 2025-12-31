#include "amaranth/amaranth.h"

#include <cassert>
#include <iostream>

using namespace amaranth;

void test_literal_match() {
  std::cout << "Testing literal match... ";
  Regex re("hello");
  MatchResult result;
  assert(re.match("hello world", result));
  assert(result.matched_text == "hello");
  std::cout << "PASS" << std::endl;
}

void test_digit_match() {
  std::cout << "Testing digit match... ";
  Regex re(R"(\d+)");
  MatchResult result;
  assert(re.match("12345", result));
  assert(result.matched_text == "12345");
  assert(!re.match("abc", result));
  std::cout << "PASS" << std::endl;
}

void test_word_match() {
  std::cout << "Testing word match... ";
  Regex re(R"(\w+)");
  MatchResult result;
  assert(re.match("hello_world", result));
  assert(result.matched_text == "hello_world");
  std::cout << "PASS" << std::endl;
}

void test_character_class() {
  std::cout << "Testing character class... ";
  Regex re("[aeiou]+");
  MatchResult result;
  assert(re.match("aeiou", result));
  assert(result.matched_text == "aeiou");
  std::cout << "PASS" << std::endl;
}

void test_negated_class() {
  std::cout << "Testing negated class... ";
  Regex re("[^0-9]+");
  MatchResult result;
  assert(re.match("abc", result));
  assert(result.matched_text == "abc");
  assert(!re.match("123", result));
  std::cout << "PASS" << std::endl;
}

void test_search() {
  std::cout << "Testing search... ";
  Regex re(R"(\d+)");
  MatchResult result;
  assert(re.search("hello 123 world", result));
  assert(result.matched_text == "123");
  assert(result.position == 6);
  std::cout << "PASS" << std::endl;
}

void test_search_all() {
  std::cout << "Testing search_all... ";
  Regex re(R"(\d+)");
  auto results = re.searchAll("a1b2c3d4");
  assert(results.size() == 4);
  assert(results[0].matched_text == "1");
  assert(results[1].matched_text == "2");
  assert(results[2].matched_text == "3");
  assert(results[3].matched_text == "4");
  std::cout << "PASS" << std::endl;
}

void test_capture_group() {
  std::cout << "Testing capture group... ";
  Regex re(R"((\d+)-(\d+))");
  MatchResult result;
  assert(re.match("123-456", result));
  assert(result.group(0) == "123-456");
  assert(result.group(1) == "123");
  assert(result.group(2) == "456");
  std::cout << "PASS" << std::endl;
}

void test_anchors() {
  std::cout << "Testing anchors... ";
  Regex start_re("^hello");
  assert(start_re.match("hello world"));

  Regex end_re("world$");
  MatchResult result;
  assert(end_re.search("hello world", result));
  assert(result.matched_text == "world");

  std::cout << "PASS" << std::endl;
}

void test_replace() {
  std::cout << "Testing replace... ";
  Regex re(R"(\d+)");
  std::string result = re.replace("abc123def456ghi", "[#]");
  assert(result == "abc[#]def[#]ghi");
  std::cout << "PASS" << std::endl;
}

int main() {
  std::cout << "=== Amarantine Simple Tests ===" << std::endl << std::endl;

  test_literal_match();
  test_digit_match();
  test_word_match();
  test_character_class();
  test_negated_class();
  test_search();
  test_search_all();
  test_capture_group();
  test_anchors();
  test_replace();

  std::cout << std::endl << "=== All Tests Passed! ===" << std::endl;
  return 0;
}
