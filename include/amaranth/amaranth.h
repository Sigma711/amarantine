#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace amaranth {

// Amarantine - Named after the mythical flower that never fades
// A high-performance, lightweight regex engine using VM-based execution

// ============================================================================
// Forward Declarations
// ============================================================================
class Regex;
class RegexCompiler;
class RegexEngine;

// ============================================================================
// Error Handling
// ============================================================================
class RegexError : public std::runtime_error {
 public:
  const size_t position;

  explicit RegexError(const std::string& msg, size_t pos = 0)
      : std::runtime_error(msg), position(pos) {}
};

// ============================================================================
// Result Structure
// ============================================================================
struct Match {
  size_t start;
  size_t end;
  std::string captured;

  operator bool() const {
    return start != std::string::npos;
  }
};

struct MatchResult {
  bool matched;
  size_t position;
  std::string matched_text;
  std::vector<Match> captures;

  size_t length() const {
    return matched_text.length();
  }
  operator bool() const {
    return matched;
  }
  std::string group(int idx = 0) const {
    if (idx == 0)
      return matched_text;
    if (idx > 0 && idx <= (int)captures.size())
      return captures[idx - 1].captured;
    return "";
  }
  size_t group_start(int idx = 0) const {
    if (idx == 0)
      return position;
    if (idx > 0 && idx <= (int)captures.size())
      return captures[idx - 1].start;
    return std::string::npos;
  }
  size_t group_end(int idx = 0) const {
    if (idx == 0)
      return position + length();
    if (idx > 0 && idx <= (int)captures.size())
      return captures[idx - 1].end;
    return std::string::npos;
  }
};

// ============================================================================
// Virtual Machine Instruction Set
// ============================================================================
enum class Opcode : uint8_t {
  CHAR,          // Match literal character
  ANY,           // Match any character
  RANGE,         // Match character in range [min,max]
  CLASS,         // Match character class
  NOT_CLASS,     // Match negated character class
  CLASS_PRED,    // Match using predicate function (for chars >= 64)
  JUMP,          // Unconditional jump
  SPLIT,         // Split (fork) to two paths
  SAVE,          // Save capture group position
  MATCH,         // Success/accept state
  ANCHOR_START,  // Anchor at start
  ANCHOR_END,    // Anchor at end
  BACKREF,       // Backreference to capture group
};

#pragma pack(push, 1)
struct Instruction {
  Opcode opcode;
  uint32_t operand;
  union {
    char ch;
    struct {
      char lo, hi;
    };
    uint64_t charset;  // For ASCII character classes (low 64 bits)
    struct {
      uint64_t charset_low;   // Characters 0-63
      uint64_t charset_high;  // Characters 64-127
    };
    uint64_t class_type;  // Class type for predicate matches
  };

  Instruction() : opcode(Opcode::CHAR), operand(0), ch(0) {}

  static Instruction Char(char c, uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::CHAR;
    inst.ch = c;
    inst.operand = next;
    return inst;
  }

  static Instruction Any(uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::ANY;
    inst.operand = next;
    return inst;
  }

  static Instruction Range(char lo, char hi, uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::RANGE;
    inst.lo = lo;
    inst.hi = hi;
    inst.operand = next;
    return inst;
  }

  static Instruction Jump(uint32_t target) {
    Instruction inst;
    inst.opcode = Opcode::JUMP;
    inst.operand = target;
    return inst;
  }

  static Instruction Split(uint32_t target1, uint32_t target2) {
    Instruction inst;
    inst.opcode = Opcode::SPLIT;
    inst.operand = target1;
    inst.charset = target2;
    return inst;
  }

  static Instruction Save(uint32_t group) {
    Instruction inst;
    inst.opcode = Opcode::SAVE;
    inst.operand = group;  // next = 0 means just advance
    return inst;
  }

  static Instruction Match() {
    Instruction inst;
    inst.opcode = Opcode::MATCH;
    return inst;
  }

  static Instruction AnchorStart() {
    Instruction inst;
    inst.opcode = Opcode::ANCHOR_START;
    inst.operand = 0;  // Not used - anchor just advances pc
    return inst;
  }

  static Instruction AnchorEnd() {
    Instruction inst;
    inst.opcode = Opcode::ANCHOR_END;
    inst.operand = 0;  // Not used - anchor just advances pc
    return inst;
  }

  static Instruction Class(uint64_t classbits_low, uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::CLASS;
    inst.charset_low = classbits_low;
    inst.charset_high = 0;
    inst.operand = next;
    return inst;
  }

  static Instruction ClassExt(uint64_t classbits_low, uint64_t classbits_high, uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::CLASS;
    inst.charset_low = classbits_low;
    inst.charset_high = classbits_high;
    inst.operand = next;
    return inst;
  }

  static Instruction NotClass(uint64_t classbits_low, uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::NOT_CLASS;
    inst.charset_low = classbits_low;
    inst.charset_high = 0;
    inst.operand = next;
    return inst;
  }

  static Instruction NotClassExt(uint64_t classbits_low, uint64_t classbits_high,
                                 uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::NOT_CLASS;
    inst.charset_low = classbits_low;
    inst.charset_high = classbits_high;
    inst.operand = next;
    return inst;
  }

  static Instruction ClassPred(int class_type, uint32_t next = 1) {
    Instruction inst;
    inst.opcode = Opcode::CLASS_PRED;
    inst.class_type = (uint64_t)class_type;
    inst.operand = next;
    return inst;
  }

  static Instruction Backref(uint32_t group, uint32_t next = 0) {
    Instruction inst;
    inst.opcode = Opcode::BACKREF;
    inst.operand = group | (next << 16);
    return inst;
  }
};
#pragma pack(pop)

