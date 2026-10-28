#pragma once

#include "internal.hpp"

#include <cerrno>
#include <cstdio>
#include <unistd.h>

namespace rosbags::internal {

class TemporaryFile {
 public:
  explicit TemporaryFile(std::string_view prefix) {
    const auto pattern = (std::filesystem::temp_directory_path() / (std::string(prefix) + "_XXXXXX")).string();
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    descriptor_ = ::mkstemp(buffer.data());
    if (descriptor_ < 0) throw RosbagsError("cannot create temporary file: " + std::string(std::strerror(errno)));
    try { path_ = buffer.data(); }
    catch (...) { ::close(descriptor_); std::remove(buffer.data()); throw; }
  }
  ~TemporaryFile() {
    if (descriptor_ >= 0) ::close(descriptor_);
    std::remove(path_.c_str());
  }
  TemporaryFile(const TemporaryFile&) = delete;
  TemporaryFile& operator=(const TemporaryFile&) = delete;
  const std::string& path() const noexcept { return path_; }
  void write(ByteView bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size) {
      const auto size = std::min(bytes.size-offset, static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
      const auto count = ::write(descriptor_, bytes.data+offset, size);
      if (count < 0 && errno == EINTR) continue;
      if (count <= 0) throw RosbagsError(context(path_, "cannot write temporary file"));
      offset += static_cast<std::size_t>(count);
    }
  }
  void close() {
    const auto descriptor = descriptor_;
    descriptor_ = -1;
    if (descriptor >= 0 && ::close(descriptor) != 0)
      throw RosbagsError(context(path_, "cannot close temporary file"));
  }
 private:
  std::string path_;
  int descriptor_ = -1;
};

}  // namespace rosbags::internal
