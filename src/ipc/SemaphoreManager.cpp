#include "ipc/SemaphoreManager.h"

#include "output/Logger.h"
#include <sys/sem.h>

SemaphoreManager &SemaphoreManager::shared() {
  static SemaphoreManager instance;
  return instance;
}

SemaphoreManager::~SemaphoreManager() {}

bool SemaphoreManager::initialize(int candidateCount) {
  Logger::shared().log("SemaphoreManager::initialize()");

  numSemaphores_ = getNumSemaphores(candidateCount);
  key_t semKey = getSemKey();

  // TODO: Clean it up
  int tempSemId = semget(semKey, 1, 0600);
  if (tempSemId != -1) {
    semctl(tempSemId, 0, IPC_RMID, nullptr);
  }

  Logger::shared().log(
      "Initializing semaphores with count: " + std::to_string(numSemaphores_) +
      " for key: " + std::to_string(semKey));

  semId_ = semget(semKey, numSemaphores_, IPC_CREAT | IPC_EXCL | 0600);
  if (semId_ == -1 && errno == EEXIST) {
    // TODO: Redundant?
    int oldSemId = semget(semKey, 0, 0600);
    if (oldSemId != -1) {
      semctl(oldSemId, 0, IPC_RMID);
    }
    semId_ = semget(semKey, numSemaphores_, IPC_CREAT | IPC_EXCL | 0600);
  }

  if (semId_ == -1) {
    // TODO: Error handling
    Logger::shared().log("ERROR: semget failed: " +
                         std::string(strerror(errno)));
    return false;
  }

  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  } arg;

  arg.val = 0;
  for (int i = 0; i < numSemaphores_; i++) {
    if (semctl(semId_, i, SETVAL, arg) == -1) {
      // TODO: Error handling
      Logger::shared().log(
          "ERROR: semctl SETVAL failed for sem: " + std::to_string(i) + ": " +
          std::string(strerror(errno)));
      return false;
    }
  }

  return true;
}

bool SemaphoreManager::attach() {
  Logger::shared().log("SemaphoreManager::attach()");

  key_t semKey = getSemKey();
  semId_ = semget(semKey, 0, 0600);
  if (semId_ == -1) {
    // TODO: Error handling
    Logger::shared().log("ERROR: semget attach failed: " +
                         std::string(strerror(errno)));
    return false;
  }

  struct semid_ds buf;
  union semun {
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO */
  } arg;
  arg.buf = &buf;

  if (semctl(semId_, 0, IPC_STAT, arg) == -1) {
    // TODO: Error handling
    Logger::shared().log("ERROR: semctl IPC_STAT failed");
    return false;
  }

  numSemaphores_ = buf.sem_nsems;
  return true;
}

void SemaphoreManager::wait(int semIndex) {
  struct sembuf op;
  op.sem_num = static_cast<unsigned short>(semIndex);
  op.sem_op = -1;
  op.sem_flg = 0;

  if (semop(semId_, &op, 1) == -1) {
    // TODO: Error handling
    Logger::shared().log("ERROR: semop wait failed for sem " +
                         std::to_string(semIndex) + ": " +
                         std::string(strerror(errno)));
  }
}

void SemaphoreManager::signal(int semIndex) {
  struct sembuf op;
  op.sem_num = static_cast<unsigned short>(semIndex);
  op.sem_op = 1;
  op.sem_flg = 0;

  if (semop(semId_, &op, 1) == -1) {
    // TODO: Error handling
    Logger::shared().log("ERROR: semop signal failed for sem " +
                         std::to_string(semIndex) + ": " +
                         std::string(strerror(errno)));
  }
}

void SemaphoreManager::destroy() {
  Logger::shared().log("SemaphoreManager::destroy()");
  if (semId_ != -1) {
    if (semctl(semId_, 0, IPC_RMID) == -1) {
      // TODO: Error handling
      Logger::shared().log("ERROR: semctl IPC_RMID failed");
    }
    semId_ = -1;
  }
}

key_t SemaphoreManager::getSemKey() { return ftok("/tmp", PROJECT_IDENTIFIER); }

int SemaphoreManager::getNumSemaphores(int candidateCount) {
  return totalSemaphores(candidateCount);
}