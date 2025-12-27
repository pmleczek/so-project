#include "utils/validation.h"

#include "output/Logger.h"

ArgValidator::ArgValidator(std::vector<ArgValidatorRule> rules)
    : rules_(rules) {}

std::optional<std::unordered_map<std::string, int>>
ArgValidator::validate(int argc, char *argv[]) {
  Logger::shared().log("Validating arguments");

  if (argc - 1 != rules_.size()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, int> args;
  for (int i = 0; i < rules_.size(); i++) {
    auto rule = rules_[i];
    int arg = std::stoi(argv[i + 1]);

    if (rule.min.has_value() && arg < rule.min.value()) {
      return std::nullopt;
    }

    if (rule.max.has_value() && arg > rule.max.value()) {
      return std::nullopt;
    }

    args[rule.name] = arg;
  }

  return args;
}