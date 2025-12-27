#pragma once

#include <sys/shm.h>
#include <unistd.h>

struct SharedCandidateState {
  pid_t pid;
  double scoreA;
  double scoreB;
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

  size_t getShmSize(int candidateCount);

  int shmId = -1;
  SharedMemoryData *data_ = nullptr;
  // TODO: Consider using some constant that makes sense as a project identifier
  static const int PROJECT_IDENTIFIER = 0x155288;
};
