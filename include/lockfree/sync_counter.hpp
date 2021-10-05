#pragma once

#include <atomic>
#include <optional>
#include <assert.h>
#include <chrono>
#include <thread>

class SyncCounter
{
    static constexpr int N = 1024;

public:
    SyncCounter() : m_count1(m_counters[0]), m_count2(m_counters[N - 1])
    {
        m_count1 = 0;
        m_count2 = 0;
    }
#if 0
    void increment_commented()
    {
        uint64_t count1 = m_count1.load();
        uint64_t count2 = m_count2.load();

        while (true)
        {
            // load state in 2 loads
            count1 = m_count1.load();
            count2 = m_count2.load();
            // they may be different if we look right during another operation

            // determine whether there may be an incomplete operation
            while (count1 != count2)
            {
                //TODO: the counters may be out of sync in another way
                // there is an incomplete operation, try to complete it

                // optional invariant check (if not equal, this must hold)
                // we cannot assume this due to the way the atomic loads work
                // assert(count1 == count2 + 1);
                // if (count1 != count2 + 1)
                // {
                //     std::terminate();
                // }
                // operation incomplete, count1 < count2 (count1 == count2 + 1)

                // there is small optimization potential in the retry loads (at cost of readability)

                try_help(count1, count2);
                // we reload the state (both counters again)
            }

            // we can assume that they are equal now (they may have changed but then the next CAS fails)
            // unless someone changes them concurrently both equal count1

            if (m_count1.compare_exchange_strong(count1, count1 + 1))
            {
                // we incremented m_count1
                // partially complete, we will not try helping again but someone may help us
                // we still need to increment m_count2
                break;
            }

            // we failed, someone else incremented m_count1 and potentially m_count2 concurrently, retry
        };

        // add some sleep here to provoke incomplete operations with high probability

        // finalize operation by incrementing second count
        // may fail, but only if someone else concurrently helps us and completes the operation for us
        m_count2.compare_exchange_strong(count2, count1 + 1);

        // we do not care for the result of CAS (either it worked because we incremented or someone else did it for us
        // and then we do not need to retry)
    }
#endif

    void increment()
    {
        uint64_t count1, count2;

        do
        {
            count1 = m_count1.load();
            count2 = m_count2.load();

            while (count1 != count2)
            {
                try_help(count1, count2);
            }

            if (m_count1.compare_exchange_strong(count1, count1 + 1))
            {
                break;
            }
        } while (true);

        sleep();
        m_count2.compare_exchange_strong(count2, count1 + 1);
    }

    void unsynced_increment()
    {
        m_count1.fetch_add(1);
        sleep();
        m_count2.fetch_add(1);
    }

    uint64_t sync_get()
    {
        uint64_t count1 = m_count1.load();
        uint64_t count2 = m_count2.load();
        while (count1 != count2)
        {
            try_help(count1, count2);
        }
        return count1;
    }

    std::pair<uint64_t, uint64_t> get()
    {
        uint64_t count1 = m_count1.load();
        uint64_t count2;
        do
        {
            count2 = m_count2.load();
        } while (!m_count1.compare_exchange_strong(count1, count1));
        return {count1, count2};
    }

private:
    using counter_t = std::atomic<uint64_t>;
    counter_t m_counters[1024]; // separation of counters by dummy memory

    counter_t &m_count1;
    counter_t &m_count2;

    // PRE: count1 != count2
    // POST: either will have helped changing the remaining value or reloaded the counters
    void try_help(uint64_t &count1, uint64_t &count2)
    {
        if (count1 < count2)
        {
            // we are completely outdated, reload at least count1

            count1 = m_count1.load();
            count2 = m_count2.load();
            return;
        }

        // TODO: can we observe count2 > count1?
        if (m_count2.compare_exchange_strong(count1, count1))
        {
            // we completed the operation, and can try our own increment again
            count2 = count1;
        }
        else
        {
            // we failed so either the original operation completed on its own or someone helped
            // completing it (so we do not try again for this count)
            count1 = m_count1.load();
        }
    }

    void sleep()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
};

// concurrent test: many threads increment, we know how many increments we issue, check final values

// there should be no point in time were we can read values more than 1 apart

// when an operation is complete someone else will observe:
// c1 < c2 -> definitely outdated, retry
// c1 == c2 -> potentially outdated but valid, try update
// c1 > c2 -> potentially incomplete concurrent operation, try to help