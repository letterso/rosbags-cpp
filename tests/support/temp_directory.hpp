#pragma once

#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

namespace rosbags::test {

class TempDirectory {
public:
  TempDirectory() {
    std::random_device random;
    for (int attempt = 0; attempt < 100; ++attempt) {
      auto candidate = std::filesystem::temp_directory_path() /
          ("rosbags_cpp_test_" + std::to_string(random()) + "_" + std::to_string(random()));
      if (std::filesystem::create_directory(candidate)) {
        path_ = std::move(candidate);
        return;
      }
    }
    throw std::runtime_error("cannot create unique test directory");
  }
  ~TempDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }
  TempDirectory(const TempDirectory&) = delete;
  TempDirectory& operator=(const TempDirectory&) = delete;
  const std::filesystem::path& path() const { return path_; }

private:
  std::filesystem::path path_;
};

}  // namespace rosbags::test
