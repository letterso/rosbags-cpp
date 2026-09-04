#include "rosbags/codegen.hpp"

#include "rosbags/rosbags.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace rosbags::codegen {
namespace {

struct Field {
  std::string type;
  std::string name;
  enum class Container { Scalar, Array, Sequence } container = Container::Scalar;
  std::size_t length = 0;
  std::size_t bound = 0;
};
struct Definition {
  std::string package;
  std::string name;
  std::string canonical;
  std::vector<Field> fields;
};

std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}
std::string sanitize(std::string value) {
  for (auto& character : value) if (!std::isalnum(static_cast<unsigned char>(character))) character = '_';
  return value;
}
std::string primitive_cpp(const std::string& type) {
  if (type == "byte" || type == "char" || type == "uint8" || type == "octet") return "std::uint8_t";
  if (type == "bool" || type == "boolean") return "bool";
  if (type == "int8") return "std::int8_t";
  if (type == "int16" || type == "short") return "std::int16_t";
  if (type == "uint16" || type == "unsigned short") return "std::uint16_t";
  if (type == "int32" || type == "long") return "std::int32_t";
  if (type == "uint32" || type == "unsigned long") return "std::uint32_t";
  if (type == "int64" || type == "long long") return "std::int64_t";
  if (type == "uint64" || type == "unsigned long long") return "std::uint64_t";
  if (type == "float32" || type == "float") return "float";
  if (type == "float64" || type == "double") return "double";
  if (type == "string" || type.rfind("string<", 0) == 0) return "std::string";
  return {};
}
bool is_primitive(const std::string& type) { return !primitive_cpp(type).empty(); }
std::string canonical_type(const std::string& package, const std::string& type) {
  if (type == "Header") return "std_msgs/msg/Header";
  if (type.find("/msg/") != std::string::npos || type.find("/action/") != std::string::npos) return type;
  const auto slash = type.find('/');
  if (slash != std::string::npos) return type.substr(0, slash) + "/msg/" + type.substr(slash + 1);
  return package + "/msg/" + type;
}
std::string namespace_for(const Definition& definition, std::string_view profile) {
  return "rosbags::generated::" + sanitize(std::string(profile)) + "::" + sanitize(definition.package) + "::msg";
}
bool is_builtin_package(std::string_view package) {
  return package == "std_msgs" || package == "geometry_msgs" || package == "sensor_msgs" || package == "nav_msgs";
}
std::string local_type(const Definition& owner, const std::string& type, std::string_view profile) {
  if (is_primitive(type)) return primitive_cpp(type);
  const auto canonical = canonical_type(owner.package, type);
  // Header is a ROS common type whose ROS1 wire representation contains the
  // legacy sequence field while ROS2 CDR does not. Reuse the profile type and
  // emit the two wire readers below instead of generating one incompatible
  // struct from a single .msg definition.
  if (canonical == "builtin_interfaces/msg/Time") return "::rosbags::profiles::Time";
  if (canonical == "builtin_interfaces/msg/Duration") return "::rosbags::profiles::Duration";
  const auto slash = canonical.find("/msg/");
  if (slash == std::string::npos) return "std::string";
  if (is_builtin_package(canonical.substr(0, slash))) {
    return "::rosbags::profiles::" + sanitize(canonical.substr(0, slash)) + "::" +
           sanitize(canonical.substr(slash + 5));
  }
  return "rosbags::generated::" + sanitize(std::string(profile)) + "::" + sanitize(canonical.substr(0, slash)) + "::msg::" + sanitize(canonical.substr(slash + 5));
}
void parse_type_suffix(std::string type, Field& field) {
  const auto open = type.find('[');
  if (open == std::string::npos) { field.type = std::move(type); return; }
  field.type = type.substr(0, open);
  const auto close = type.find(']', open);
  if (close == std::string::npos) throw RosbagsError("malformed array type: " + type);
  const auto extent = type.substr(open + 1, close - open - 1);
  if (extent.empty()) field.container = Field::Container::Sequence;
  else if (extent.rfind("<=", 0) == 0) { field.container = Field::Container::Sequence; field.bound = std::stoull(extent.substr(2)); }
  else { field.container = Field::Container::Array; field.length = std::stoull(extent); }
}
std::string package_from_file(const std::filesystem::path& file, const std::filesystem::path& root) {
  std::error_code error;
  const auto relative = std::filesystem::relative(file, root, error);
  std::vector<std::string> parts;
  for (const auto& item : relative) parts.push_back(item.string());
  for (std::size_t i = 0; i < parts.size(); ++i) if (parts[i] == "msg" || parts[i] == "idl") return i ? parts[i - 1] : root.filename().string();
  return file.parent_path().filename().string();
}

