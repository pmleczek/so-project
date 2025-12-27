#pragma once

#include <sys/shm.h>
#include <unistd.h>

struct SharedCandidateState {
  int id;
  pid_t pid;
  double scoreA;
  double scoreB;
  double finalScore;
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