// Class type constants for CLASS_PRED instruction
enum ClassType : int { CLASS_DIGIT = 0, CLASS_WORD = 1, CLASS_SPACE = 2 };

// Compile-time constant config for performance
static constexpr size_t MAX_INSTRUCTIONS = 16384;
static constexpr size_t MAX_CAPTURES = 32;
static constexpr size_t VM_STACK_SIZE = 65536;

// ============================================================================
// Character Classes with Bitsets
// ============================================================================
class CharClass {
 public:
  // Digit '0'-'9' -> bits 48-57
  static constexpr uint64_t DIGIT_MASK =
      (1ULL << '0') | (1ULL << '1') | (1ULL << '2') | (1ULL << '3') | (1ULL << '4') |
      (1ULL << '5') | (1ULL << '6') | (1ULL << '7') | (1ULL << '8') | (1ULL << '9');

  // Space: space (32), tab (9), \f (12), \r (13), \n (10), \v (11)
  static constexpr uint64_t SPACE_MASK = (1ULL << ' ') | (1ULL << '\t') | (1ULL << '\f') |
                                         (1ULL << '\r') | (1ULL << '\n') | (1ULL << '\v');

  static inline bool isDigit(char c) {
    return c >= '0' && c <= '9';
  }
  static inline bool isUpper(char c) {
    return c >= 'A' && c <= 'Z';
  }
  static inline bool isLower(char c) {
    return c >= 'a' && c <= 'z';
  }
  static inline bool isAlpha(char c) {
    return isUpper(c) || isLower(c);
  }
  static inline bool isWord(char c) {
    return isAlpha(c) || isDigit(c) || c == '_';
  }
  static inline bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
  }

  // Extended match for characters outside bitmask range
  static inline bool isWordChar(char c) {
    return isAlpha(c) || isDigit(c) || c == '_';
  }

  // Check if bit is set in uint64_t bitmask
  static inline bool inUint64T(uint64_t mask, char c) {
    uint8_t uc = static_cast<uint8_t>(c);
    if (uc < 64) {
      return (mask & (1ULL << uc)) != 0;
    }
    return false;
  }

  // Check if a character class matches (handles chars >= 64)
  static inline bool classMatches(uint64_t mask, char c, int classType) {
    // For \d, \D, \s, \S - use function check
    uint8_t uc = static_cast<uint8_t>(c);
    if (uc < 64) {
      return (mask & (1ULL << uc)) != 0;
    }
    // For chars >= 64, use function check
    switch (classType) {
      case CLASS_DIGIT:
        return isDigit(c);
      case CLASS_WORD:
        return isWordChar(c);
      case CLASS_SPACE:
        return isSpace(c);
      default:
        return false;
    }
  }

  // Extended character class matching (128 bits total)
  static inline bool inClassExt(uint64_t low, uint64_t high, char c) {
    uint8_t uc = static_cast<uint8_t>(c);
    if (uc < 64) {
      return (low & (1ULL << uc)) != 0;
    } else if (uc < 128) {
      return (high & (1ULL << (uc - 64))) != 0;
    }
    return false;
  }
};

// ============================================================================
// Lexer - Tokenizer for Regex Patterns
// ============================================================================
enum class TokenType {
  UNKNOWN,
  LITERAL,
  DOT,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  LBRACKET,
  RBRACKET,
  PIPE,
  STAR,
  PLUS,
  QUESTION,
  CARET,
  DOLLAR,
  ESCAPE,
  BACKREF,
  RANGE,
  COMMA
};

struct Token {
  TokenType type;
  char value;
  size_t position;

  Token(TokenType t, char v = 0, size_t pos = 0) : type(t), value(v), position(pos) {}
};

class Lexer {
 public:
  explicit Lexer(const std::string& pattern) : pattern_(pattern), pos_(0) {}

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(pattern_.length());

    while (pos_ < pattern_.length()) {
      Token tok = nextToken();
      tokens.push_back(tok);
    }

    return tokens;
  }

 private:
  const std::string& pattern_;
  size_t pos_;

  Token nextToken() {
    if (pos_ >= pattern_.length()) {
      return Token(TokenType::UNKNOWN, 0, pos_);
    }

    char c = pattern_[pos_++];
    size_t position = pos_ - 1;

    switch (c) {
      case '.':
        return Token(TokenType::DOT, c, position);
      case '(':
        return Token(TokenType::LPAREN, c, position);
      case ')':
        return Token(TokenType::RPAREN, c, position);
      case '{':
        return Token(TokenType::LBRACE, c, position);
      case '}':
        return Token(TokenType::RBRACE, c, position);
      case '[':
        return Token(TokenType::LBRACKET, c, position);
      case ']':
        return Token(TokenType::RBRACKET, c, position);
      case '|':
        return Token(TokenType::PIPE, c, position);
      case '*':
        return Token(TokenType::STAR, c, position);
      case '+':
        return Token(TokenType::PLUS, c, position);
      case '?':
        return Token(TokenType::QUESTION, c, position);
      case '^':
        return Token(TokenType::CARET, c, position);
      case '$':
        return Token(TokenType::DOLLAR, c, position);
      case '-':
        return Token(TokenType::RANGE, c, position);
      case ',':
        return Token(TokenType::COMMA, c, position);
      case '\\': {
        if (pos_ < pattern_.length()) {
          char esc = pattern_[pos_++];
          return Token(TokenType::ESCAPE, esc, position);
        }
        throw RegexError("Incomplete escape sequence", position);
      }
      default:
        if (c == ' ' || c == '\t') {
          return nextToken();  // Skip whitespace
        }
        return Token(TokenType::LITERAL, c, position);
    }
  }
};

