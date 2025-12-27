#pragma once

#include "CandidateMessage.h"
#include "MessageType.h"
#include <sys/msg.h>
#include <sys/ipc.h>
#include <unistd.h>

class MessageQueueManager {
public:
  static MessageQueueManager &shared();
  bool initialize();
  bool attach();
  bool send(MessageType type, int candidateId, pid_t candidatePid,
            const char *data = nullptr);
  bool receive(MessageType type, CandidateMessage &msg);
  bool receiveNonBlocking(MessageType type, CandidateMessage &msg);
  void destroy();

private:
  MessageQueueManager() = default;
  ~MessageQueueManager();

  key_t getMsgKey();

  int msgId_ = -1;
  static const int PROJECT_IDENTIFIER = 157;
};