#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct ArgValidatorRule {
  std::string name;
  std::optional<int> min;
  std::optional<int> max;
};

class ArgValidator {
public:
  ArgValidator(std::vector<ArgValidatorRule> rules);

  std::optional<std::unordered_map<std::string, int>> validate(int argc,
                                                               char *argv[]);

private:
  std::vector<ArgValidatorRule> rules_;
};