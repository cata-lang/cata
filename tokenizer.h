#pragma once

#include <fstream>
#include <optional>
#include <string>

#include "token.h"

class Tokenizer {
 public:
  Tokenizer(const std::string& file_name);
  ~Tokenizer();

  Token next_token(bool keep_comment = false);
  const Token& cur_token() const;
  void putback(const Token& token);

  int line() const;

 private:
  std::ifstream file_;
  int line_{1};
  Token cur_token_{Token::Kind::Unknown};
  std::optional<Token> putback_{};

  std::string peek(int n);
  Token next_token_internal();
  void skip_whitespace();
};
