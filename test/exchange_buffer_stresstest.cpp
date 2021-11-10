#include <gtest/gtest.h>

#include "lockfree/exchange_buffer.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

namespace {

// All tests use concurrent threads for reading and writing and check for
// specific conditions during the test or at the end of the test. E.g. 1) No
// data gets lost
//      2) data arrives in order
// Failures (including crashes) indicate the data structure being corrupted
// somehow. However, success cannot prove correctness, since (rare) race
// conditions may be avoided by chance.

// We can vary the number of writer threads, reader threads and time to run the
// test. The idea is that the longer we run the test, the larger the chance to
// encounter a possible defect caused by e.g. a data race.

// TODO: make more/easier configurable in e.g. a test fixture.
constexpr int NUM_WRITER_THREADS = 4;
constexpr int NUM_READER_THREADS = 4;
constexpr int NUM_THREADS = NUM_WRITER_THREADS + NUM_READER_THREADS;

constexpr std::chrono::seconds runtime(5);

using Buffer = lockfree::ExchangeBuffer<uint64_t, NUM_THREADS + 1>;

void try_write(Buffer &buffer, std::atomic<bool> &run, int id, uint64_t &max) {
  max;
  while (run) {
    // write and increase max value written if successful
    if (buffer.try_write(max + 1)) {
      ++max;
      // std::cout << "thread " << id << " wrote " << max << std::endl;
    }
  }
}

void take(Buffer &buffer, std::atomic<bool> &run, int id, uint64_t &sum) {
  sum = 0;
  while (run) {
    // add value to local sum if it is successfully taken from buffer
    auto result = buffer.take();
    if (result.has_value()) {
      // std::cout << "thread " << id << " took " << *result << std::endl;
      sum += *result;
    }
  }
}

uint64_t gauss_sum(uint64_t n) { return n * (n + 1) / 2; }

// Using try_write data cannot disappear by being discarded and can only be
// taken by exactly one thread. We hence can check whether no data is lost.
TEST(ExchangeBufferStressTest, using_try_write_and_take_we_lose_no_data) {

  Buffer buffer;
  std::vector<uint64_t> maxs(NUM_WRITER_THREADS, 0);
  std::vector<uint64_t> sums(NUM_READER_THREADS, 0);
  std::vector<std::thread> writers;
  std::vector<std::thread> readers;
  writers.reserve(NUM_WRITER_THREADS);
  readers.reserve(NUM_READER_THREADS);

  std::atomic<bool> run{true};
  for (int i = 0; i < NUM_READER_THREADS; ++i) {
    readers.emplace_back(&take, std::ref(buffer), std::ref(run), i,
                         std::ref(sums[i]));
  }

  for (int i = 0; i < NUM_WRITER_THREADS; ++i) {
    writers.emplace_back(&try_write, std::ref(buffer), std::ref(run), i,
                         std::ref(maxs[i]));
  }

  std::this_thread::sleep_for(runtime);
  run = false;

  for (auto &writer : writers) {
    writer.join();
  }

  for (auto &reader : readers) {
    reader.join();
  }

  uint64_t expectedSum = 0;
  for (auto max : maxs) {
    std::cout << max << std::endl;
    expectedSum += gauss_sum(max);
  }

  auto sum = std::accumulate(sums.begin(), sums.end(), 0ULL);

  // there may be one value still in the buffer
  auto value = buffer.take();
  if (value) {
    sum += *value;
  }

  // NB: The sum can be equal even though we receive not all elements, but this
  // is unlikely. We could also just write 1 but this increases the chance for
  // false positives.
  // By just keeping track of the sum We avoid memorizing what
  // data we have already seen (requires synchronization across threads or large
  // containers to store data received per thread)
  EXPECT_EQ(expectedSum, sum);
}

void write1(Buffer &buffer, std::atomic<bool> &run, int id, uint64_t &max) {
  max = 0;
  while (run) {
    // write and increase max value written if successful
    if (buffer.write(max + 1)) {
      ++max;
      // std::cout << "thread " << id << " wrote " << max << std::endl;
    }
  }
}

void read1(Buffer &buffer, std::atomic<bool> &run, int id, int &ordered,
           uint64_t &max) {
  ordered = 1;
  uint64_t prevValue = 0;
  max = 0;
  while (run) {
    auto result = buffer.read();
    if (result.has_value()) {
      if (*result < prevValue) {
        ordered = 0;
      }
      prevValue = *result;
      if (max < prevValue) {
        max = prevValue;
      }
    }
  }
}

// If we use write we cannot guarantee that all data arrives at readers
// since write discards data at overflow. But we expect to receive data in order
// per thread.
// If we modify write to return the evicted value from buffer we could again
// check that nothing is lost.
// This can be extended to multiple writers in which
// case each reader must keep track of the the latest data it has seen from each
// individual writer (by adding the id to the data written) like so
// struct Data {
//   int id;
//   uint64_t value;
// };
//
// using DataBuffer = lockfree::ExchangeBuffer<Data, NUM_THREADS + 1>;
//
// This is actually more interesting for queues (e.g. more than one slot in the
// buffer).
TEST(ExchangeBufferStressTest,
     using_single_writer_write_and_we_read_data_in_ascending_order) {

  Buffer buffer;
  uint64_t maxWritten;
  std::vector<int> ordered(NUM_READER_THREADS, 1);
  std::vector<uint64_t> maxs(NUM_READER_THREADS, 0);
  std::vector<std::thread> readers;
  readers.reserve(NUM_READER_THREADS);

  std::atomic<bool> run{true};
  for (int i = 0; i < NUM_READER_THREADS; ++i) {
    readers.emplace_back(&read1, std::ref(buffer), std::ref(run), i,
                         std::ref(ordered[i]), std::ref(maxs[i]));
  }

  std::thread writer(&write1, std::ref(buffer), std::ref(run), 0,
                     std::ref(maxWritten));

  std::this_thread::sleep_for(runtime);
  run = false;

  writer.join();

  for (auto &reader : readers) {
    reader.join();
  }

  // NB: this is not able to catch all errors, such as reading data that was not
  // yet written etc.
  // We do not want to introduce much extra synchronization between the threads
  // in the test (in addition to sync of the // Buffer itself).
  for (auto maxRead : maxs) {
    std::cout << maxRead << std::endl;
    EXPECT_LE(maxRead, maxWritten);
  }

  for (auto orderedRead : ordered) {
    EXPECT_EQ(orderedRead, 1);
  }
}

struct Data {
  int id;
  uint64_t value;
};

using DataBuffer = lockfree::ExchangeBuffer<Data, NUM_THREADS + 1>;

void write2(DataBuffer &buffer, std::atomic<bool> &run, int id, uint64_t &max) {
  max = 0;
  Data data{.id = id, .value = 1};
  // data.id = id;
  while (run) {
    // write and increase max value written if successful
    if (buffer.write(data)) {
      max = data.value;
      ++data.value;
      // std::cout << "thread " << id << " wrote " << max << std::endl;
    }
  }
}

void read2(DataBuffer &buffer, std::atomic<bool> &run, int id, int &ordered,
           uint64_t &max) {
  ordered = 1;
  max = 0; // could do this also on a per writer basis to evaluate by the test
           // later
  std::array<uint64_t, NUM_WRITER_THREADS> prevValue{0};
  while (run) {
    auto result = buffer.read();
    if (result.has_value()) {
      Data &data = *result;
      // is it at least as large as the last value from this writer?
      if (data.value < prevValue[data.id]) {
        ordered = 0;
      }
      if (max < data.value) {
        max = data.value;
      }
    }
  }
}

// This is more general then the single writer test since it uses multiple
// concurrent writers.
TEST(ExchangeBufferStressTest,
     using_multiple_writers_write_and_we_read_data_in_ascending_order) {

  DataBuffer buffer;
  std::vector<int> ordered(NUM_READER_THREADS, 1);
  std::vector<uint64_t> maxWritten(NUM_READER_THREADS, 0);
  std::vector<uint64_t> maxRead(NUM_READER_THREADS, 0);
  std::vector<std::thread> writers;
  std::vector<std::thread> readers;
  writers.reserve(NUM_WRITER_THREADS);
  readers.reserve(NUM_READER_THREADS);

  std::atomic<bool> run{true};
  for (int i = 0; i < NUM_READER_THREADS; ++i) {
    readers.emplace_back(&read2, std::ref(buffer), std::ref(run), i,
                         std::ref(ordered[i]), std::ref(maxRead[i]));
  }

  for (int i = 0; i < NUM_WRITER_THREADS; ++i) {
    writers.emplace_back(&write2, std::ref(buffer), std::ref(run), i,
                         std::ref(maxWritten[i]));
  }

  std::thread writer();

  std::this_thread::sleep_for(runtime);
  run = false;

  for (auto &writer : writers) {
    writer.join();
  }

  for (auto &reader : readers) {
    reader.join();
  }

  // could be done on a per writer thread basis but it is a weak check
  // regardless since it is just performed at the end
  uint64_t maxW = std::accumulate(
      maxWritten.begin() + 1, maxWritten.end(), maxWritten.front(),
      [](uint64_t a, uint64_t b) { return std::max(a, b); });

  for (auto maxR : maxRead) {
    EXPECT_LE(maxR, maxW);
  }

  for (auto orderedRead : ordered) {
    EXPECT_EQ(orderedRead, 1);
  }
}

} // namespace