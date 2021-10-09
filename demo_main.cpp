#include "demo/d1_cas_loop.hpp"
#include "demo/d2_take_buffer.hpp"
#include "demo/d3_index_pool.hpp"
#include "demo/d4_take_buffer.hpp"
#include "demo/d5_read_buffer.hpp"
#include "demo/d6_read_buffer.hpp"

#include <atomic>
#include <iostream>

using namespace lockfree;
using namespace std;

void print(std::optional<int> &maybe) {
  if (maybe) {
    std::cout << "got " << *maybe << std::endl;
  } else {
    std::cout << "got no value" << std::endl;
  }
}

void print_index(std::optional<uint32_t> &maybe) {
  if (maybe) {
    std::cout << "got index " << *maybe << std::endl;
  } else {
    std::cout << "got no index" << std::endl;
  }
}

int main(int argc, char **argv) {
  {
    cout << "CAS loop" << endl;
    std::atomic<int> atom{73};
    cout << "atom = " << atom << endl;
    compare_exchange_if_not_equal(atom, 73, 42);
    cout << "atom = " << atom << endl;
    compare_exchange_if_not_equal(atom, 37, 42);
    cout << "atom = " << atom << endl;

    int expected = 21;
    not_lockfree::compare_exchange_non_atomic(atom, expected, 12);
    cout << "atom = " << atom << " expected " << expected << endl;
    not_lockfree::compare_exchange_non_atomic(atom, expected, 12);
    cout << "atom = " << atom << " expected " << expected << endl;
  }

  {
    cout << "\nnon-lockfree take" << endl;
    not_lockfree::TakeBuffer<int> buffer;
    auto maybe = buffer.take();
    print(maybe);

    maybe = buffer.write(73);
    print(maybe);

    maybe = buffer.write(37);
    print(maybe);

    maybe = buffer.take();
    print(maybe);

    maybe = buffer.take();
    print(maybe);
  }

  {
    cout << "\nindex pool" << endl;
    IndexPool<2> pool;
    auto maybe1 = pool.get();
    auto maybe2 = pool.get();
    auto maybe3 = pool.get();
    print_index(maybe1);
    print_index(maybe2);
    print_index(maybe3);
    pool.free(*maybe2);
    auto maybe4 = pool.get();
    print_index(maybe4);
  }

  {
    cout << "\nlockfree take" << endl;
    lockfree::TakeBuffer<int> buffer;
    auto maybe = buffer.take();
    print(maybe);

    maybe = buffer.write(73);
    print(maybe);

    maybe = buffer.write(37);
    print(maybe);

    auto success = buffer.try_write(42);
    if (!success) {
      cout << "buffer full " << endl;
    }

    maybe = buffer.take();
    print(maybe);

    maybe = buffer.take();
    print(maybe);

    success = buffer.try_write(42);
    if (!success) {
      cout << "buffer full " << endl;
    }

    maybe = buffer.take();
    print(maybe);
  }

  {
    cout << "\nnon-lockfree read" << endl;
    not_lockfree::ReadBuffer<int> buffer;

    auto maybe = buffer.read1();
    print(maybe);

    maybe = buffer.read2();
    print(maybe);

    buffer.write(73);

    maybe = buffer.read1();
    print(maybe);

    maybe = buffer.read1();
    print(maybe);

    maybe = buffer.read2();
    print(maybe);
  }

  {
    cout << "\nlockfree read" << endl;
    lockfree::ReadBuffer<int> buffer;

    auto maybe = buffer.read1();
    print(maybe);

    maybe = buffer.read2();
    print(maybe);

    maybe = buffer.write(73);
    print(maybe);

    maybe = buffer.read1();
    print(maybe);

    maybe = buffer.read1();
    print(maybe);

    maybe = buffer.read2();
    print(maybe);

    maybe = buffer.take();
    print(maybe);

    maybe = buffer.read1();
    print(maybe);

    maybe = buffer.read2();
    print(maybe);
  }
  return 0;
}