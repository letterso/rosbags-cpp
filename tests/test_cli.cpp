#include <doctest/doctest.h>
#include "support/process.hpp"

#include <sqlite3.h>
#include <memory>

namespace {

using rosbags::test::run_process;

void check_cli(const std::string& program, const std::vector<std::string>& arguments,
               int code, const std::string& text) {
  const auto result = run_process(program, arguments);
  INFO("program: ", program);
  std::string argument_text;
  for (const auto& argument : arguments) argument_text += "[" + argument + "] ";
  INFO("arguments: ", argument_text);
  INFO("stdout: ", result.out, "\nstderr: ", result.err);
  CHECK(result.exit_code == code);
  if (code == 0) {
    CHECK(result.err.empty());
    CHECK(result.out.find(text) != std::string::npos);
  } else {
    CHECK(result.err.find(text) != std::string::npos);
  }
}

}  // namespace

TEST_SUITE("cli") {

TEST_CASE("all tools provide help and reject invalid arguments") {
  for (const auto* program : {ROSBAGS_INFO_PATH, ROSBAGS_READ_PATH, ROSBAGS_GEN_PATH}) {
    check_cli(program, {"--help"}, 0, "Usage:");
    check_cli(program, {"-h"}, 0, "Usage:");
    check_cli(program, {}, 2, "error:");
    check_cli(program, {"--unknown"}, 2, "error:");
  }
}

TEST_CASE("tools reject missing option values and extra positional arguments") {
  check_cli(ROSBAGS_INFO_PATH, {"first.bag", "second.bag"}, 2, "exactly one PATH");
  check_cli(ROSBAGS_READ_PATH, {"missing.bag", "--decode"}, 2, "error:");
  check_cli(ROSBAGS_READ_PATH, {"missing.bag", "--unknown"}, 2, "error:");
  for (const auto& arguments : std::vector<std::vector<std::string>>{
      {"--profile"}, {"--profile", "custom", "--input"},
      {"--profile", "custom", "--input", "missing", "--output"},
      {"--profile", "custom", "--output", "unused"},
      {"--profile", "custom", "--input", "missing", "--output", "unused", "stray"}}) {
    check_cli(ROSBAGS_GEN_PATH, arguments, 2, "error:");
  }
}

TEST_CASE("generator accepts repeated inputs and literal paths") {
  rosbags::test::TempDirectory directory;
  const auto input = directory.path() / "input, space ' $/simple/msg/Pair.msg";
  std::filesystem::create_directories(input.parent_path());
  std::filesystem::copy_file(std::filesystem::path(ROSBAGS_TEST_SOURCE_DIR) / "simple/msg/Pair.msg", input);
  const auto output = directory.path() / "generated";
  check_cli(ROSBAGS_GEN_PATH, {"--profile=cli_test", "--input", input.string(),
      "--input=" + std::string(ROSBAGS_TEST_SOURCE_DIR) + "/dependencies",
      "--output", output.string()}, 0, "");
  const auto messages = rosbags::test::read_text(output / "cli_test_messages.hpp");
  for (const auto* type : {"Pair", "AEnvelope", "ZPayload"}) {
    CAPTURE(type);
    CHECK(messages.find(std::string("struct ") + type) != std::string::npos);
  }
  CHECK(std::filesystem::exists(output / "cli_test_registry.hpp"));
}

TEST_CASE("reader filters multiple topics and decodes a real SQLite fixture") {
  rosbags::test::TempDirectory directory;
  const auto bag = directory.path() / "sample bag.db3";
  sqlite3* raw = nullptr;
  const int opened = sqlite3_open(bag.c_str(), &raw);
  const std::unique_ptr<sqlite3, decltype(&sqlite3_close)> database(raw, sqlite3_close);
  REQUIRE(opened == SQLITE_OK);
  REQUIRE(sqlite3_exec(database.get(),
      "CREATE TABLE topics(id INTEGER PRIMARY KEY, name TEXT, type TEXT, serialization_format TEXT, offered_qos_profiles TEXT);"
      "CREATE TABLE messages(id INTEGER PRIMARY KEY, topic_id INTEGER, timestamp INTEGER, data BLOB);"
      "INSERT INTO topics VALUES(1,'/first','std_msgs/msg/String','cdr','');"
      "INSERT INTO topics VALUES(2,'/second','std_msgs/msg/String','cdr','');"
      "INSERT INTO topics VALUES(3,'/skip','std_msgs/msg/String','cdr','');"
      "INSERT INTO messages VALUES(1,1,10,X'0001000003000000686900');"
      "INSERT INTO messages VALUES(2,2,20,X'0001000003000000686900');"
      "INSERT INTO messages VALUES(3,3,30,X'0001000003000000686900');",
      nullptr, nullptr, nullptr) == SQLITE_OK);
  check_cli(ROSBAGS_INFO_PATH, {"--", bag.string()}, 0, "messages: 3");
  const auto raw_result = run_process(ROSBAGS_READ_PATH, {bag.string(), "/first", "/second"});
  CHECK(raw_result.exit_code == 0);
  CHECK(raw_result.err.empty());
  CHECK(raw_result.out == "10 /first 11\n20 /second 11\n");
  check_cli(ROSBAGS_READ_PATH, {bag.string(), "/second", "--decode=ros2_humble"},
            0, "20 /second std_msgs/msg/String");
  check_cli(ROSBAGS_INFO_PATH, {(directory.path() / "missing.db3").string()}, 1, "error:");
}

}  // TEST_SUITE
