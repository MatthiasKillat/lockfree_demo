#pragma once

#include <assert.h>
#include <atomic>
#include <chrono>
#include <thread>

namespace lockfree {
class SyncCounter {
  static constexpr int N = 1024;

public:
  SyncCounter();

  void increment();

  void unsynced_increment();

  uint64_t sync_get();

  std::pair<uint64_t, uint64_t> get_if_equal();
  {
    uint64_t count1 = m_count1.load();
    uint64_t count2;
    do {
      count2 = m_count2.load();
    } while (!m_count1.compare_exchange_strong(count1, count1));
    return {count1, count2};
  }

private:
  using counter_t = std::atomic<uint64_t>;
  counter_t m_counters[1024]; // separation of counters by dummy memory

  counter_t &m_count1;
  counter_t &m_count2;

  void try_help(uint64_t &count1, uint64_t &count2);

  void sleep(int ms = 1);
};

} // namespace lockfree
