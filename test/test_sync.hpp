#pragma once

#include "lockfree/sync_counter.hpp"

#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <iostream>
#include <atomic>
#include <cmath>

// TODO: to tests

constexpr int NUM_THREADS = 32;
void work(SyncCounter &counter, std::atomic<bool> &run, uint64_t &failures)
{
    uint64_t local_failures = 0;
    while (run)
    {
        //counter.unsynced_increment();
        counter.increment();

        auto pair = counter.get();

        uint64_t d;
        if (pair.first > pair.second)
        {
            d = pair.first - pair.second;
        }
        else
        {
            d = pair.second - pair.first;
        }

        if (d > 1)
        {
            local_failures++;
            std::cout << d << std::endl;
        }
    }
    failures = local_failures;
}

bool test_case()
{
    SyncCounter counter;

    std::vector<uint64_t> failures(NUM_THREADS, 0);
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    std::chrono::seconds runtime(3);
    std::atomic<bool> run{true};
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back(&work, std::ref(counter), std::ref(run), std::ref(failures[i]));
    }

    std::this_thread::sleep_for(runtime);
    run = false;

    for (auto &thread : threads)
    {
        thread.join();
    }

    for (auto failure : failures)
    {
        std::cout << "failures " << failure << std::endl;
    }

    return true;
}
