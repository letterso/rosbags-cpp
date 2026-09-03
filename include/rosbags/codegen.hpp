#pragma once

#include <string>
#include <vector>

namespace rosbags::codegen {

struct GenerateOptions {
  std::string profile;
  std::vector<std::string> inputs;
  std::string output_directory;
};

void generate(const GenerateOptions& options);

}  // namespace rosbags::codegen
