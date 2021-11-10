#include <gtest/gtest.h>

#include "lockfree/sync_counter.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

namespace {

constexpr int NUM_WRITER_THREADS = 8;
constexpr int NUM_READER_THREADS = 8;
constexpr int NUM_THREADS = NUM_WRITER_THREADS + NUM_READER_THREADS;
constexpr std::chrono::seconds runtime(2);

using SyncCounter = lockfree::SyncCounter;

void increment(SyncCounter &counter, std::atomic<bool> &run, int id,
               uint64_t &numIncs) {
  numIncs = 0;
  while (run) {
    // unsycned increment is actually also ok as long as we read synced
    // counter.unsynced_increment();
    counter.increment();
    ++numIncs;
  }
}

void read(SyncCounter &counter, std::atomic<bool> &run, int id, uint64_t &max) {
  max = 0;
  while (run) {
    auto value = counter.sync();
    if (value > max) {
      max = value;
    }
  }
}

// Using try_write data cannot disappear by being discarded and can only be
// taken by exactly one thread. We hence can check whether no data is lost.
TEST(SyncCounterStressTest, counters_are_always_in_sync_when_read) {

  SyncCounter counter;
  std::vector<uint64_t> incs(NUM_WRITER_THREADS, 0);
  std::vector<uint64_t> maxRead(NUM_READER_THREADS, 1);
  std::vector<std::thread> writers;
  std::vector<std::thread> readers;
  writers.reserve(NUM_WRITER_THREADS);
  readers.reserve(NUM_READER_THREADS);

  std::atomic<bool> runReaders{true};
  std::atomic<bool> runWriters{true};

  for (int i = 0; i < NUM_READER_THREADS; ++i) {
    readers.emplace_back(&read, std::ref(counter), std::ref(runReaders), i,
                         std::ref(maxRead[i]));
  }

  for (int i = 0; i < NUM_WRITER_THREADS; ++i) {
    writers.emplace_back(&increment, std::ref(counter), std::ref(runWriters), i,
                         std::ref(incs[i]));
  }

  std::this_thread::sleep_for(runtime);
  runWriters = false;

  for (auto &writer : writers) {
    writer.join();
  }

  runReaders = false;

  // This is brittle but used to check whether all readers arrive at the same
  // counter once the increments stop To do this properly we would need state
  // information (that the increments stopped).
  std::this_thread::sleep_for(std::chrono::seconds(1));

  for (auto &reader : readers) {
    reader.join();
  }

  auto totalIncs = std::accumulate(incs.begin(), incs.end(), 0ULL);

  auto finalCount = counter.sync();

  std::cout << "final counter " << finalCount << std::endl;

  // final counter is equal to number of increments
  EXPECT_EQ(finalCount, totalIncs);

  // another sync should not change it
  finalCount = counter.sync();
  EXPECT_EQ(finalCount, totalIncs);

  auto values = counter.get_if_equal();
  EXPECT_EQ(values.first, values.second);

  // all readers agree on the final value
  for (auto &finalRead : maxRead) {
    std::cout << "final read " << finalRead << std::endl;
    EXPECT_EQ(finalCount, finalRead);
  }
}

} // namespace
