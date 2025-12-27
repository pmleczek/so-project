#include "utils/constants.h"

std::string Constants::logFilePath() {
  return std::string("../") + GeneralConstants::OUTPUT_DIRECTORY + "/" +
         GeneralConstants::LOG_FILENAME;
}

std::string Constants::resultsFilePath() {
  return std::string("../") + GeneralConstants::OUTPUT_DIRECTORY + "/" +
         GeneralConstants::RESULTS_FILENAME;
}

std::string Constants::outputDirectory() {
  return std::string("../") + GeneralConstants::OUTPUT_DIRECTORY;
}
