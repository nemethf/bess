#include "module.h"

#include <array>

#include <benchmark/benchmark.h>
#include <glog/logging.h>

namespace {

class DummySourceModule : public Module {
 public:
  virtual struct task_result RunTask(void *arg) override;
};

[[gnu::noinline]] struct task_result DummySourceModule::RunTask(void *arg) {
  const size_t batch_size = reinterpret_cast<size_t>(arg);
  bess::PacketBatch batch;

  bess::Packet pkts[bess::PacketBatch::kMaxBurst];

  batch.clear();
  for (size_t i = 0; i < batch_size; i++) {
    bess::Packet *pkt = &pkts[i];

    // this fake packet must not be freed
    rte_mbuf_refcnt_set(&pkt->mbuf(), 2);

    // not chained
    struct rte_mbuf &m = pkt->mbuf();
    m.next = nullptr;

    batch.add(pkt);
  }

  RunNextModule(&batch);

  return {.packets = static_cast<uint64_t>(batch_size), .bits = 0};
}

class DummyRelayModule : public Module {
 public:
  virtual void ProcessBatch(bess::PacketBatch *batch) override;
};

[[gnu::noinline]] void DummyRelayModule::ProcessBatch(
    bess::PacketBatch *batch) {
  RunNextModule(batch);
}

// Simple harness for testing the Module class.
class ModuleFixture : public benchmark::Fixture {
 protected:
  virtual void SetUp(benchmark::State &state) override {
    const int chain_length = state.range(0);

    ADD_MODULE(DummySourceModule, "src", "the most sophisticated modue ever");
    ADD_MODULE(DummyRelayModule, "relay", "the most sophisticated modue ever");
    assert(__module__DummySourceModule);
    assert(__module__DummyRelayModule);

    const auto &builders = ModuleBuilder::all_module_builders();
    const auto &builder_src = builders.find("DummySourceModule")->second;
    const auto &builder_relay = builders.find("DummyRelayModule")->second;
    Module *last;

    src_ = builder_src.CreateModule("src0", &bess::metadata::default_pipeline);
    last = src_;

    for (int i = 0; i < chain_length; i++) {
      Module *relay = builder_relay.CreateModule(
          "relay" + std::to_string(i), &bess::metadata::default_pipeline);
      relays.push_back(relay);
      int ret = last->ConnectModules(0, relay, 0);
      assert(ret == 0);
      last = relay;
    }
  }

  virtual void TearDown(benchmark::State &) override {
    ModuleBuilder::DestroyAllModules();
    ModuleBuilder::all_module_builders_holder(true);
  }

  Module *src_;
  std::vector<Module *> relays;
};

}  // namespace (unnamed)

BENCHMARK_DEFINE_F(ModuleFixture, Chain)(benchmark::State &state) {
  const size_t batch_size = bess::PacketBatch::kMaxBurst;

  struct task t;
  t.m = src_;
  t.arg = reinterpret_cast<void *>(batch_size);

  while (state.KeepRunning()) {
    struct task_result ret = task_scheduled(&t);
    assert(ret.packets == batch_size);
  }

  state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK_REGISTER_F(ModuleFixture, Chain)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Arg(5)
    ->Arg(6)
    ->Arg(7)
    ->Arg(8)
    ->Arg(9)
    ->Arg(10);

BENCHMARK_MAIN()