// ============================================================================
// AST Nodes
// ============================================================================
struct ASTNode {
  enum class Type {
    LITERAL,
    CONCAT,
    ALTERNATE,
    REPEAT,
    DOT,
    CLASS,
    NOT_CLASS,
    ANCHOR_START,
    ANCHOR_END,
    GROUP,
    BACKREF
  };

  Type type;
  char ch = 0;
  uint64_t classbits = 0;       // Characters 0-63
  uint64_t classbits_high = 0;  // Characters 64-127 (bit i represents char 64+i)
  int class_type = -1;          // For CLASS_PRED: 0=digit, 1=word, 2=space
  uint32_t minRepeat = 0;
  uint32_t maxRepeat = 0;
  bool greedy = true;
  int groupIndex = -1;

  std::vector<std::unique_ptr<ASTNode>> children;

  explicit ASTNode(Type t) : type(t) {}

  static std::unique_ptr<ASTNode> Literal(char c) {
    auto node = std::make_unique<ASTNode>(Type::LITERAL);
    node->ch = c;
    return node;
  }

  static std::unique_ptr<ASTNode> Dot() {
    return std::make_unique<ASTNode>(Type::DOT);
  }

  static std::unique_ptr<ASTNode> Concat(std::unique_ptr<ASTNode> left,
                                         std::unique_ptr<ASTNode> right) {
    auto node = std::make_unique<ASTNode>(Type::CONCAT);
    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));
    return node;
  }

  static std::unique_ptr<ASTNode> Alternate(std::unique_ptr<ASTNode> left,
                                            std::unique_ptr<ASTNode> right) {
    auto node = std::make_unique<ASTNode>(Type::ALTERNATE);
    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));
    return node;
  }

  static std::unique_ptr<ASTNode> Repeat(std::unique_ptr<ASTNode> child, uint32_t min, uint32_t max,
                                         bool g = true) {
    auto node = std::make_unique<ASTNode>(Type::REPEAT);
    node->children.push_back(std::move(child));
    node->minRepeat = min;
    node->maxRepeat = max;
    node->greedy = g;
    return node;
  }

  static std::unique_ptr<ASTNode> Group(std::unique_ptr<ASTNode> child, int idx) {
    auto node = std::make_unique<ASTNode>(Type::GROUP);
    node->groupIndex = idx;
    node->children.push_back(std::move(child));
    return node;
  }

  static std::unique_ptr<ASTNode> Backref(int group) {
    auto node = std::make_unique<ASTNode>(Type::BACKREF);
    node->groupIndex = group;
    return node;
  }
};

// ============================================================================
// Parser - Recursive Descent Parser
// ============================================================================
class Parser {
 public:
  explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

  std::unique_ptr<ASTNode> parse() {
    currentCapture_ = 0;
    auto result = parseAlternation();
    if (pos_ < tokens_.size()) {
      throw RegexError("Unexpected tokens at end of pattern", tokens_[pos_].position);
    }
    return result;
  }

  int numCaptures() const {
    return currentCapture_;
  }

 private:
  const std::vector<Token>& tokens_;
  size_t pos_ = 0;
  int currentCapture_ = 0;

  Token peek() const {
    return (pos_ < tokens_.size()) ? tokens_[pos_] : Token(TokenType::UNKNOWN);
  }

  Token consume() {
    return (pos_ < tokens_.size()) ? tokens_[pos_++] : Token(TokenType::UNKNOWN);
  }

  bool match(TokenType type) {
    if (peek().type == type) {
      ++pos_;
      return true;
    }
    return false;
  }

  // Grammar:
  // alternation ::= concatenation ('|' concatenation)*
  // concatenation ::= quantifier+
  // quantifier ::= atom [?*+|{n,m}]
  // atom ::= literal | '.' | '(' group ')' | '[' class ']' | escape
  // group ::= alternation
  // class ::= char [- char] ...

  std::unique_ptr<ASTNode> parseAlternation() {
    auto left = parseConcatenation();
    while (match(TokenType::PIPE)) {
      auto right = parseConcatenation();
      left = ASTNode::Alternate(std::move(left), std::move(right));
    }
    return left;
  }

  std::unique_ptr<ASTNode> parseConcatenation() {
    auto left = parseQuantifier();
    while (isQuantifierStart()) {
      auto right = parseQuantifier();
      left = ASTNode::Concat(std::move(left), std::move(right));
    }
    return left;
  }

  bool isQuantifierStart() {
    Token t = peek();
    switch (t.type) {
      case TokenType::LITERAL:
      case TokenType::RANGE:  // '-' can have quantifiers
      case TokenType::DOT:
      case TokenType::LPAREN:
      case TokenType::LBRACKET:
      case TokenType::ESCAPE:
      case TokenType::CARET:
      case TokenType::DOLLAR:
        return true;
      default:
        return false;
    }
  }

  std::unique_ptr<ASTNode> parseQuantifier() {
    auto atom = parseAtom();

    Token t = peek();
    uint32_t min = 0, max = 0;
    bool hasQuant = false;
    bool greedy = true;

    switch (t.type) {
      case TokenType::STAR:
        consume();
        min = 0;
        max = UINT32_MAX;
        hasQuant = true;
        break;
      case TokenType::PLUS:
        consume();
        min = 1;
        max = UINT32_MAX;
        hasQuant = true;
        break;
      case TokenType::QUESTION:
        consume();
        min = 0;
        max = 1;
        hasQuant = true;
        break;
      case TokenType::LBRACE:
        consume();
        min = parseNumber();
        max = min;
        if (match(TokenType::COMMA)) {
          max = parseNumber();
        }
        if (!match(TokenType::RBRACE)) {
          throw RegexError("Expected '}' after quantifier", t.position);
        }
        hasQuant = true;
        break;
      default:
        break;
    }

    if (hasQuant) {
      return ASTNode::Repeat(std::move(atom), min, max, greedy);
    }
    return atom;
  }

