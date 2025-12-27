#pragma once

#include <unistd.h>

struct CandidateMessage {
  long mtype;
  int candidateId;
  pid_t candidatePid;
  char data[256];
};
