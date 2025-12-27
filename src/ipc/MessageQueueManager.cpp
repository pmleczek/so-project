#include "ipc/MessageQueueManager.h"

#include "output/Logger.h"
#include <cerrno>
#include <cstring>
#include <unistd.h>

MessageQueueManager &MessageQueueManager::shared() {
  static MessageQueueManager instance;
  return instance;
}

MessageQueueManager::~MessageQueueManager() {}

bool MessageQueueManager::initialize() {
  Logger::shared().log("MessageQueueManager::initialize()");

  key_t msgKey = getMsgKey();

  int tempId = msgget(msgKey, 0600);
  if (tempId != -1) {
    msgctl(tempId, IPC_RMID, nullptr);
  }

  // Try to remove existing queue first
  // int oldMsgId = msgget(msgKey, 0600);
  // if (oldMsgId != -1) {
  //   Logger::shared().log("Found existing message queue, removing...");
  //   if (msgctl(oldMsgId, IPC_RMID, nullptr) == -1) {
  //     Logger::shared().log("WARNING: Failed to remove old message queue: " +
  //                          std::string(strerror(errno)));
  //   } else {
  //     Logger::shared().log("Removed old message queue");
  //     usleep(10000);
  //   }
  // }

  msgId_ = msgget(msgKey, IPC_CREAT | IPC_EXCL | 0600);
  if (msgId_ == -1) {
    Logger::shared().log("ERROR: msgget failed: " +
                         std::string(strerror(errno)));
    return false;
  }

  Logger::shared().log("Message queue initialized successfully (key: " +
                       std::to_string(msgKey) + ")");
  return true;
}

bool MessageQueueManager::attach() {
  Logger::shared().log("MessageQueueManager::attach()");

  key_t msgKey = getMsgKey();
  msgId_ = msgget(msgKey, 0600);
  if (msgId_ == -1) {
    Logger::shared().log("ERROR: msgget attach failed: " +
                         std::string(strerror(errno)));
    return false;
  }

  Logger::shared().log("Attached to message queue successfully");
  return true;
}

bool MessageQueueManager::send(MessageType type, int candidateId,
                               pid_t candidatePid, const char *data) {
  if (msgId_ == -1) {
    Logger::shared().log("ERROR: Message queue not initialized/attached");
    return false;
  }

  CandidateMessage msg;
  msg.mtype = type;
  msg.candidateId = candidateId;
  msg.candidatePid = candidatePid;

  if (data != nullptr) {
    strncpy(msg.data, data, 255);
    msg.data[255] = '\0';
  } else {
    msg.data[0] = '\0';
  }

  if (msgsnd(msgId_, &msg, sizeof(CandidateMessage) - sizeof(long), 0) == -1) {
    Logger::shared().log("ERROR: msgsnd failed: " +
                         std::string(strerror(errno)));
    return false;
  }

  return true;
}

bool MessageQueueManager::receive(MessageType type, CandidateMessage &msg) {
  if (msgId_ == -1) {
    Logger::shared().log("ERROR: Message queue not initialized/attached");
    return false;
  }

  ssize_t result =
      msgrcv(msgId_, &msg, sizeof(CandidateMessage) - sizeof(long), type, 0);
  if (result == -1) {
    Logger::shared().log("ERROR: msgrcv failed: " +
                         std::string(strerror(errno)));
    return false;
  }

  return true;
}

bool MessageQueueManager::receiveNonBlocking(MessageType type,
                                             CandidateMessage &msg) {
  if (msgId_ == -1) {
    return false;
  }

  ssize_t result = msgrcv(msgId_, &msg, sizeof(CandidateMessage) - sizeof(long),
                          type, IPC_NOWAIT);
  if (result == -1) {
    if (errno == ENOMSG) {
      return false;
    }
    Logger::shared().log("ERROR: msgrcv failed: " +
                         std::string(strerror(errno)));
    return false;
  }

  return true;
}

void MessageQueueManager::destroy() {
  Logger::shared().log("MessageQueueManager::destroy()");

  if (msgId_ != -1) {
    if (msgctl(msgId_, IPC_RMID, nullptr) == -1) {
      Logger::shared().log("ERROR: msgctl IPC_RMID failed: " +
                           std::string(strerror(errno)));
    } else {
      Logger::shared().log("Message queue destroyed successfully");
    }
    msgId_ = -1;
  }
}

key_t MessageQueueManager::getMsgKey() {
  return ftok("/tmp", PROJECT_IDENTIFIER);
}