  uint32_t parseNumber() {
    uint32_t result = 0;
    while (peek().type == TokenType::LITERAL && ::isdigit(peek().value)) {
      result = result * 10 + (peek().value - '0');
      consume();
    }
    return result;
  }

  std::unique_ptr<ASTNode> parseAtom() {
    Token t = consume();

    switch (t.type) {
      case TokenType::LITERAL:
        return ASTNode::Literal(t.value);

      case TokenType::RANGE:  // '-' outside of character class
        return ASTNode::Literal('-');

      case TokenType::COMMA:  // ',' outside of quantifier
        return ASTNode::Literal(',');

      case TokenType::DOT:
        return ASTNode::Dot();

      case TokenType::LPAREN:
        // Check for non-capturing group (?:...)
        if (peek().type == TokenType::LITERAL && peek().value == '?') {
          consume();
          if (peek().type == TokenType::LITERAL && peek().value == ':') {
            consume();
            auto node = parseAlternation();
            if (!match(TokenType::RPAREN)) {
              throw RegexError("Expected ')' to close non-capturing group", t.position);
            }
            return node;
          }
          throw RegexError("Invalid group modifier", t.position);
        }

        // Check for positive lookahead (?=...)
        if (peek().type == TokenType::LITERAL && peek().value == '=') {
          consume();
          auto node = parseAlternation();
          if (!match(TokenType::RPAREN)) {
            throw RegexError("Expected ')' to close lookahead", t.position);
          }
          return node;
        }

        // Check for negative lookahead (?!...)
        if (peek().type == TokenType::LITERAL && peek().value == '!') {
          consume();
          auto node = parseAlternation();
          if (!match(TokenType::RPAREN)) {
            throw RegexError("Expected ')' to close lookahead", t.position);
          }
          return node;
        }

        ++currentCapture_;
        {
          auto node = parseAlternation();
          if (!match(TokenType::RPAREN)) {
            throw RegexError("Expected ')' to close group", t.position);
          }
          return ASTNode::Group(std::move(node), currentCapture_);
        }

      case TokenType::LBRACKET: {
        auto classNode = parseCharacterClass();
        if (!match(TokenType::RBRACKET)) {
          throw RegexError("Expected ']' to close character class", t.position);
        }
        return classNode;
      }

      case TokenType::ESCAPE: {
        return parseEscape(t);
      }

      case TokenType::BACKREF: {
        uint32_t group = parseNumber();
        return ASTNode::Backref(group);
      }

      case TokenType::CARET: {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::ANCHOR_START);
        return node;
      }

      case TokenType::DOLLAR: {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::ANCHOR_END);
        return node;
      }

      default:
        throw RegexError("Unexpected token", t.position);
    }
  }

  std::unique_ptr<ASTNode> parseCharacterClass() {
    bool negated = false;
    uint64_t low_mask = 0;   // Characters 0-63
    uint64_t high_mask = 0;  // Characters 64-127

    if (peek().type == TokenType::CARET) {
      consume();
      negated = true;
    }

    while (peek().type != TokenType::RBRACKET && peek().type != TokenType::UNKNOWN) {
      if (peek().type == TokenType::ESCAPE) {
        char esc = consume().value;
        uint64_t bits = 0;
        uint64_t bits_high = 0;
        if (esc == 'd')
          bits = CharClass::DIGIT_MASK;
        else if (esc == 'D')
          bits = CharClass::DIGIT_MASK;
        else if (esc == 'w')
          bits = 0ULL;  // Will use function
        else if (esc == 'W')
          bits = 0ULL;
        else if (esc == 's')
          bits = CharClass::SPACE_MASK;
        else if (esc == 'S')
          bits = CharClass::SPACE_MASK;
        else if (esc == 't')
          bits = (1ULL << '\t');
        else if (esc == 'r')
          bits = (1ULL << '\r');
        else if (esc == 'n')
          bits = (1ULL << '\n');
        else if (esc == 'f')
          bits = (1ULL << '\f');
        else if (esc == 'v')
          bits = (1ULL << '\v');
        else if (esc == 'a')
          bits = (1ULL << '\a');
        else if (esc == 'e')
          bits = (1ULL << '\x1b');
        else if (esc == 'x') {
          uint8_t value = 0;
          for (int i = 0; i < 2 && peek().type == TokenType::LITERAL; ++i) {
            char c = consume().value;
            value = value * 16 + parseHexDigit(c);
          }
          if (value < 64)
            bits |= (1ULL << value);
          else if (value < 128)
            bits_high |= (1ULL << (value - 64));
        } else {
          if (static_cast<uint8_t>(esc) < 64)
            bits = (1ULL << esc);
          else if (static_cast<uint8_t>(esc) < 128)
            bits_high = (1ULL << (static_cast<uint8_t>(esc) - 64));
        }
        if (negated) {
          low_mask |= bits;
          high_mask |= bits_high;
        } else {
          low_mask |= bits;
          high_mask |= bits_high;
        }
        continue;
      } else {
        char c = consume().value;
        uint8_t uc = static_cast<uint8_t>(c);
        if (peek().type == TokenType::RANGE) {
          consume();
          char end_char = consume().value;
          for (int i = c; i <= end_char; ++i) {
            uc = static_cast<uint8_t>(i);
            if (uc < 64)
              low_mask |= (1ULL << uc);
            else if (uc < 128)
              high_mask |= (1ULL << (uc - 64));
          }
        } else {
          if (uc < 64)
            low_mask |= (1ULL << uc);
          else if (uc < 128)
            high_mask |= (1ULL << (uc - 64));
        }
      }
    }

    auto node =
        std::make_unique<ASTNode>(negated ? ASTNode::Type::NOT_CLASS : ASTNode::Type::CLASS);
    // For negated class, store the EXCLUDED characters, engine will handle NOT
    node->classbits = low_mask;
    node->classbits_high = high_mask;
    return node;
  }

  std::unique_ptr<ASTNode> parseEscape(const Token& escTok) {
    char esc = escTok.value;  // The escaped character is already in the token!

    switch (esc) {
      case 'd': {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::CLASS);
        node->class_type = CLASS_DIGIT;
        return node;
      }
      case 'D': {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::NOT_CLASS);
        node->class_type = CLASS_DIGIT;
        return node;
      }
      case 'w': {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::CLASS);
        node->class_type = CLASS_WORD;
        return node;
      }
      case 'W': {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::NOT_CLASS);
        node->class_type = CLASS_WORD;
        return node;
      }
      case 's': {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::CLASS);
        node->class_type = CLASS_SPACE;
        return node;
      }
      case 'S': {
        auto node = std::make_unique<ASTNode>(ASTNode::Type::NOT_CLASS);
        node->class_type = CLASS_SPACE;
        return node;
      }
      case 'b':
      case 'B':  // Word boundary - not implemented yet, treat as literal 'b'
        return ASTNode::Literal('b');
      case 't':
        return ASTNode::Literal('\t');
      case 'r':
        return ASTNode::Literal('\r');
      case 'n':
        return ASTNode::Literal('\n');
      case 'f':
        return ASTNode::Literal('\f');
      case 'v':
        return ASTNode::Literal('\v');
      default:
        return ASTNode::Literal(esc);
    }
  }

  uint8_t parseHexDigit(char c) {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return 0;
  }

  std::unique_ptr<ASTNode> parseAnchorStart() {
    return std::make_unique<ASTNode>(ASTNode::Type::ANCHOR_START);
  }

  std::unique_ptr<ASTNode> parseAnchorEnd() {
    return std::make_unique<ASTNode>(ASTNode::Type::ANCHOR_END);
  }
};

