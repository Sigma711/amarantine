#include "amaranth/amaranth.h"

#include <cassert>
#include <iostream>

using namespace amaranth;

void test_escapes() {
  std::cout << "Testing escape sequences... ";
  {
    Regex re(R"(\t)");
    MatchResult result;
    assert(re.match("\t", result));
  }
  {
    Regex re(R"(\n)");
    MatchResult result;
    assert(re.match("\n", result));
  }
  {
    Regex re(R"(\r)");
    MatchResult result;
    assert(re.match("\r", result));
  }
  std::cout << "PASS" << std::endl;
}

void test_digit_escape() {
  std::cout << "Testing \\d escape... ";
  Regex re(R"(\d)");
  MatchResult result;
  assert(re.match("5", result));
  assert(!re.match("a", result));
  std::cout << "PASS" << std::endl;
}

void test_word_escape() {
  std::cout << "Testing \\w escape... ";
  Regex re(R"(\w)");
  MatchResult result;
  assert(re.match("a", result));
  assert(re.match("5", result));
  assert(re.match("_", result));
  assert(!re.match(" ", result));
  std::cout << "PASS" << std::endl;
}

void test_space_escape() {
  std::cout << "Testing \\s escape... ";
  Regex re(R"(\s)");
  MatchResult result;
  assert(re.match(" ", result));
  assert(re.match("\t", result));
  assert(!re.match("a", result));
  std::cout << "PASS" << std::endl;
}

void test_dot() {
  std::cout << "Testing dot (.)... ";
  Regex re("a.b");
  MatchResult result;
  assert(re.match("axb", result));
  assert(re.match("a b", result));
  assert(!re.match("ab", result));
  std::cout << "PASS" << std::endl;
}

void test_quantifiers() {
  std::cout << "Testing quantifiers... ";
  {
    Regex re("a*");
    MatchResult result;
    assert(re.match("", result) || re.match("aaa", result));
  }
  {
    Regex re("a+");
    MatchResult result;
    assert(re.match("a", result));
    assert(re.match("aaa", result));
  }
  {
    Regex re("a?");
    MatchResult result;
    assert(re.match("", result) || re.match("a", result));
  }
  std::cout << "PASS" << std::endl;
}

int main() {
  std::cout << "=== Amarantine Debug Tests ===" << std::endl << std::endl;

  test_escapes();
  test_digit_escape();
  test_word_escape();
  test_space_escape();
  test_dot();
  test_quantifiers();

  std::cout << std::endl << "=== All Debug Tests Passed! ===" << std::endl;
  return 0;
}
