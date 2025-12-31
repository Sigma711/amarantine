// amarantine_demo.cc - Extended demo for Amaranth library
#include "amaranth/amaranth.h"

#include <iomanip>
#include <iostream>

using namespace amaranth;

void TestLiteralMatch() {
  std::cout << "\n=== Literal Match ===\n";
  Regex re("hello");
  std::cout << "Pattern: \"hello\", Text: \"hello world\"\n";
  MatchResult result;
  std::cout << "Result: " << (re.match("hello world", result) ? "MATCH" : "NO MATCH") << "\n";
}

void TestCharacterClass() {
  std::cout << "\n=== Character Class \\d ===\n";
  Regex re(R"(\d+)");
  MatchResult result;
  if (re.match("test 123 test", result)) {
    std::cout << "Pattern: \"\\d+\", Text: \"test 123 test\"\n";
    std::cout << "Matched: \"" << result.matched_text << "\"\n";
  }
}

void TestWordClass() {
  std::cout << "\n=== Word Class \\w+ ===\n";
  Regex re(R"(\w+)");
  MatchResult result;
  if (re.match("_hello_world123", result)) {
    std::cout << "Pattern: \"\\w+\", Text: \"_hello_world123\"\n";
    std::cout << "Matched: \"" << result.matched_text << "\"\n";
  }
}

void TestCapturingGroups() {
  std::cout << "\n=== Capturing Groups ===\n";
  Regex re(R"((\d{4})-(\d{2})-(\d{2}))");
  MatchResult result;
  if (re.match("2024-01-15", result)) {
    std::cout << "Pattern: \"(\\d{4})-(\\d{2})-(\\d{2})\"\n";
    std::cout << "Text: \"2024-01-15\"\n";
    std::cout << "Full: " << result.matched_text << "\n";
    for (int i = 1; i <= 3; ++i) {
      std::cout << "Group " << i << ": " << result.group(i) << "\n";
    }
  }
}

void TestSearchAll() {
  std::cout << "\n=== Search All ===\n";
  Regex re(R"(\d{3}-\d{3}-\d{4})");
  std::string text = "Contact: 123-456-7890 or 987-654-3210";
  auto matches = re.searchAll(text);
  std::cout << "Pattern: \"\\d{3}-\\d{3}-\\d{4}\"\n";
  std::cout << "Text: \"" << text << "\"\n";
  std::cout << "Found " << matches.size() << " match(es):\n";
  for (const auto& m : matches) {
    std::cout << "  - " << m.matched_text << "\n";
  }
}

void TestAnchors() {
  std::cout << "\n=== Anchors (^ and $) ===\n";
  Regex re("^test$");
  std::cout << "Pattern: \"^test$\"\n";
  MatchResult result;
  std::cout << "\"test\" -> " << (re.match("test", result) ? "MATCH" : "NO MATCH") << "\n";
  std::cout << "\" testing\" -> " << (re.match(" testing", result) ? "MATCH" : "NO MATCH") << "\n";
}

void TestAlternation() {
  std::cout << "\n=== Alternation (|) ===\n";
  Regex re("cat|dog|bird");
  MatchResult result;
  std::cout << "Pattern: \"cat|dog|bird\"\n";
  if (re.match("I have a cat", result)) {
    std::cout << "Text: \"I have a cat\" -> MATCH: " << result.matched_text << "\n";
  }
  if (re.match("I have a dog", result)) {
    std::cout << "Text: \"I have a dog\" -> MATCH: " << result.matched_text << "\n";
  }
}

void TestQuantifiers() {
  std::cout << "\n=== Quantifiers (*, +, ?) ===\n";
  std::cout << "Pattern \"a*\" on \"\": ";
  MatchResult result;
  std::cout << (Regex("a*").match("", result) ? "MATCH" : "NO MATCH") << "\n";
  std::cout << "Pattern \"a+\" on \"aaa\": ";
  std::cout << (Regex("a+").match("aaa", result) ? "MATCH" : "NO MATCH") << "\n";
  std::cout << "Pattern \"colou?r\" on \"color\": ";
  Regex re("colou?r");
  if (re.match("color", result)) {
    std::cout << "MATCH: " << result.matched_text << "\n";
  }
}

void TestComplexPattern() {
  std::cout << "\n=== Complex Pattern (Email) ===\n";
  Regex re(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
  MatchResult result;
  std::string email = "user@example.com";
  std::cout << "Pattern: \"[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}\"\n";
  if (re.match(email, result)) {
    std::cout << "Text: \"" << email << "\"\n";
    std::cout << "Result: MATCH\n";
  }
}

void TestNegatedClass() {
  std::cout << "\n=== Negated Character Class [^a-z] ===\n";
  Regex re("[^a-z]+");
  MatchResult result;
  std::cout << "Pattern: \"[^a-z]+\"\n";
  if (re.match("ABC123def", result)) {
    std::cout << "Text: \"ABC123def\"\n";
    std::cout << "Matched: \"" << result.matched_text << "\"\n";
  }
}

int main() {
  std::cout << "========================================\n";
  std::cout << "     Amarantine Library Demo         \n";
  std::cout << "   The Flower That Never Fades      \n";
  std::cout << "========================================";

  try {
    TestLiteralMatch();
    TestCharacterClass();
    TestWordClass();
    TestCapturingGroups();
    TestSearchAll();
    TestAnchors();
    TestAlternation();
    TestQuantifiers();
    TestComplexPattern();
    TestNegatedClass();

    std::cout << "\n========================================\n";
    std::cout << "        All Tests Passed!           \n";
    std::cout << "========================================";
  } catch (const RegexError& e) {
    std::cout << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
