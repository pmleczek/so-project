#include "dean/DeanProcess.h"

#include "common/ipc/SemaphoreManager.h"
#include "common/ipc/SharedMemoryManager.h"
#include "common/output/Logger.h"
#include "common/process/ProcessRegistry.h"
#include "common/utils/Random.h"
#include "common/utils/Time.h"
#include <signal.h>
#include <unistd.h>

DeanProcess *DeanProcess::instance_ = nullptr;

DeanProcess::DeanProcess(int argc, char *argv[]) : config() {
  Logger::setupLogFile();
  instance_ = this;
  Logger::info("Dean process started (pid=" + std::to_string(getpid()) + ")");
  initialize(argc, argv);
}

int DeanProcess::assertPlaceCount(int argc, char *argv[]) {
  if (argc != 3) {
    Logger::error("Usage: ./dean <place count> <start time>");
    Logger::error("Example: ./dean 120 12:34");
    exit(1);
  }

  int placeCount = std::stoi(argv[1]);
  if (placeCount < 1) {
    Logger::error("Place count must be greater than 0");
    exit(1);
  }

  return placeCount;
}

void DeanProcess::initialize(int argc, char *argv[]) {
  Logger::info("DeanProcess::initialize()");

  signal(SIGINT, DeanProcess::terminationHandler);
  signal(SIGTERM, DeanProcess::terminationHandler);
  signal(SIGQUIT, DeanProcess::terminationHandler);
  signal(SIGUSR1, DeanProcess::evacuationHandler);

  int placeCount = assertPlaceCount(argc, argv);
  std::string startTime = argv[2];

  config = DeanConfig(placeCount, startTime);

  SharedMemoryManager::initialize(config.candidateCount);
  SemaphoreManager::create("commissionA", 0);
  SemaphoreManager::create("commissionB", 0);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&SharedMemoryManager::data()->seatsMutex, &attr);
  pthread_mutexattr_destroy(&attr);

  for (int i = 0; i < 3; i++) {
    SharedMemoryManager::data()->commissionA.seats[i].pid = -1;
    SharedMemoryManager::data()->commissionA.seats[i].questionsCount = 0;
    SharedMemoryManager::data()->commissionA.seats[i].answered = false;

    SharedMemoryManager::data()->commissionB.seats[i].pid = -1;
    SharedMemoryManager::data()->commissionB.seats[i].questionsCount = 0;
    SharedMemoryManager::data()->commissionB.seats[i].answered = false;
  }

  SharedMemoryManager::data()->candidateCount = config.candidateCount;
}

void DeanProcess::waitForExamStart() {
  Logger::info("DeanProcess::waitForExamStart()");
  int start = Time::seconds(config.startTime);
  int current = Time::now();
  int sleepTime = start - current;

  if (sleepTime < 0) {
    // TODO: Add better sleep logic or handle invalid start time
    sleepTime = 15;
  }

  Logger::info("Sleeping for " + std::to_string(sleepTime) + " seconds");
  sleep(sleepTime);
}

void DeanProcess::start() {
  Logger::info("DeanProcess::start()");
  SharedMemoryManager::data()->examStarted = true;

  while (SharedMemoryManager::data()->examStarted) {
    usleep(1000000);
  }

  Logger::info("Exam ended");
}

void DeanProcess::spawnComissions() {
  Logger::info("DeanProcess::spawnComissions()");

  // Comission A
  pid_t pidA = fork();
  if (pidA == 0) {
    execlp("./commission", "./commission", "A", NULL);
  }
  ProcessRegistry::registerCommission(pidA, 'A');

  // Comission B
  pid_t pidB = fork();
  if (pidB == 0) {
    execlp("./commission", "./commission", "B", NULL);
  }
  ProcessRegistry::registerCommission(pidB, 'B');
}

std::unordered_set<int> DeanProcess::getFailedExamIndices() {
  auto indices =
      Random::randomInts(config.failedExamCount, 0, config.candidateCount - 1);

  Logger::info("Failed exam indices (" + std::to_string(indices.size()) +
               "): ");
  for (int index : indices) {
    Logger::info("- " + std::to_string(index));
  }

  return indices;
}