// ============================================================================
// Compiler - AST to Bytecode
// ============================================================================
class Compiler {
 public:
  Compiler() : captureCount_(0) {}

  std::vector<Instruction> compile(const std::unique_ptr<ASTNode>& root, int numCaptures) {
    captureCount_ = numCaptures + 1;
    instructions_.clear();
    compileNode(root.get());
    instructions_.push_back(Instruction::Match());
    return std::move(instructions_);
  }

 private:
  std::vector<Instruction> instructions_;
  int captureCount_;

  void emit(const Instruction& inst) {
    instructions_.push_back(inst);
  }

  uint32_t emitJump() {
    uint32_t pos = instructions_.size();
    emit(Instruction::Jump(0));
    return pos;
  }

  void patchJump(uint32_t pos, uint32_t target) {
    if (pos < instructions_.size()) {
      ((Instruction&)instructions_[pos]).operand = target;
    }
  }

  void compileNode(ASTNode* node) {
    if (!node)
      return;

    switch (node->type) {
      case ASTNode::Type::LITERAL:
        emit(Instruction::Char(node->ch));
        break;

      case ASTNode::Type::DOT:
        emit(Instruction::Any());
        break;

      case ASTNode::Type::CLASS: {
        if (node->class_type >= 0) {
          emit(Instruction::ClassPred(node->class_type));
        } else {
          // Use extended class if high bits are non-zero
          if (node->classbits_high != 0) {
            emit(Instruction::ClassExt(node->classbits, node->classbits_high));
          } else {
            emit(Instruction::Class(node->classbits));
          }
        }
        break;
      }

      case ASTNode::Type::NOT_CLASS: {
        if (node->class_type >= 0) {
          emit(Instruction::ClassPred(node->class_type | 0x10));
        } else {
          if (node->classbits_high != 0) {
            emit(Instruction::NotClassExt(node->classbits, node->classbits_high));
          } else {
            emit(Instruction::NotClass(node->classbits));
          }
        }
        break;
      }

      case ASTNode::Type::ANCHOR_START:
        emit(Instruction::AnchorStart());
        break;

      case ASTNode::Type::ANCHOR_END:
        emit(Instruction::AnchorEnd());
        break;

      case ASTNode::Type::CONCAT:
        for (auto& child : node->children) {
          compileNode(child.get());
        }
        break;

      case ASTNode::Type::ALTERNATE: {
        if (node->children.size() != 2) {
          for (auto& child : node->children) {
            compileNode(child.get());
          }
          break;
        }
        // Emit SPLIT instruction for alternation
        // SPLIT(target1, target2) where target1 = first alternative, target2 = second alternative
        uint32_t splitPos = instructions_.size();
        emit(Instruction::Split(0, 0));        // Will patch later
        compileNode(node->children[0].get());  // First alternative
        uint32_t jumpPos = instructions_.size();
        emit(Instruction::Jump(0));  // Jump over second alternative
        // Patch SPLIT to point to second alternative
        ((Instruction&)instructions_[splitPos]).operand =
            splitPos + 1;  // target1 = next instruction
        ((Instruction&)instructions_[splitPos]).charset = jumpPos + 1;  // target2 = after the JUMP
        compileNode(node->children[1].get());                           // Second alternative
        // Patch JUMP to go to end
        ((Instruction&)instructions_[jumpPos]).operand = instructions_.size();
        break;
      }

      case ASTNode::Type::REPEAT: {
        auto& child = node->children[0];
        uint32_t min = node->minRepeat;
        uint32_t max = node->maxRepeat;

        if (max == UINT32_MAX) {
          if (min == 0) {
            // * - zero or more
            // Structure: SPLIT(body, Skip) ... body ... JUMP(SPLIT)
            uint32_t splitPos = instructions_.size();
            emit(Instruction::Split(0, 0));  // Will patch
            uint32_t bodyStart = splitPos + 1;
            compileNode(child.get());
            uint32_t jumpPos = instructions_.size();
            emit(Instruction::Jump(0));  // JUMP back to SPLIT
            // Patch SPLIT: target1 = bodyStart, target2 = after JUMP
            ((Instruction&)instructions_[splitPos]).operand = bodyStart;
            ((Instruction&)instructions_[splitPos]).charset = jumpPos + 1;
            // Patch JUMP to go back to SPLIT
            ((Instruction&)instructions_[jumpPos]).operand = splitPos;
          } else {
            // + - one or more
            // Structure: body ... SPLIT(body, Skip)
            compileNode(child.get());
            uint32_t splitPos = instructions_.size();
            emit(Instruction::Split(0, 0));  // Will patch
            uint32_t repeatStart = splitPos + 1;
            compileNode(child.get());
            uint32_t jumpPos = instructions_.size();
            emit(Instruction::Jump(0));  // JUMP back to SPLIT
            // Patch SPLIT: target1 = repeatStart, target2 = after JUMP
            ((Instruction&)instructions_[splitPos]).operand = repeatStart;
            ((Instruction&)instructions_[splitPos]).charset = jumpPos + 1;
            // Patch JUMP to go back to SPLIT
            ((Instruction&)instructions_[jumpPos]).operand = splitPos;
          }
        } else if (max == 1 && min == 0) {
          // ? - zero or one
          // Structure: SPLIT(body, Skip) ... body
          uint32_t splitPos = instructions_.size();
          emit(Instruction::Split(0, 0));  // Will patch
          uint32_t bodyStart = splitPos + 1;
          compileNode(child.get());
          // Patch SPLIT: target1 = bodyStart, target2 = after body
          ((Instruction&)instructions_[splitPos]).operand = bodyStart;
          ((Instruction&)instructions_[splitPos]).charset = instructions_.size();
        } else if (max == 1) {
          // Exactly one - just emit body
          compileNode(child.get());
        } else {
          // {n,m} - fixed range
          // For now, only handle the minimum required
          for (uint32_t i = 0; i < min; ++i) {
            compileNode(child.get());
          }
        }
        break;
      }

      case ASTNode::Type::GROUP: {
        uint32_t groupIdx = node->groupIndex;
        emit(Instruction::Save(groupIdx * 2));
        compileNode(node->children[0].get());
        emit(Instruction::Save(groupIdx * 2 + 1));
        break;
      }

      case ASTNode::Type::BACKREF: {
        emit(Instruction::Backref(node->groupIndex));
        break;
      }
    }
  }
};

