#pragma once

#include "temp_directory.hpp"

#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <spawn.h>
#include <string>
#include <system_error>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace rosbags::test {

inline std::string read_text(const std::filesystem::path& path) {
  std::ifstream stream(path);
  if (!stream) throw std::runtime_error("cannot read " + path.string());
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

struct ProcessResult {
  int exit_code;
  std::string out;
  std::string err;
};

// POSIX argv invocation avoids shell interpretation. Separate files capture
// both streams without pipe-buffer deadlocks; CTest bounds the whole test.
inline ProcessResult run_process(const std::string& program,
                                 const std::vector<std::string>& arguments) {
  TempDirectory directory;
  const auto stdout_path = directory.path() / "stdout";
  const auto stderr_path = directory.path() / "stderr";
  std::vector<std::string> storage{program};
  storage.insert(storage.end(), arguments.begin(), arguments.end());
  std::vector<char*> argv;
  for (auto& argument : storage) argv.push_back(argument.data());
  argv.push_back(nullptr);

  const auto check = [](int error) {
    if (error) throw std::system_error(error, std::generic_category(), "spawn CLI");
  };
  posix_spawn_file_actions_t actions;
  check(posix_spawn_file_actions_init(&actions));
  pid_t pid;
  try {
    check(posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0));
    check(posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, stdout_path.c_str(),
                                         O_WRONLY | O_CREAT | O_TRUNC, 0600));
    check(posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, stderr_path.c_str(),
                                         O_WRONLY | O_CREAT | O_TRUNC, 0600));
    check(posix_spawn(&pid, program.c_str(), &actions, nullptr, argv.data(), environ));
  } catch (...) {
    posix_spawn_file_actions_destroy(&actions);
    throw;
  }
  posix_spawn_file_actions_destroy(&actions);
  int status = 0;
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) throw std::system_error(errno, std::generic_category(), "wait for CLI");
  }
  return {WIFEXITED(status) ? WEXITSTATUS(status) : -1,
          read_text(stdout_path), read_text(stderr_path)};
}

}  // namespace rosbags::test
