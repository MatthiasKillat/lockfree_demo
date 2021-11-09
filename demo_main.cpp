#include <iostream>

#include "lockfree/cas_loop.hpp"
#include "lockfree/exchange_buffer.hpp"
#include "lockfree/storage.hpp"
#include "lockfree/take_buffer.hpp"
#include "not_lockfree/cas_semantics.hpp"
#include "not_lockfree/exchange_buffer.hpp"
#include "not_lockfree/take_buffer.hpp"

namespace lf = lockfree;
namespace nlf = not_lockfree;
using namespace std;

// TODO: gtest

void end_case() { cout << "\n*********************" << endl; }

template <typename Buffer> void write_take() {
  cout << "WRITE - TAKE" << endl;
  Buffer b;

  auto result = b.take();
  if (result) {
    cout << "take " << *result << endl;
  } else {
    cout << "take nothing" << endl;
  }

  auto success = b.write(73);
  if (success) {
    cout << "write 73" << endl;
  } else {
    cout << "write failed" << endl;
  }

  success = b.write(37);
  if (success) {
    cout << "write 37" << endl;
  } else {
    cout << "write failed" << endl;
  }

  result = b.take();
  if (result) {
    cout << "take " << *result << endl;
  } else {
    cout << "take nothing" << endl;
  }

  result = b.take();
  if (result) {
    cout << "take " << *result << endl;
  } else {
    cout << "take nothing" << endl;
  }
}

template <typename Buffer> void try_write_take() {
  cout << "TRY_WRITE - TAKE" << endl;
  Buffer b;

  auto result = b.take();
  if (result) {
    cout << "take " << *result << endl;
  } else {
    cout << "take nothing" << endl;
  }

  auto success = b.try_write(73);
  if (success) {
    cout << "try write 73" << endl;
  } else {
    cout << "try write failed" << endl;
  }

  success = b.try_write(42);
  if (success) {
    cout << "try write 42" << endl;
  } else {
    cout << "try write failed" << endl;
  }

  result = b.take();
  if (result) {
    cout << "take " << *result << endl;
  } else {
    cout << "take nothing" << endl;
  }

  result = b.take();
  if (result) {
    cout << "take " << *result << endl;
  } else {
    cout << "take nothing" << endl;
  }
}

void cas_loop() {
  cout << "CAS LOOP" << endl;
  std::atomic<int> a{3};  
  cout << "a = " << a.load() << endl;
  auto prev = lf::fetch_multiply(a, 4);
  cout << "fetch_mutliply(a, 3) returns " << prev << endl; 
  cout << "a = " << a.load() << endl;
}

void cas_semantics() {
  cout << "CAS SEMANTICS" << endl;
  nlf::atomic<int> a{73};
  cout << "a = " << a.load() << endl;

  int expected{73};
  a.compare_exchange_strong(expected, 37);
  cout << "a = " << a.load() << endl;

  a.compare_exchange_strong(expected, 42);
  cout << "a = " << a.load() << endl;
}

template <typename Buffer> void write_read() {
  cout << "WRITE - READ" << endl;
  Buffer b;

  auto result = b.read();
  if (result) {
    cout << "read " << *result << endl;
  } else {
    cout << "read nothing" << endl;
  }

  auto success = b.write(73);
  if (success) {
    cout << "write 73" << endl;
  } else {
    cout << "write failed" << endl;
  }

  result = b.read();
  if (result) {
    cout << "read " << *result << endl;
  } else {
    cout << "read nothing" << endl;
  }

  result = b.read();
  if (result) {
    cout << "read " << *result << endl;
  } else {
    cout << "read nothing" << endl;
  }
}

int main(int argc, char **argv) {
  cas_semantics();
  end_case();

  cas_loop();
  end_case();

  cout << "NOT LOCK_FREE\n" << endl;
  cout << "take buffer" << endl;
  write_take<nlf::TakeBuffer<int>>();
  end_case();

  cout << "\nexchange buffer" << endl;
  write_take<nlf::ExchangeBuffer<int>>();
  end_case();

  try_write_take<nlf::ExchangeBuffer<int>>();
  end_case();

  write_read<nlf::ExchangeBuffer<int>>();
  end_case();

  cout << "LOCK_FREE\n" << endl;
  cout << "take buffer" << endl;
  write_take<lf::TakeBuffer<int>>();
  end_case();

  try_write_take<lf::ExchangeBuffer<int>>();
  end_case();

  cout << "\nexchange buffer" << endl;
  write_take<lf::ExchangeBuffer<int>>();
  end_case();

  try_write_take<lf::ExchangeBuffer<int>>();
  end_case();

  write_read<lf::ExchangeBuffer<int>>();
  end_case();


  return EXIT_SUCCESS;
}
