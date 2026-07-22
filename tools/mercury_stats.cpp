#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct FileStats {
  std::filesystem::path path;
  std::uint64_t physical_lines = 0;
  std::uint64_t filtered_lines = 0;
};

struct Totals {
  std::uint64_t files = 0;
  std::uint64_t physical_lines = 0;
  std::uint64_t filtered_lines = 0;
};

std::string trim(std::string_view value) {
  const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
    return std::isspace(c) != 0;
  });
  const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
    return std::isspace(c) != 0;
  }).base();
  if (begin >= end) {
    return {};
  }
  return {begin, end};
}

bool starts_with(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool has_source_extension(const std::filesystem::path& path) {
  static const std::set<std::string> extensions{
      ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".cmake", ".md"};
  if (path.filename() == "CMakeLists.txt") {
    return true;
  }
  return extensions.contains(path.extension().string());
}

bool ignored_directory(const std::filesystem::path& path) {
  const auto name = path.filename().string();
  return name == ".git" || name == "build" || name == "build-fuzz" || name == "cmake-build-debug" ||
         name == "cmake-build-release" || name == ".cache" || name == ".idea" || name == ".vscode";
}

bool is_boilerplate_line(std::string_view raw_line, bool& in_block_comment) {
  auto line = trim(raw_line);
  if (line.empty()) {
    return true;
  }

  if (in_block_comment) {
    const auto end = line.find("*/");
    if (end == std::string::npos) {
      return true;
    }
    line = trim(line.substr(end + 2));
    in_block_comment = false;
    if (line.empty()) {
      return true;
    }
  }

  while (starts_with(line, "/*")) {
    const auto end = line.find("*/", 2);
    if (end == std::string::npos) {
      in_block_comment = true;
      return true;
    }
    line = trim(line.substr(end + 2));
    if (line.empty()) {
      return true;
    }
  }

  if (starts_with(line, "//") || starts_with(line, "*")) {
    return true;
  }

  if (starts_with(line, "#pragma once") || starts_with(line, "#include")) {
    return true;
  }

  return line == "{" || line == "}" || line == "};" || line == "};\r" || line == "}; // namespace mercury" ||
         starts_with(line, "} // namespace");
}

FileStats scan_file(const std::filesystem::path& root, const std::filesystem::path& path) {
  std::ifstream input(path);
  FileStats stats;
  stats.path = std::filesystem::relative(path, root);

  bool in_block_comment = false;
  std::string line;
  while (std::getline(input, line)) {
    ++stats.physical_lines;
    if (!is_boilerplate_line(line, in_block_comment)) {
      ++stats.filtered_lines;
    }
  }

  return stats;
}

std::vector<FileStats> scan_tree(const std::filesystem::path& root) {
  std::vector<FileStats> files;
  std::filesystem::recursive_directory_iterator iter(root);
  const std::filesystem::recursive_directory_iterator end;
  while (iter != end) {
    const auto& entry = *iter;
    if (entry.is_directory() && ignored_directory(entry.path())) {
      iter.disable_recursion_pending();
    } else if (entry.is_regular_file() && has_source_extension(entry.path())) {
      files.push_back(scan_file(root, entry.path()));
    }
    ++iter;
  }
  std::sort(files.begin(), files.end(), [](const FileStats& lhs, const FileStats& rhs) {
    return lhs.path.generic_string() < rhs.path.generic_string();
  });
  return files;
}

Totals summarize(const std::vector<FileStats>& files) {
  Totals totals;
  totals.files = files.size();
  for (const auto& file : files) {
    totals.physical_lines += file.physical_lines;
    totals.filtered_lines += file.filtered_lines;
  }
  return totals;
}

void print_usage() {
  std::cerr << "usage: mercury-stats [--all] [root]\n";
}

} // namespace

int main(int argc, char** argv) {
  bool show_all = false;
  std::filesystem::path root = ".";

  for (int index = 1; index < argc; ++index) {
    const std::string_view arg(argv[index]);
    if (arg == "--help" || arg == "-h") {
      print_usage();
      return 0;
    }
    if (arg == "--all") {
      show_all = true;
      continue;
    }
    if (arg.starts_with("-")) {
      print_usage();
      return 2;
    }
    root = std::filesystem::path(arg);
  }

  std::error_code error;
  root = std::filesystem::weakly_canonical(root, error);
  if (error || !std::filesystem::is_directory(root)) {
    std::cerr << "error: root is not a readable directory\n";
    return 1;
  }

  const auto files = scan_tree(root);
  const auto totals = summarize(files);

  std::cout << "files=" << totals.files << '\n';
  std::cout << "physical_lines=" << totals.physical_lines << '\n';
  std::cout << "filtered_working_lines=" << totals.filtered_lines << '\n';

  if (show_all) {
    std::cout << "\n";
    std::cout << std::left << std::setw(48) << "path" << std::right << std::setw(12) << "physical"
              << std::setw(12) << "filtered" << '\n';
    for (const auto& file : files) {
      std::cout << std::left << std::setw(48) << file.path.generic_string() << std::right << std::setw(12)
                << file.physical_lines << std::setw(12) << file.filtered_lines << '\n';
    }
  }

  return 0;
}