std::unordered_set<int>
DeanProcess::getRetakeExamIndices(std::unordered_set<int> excludedIndices) {
  auto indices = Random::randomInts(config.retakeExamCount, 0,
                                    config.candidateCount - 1, excludedIndices);

  Logger::info("Retake exam indices (" + std::to_string(indices.size()) +
               "): ");
  for (int index : indices) {
    Logger::info("- " + std::to_string(index));
  }

  return indices;
}

void DeanProcess::spawnCandidates() {
  Logger::info("DeanProcess::spawnCandidates()");

  Logger::info("Spawning " + std::to_string(config.candidateCount) +
               " candidates");

  std::unordered_set<int> failedExamIndices = getFailedExamIndices();
  std::unordered_set<int> retakeExamIndices =
      getRetakeExamIndices(failedExamIndices);

  pid_t candidatePid;
  bool failed, retake;

  for (int i = 0; i < config.candidateCount; i++) {
    candidatePid = fork();
    // TODO: Handle fork error
    if (candidatePid < 0) {
      Logger::error("Failed to fork candidate process " + std::to_string(i));
      exit(1);
    }

    if (candidatePid == 0) {
      execlp("./candidate", "./candidate", std::to_string(i).c_str(), NULL);
    } else {
      SharedMemoryManager::data()->candidates[i].pid = candidatePid;
      SharedMemoryManager::data()->candidates[i].practicalScore = -1.0;
      SharedMemoryManager::data()->candidates[i].finalScore = -1.0;

      failed = failedExamIndices.find(i) != failedExamIndices.end();
      retake = retakeExamIndices.find(i) != retakeExamIndices.end();

      if (failed) {
        SharedMemoryManager::data()->candidates[i].status = NotEligible;
      } else {
        SharedMemoryManager::data()->candidates[i].status = PendingCommissionA;
      }

      if (retake) {
        double theoreticalScore = Random::randomDouble(30.0, 100.0);
        SharedMemoryManager::data()->candidates[i].theoreticalScore =
            theoreticalScore;
        retaking++;
      } else {
        SharedMemoryManager::data()->candidates[i].theoreticalScore = -1.0;
      }
    }

    // TODO: Investigate
    // For stability reasons
    usleep(10000);
  }
}

void DeanProcess::verifyCandidates() {
  Logger::info("DeanProcess::verifyCandidates()");

  int rejected = 0;
  for (int i = 0; i < config.candidateCount; i++) {
    if (SharedMemoryManager::data()->candidates[i].status == NotEligible) {
      kill(SharedMemoryManager::data()->candidates[i].pid, SIGUSR2);
      rejected++;
    }
  }

  int commissonACount = config.candidateCount - rejected - retaking;
  SharedMemoryManager::data()->commissionACandidateCount = commissonACount;
  SharedMemoryManager::data()->commissionBCandidateCount = commissonACount;
}

void DeanProcess::cleanup() {
  Logger::info("DeanProcess::cleanup()");

  SharedMemoryManager::destroy();
  SemaphoreManager::unlink("/commissionA");
  SemaphoreManager::unlink("/commissionB");

  Logger::info("Dean process exiting with status 0");
}

void DeanProcess::evacuationHandler(int signal) {
  Logger::info("DeanProcess::evacuationHandler()");
  Logger::info("Evacuation signal received");

  ProcessRegistry::propagateSignal(signal);

  if (instance_) {
    instance_->cleanup();
    instance_ = nullptr;
  }

  exit(0);
}

void DeanProcess::terminationHandler(int signal) {
  Logger::info("DeanProcess::terminationHandler()");
  Logger::info("Termination signal: SIG " + std::to_string(signal) +
               " received");

  ProcessRegistry::propagateSignal(SIGTERM);

  if (instance_) {
    instance_->cleanup();
    instance_ = nullptr;
  }

  exit(0);
}
