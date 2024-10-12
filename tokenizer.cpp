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
  static const std::unordered_map<char, Token::Kind> single_char_tokens = {
      {'!', Token::Kind::Not},        {'+', Token::Kind::Plus},
      {'-', Token::Kind::Minus},      {'*', Token::Kind::Star},
      {'/', Token::Kind::Slash},      {'%', Token::Kind::Remainder},
      {'=', Token::Kind::Equals},     {'&', Token::Kind::Ampersand},
      {'|', Token::Kind::Pipe},       {'^', Token::Kind::Caret},
      {'~', Token::Kind::Tilde},      {'<', Token::Kind::Lt},
      {'>', Token::Kind::Gt},         {'(', Token::Kind::LeftParen},
      {')', Token::Kind::RightParen}, {'{', Token::Kind::LeftBrace},
      {'}', Token::Kind::RightBrace}, {',', Token::Kind::Comma},
      {';', Token::Kind::Semicolon},
  };
  auto it = single_char_tokens.find(c);
  if (it == single_char_tokens.end()) return Token::Kind::Unknown;
  return it->second;
}

static Token::Kind get_double_char_kind(Token::Kind kind, char c) {
  static const std::unordered_map<Token::Kind,
                                  std::unordered_map<char, Token::Kind>>
      double_char_tokens = {
          {Token::Kind::Slash,
           {{'/', Token::Kind::Comment}, {'*', Token::Kind::Comment}}},
          {Token::Kind::Lt,
           {{'<', Token::Kind::LeftShift}, {'=', Token::Kind::Le}}},
          {Token::Kind::Gt,
           {{'>', Token::Kind::RightShift}, {'=', Token::Kind::Ge}}},
          {Token::Kind::Equals, {{'=', Token::Kind::Eq}}},
          {Token::Kind::Not, {{'=', Token::Kind::Ne}}},
          {Token::Kind::Ampersand, {{'&', Token::Kind::And}}},
          {Token::Kind::Pipe, {{'|', Token::Kind::Or}}},
      };
  auto it = double_char_tokens.find(kind);
  if (it == double_char_tokens.end()) return Token::Kind::Unknown;
  auto it2 = it->second.find(c);
  if (it2 == it->second.end()) return Token::Kind::Unknown;
  return it2->second;
}

static Token::Kind get_keyword_kind(const std::string& lexeme) {
  static const std::unordered_map<std::string, Token::Kind> keywords = {
      {"let", Token::Kind::Let},       {"def", Token::Kind::Def},
      {"extern", Token::Kind::Extern}, {"if", Token::Kind::If},
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
    Token::Kind double_kind = get_double_char_kind(kind, file_.peek());
    if (double_kind == Token::Kind::Unknown)
      return Token(kind, std::string(1, c));
    std::string lexeme = std::string(1, c) + (char)file_.get();
    if (double_kind == Token::Kind::Comment) {
      if (lexeme == "//") {
        std::getline(file_, lexeme);
      } else {
        // block comment
        lexeme.clear();
        while (true) {
          if (file_.get(c) && c == '*' && file_.peek() == '/') {
            file_.get(c);
            break;
          }
          if (!file_)
            error_expected((*this), Token(Token::Kind::Comment, "EOF"), "*/");
          lexeme += c;
        }
      }
      return Token(Token::Kind::Comment, lexeme);
    }
    return Token(double_kind, lexeme);
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
