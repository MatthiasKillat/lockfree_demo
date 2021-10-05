#include <iostream>

#include "broken/take_buffer.hpp"

int main(int argc, char **argv)
{
    broken::TakeBuffer<int> buffer;

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

    return 0;
}
