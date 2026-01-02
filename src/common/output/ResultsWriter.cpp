#include "common/output/ResultsWriter.h"

#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include <fcntl.h>
#include <vector>

const char *ResultsWriter::fileName = "../output/lista_rankingowa.txt";

void ResultsWriter::publishResults(bool evacuation) {
  calculateScores(evacuation);

  int result = unlink(fileName);
  if (result != 0 && errno != ENOENT) {
    std::string errorMessage = "Failed to unlink file " +
                               std::string(fileName) + ": " +
                               std::strerror(errno);
    throw std::runtime_error(errorMessage);
  }

  int fileDescriptor = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0600);
  if (fileDescriptor == -1) {
    std::string errorMessage = "Failed to open file " + std::string(fileName) +
                               ": " + std::strerror(errno);
    throw std::runtime_error(errorMessage);
  }

  std::string content = std::string("| ==== Lista Rankingowa") +
                        (evacuation ? " (Ewakuacja)" : "") + " ==== |\n";
  content += "| PID | Numer | Matura | Cz. teoretyczna | Cz. "
             "praktyczna | Ostateczny wynik |\n";
  content += "|-----|-------|--------|----------------|--------"
             "--------|----------------|\n";

  content += getTableContent(evacuation);

  ssize_t bytesWritten =
      write(fileDescriptor, content.c_str(), content.length());
  if (bytesWritten < 0) {
    std::string errorMessage = std::string("Failed to write exam results to file: ") +
                               std::strerror(errno);
    throw std::runtime_error(errorMessage);
  }

  if (static_cast<size_t>(bytesWritten) != content.length()) {
    Logger::warn("Not all bytes written to file. Expected: " +
                 std::to_string(content.length()) +
                 ", Written: " + std::to_string(bytesWritten));
  }

  if (close(fileDescriptor) < 0) {
    std::string errorMessage = "Failed to close file " + std::string(fileName) +
                               ": " + std::strerror(errno);
    throw std::runtime_error(errorMessage);
  }
}

void ResultsWriter::calculateScores(bool evacuation) {
  for (int i = 0; i < SharedMemoryManager::data()->candidateCount; i++) {
    CandidateInfo *candidate = &SharedMemoryManager::data()->candidates[i];
    if (evacuation) {
      bool incomplete =
          candidate->theoreticalScore < 0.0 || candidate->practicalScore < 0.0;
      candidate->theoreticalScore = std::max(0.0, candidate->theoreticalScore);
      candidate->practicalScore = std::max(0.0, candidate->practicalScore);
      if (!incomplete) {
        candidate->finalScore =
            candidate->theoreticalScore * 0.5 + candidate->practicalScore * 0.5;
      }
    } else if (candidate->theoreticalScore < 30.0) {
      candidate->practicalScore = 0.0;
      candidate->finalScore = 0.0;
    } else {
      candidate->finalScore =
          candidate->theoreticalScore * 0.5 + candidate->practicalScore * 0.5;
    }
  }
}

std::string ResultsWriter::getTableContent(bool evacuation) {
  std::string notCompletedContent = "";
  std::string completedContent = "";
  std::vector<CandidateInfo> candidates;

  for (int i = 0; i < SharedMemoryManager::data()->candidateCount; i++) {
    CandidateInfo *candidate = &SharedMemoryManager::data()->candidates[i];
    if (evacuation && candidate->finalScore < 0.0) {
      notCompletedContent +=
          "| " + std::to_string(candidate->pid) + " | " + std::to_string(i) +
          " | " + (candidate->status != NotEligible ? "TAK" : "NIE") + " | " +
          std::to_string(candidate->theoreticalScore) + " | " +
          std::to_string(candidate->practicalScore) + " | NIE UKONCZONO |\n";
    } else {
      candidate->index = i;
      candidates.push_back(*candidate);
    }
  }

  std::sort(candidates.begin(), candidates.end(),
            [](const CandidateInfo &a, const CandidateInfo &b) {
              return a.finalScore > b.finalScore;
            });

  for (const CandidateInfo &candidate : candidates) {
    completedContent += "| " + std::to_string(candidate.pid) + " | " +
                        std::to_string(candidate.index) + " | " +
                        (candidate.status != NotEligible ? "TAK" : "NIE") +
                        " | " + std::to_string(candidate.theoreticalScore) +
                        " | " + std::to_string(candidate.practicalScore) +
                        " | " + std::to_string(candidate.finalScore) + " |\n";
  }

  if (!evacuation) {
    return completedContent;
  }

  return " === Ukonczyli egzamin ===\n" + completedContent +
         " === Nie ukończyli egzaminu ===\n" + notCompletedContent;
}
