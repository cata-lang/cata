#include <unordered_map>
#include <vector>

#include "fmt.h"
#include "tokenizer.h"

Tokenizer::Tokenizer(const std::string& file_name)
    : file_{file_name, std::ios::binary} {
  if (!file_) error("could not open file");
}

Tokenizer::~Tokenizer() {
  file_.close();
}

static Token::Kind get_single_char_kind(char c) {
  static std::unordered_map<char, Token::Kind> single_char_tokens = {
      {'!', Token::Kind::Not},       {'+', Token::Kind::Plus},
      {'-', Token::Kind::Minus},     {'*', Token::Kind::Star},
      {'/', Token::Kind::Slash},     {'=', Token::Kind::Equals},
      {'(', Token::Kind::LeftParen}, {')', Token::Kind::RightParen},
      {'{', Token::Kind::LeftBrace}, {'}', Token::Kind::RightBrace},
      {',', Token::Kind::Comma},     {';', Token::Kind::Semicolon},
  };
  auto it = single_char_tokens.find(c);
  if (it == single_char_tokens.end()) return Token::Kind::Unknown;
  return it->second;
}

static Token::Kind get_keyword_kind(const std::string& lexeme) {
  static std::unordered_map<std::string, Token::Kind> keywords = {
      {"def", Token::Kind::Def},
      {"extern", Token::Kind::Extern},
      {"if", Token::Kind::If},
      {"else", Token::Kind::Else},
  };
  auto it = keywords.find(lexeme);
  if (it == keywords.end()) return Token::Kind::Unknown;
  return it->second;
}

Token Tokenizer::next_token(bool keep_comment) {
  Token token = Token::Kind::Unknown;
  while (true) {
    token = next_token_internal();
    if (keep_comment || token.kind() != Token::Kind::Comment) break;
    // log("comment %s", token.lexeme().c_str());
  }
  cur_token_ = token;
  return token;
}

const Token& Tokenizer::cur_token() const {
  return cur_token_;
}

void Tokenizer::putback(const Token& token) {
  if (putback_) error("putback buffer is full");
  putback_ = token;
}

int Tokenizer::line() const {
  return line_;
}

std::string Tokenizer::peek(int n) {
  int pos = file_.tellg();
  std::vector<char> buffer(n);
  file_.read(buffer.data(), n);
  file_.seekg(pos);
  return std::string(buffer.begin(), buffer.end());
}

Token Tokenizer::next_token_internal() {
  if (putback_) {
    Token token = *putback_;
    putback_.reset();
    return token;
  }
  skip_whitespace();
  char c;
  file_.get(c);
  if (!file_) return Token::Kind::Eof;
  Token::Kind kind = get_single_char_kind(c);
  if (kind != Token::Kind::Unknown) {
    if (kind == Token::Kind::Slash &&
        get_single_char_kind(file_.peek()) == Token::Kind::Slash) {
      file_.get(c);
      std::string comment;
      std::getline(file_, comment);
      return Token(Token::Kind::Comment, comment);
    }
    return Token(kind, std::string(1, c));
  }
  if (isdigit(c)) {
    file_.putback(c);
    std::string lexeme;
    int int_value = 0;
    while (isdigit(file_.peek())) {
      file_.get(c);
      lexeme += c;
      int_value = int_value * 10 + (c - '0');
    }
    return Token(Token::Kind::IntLiteral, lexeme, int_value);
  }
  if (isalpha(c) || c == '_') {
    std::string lexeme{c};
    while (isalnum(file_.peek()) || file_.peek() == '_') {
      file_.get(c);
      lexeme += c;
    }
    Token::Kind kind = get_keyword_kind(lexeme);
    if (kind != Token::Kind::Unknown) return Token(kind, lexeme);
    return Token(Token::Kind::Identifier, lexeme);
  }
  error("unknown character: %d", (int)c);
  return Token(Token::Kind::Unknown, std::string(1, c));
}

void Tokenizer::skip_whitespace() {
  char c;
  while (file_.get(c)) {
    if (!isspace(c)) {
      file_.putback(c);
      break;
    }
    if (c == '\n') ++line_;
  }
}
