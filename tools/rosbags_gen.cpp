#include <rosbags/codegen.hpp>
#include <cxxopts.hpp>

#include <iostream>

int main(int argc, char** argv) {
  rosbags::codegen::GenerateOptions options;
  cxxopts::Options cli("rosbags-gen", "Generate C++ message types and a registry.");
  cli.add_options()
    ("h,help", "Show help")
    ("profile", "Generated profile name", cxxopts::value<std::string>(), "PROFILE")
    ("input", "Message directory or file (repeatable)", cxxopts::value<std::vector<std::string>>(), "DIR_OR_FILE")
    ("output", "Output directory", cxxopts::value<std::string>(), "DIR");
  try {
    const auto arguments = cli.parse(argc, argv);
    if (arguments.count("help")) {
      std::cout << cli.help();
      return 0;
    }
    if (!arguments.count("profile") || !arguments.count("input") ||
        !arguments.count("output") || !arguments.unmatched().empty()) {
      std::cerr << "error: --profile, --input and --output are required; positional arguments are not supported\n"
                << cli.help();
      return 2;
    }
    options.profile = arguments["profile"].as<std::string>();
    options.output_directory = arguments["output"].as<std::string>();
    // Use raw occurrences so commas in file names are not treated as separators.
    for (const auto& argument : arguments.arguments()) {
      if (argument.key() == "input") options.inputs.emplace_back(argument.value());
    }
  } catch (const cxxopts::exceptions::exception& error) {
    std::cerr << "error: " << error.what() << '\n' << cli.help();
    return 2;
  }
  try {
    rosbags::codegen::generate(options);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
