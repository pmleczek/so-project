#pragma once

#include <string>

class Time {
public:
  static int seconds(const std::string &timeStr);
  static int now();
};