// ============================================================================
// Virtual Machine - Non-recursive Execution Engine
// ============================================================================
class VM {
 public:
  explicit VM(const std::vector<Instruction>& instructions, int captureCount)
      : instructions_(instructions), captureCount_(captureCount) {
    captures_.assign(captureCount * 2, std::string::npos);
    hasCapture_ = false;
  }

  // Try to match starting at exactly one position
  bool executeAt(const std::string& text, size_t pos, MatchResult& result) {
    const size_t textLen = text.length();

    // Reset state
    captures_.assign(captureCount_ * 2, std::string::npos);
    backtrackStack_.clear();
    backtrackStack_.reserve(256);  // Pre-allocate for performance

    size_t startPos = pos;
    size_t textPos = startPos;
    uint32_t pc = 0;
    bool matched = false;

    captures_[0] = startPos;

    // Main execution loop
    while (true) {
      // Execute current instruction path
      while (pc < instructions_.size() && !matched) {
        const Instruction& inst = instructions_[pc];

        switch (inst.opcode) {
          case Opcode::CHAR:
            if (textPos < textLen && text[textPos] == inst.ch) {
              ++textPos;
              ++pc;
            } else {
              goto fail;
            }
            break;

          case Opcode::ANY:
            if (textPos < textLen) {
              ++textPos;
              ++pc;
            } else {
              goto fail;
            }
            break;

          case Opcode::RANGE:
            if (textPos < textLen && text[textPos] >= inst.lo && text[textPos] <= inst.hi) {
              ++textPos;
              ++pc;
            } else {
              goto fail;
            }
            break;

          case Opcode::CLASS: {
            if (textPos < textLen &&
                CharClass::inClassExt(inst.charset_low, inst.charset_high, text[textPos])) {
              ++textPos;
              ++pc;
            } else {
              goto fail;
            }
            break;
          }

          case Opcode::NOT_CLASS: {
            if (textPos < textLen &&
                !CharClass::inClassExt(inst.charset_low, inst.charset_high, text[textPos])) {
              ++textPos;
              ++pc;
            } else {
              goto fail;
            }
            break;
          }

          case Opcode::CLASS_PRED: {
            bool pred_matched = false;
            bool negated = (inst.class_type & 0x10) != 0;
            int base_type = inst.class_type & 0x0F;
            char c = textPos < textLen ? text[textPos] : '\0';
            switch (base_type) {
              case CLASS_DIGIT:
                pred_matched = textPos < textLen && CharClass::isDigit(c);
                break;
              case CLASS_WORD:
                pred_matched = textPos < textLen && CharClass::isWordChar(c);
                break;
              case CLASS_SPACE:
                pred_matched = textPos < textLen && CharClass::isSpace(c);
                break;
            }
            if (negated)
              pred_matched = !pred_matched;

            if (pred_matched) {
              ++textPos;
              ++pc;
            } else {
              goto fail;
            }
            break;
          }

          case Opcode::JUMP:
            pc = inst.operand;
            break;

          case Opcode::SPLIT: {
            // inst.operand = target1, inst.charset = target2
            // Push second alternative onto backtrack stack
            BacktrackPoint bp;
            bp.pc = inst.charset;  // target2
            bp.textPos = textPos;
            bp.savedCaptures = captures_;
            backtrackStack_.push_back(bp);
            // Take first alternative
            pc = inst.operand;  // target1
            break;
          }

          case Opcode::SAVE: {
            uint32_t group = inst.operand & 0xFFFF;
            uint32_t next = inst.operand >> 16;
            captures_[group] = textPos;
            // Use next if it's a real jump target, otherwise just advance
            pc = (next > 0) ? next : (pc + 1);
            hasCapture_ = true;
            break;
          }

          case Opcode::MATCH:
            captures_[1] = textPos;
            matched = true;
            break;

          case Opcode::ANCHOR_START: {
            if (textPos == 0) {
              ++pc;  // Advance to next instruction
            } else {
              goto fail;
            }
            break;
          }

          case Opcode::ANCHOR_END: {
            if (textPos == textLen) {
              ++pc;  // Advance to next instruction
            } else {
              goto fail;
            }
            break;
          }

          case Opcode::BACKREF: {
            goto fail;  // Not implemented yet
          }

          default:
            ++pc;
            break;
        }
      }

      // If we matched, return success
      if (matched) {
        hasCapture_ = true;
        buildResult(text, result);
        return true;
      }

    fail:
      // No match on current path - try backtracking
      if (backtrackStack_.empty()) {
        return false;
      }

      // Restore from backtrack stack
      const BacktrackPoint& bp = backtrackStack_.back();
      pc = bp.pc;
      textPos = bp.textPos;
      captures_ = bp.savedCaptures;
      backtrackStack_.pop_back();
    }

    // Should never reach here
    return false;
  }

