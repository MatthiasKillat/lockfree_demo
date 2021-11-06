#pragma once

#include "lockfree/take_buffer.hpp"

#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <iostream>
#include <atomic>

constexpr int NUM_THREADS = 8;
using Buffer = lockfree::TakeBuffer<uint64_t, NUM_THREADS>;

void work(Buffer &buffer, std::atomic<bool> &run, int id, uint64_t &sum, uint64_t &max)
{
    max = 0;
    sum = 0;
    while (run)
    {
        // //std::cout << "id " << id << std::endl;
        ++max;
        auto value = buffer.write_overflow(max);
        if (value)
        {
            sum += *value;
        }
    }
}

uint64_t gauss(uint64_t n)
{
    return n * (n + 1) / 2;
}

bool test_case()
{
    Buffer buffer;

    std::vector<uint64_t> sums(NUM_THREADS, 0);
    std::vector<uint64_t> maxs(NUM_THREADS, 0);
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    std::chrono::seconds runtime(1);
    std::atomic<bool> run{true};
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back(&work, std::ref(buffer), std::ref(run), i, std::ref(sums[i]), std::ref(maxs[i]));
    }

    std::this_thread::sleep_for(runtime);
    run = false;

    for (auto &thread : threads)
    {
        thread.join();
    }

    uint64_t expectedSum = 0;
    for (auto max : maxs)
    {
        std::cout << max << std::endl;
        expectedSum += gauss(max);
    }

    auto sum = std::accumulate(sums.begin(), sums.end(), 0ULL);
    auto value = buffer.take();
    if (value)
    {
        sum += *value;
    }

    if (expectedSum == sum)
    {
        std::cout << "passed " << expectedSum << std::endl;
        return true;
    }
    return false;
}