Definition parse_msg(const std::filesystem::path& path, const std::filesystem::path& root) {
  Definition result{package_from_file(path, root), path.stem().string(), {}, {}};
  result.canonical = result.package + "/msg/" + result.name;
  std::ifstream input(path);
  if (!input) throw RosbagsError("cannot open message definition: " + path.string());
  std::string line;
  while (std::getline(input, line)) {
    const auto comment = line.find('#');
    if (comment != std::string::npos) line.resize(comment);
    line = trim(line);
    if (line.empty() || line.find('=') != std::string::npos) continue;
    std::istringstream tokens(line);
    std::string type;
    std::string name;
    if (!(tokens >> type >> name)) throw RosbagsError("malformed .msg field in " + path.string());
    Field field;
    field.name = name;
    parse_type_suffix(type, field);
    result.fields.push_back(std::move(field));
  }
  return result;
}

Definition parse_idl(const std::filesystem::path& path, const std::filesystem::path& root) {
  Definition result{package_from_file(path, root), path.stem().string(), {}, {}};
  result.canonical = result.package + "/msg/" + result.name;
  std::ifstream input(path);
  if (!input) throw RosbagsError("cannot open IDL definition: " + path.string());
  std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  text = std::regex_replace(text, std::regex(R"(//[^\n]*|/\*[\s\S]*?\*/)"), " ");
  const auto struct_pos = text.find("struct ");
  const auto brace = text.find('{', struct_pos);
  const auto close = text.find('}', brace);
  if (struct_pos == std::string::npos || brace == std::string::npos || close == std::string::npos) throw RosbagsError("malformed IDL struct: " + path.string());
  std::istringstream name_stream(text.substr(struct_pos + 7, brace - struct_pos - 7));
  name_stream >> result.name;
  result.canonical = result.package + "/msg/" + result.name;
  std::istringstream body(text.substr(brace + 1, close - brace - 1));
  std::string line;
  while (std::getline(body, line, ';')) {
    line = trim(line);
    if (line.empty() || line.rfind("const ", 0) == 0 || line.find('@') != std::string::npos) continue;
    if (line.rfind("sequence<", 0) == 0) {
      const auto start = std::string("sequence<").size();
      const auto comma = line.find(',', start);
      const auto end = line.find('>', start);
      if (end == std::string::npos) throw RosbagsError("malformed IDL sequence");
      Field field;
      field.type = trim(line.substr(start, (comma == std::string::npos ? end : comma) - start));
      field.name = trim(line.substr(end + 1));
      field.container = Field::Container::Sequence;
      if (comma != std::string::npos) field.bound = std::stoull(trim(line.substr(comma + 1, end - comma - 1)));
      result.fields.push_back(std::move(field));
    } else {
      const auto separator = line.find_last_of(" \t");
      if (separator == std::string::npos) continue;
      auto type = trim(line.substr(0, separator));
      auto name = trim(line.substr(separator + 1));
      Field field;
      const auto array = name.find('[');
      if (array != std::string::npos) {
        type += name.substr(array);
        name.resize(array);
      }
      field.name = name;
      parse_type_suffix(type, field);
      result.fields.push_back(std::move(field));
    }
  }
  return result;
}

