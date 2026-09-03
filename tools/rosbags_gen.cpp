#include <rosbags/codegen.hpp>

#include <iostream>

int main(int argc, char** argv) {
  rosbags::codegen::GenerateOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument(argv[index]);
    if (argument == "--profile" && index + 1 < argc) options.profile = argv[++index];
    else if (argument == "--input" && index + 1 < argc) options.inputs.emplace_back(argv[++index]);
    else if (argument == "--output" && index + 1 < argc) options.output_directory = argv[++index];
    else {
      std::cerr << "usage: rosbags-gen --profile PROFILE --input DIR_OR_FILE --output DIR\n";
      return 2;
    }
  }
  try {
    rosbags::codegen::generate(options);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