  // Try to match starting at any position (for search)
  bool search(const std::string& text, size_t start, MatchResult& result) {
    const size_t textLen = text.length();

    // Track previous match length to prevent infinite loops with zero-width matches
    size_t prevMatchLen = 0;
    for (size_t pos = start; pos <= textLen;) {
      MatchResult tempResult;
      if (executeAt(text, pos, tempResult)) {
        size_t matchLen = tempResult.length();
        // Prevent infinite loop for zero-width matches
        if (matchLen == 0 && matchLen == prevMatchLen && pos < textLen) {
          pos++;  // Force advance to prevent loop
        } else {
          result = tempResult;
          return true;
        }
      } else {
        // No match, try next position
        pos++;
      }
      prevMatchLen = (pos < textLen && tempResult.matched) ? (size_t)tempResult.length() : 0;
    }
    return false;
  }

 private:
  std::vector<Instruction> instructions_;  // Own a copy of instructions
  std::vector<size_t> captures_;
  int captureCount_;
  bool hasCapture_;

  // Stack for backtracking - stores {pc, textPos}
  struct BacktrackPoint {
    uint32_t pc;
    size_t textPos;
    std::vector<size_t> savedCaptures;
  };
  std::vector<BacktrackPoint> backtrackStack_;

  void buildResult(const std::string& text, MatchResult& result) {
    result.matched = true;
    result.position = captures_[0];
    size_t length = captures_[1] - captures_[0];
    result.matched_text = text.substr(captures_[0], length);

    result.captures.clear();
    // Find groups that are NOT contained in other groups (excluding the full match)
    // A group is contained in another if its start >= other.start and end <= other.end
    std::vector<bool> is_contained(captureCount_, false);
    for (int i = 1; i < captureCount_; ++i) {
      size_t i_start = captures_[i * 2];
      size_t i_end = captures_[i * 2 + 1];
      if (i_start == std::string::npos || i_end == std::string::npos)
        continue;
      for (int j = 1; j < captureCount_; ++j) {
        if (i == j || is_contained[j])
          continue;
        size_t j_start = captures_[j * 2];
        size_t j_end = captures_[j * 2 + 1];
        if (j_start == std::string::npos || j_end == std::string::npos)
          continue;
        // i is contained in j if j's range covers i's range and they're not equal
        if (j_start <= i_start && i_end <= j_end && (j_start < i_start || i_end < j_end)) {
          is_contained[i] = true;
          break;
        }
      }
    }
    // Only add non-contained groups to captures
    for (int i = 1; i < captureCount_; ++i) {
      if (!is_contained[i]) {
        size_t start = captures_[i * 2];
        size_t end = captures_[i * 2 + 1];
        if (start != std::string::npos && end != std::string::npos && end > start) {
          result.captures.push_back({start, end, text.substr(start, end - start)});
        } else {
          result.captures.push_back({std::string::npos, std::string::npos, ""});
        }
      }
    }
  }
};

// ============================================================================
// FastRegex Main Class
// ============================================================================
class Regex {
 public:
  enum class CompileFlag {
    DEFAULT = 0,
    CASE_INSENSITIVE = 1,
    MULTILINE = 2,
    DOTALL = 4,
    EXTENDED = 8
  };

  explicit Regex(const std::string& pattern, CompileFlag flags = CompileFlag::DEFAULT)
      : pattern_(pattern), flags_(flags), compiled_(false) {
    compile();
  }

  // Copy constructor
  Regex(const Regex& other)
      : pattern_(other.pattern_),
        flags_(other.flags_),
        instructions_(other.instructions_),
        engine_(nullptr),
        compiled_(other.compiled_),
        numCaptures_(other.numCaptures_) {
    if (compiled_) {
      engine_ = std::make_unique<VM>(instructions_, numCaptures_ + 1);
    }
  }

