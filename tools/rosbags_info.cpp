#include <rosbags/rosbags.hpp>

#include <iostream>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: rosbags-info PATH\n";
    return 2;
  }
  try {
    rosbags::Reader reader(argv[1]);
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
