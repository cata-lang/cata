#include <iostream>

#include "codegen.h"
#include "parser.h"

int main(int argc, char* argv[]) {
  // Tokenizer tokenizer{"./program.cata"};
  // while (Token token = tokenizer.next_token(true)) {
  //   std::cout << token << " ";
  // }
  auto parser = Parser{"./program.cata"};
  // Codegen::instance().visitNode(parser.definition().get());
  // Codegen::instance().visitNode(parser.top_level().get());
  std::cout << Codegen::instance().get_ir() << std::endl;
}
