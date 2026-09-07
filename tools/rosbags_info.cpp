#include <rosbags/rosbags.hpp>
#include <cxxopts.hpp>

#include <iostream>

int main(int argc, char** argv) {
  cxxopts::Options options("rosbags-info", "Show bag metadata and connections.");
  options.positional_help("PATH");
  options.add_options()
    ("h,help", "Show help")
    ("path", "Bag path", cxxopts::value<std::string>(), "PATH");
  options.parse_positional({"path"});
  std::string path;
  try {
    const auto arguments = options.parse(argc, argv);
    if (arguments.count("help")) {
      std::cout << options.help();
      return 0;
    }
    if (arguments.count("path") != 1 || !arguments.unmatched().empty()) {
      std::cerr << "error: exactly one PATH is required\n" << options.help();
      return 2;
    }
    path = arguments["path"].as<std::string>();
  } catch (const cxxopts::exceptions::exception& error) {
    std::cerr << "error: " << error.what() << '\n' << options.help();
    return 2;
  }
  try {
    rosbags::Reader reader(path);
    reader.open();
    const auto& metadata = reader.metadata();
    std::cout << "storage: " << rosbags::storage_kind_name(metadata.storage) << '\n'
              << "start_ns: " << metadata.start_time << '\n'
              << "end_ns: " << metadata.end_time << '\n'
              << "duration_ns: " << metadata.duration << '\n'
              << "messages: " << metadata.message_count << '\n';
    for (const auto& connection : reader.connections()) {
      std::cout << connection.topic << " | " << connection.type << " | " << connection.message_count
                << " | " << connection.serialization_format << " | definition="
                << rosbags::definition_format_name(connection.definition.format) << '\n';
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
