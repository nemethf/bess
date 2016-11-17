#ifndef BESS_MODULES_BYPASS_H_
#define BESS_MODULES_BYPASS_H_

#include "../module.h"

/* This module simply passes packets from input gate X down to output gate X
 * (the same gate index) */

class Bypass : public Module {
 public:
  static const gate_idx_t kNumIGates = MAX_GATES;
  static const gate_idx_t kNumOGates = MAX_GATES;

  virtual void ProcessBatch(bess::PacketBatch *batch);
};

#endif  // BESS_MODULES_BYPASS_H_
