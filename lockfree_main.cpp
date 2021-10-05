#include <iostream>
#include <atomic>

#include "lockfree/cas_loop.hpp"
#include "lockfree/take_buffer.hpp"
#include "lockfree/read_buffer.hpp"

int main(int argc, char **argv)
{
    std::atomic<int> x{73};

    lockfree::compare_exchange_if_not_equal(x, 73, 42);
    std::cout << x.load() << std::endl;
    lockfree::compare_exchange_if_not_equal(x, 37, 42);
    std::cout << x.load() << std::endl;
    {
        lockfree::TakeBuffer<int> buffer;
        buffer.write(73);
    }

    {
        broken::ReadBuffer<int> buffer;
        buffer.write(73);
        auto value = buffer.read();
        if (*value)
        {
            std::cout << "read " << *value << std::endl;
        }
    }

    {
        lockfree::ReadBuffer<int> buffer;
        //buffer.write(42);
        buffer.try_write(37);
        auto value = buffer.read();
        if (*value)
        {
            std::cout << "read " << *value << std::endl;
        }
    }

    return 0;
}

/*
  lockfree::TakeBuffer<int> buffer;

    buffer.write(42);

    buffer.try_write(73);

    auto value = buffer.take();

    if (value)
    {
        std::cout << *value << std::endl;
    }

    value = buffer.take();

    if (value)
    {
        std::cout << *value << std::endl;
    }

    buffer.try_write(73);

    value = buffer.take();

    if (value)
    {
        std::cout << *value << std::endl;
    }

    SyncCounter counter;

    counter.increment();
    counter.increment();

    std::cout << "counter " << counter.get() << std::endl;

    lockfree::ReadBuffer<int> readbuffer;

    readbuffer.write(66);

    value = readbuffer.read();

    if (value)
    {
        std::cout << "readbuffer " << *value << std::endl;
    }

    value = readbuffer.read();

    if (value)
    {
        std::cout << "readbuffer " << *value << std::endl;
    }

    */
