#include <rosbags/rosbags.hpp>
#include <rosbags/profiles.hpp>
#include <cxxopts.hpp>

#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
  cxxopts::Options options("rosbags-read", "Read raw or decoded bag messages.");
  options.positional_help("PATH [TOPIC ...]");
  options.add_options()
    ("h,help", "Show help")
    ("path", "Bag path", cxxopts::value<std::string>(), "PATH")
    ("topic", "Topics to read (repeatable)", cxxopts::value<std::vector<std::string>>(), "TOPIC")
    ("decode", "Decode using a built-in profile", cxxopts::value<std::string>(), "PROFILE");
  options.parse_positional({"path", "topic"});
  std::string path;
  std::string profile;
  rosbags::ReadFilter filter;
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
    // Preserve each topic literally, including commas accepted by the old CLI.
    for (const auto& argument : arguments.arguments()) {
      if (argument.key() == "topic") filter.topics.emplace_back(argument.value());
    }
    if (arguments.count("decode")) profile = arguments["decode"].as<std::string>();
  } catch (const cxxopts::exceptions::exception& error) {
    std::cerr << "error: " << error.what() << '\n' << options.help();
    return 2;
  }
  try {
    rosbags::Reader reader(path);
    reader.open();
    if (profile.empty()) {
      reader.read_raw(filter, [](const rosbags::Message& message) {
        std::cout << message.timestamp << ' ' << message.connection->topic << ' '
                  << message.bytes->size() << '\n';
      });
    } else {
      rosbags::TypeRegistry registry;
      rosbags::profiles::register_builtin_types(registry, profile);
      reader.read_raw(filter, [&](const rosbags::Message& message) {
        const auto decoded = rosbags::decode(message, registry, profile, rosbags::UnknownTypePolicy::WarnAndSkip,
                                             [](std::string_view warning) { std::cerr << "warning: " << warning << '\n'; });
        if (decoded) std::cout << message.timestamp << ' ' << message.connection->topic << ' '
                               << decoded->support->type_name << '\n';
      });
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
