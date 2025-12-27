#include "output/ResultsWriter.h"

#include "ipc/SharedMemoryManager.h"
#include "output/Logger.h"
#include "utils/constants.h"
#include <fstream>
#include <string>

void ResultsWriter::writeResults() {
  Logger::shared().log("Writing results...");

  SharedMemoryManager &shm = SharedMemoryManager::shared();
  SharedMemoryData *data = shm.data();
  if (data == nullptr) {
    // TODO: Error handling
    Logger::shared().log("Failed to get shared memory data");
    return;
  }

  std::ofstream resultsFile(Constants::resultsFilePath());
  if (!resultsFile.is_open()) {
    // TODO: Error handling
    Logger::shared().log("Failed to open results file");
    return;
  }

  resultsFile << "Id Kandydata, Wynik (część A), Wynik (część B), Wynik "
                 "(końcowy), Zdał egzamin, Ponowne podejście, Matura"
              << std::endl;

  for (int i = 0; i < data->candidateCount; i++) {
    SharedCandidateState &candidate = data->candidates[i];
    resultsFile << candidate.id << "," << candidate.scoreA << ","
                << candidate.scoreB << "," << candidate.finalScore << ","
                << "TBD" << "," << (candidate.reattempt ? "Tak" : "Nie") << ","
                << (candidate.passedExam ? "Tak" : "Nie") << std::endl;
  }

  resultsFile.close();
}
