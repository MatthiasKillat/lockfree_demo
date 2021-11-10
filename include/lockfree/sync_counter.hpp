#pragma once

#include <assert.h>
#include <atomic>
#include <chrono>
#include <thread>

namespace lockfree {

// TODO: document and explain, restructure (.cpp etc.)
class SyncCounter {
  static constexpr int N = 1024;

public:
  SyncCounter();

  // trigger increment of both counters
  void increment();

  // unsynced increment without trying to equalize counters
  void unsynced_increment();

  // sync the counters before retrieving the value (they are both equal)
  uint64_t sync();

  // retrieve both values, sync to ensure they are equal (redundant)
  std::pair<uint64_t, uint64_t> get_if_equal();

private:
  using counter_t = std::atomic<uint64_t>;
  // separation of counters by dummy memory
  // to have both counters in different cachelines
  counter_t m_counters[4096];

  counter_t &m_count1;
  counter_t &m_count2;

  void try_help(uint64_t &count1, uint64_t &count2);

  void sleep(int ms = 1);
};

} // namespace lockfree
