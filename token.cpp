#include <iostream>

#include "token.h"

Token::Token(Kind kind) : kind_{kind} {}

Token::Token(Kind kind, const std::string& lexeme)
    : kind_{kind}, lexeme_{lexeme} {}

Token::Token(Kind kind, const std::string& lexeme, int int_value)
    : kind_{kind}, lexeme_{lexeme}, int_value_{int_value} {}

Token::Kind Token::kind() const {
  return kind_;
}

const std::string& Token::lexeme() const {
  return lexeme_;
}

int Token::int_value() const {
  return int_value_;
}

std::string Token::as_string() const {
  std::string str = KindNames[(size_t)kind_];
  switch (kind_) {
    case Kind::IntLiteral:
      str += '(' + std::to_string(int_value_) + ')';
      break;
    case Kind::Identifier:
    case Kind::Comment:
    case Kind::Unknown:
      str += '(' + lexeme_ + ')';
      break;
  }
  return str;
}

Token::operator bool() const {
  return kind_ != Kind::Eof && kind_ != Kind::Unknown;
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
  os << token.as_string();
  return os;
}
