#include "candidate/CandidateProcess.h"
#include "ipc/MessageQueueManager.h"
#include "ipc/SemaphoreManager.h"
#include "ipc/SharedMemoryManager.h"
#include "output/Logger.h"
#include "utils/validation.h"
#include <unistd.h>

void CandidateProcess::run(int argc, char *argv[]) {
  // Step 1: Initialize candidate process
  initialize(argc, argv);
  // Step : Cleanup
  cleanup();
  // Step : Exit
  exit(0);
}

void CandidateProcess::initialize(int argc, char *argv[]) {
  Logger::shared().log("Initializing candidate process...");
  SharedMemoryManager::shared().attach();
  SemaphoreManager::shared().attach();
  MessageQueueManager::shared().attach();
  // TOOD: Handle error in above

  auto args = ArgValidator({
                               {.name = "candidateID", .min = 0},
                               {.name = "reattempt", .min = 0, .max = 1},
                           })
                  .validate(argc, argv);
  if (!args.has_value()) {
    // TODO: Error handling
    Logger::shared().log("Invalid arguments");
    exit(1);
  }

  Logger::shared().log("Initializing candidate config for process " +
                       std::to_string(getpid()));
  // TODO: Direct set?
  setConfig(CandidateConfig(args->at("candidateID"), args->at("reattempt")));
}

void CandidateProcess::waitForEligibility() {
  Logger::shared().log("Waiting for eligibility...");

  // SemaphoreManager &sm = SemaphoreManager::shared();
  // sm.wait(eligibilitySem(config_.candidateID));

  MessageQueueManager &mq = MessageQueueManager::shared();
  CandidateMessage msg;
  if (mq.receive(static_cast<MessageType>(msgType), msg)) {
    if (msg.data[0] == '0') {
      Logger::shared().log("Process " + std::to_string(getpid()) +
                           " exiting due to candidate not being eligible");
      cleanup();
      exit(0);
    }
  }

  // SharedMemoryManager &shm = SharedMemoryManager::shared();
  // SharedMemoryData *data = shm.data();
  // if (data != nullptr) {
  //   SharedCandidateState &state = data->candidates[config_.candidateID];
  //   Logger::shared().log("Process " + std::to_string(getpid()) +
  //                        " exiting due to candidate not being eligible");
  //   cleanup();
  //   exit(0);
  // }
}

void CandidateProcess::cleanup() {
  Logger::shared().log("Cleaning up candidate process...");
  SharedMemoryManager::shared().detach();
}

const CandidateConfig &CandidateProcess::getConfig() { return config_; }

void CandidateProcess::setConfig(const CandidateConfig &config) {
  config_ = config;
}
