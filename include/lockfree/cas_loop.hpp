#pragma once

#include <atomic>

// CAS semantics
namespace not_lockfree {
template <class T>
bool compare_exchange_non_atomic(std::atomic<T> &location, T &expected,
                                 const T &newValue) {
  auto value = location.load();

  if (value == expected) {
    location.store(newValue);
    return true;
  }

  expected = value;
  return false;
}

} // namespace not_lockfree
namespace lockfree
{

template <class T>
bool compare_exchange_if_not_equal(std::atomic<T> &location, const T &expected,
                                   const T &newValue) {
  auto value = location.load();

  while (value != expected) {
    if (location.compare_exchange_strong(value, newValue)) {
      // we exchanged since the value did not match the expected one
      return true;
    }
    // value contains the updated value
  }

  // value matched expected, do not exchange
  return false;
}

int fetch_multiply(std::atomic<int> &value, const int &multiplier) {
  int oldValue = value.load();

  do {
    // local computation of new value
    int newValue{oldValue*multiplier};
    if (value.compare_exchange_strong(oldValue, newValue)) {      
      break;
    }
    // concurrent update occurred, retry until success   
  } while(true);

  return oldValue;
}

} // namespace lockfree