std::vector<Definition> collect(const GenerateOptions& options) {
  std::vector<Definition> result;
  for (const auto& input_name : options.inputs) {
    const std::filesystem::path input(input_name);
    const auto root = std::filesystem::is_directory(input) ? input : input.parent_path().parent_path();
    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_directory(input)) {
      for (const auto& item : std::filesystem::recursive_directory_iterator(input)) {
        if (item.is_regular_file() && (item.path().extension() == ".msg" || item.path().extension() == ".idl"))
          files.push_back(item.path());
      }
    } else {
      files.push_back(input);
    }
    for (const auto& file : files) result.push_back(file.extension() == ".idl" ? parse_idl(file, root) : parse_msg(file, root));
  }
  std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.canonical < b.canonical; });
  result.erase(std::unique(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.canonical == b.canonical; }), result.end());
  return result;
}

std::string field_type(const Definition& definition, const Field& field, std::string_view profile) {
  const auto element = local_type(definition, field.type, profile);
  if (field.container == Field::Container::Array) return "std::array<" + element + ", " + std::to_string(field.length) + ">";
  if (field.container == Field::Container::Sequence) return "std::vector<" + element + ">";
  return element;
}
std::string scalar_call(const Field& field) {
  if (field.type == "string" || field.type.rfind("string<", 0) == 0) return "reader.string()";
  const auto primitive = primitive_cpp(field.type);
  if (primitive.empty()) return {};
  const auto method = primitive == "bool" ? "boolean" : primitive == "std::uint8_t" ? "u8" : primitive == "std::int8_t" ? "i8" : primitive == "std::uint16_t" ? "u16" : primitive == "std::int16_t" ? "i16" : primitive == "std::uint32_t" ? "u32" : primitive == "std::int32_t" ? "i32" : primitive == "std::uint64_t" ? "u64" : primitive == "std::int64_t" ? "i64" : primitive == "float" ? "f32" : "f64";
  return std::string("reader.") + method + "()";
}
void emit_reader(std::ostream& output, const Definition& definition, std::string_view profile, bool ros1) {
  const auto reader = ros1 ? "Ros1Reader" : "CdrReader";
  const auto function = ros1 ? "ros1" : "cdr";
  output << "inline void read_" << function << "(" << reader << "& reader, " << namespace_for(definition, profile) << "::" << sanitize(definition.name) << "& message) {\n";
  for (const auto& field : definition.fields) {
    const auto call = scalar_call(field);
    const bool nested = call.empty();
    if (field.container == Field::Container::Scalar) {
      if (nested) output << "  read_" << function << "(reader, message." << field.name << ");\n";
      else output << "  message." << field.name << " = " << call << ";\n";
    } else if (field.container == Field::Container::Array) {
      output << "  for (auto& value : message." << field.name << ") {\n";
      if (nested) output << "    read_" << function << "(reader, value);\n";
      else output << "    value = " << call << ";\n";
      output << "  }\n";
    } else {
      output << "  const auto size_" << field.name << " = reader.u32();\n";
      if (field.bound) output << "  if (size_" << field.name << " > " << field.bound << ") throw DecodeError(\"bounded sequence exceeds declared capacity\");\n";
      output << "  if (size_" << field.name << " > reader.remaining()) throw DecodeError(\"sequence length exceeds serialized payload\");\n";
      output << "  message." << field.name << ".clear(); message." << field.name << ".reserve(size_" << field.name << ");\n";
      output << "  for (std::uint32_t i = 0; i < size_" << field.name << "; ++i) {\n";
      if (nested) output << "    " << local_type(definition, field.type, profile) << " value{}; read_" << function << "(reader, value); message." << field.name << ".push_back(std::move(value));\n";
      else output << "    message." << field.name << ".push_back(" << call << ");\n";
      output << "  }\n";
    }
  }
  output << "}\n";
}

}  // namespace

