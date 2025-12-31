// simple_demo.cc - Quick demo for Amaranth library
#include "amaranth/amaranth.h"

#include <iostream>

using namespace amaranth;

int main() {
  std::cout << "=== Amarantine Simple Demo ===\n\n";

  // Test 1: Simple literal match
  std::cout << "1. Literal match:\n";
  Regex re1("hello");
  std::cout << "   Pattern: \"hello\"\n";
  std::cout << "   Text: \"hello world\"\n";
  MatchResult result;
  std::cout << "   Result: " << (re1.match("hello world", result) ? "MATCH" : "NO MATCH") << "\n\n";

  // Test 2: Digit matching
  std::cout << "2. Digit matching:\n";
  Regex re2(R"(\d+)");
  std::cout << "   Pattern: \"\\d+\"\n";
  std::cout << "   Text: \"test 12345 test\"\n";
  MatchResult result2;
  if (re2.match("test 12345 test", result2)) {
    std::cout << "   Result: MATCH - \"" << result2.matched_text << "\"\n";
  }
  std::cout << "\n";

  // Test 3: Capture groups
  std::cout << "3. Capture groups:\n";
  Regex re3(R"((\d{4})-(\d{2})-(\d{2}))");
  std::cout << "   Pattern: \"(\\d{4})-(\\d{2})-(\\d{2})\"\n";
  std::cout << "   Text: \"2024-01-15\"\n";
  MatchResult result3;
  if (re3.match("2024-01-15", result3)) {
    std::cout << "   Full match: " << result3.matched_text << "\n";
    std::cout << "   Year: " << result3.group(1) << "\n";
    std::cout << "   Month: " << result3.group(2) << "\n";
    std::cout << "   Day: " << result3.group(3) << "\n";
  }
  std::cout << "\n";

  // Test 4: Search all
  std::cout << "4. Search all matches:\n";
  Regex re4(R"(\d+)");
  std::cout << "   Pattern: \"\\d+\"\n";
  std::cout << "   Text: \"a1 b22 c333 d4444\"\n";
  auto matches = re4.searchAll("a1 b22 c333 d4444");
  for (const auto& m : matches) {
    std::cout << "   Found: \"" << m.matched_text << "\"\n";
  }
  std::cout << "\n";

  std::cout << "=== Demo Complete ===";
  return 0;
}
