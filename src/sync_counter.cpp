#include "lockfree/sync_counter.hpp"

namespace lockfree {

SyncCounter::SyncCounter()
    : m_count1(m_counters[0]), m_count2(m_counters[N - 1]) {
  m_count1 = 0;
  m_count2 = 0;
}

void SyncCounter::increment() {
  uint64_t count1 = m_count1.load();
  uint64_t count2 = m_count2.load();

  while (true) {
    // load state in 2 loads
    count1 = m_count1.load();
    count2 = m_count2.load();
    // they may be different if we look right during another operation

    // determine whether there may be an incomplete operation
    // (note that the counters may be out of sync due to load as well)
    while (count1 != count2) {
      // there is an incomplete operation or the loads are outdated
      //  try to complete it

      // there is small optimization potential in the retry loads (at cost of
      // readability)

      try_help(count1, count2);
      // we reload the state (both counters again)
    }

    // we can assume that they are equal now (they may have changed but then the
    // next CAS fails) unless someone changes them concurrently both equal
    // count1

    if (m_count1.compare_exchange_strong(count1, count1 + 1)) {
      // we incremented m_count1
      // our own operation is partially complete (count 1 incremented),
      // we will not try helping again but someone may help us
      // we still need to increment m_count2
      break;
    }

    // we failed, someone else incremented m_count1 and potentially m_count2
    // concurrently, retry
  };

  // could add some sleep here to provoke incomplete operations with high
  // probability

  // finalize operation by incrementing second count
  // may fail, but only if someone else concurrently helps us and completes the
  // operation for us
  m_count2.compare_exchange_strong(count2, count1 + 1);

  // we do not care for the result of CAS (either it worked because we
  // incremented or someone else did it for us and then we do not need to retry)
}
#if 0
// uncommented version
void SyncCounter::increment() {
  uint64_t count1, count2;

  do {
    count1 = m_count1.load();
    count2 = m_count2.load();

    while (count1 != count2) {
      try_help(count1, count2);
    }

    if (m_count1.compare_exchange_strong(count1, count1 + 1)) {
      break;
    }
  } while (true);

  sleep();
  m_count2.compare_exchange_strong(count2, count1 + 1);
}
#endif

void SyncCounter::unsynced_increment() {
  ++m_count1;
  sleep();
  ++m_count2;
}

uint64_t SyncCounter::get_with_help() {
  uint64_t count1 = m_count1.load();
  uint64_t count2 = m_count2.load();
  while (count1 != count2) {
    try_help(count1, count2);
  }

  // we only continue our operation if the invariant holds
  return count1;
}

std::pair<uint64_t, uint64_t> SyncCounter::get_if_equal() {
  uint64_t count1 = m_count1.load();
  uint64_t count2;
  do {
    count2 = m_count2.load();
  } while (!m_count1.compare_exchange_strong(count1, count1));
  return {count1, count2};
}

void SyncCounter::try_help(uint64_t &count1, uint64_t &count2) {
  if (count1 < count2) {
    // we are completely outdated, reload at least count1

    count1 = m_count1.load();
    count2 = m_count2.load();
    return;
  }

  // TODO: can we observe count2 > count1?
  if (m_count2.compare_exchange_strong(count1, count1)) {
    // we completed the operation, and can try our own increment again
    count2 = count1;
  } else {
    // we failed so either the original operation completed on its own or
    // someone helped completing it (so we do not try again for this count)
    count1 = m_count1.load();
  }
}

void SyncCounter::sleep(int ms = 1) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace lockfree
