#pragma once

#include <sys/shm.h>
#include <unistd.h>

enum CandidateStatus {
  PENDING = 0,
  ELIGIBLE = 1,
  INELIGIBLE = 2,
  PENDING_COMMISSION_A = 3,
  FAILED_COMMISSION_A = 4,
  PENDING_COMMISSION_B = 5,
  COMPLETED = 6,
};

struct SharedCandidateState {
  pid_t pid;
  double scoreA;
  double scoreB;
  double finalScore;
  CandidateStatus status;
  bool passedExam;
  bool reattempt;
};

struct SharedMemoryData {
  int candidateCount;
  SharedCandidateState candidates[];
};

class SharedMemoryManager {
public:
  static SharedMemoryManager &shared();
  bool attach();
  SharedMemoryData *data();
  void destroy();
  void detach();
  bool initialize(int candidateCount);

private:
  SharedMemoryManager() = default;
  ~SharedMemoryManager();

  key_t getShmKey();
  size_t getShmSize(int candidateCount);

  int shmId = -1;
  SharedMemoryData *data_ = nullptr;
  // TODO: Consider using some constant that makes sense as a project identifier
  static const int PROJECT_IDENTIFIER = 155;
};
