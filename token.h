#pragma once

#include <iosfwd>
#include <string>

class Token {
 public:
  enum class Kind {
    Eof,
    // prefix operators
    Not,
    // binary operators
    Plus,
    Minus,
    Star,
    Slash,
    Remainder,
    Equals,
    // bitwise
    Ampersand,
    Pipe,
    Caret,
    Tilde,
    LeftShift,
    RightShift,
    // logical
    And,
    Or,
    // comparison
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    // literals
    IntLiteral,
    // keywords
    Let,
    Def,
    Extern,
    If,
    Else,
    Identifier,
    // separators
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Semicolon,
    // misc
    Comment,
    Unknown,
  };
  inline static const std::string KindNames[] = {
      "Eof",        "Not",       "Plus",       "Minus",      "Star",
      "Slash",      "Remainder", "Equals",     "Ampersand",  "Pipe",
      "Caret",      "Tilde",     "LeftShift",  "RightShift", "And",
      "Or",         "Eq",        "Ne",         "Lt",         "Le",
      "Gt",         "Ge",        "IntLiteral", "Let",        "Def",
      "Extern",     "If",        "Else",       "Identifier", "LeftParen",
      "RightParen", "LeftBrace", "RightBrace", "Comma",      "Semicolon",
      "Comment",    "Unknown",
  };

  Token(Kind kind);
  Token(Kind kind, const std::string& lexeme);
  Token(Kind kind, const std::string& lexeme, int int_value);

  Kind kind() const;
  const std::string& lexeme() const;
  int int_value() const;
  std::string as_string() const;

  explicit operator bool() const;

  friend std::ostream& operator<<(std::ostream& os, const Token& token);

 private:
  Kind kind_;
  std::string lexeme_;
  int int_value_;
};
