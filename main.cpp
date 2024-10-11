#include <fstream>
#include <iostream>

#include "codegen.h"
#include "parser.h"

int main(int argc, char* argv[]) {
  std::ofstream output_file{"./ir/program.ll", std::fstream::trunc};
  // Tokenizer tokenizer{"./program.cata"};
  // while (Token token = tokenizer.next_token(true)) {
  //   std::cout << token << " ";
  // }
  auto parser = Parser{"./program.cata"};
  // Codegen::instance().visitNode(parser.definition().get());
  // Codegen::instance().visitNode(parser.top_level().get());

  // std::cout << Codegen::instance().get_ir() << std::endl;
  output_file << Codegen::instance().get_ir() << std::endl;
  system("cd ./ir/ && sh ./compile.sh");
}