void generate(const GenerateOptions& options) {
  if (options.profile.empty() || options.inputs.empty() || options.output_directory.empty()) throw RosbagsError("generator requires profile, input and output directory");
  const auto definitions = collect(options);
  if (definitions.empty()) throw RosbagsError("no .msg or .idl definitions found");
  const std::filesystem::path output_dir(options.output_directory);
  std::filesystem::create_directories(output_dir);
  const auto header_path = output_dir / (sanitize(options.profile) + "_messages.hpp");
  std::ofstream header(header_path);
  if (!header) throw RosbagsError("cannot create generated header: " + header_path.string());
  header << "#pragma once\n#include <array>\n#include <cstdint>\n#include <memory>\n#include <string>\n#include <utility>\n#include <vector>\n#include <rosbags/serialization.hpp>\n#include <rosbags/rosbags.hpp>\n#include <rosbags/profiles.hpp>\n\n";
  for (const auto& definition : definitions) {
    header << "namespace " << namespace_for(definition, options.profile) << " { struct "
           << sanitize(definition.name) << "; }\n";
  }
  header << "\n";
  for (const auto& definition : definitions) {
    const auto ns = namespace_for(definition, options.profile);
    header << "namespace " << ns << " {\n";
    header << "struct " << sanitize(definition.name) << " {\n";
    for (const auto& field : definition.fields) header << "  " << field_type(definition, field, options.profile) << " " << field.name << "{};\n";
    header << "};\n";
    header << "}\n";
  }
  header << "\nnamespace rosbags::generated::" << sanitize(options.profile)
         << "::detail {\nusing namespace rosbags::serialization;\n"
         << "using rosbags::profiles::detail::read_ros1;\n"
         << "using rosbags::profiles::detail::read_cdr;\n";
  for (const auto& definition : definitions) {
    const auto qname = namespace_for(definition, options.profile) + "::" + sanitize(definition.name);
    header << "inline void read_ros1(Ros1Reader&, " << qname << "&); inline void read_cdr(CdrReader&, " << qname << "&);\n";
  }
  for (const auto& definition : definitions) { emit_reader(header, definition, options.profile, true); emit_reader(header, definition, options.profile, false); }
  for (const auto& definition : definitions) {
    const auto qname = namespace_for(definition, options.profile) + "::" + sanitize(definition.name);
    const auto safe = sanitize(definition.canonical);
    header << "inline " << qname << " deserialize_ros1_" << safe << "(ByteView bytes) { Ros1Reader reader(bytes); " << qname << " value{}; read_ros1(reader, value); reader.finish(); return value; }\n";
    header << "inline " << qname << " deserialize_cdr_" << safe << "(ByteView bytes) { CdrReader reader(bytes); " << qname << " value{}; read_cdr(reader, value); reader.finish(); return value; }\n";
  }
  header << "}\n";

  const auto registry_path = output_dir / (sanitize(options.profile) + "_registry.hpp");
  std::ofstream registry(registry_path);
  if (!registry) throw RosbagsError("cannot create generated registry: " + registry_path.string());
  registry << "#pragma once\n#include \"" << header_path.filename().string() << "\"\nnamespace rosbags::generated::" << sanitize(options.profile) << " {\ninline void register_types(rosbags::TypeRegistry& registry) {\n";
  for (const auto& definition : definitions) {
    const auto qname = namespace_for(definition, options.profile) + "::" + sanitize(definition.name);
    const auto safe = sanitize(definition.canonical);
    registry << "  registry.register_type(std::make_shared<rosbags::TypeSupport<" << qname << ">>(\"" << definition.canonical << "\", \"" << options.profile << "\", [](rosbags::ByteView bytes) { return detail::deserialize_ros1_" << safe << "(bytes); }, [](rosbags::ByteView bytes) { return detail::deserialize_cdr_" << safe << "(bytes); }));\n";
  }
  registry << "}\n}\n";
}

}  // namespace rosbags::codegen
