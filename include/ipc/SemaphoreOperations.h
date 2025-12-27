#pragma once

const int BASE_ELIGIBILITY = 0;
const int BASE_COMMISSION_A_INVITE = 1;
const int BASE_COMMISSION_A_QUESTION = 2;
const int BASE_COMMISSION_B_REQUEST = 3;
const int BASE_COMMISSION_B_QUESTION = 4;
const int BASE_ANSWER_READY = 5;

const int COMMISSION_A_ELIGIBLE_LIST = 1000;
const int COMMISSION_B_READY = 1001;
const int COMMISSION_A_RESULTS = 1002;
const int COMMISSION_B_RESULTS = 1003;

inline int eligibilitySem(int candidateId) {
  return BASE_ELIGIBILITY + candidateId;
}

inline int commissionAInviteSem(int candidateId) {
  return BASE_COMMISSION_A_INVITE + candidateId;
}

inline int commissionAQuestionSem(int candidateId) {
  return BASE_COMMISSION_A_QUESTION + candidateId;
}

inline int commissionBRequestSem(int candidateId) {
  return BASE_COMMISSION_B_REQUEST + candidateId;
}

inline int commissionBQuestionSem(int candidateId) {
  return BASE_COMMISSION_B_QUESTION + candidateId;
}

inline int answerReadySem(int candidateId) {
  return BASE_ANSWER_READY + candidateId;
}

inline int totalSemaphores(int candidateCount) {
  return 1004 + candidateCount * 6;
}
