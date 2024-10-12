#pragma once

// #define NDEBUG

#ifdef NDEBUG
#define log(fmt, ...)
#else
#define log(fmt, ...)                                                 \
  printf("[%s at %s:%d] " fmt "\n", __FUNCTION__, __FILE__, __LINE__, \
         ##__VA_ARGS__)
#endif

#define error_raw(msg) throw std::runtime_error(msg)

#define error(fmt, ...)                                                      \
  do {                                                                       \
    char buf[1024];                                                          \
    snprintf(buf, sizeof(buf), "[%s at %s:%d] " fmt, __FUNCTION__, __FILE__, \
             __LINE__, ##__VA_ARGS__);                                       \
    throw std::runtime_error(buf);                                           \
  } while (0)

#define error_expected(tokenizer, token, fmt, ...)                    \
  error("in line %d: expected " fmt " (got %s/%s)", tokenizer.line(), \
        ##__VA_ARGS__, token.as_string().c_str(), token.lexeme().c_str())
