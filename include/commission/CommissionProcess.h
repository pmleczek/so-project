#pragma once

#include <string>

class CommissionProcess {
public:
  void run(int argc, char *argv[]);

private:
  void initialize(int argc, char *argv[]);

  std::string name_;
};
