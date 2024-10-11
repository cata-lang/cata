#include <iostream>

#include "codegen.h"
#include "parser.h"

int main(int argc, char* argv[]) {
  // Tokenizer tokenizer{"./program.txt"};
  // while (Token token = tokenizer.next_token(true)) {
  //   std::cout << token << " ";
  // }
  auto parser = Parser{"./program.txt"};
  // Codegen::instance().visitNode(parser.definition().get());
  // Codegen::instance().visitNode(parser.top_level().get());
  Codegen::instance().module()->print(errs(), nullptr);
}
