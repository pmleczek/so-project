#pragma once

#include <string>

class ResultsWriter {
public:
  static void publishResults(bool evacuation);

private:
  static void calculateScores(bool evacuation);
  static std::string getTableContent(bool evacuation);

  static const char *fileName;
};
