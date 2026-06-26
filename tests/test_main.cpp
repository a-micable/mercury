#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mercury::test {

using Test = std::pair<std::string, std::function<void()>>;

std::vector<Test>& registry() {
  static std::vector<Test> tests;
  return tests;
}

void add(std::string name, std::function<void()> fn) {
  registry().push_back({std::move(name), std::move(fn)});
}

} // namespace mercury::test

int main() {
  int failed = 0;
  for (const auto& [name, test] : mercury::test::registry()) {
    try {
      test();
      std::cout << "[pass] " << name << '\n';
    } catch (const std::exception& ex) {
      ++failed;
      std::cerr << "[fail] " << name << ": " << ex.what() << '\n';
    }
  }
  return failed == 0 ? 0 : 1;
}