  // Copy assignment
  Regex& operator=(const Regex& other) {
    if (this != &other) {
      pattern_ = other.pattern_;
      flags_ = other.flags_;
      instructions_ = other.instructions_;
      numCaptures_ = other.numCaptures_;
      compiled_ = other.compiled_;
      if (compiled_) {
        engine_ = std::make_unique<VM>(instructions_, numCaptures_ + 1);
      } else {
        engine_.reset();
      }
    }
    return *this;
  }

  // Move constructor
  Regex(Regex&& other) noexcept
      : pattern_(std::move(other.pattern_)),
        flags_(other.flags_),
        instructions_(std::move(other.instructions_)),
        engine_(std::move(other.engine_)),
        compiled_(other.compiled_),
        numCaptures_(other.numCaptures_) {
    other.compiled_ = false;
  }

  // Move assignment
  Regex& operator=(Regex&& other) noexcept {
    if (this != &other) {
      pattern_ = std::move(other.pattern_);
      flags_ = other.flags_;
      instructions_ = std::move(other.instructions_);
      engine_ = std::move(other.engine_);
      compiled_ = other.compiled_;
      numCaptures_ = other.numCaptures_;
      other.compiled_ = false;
    }
    return *this;
  }

  bool match(const std::string& text) {
    if (!compiled_)
      return false;
    MatchResult result;
    return engine_->executeAt(text, 0, result);
  }

  bool match(const std::string& text, MatchResult& result, size_t start = 0) {
    if (!compiled_)
      return false;
    return engine_->executeAt(text, start, result);
  }

  bool search(const std::string& text, MatchResult& result, size_t start = 0) {
    if (!compiled_)
      return false;
    return engine_->search(text, start, result);
  }

  std::vector<MatchResult> searchAll(const std::string& text) {
    std::vector<MatchResult> results;
    if (!compiled_)
      return results;

    size_t pos = 0;
    const size_t textLen = text.length();
    size_t prevMatchLen = 0;

    while (pos <= textLen) {
      MatchResult result;
      if (engine_->executeAt(text, pos, result)) {
        size_t matchLen = result.length();
        // Prevent infinite loop for zero-width matches
        if (matchLen == 0 && matchLen == prevMatchLen && pos < textLen) {
          pos++;  // Force advance to prevent loop
          continue;
        }
        results.push_back(result);
        pos = result.position + (result.length() > 0 ? result.length() : 1);
        prevMatchLen = matchLen;
      } else {
        // No match found at this position, move to next
        pos++;
        prevMatchLen = 0;
      }
    }
    return results;
  }

  std::string replace(const std::string& text, const std::string& replacement, bool all = true) {
    if (!compiled_)
      return text;

    std::string result = text;
    if (all) {
      size_t pos = 0;
      while (pos < result.length()) {
        MatchResult match;
        if (search(result, match, pos)) {
          std::string repl = expandReplacement(replacement, match);
          result.replace(match.position, match.length(), repl);
          pos = match.position + repl.length();
        } else {
          break;  // No more matches found
        }
      }
    } else {
      MatchResult match;
      if (search(result, match, 0)) {
        std::string repl = expandReplacement(replacement, match);
        result.replace(match.position, match.length(), repl);
      }
    }
    return result;
  }

  const std::string& pattern() const {
    return pattern_;
  }
  bool isCompiled() const {
    return compiled_;
  }

 private:
  std::string pattern_;
  CompileFlag flags_;
  std::vector<Instruction> instructions_;
  std::unique_ptr<VM> engine_;
  bool compiled_;
  int numCaptures_;

  void compile() {
    try {
      Lexer lexer(pattern_);
      auto tokens = lexer.tokenize();

      Parser parser(tokens);
      auto ast = parser.parse();

      numCaptures_ = parser.numCaptures();

      Compiler compiler;
      instructions_ = compiler.compile(ast, numCaptures_);

      engine_ = std::make_unique<VM>(instructions_, numCaptures_ + 1);
      compiled_ = true;
    } catch (const RegexError& e) {
      compiled_ = false;
      throw;
    }
  }

  std::string expandReplacement(const std::string& repl, const MatchResult& match) {
    std::string result;
    for (size_t i = 0; i < repl.length(); ++i) {
      if (repl[i] == '\\' && i + 1 < repl.length()) {
        char next = repl[++i];
        if (::isdigit(next)) {
          int group = next - '0';
          result += match.group(group);
        } else {
          switch (next) {
            case 'n':
              result += '\n';
              break;
            case 'r':
              result += '\r';
              break;
            case 't':
              result += '\t';
              break;
            default:
              result += next;
              break;
          }
        }
      } else if (repl[i] == '$' && i + 1 < repl.length()) {
        char next = repl[++i];
        if (::isdigit(next)) {
          int group = next - '0';
          result += match.group(group);
        } else {
          result += '$';
          result += next;
        }
      } else {
        result += repl[i];
      }
    }
    return result;
  }
};

// ============================================================================
// Factory functions
// ============================================================================
inline std::unique_ptr<Regex> compile(const std::string& pattern,
                                      Regex::CompileFlag flags = Regex::CompileFlag::DEFAULT) {
  return std::make_unique<Regex>(pattern, flags);
}

inline bool matches(const std::string& text, const std::string& pattern) {
  Regex regex(pattern);
  return regex.match(text);
}

inline std::vector<MatchResult> search(const std::string& text, const std::string& pattern) {
  Regex regex(pattern);
  return regex.searchAll(text);
}

inline std::string replace(const std::string& text, const std::string& pattern,
                           const std::string& replacement, bool all = true) {
  Regex regex(pattern);
  return regex.replace(text, replacement, all);
}

}  // namespace amaranth
