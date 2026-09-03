#include <rosbags/rosbags.hpp>
#include <rosbags/profiles.hpp>

#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: rosbags-read PATH [TOPIC] [--decode PROFILE]\n";
    return 2;
  }
  try {
    rosbags::Reader reader(argv[1]);
    reader.open();
    rosbags::ReadFilter filter;
    std::string profile;
    for (int index = 2; index < argc; ++index) {
      if (std::string(argv[index]) == "--decode" && index + 1 < argc) profile = argv[++index];
      else if (std::string(argv[index]).rfind("--", 0) != 0) filter.topics.emplace_back(argv[index]);
      else throw rosbags::RosbagsError("unknown option: " + std::string(argv[index]));
    }
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
