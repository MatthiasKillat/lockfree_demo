#include <gtest/gtest.h>

#include "lockfree/exchange_buffer.hpp"

namespace {

// only test the lock-free implementation of ExchangeBuffer
// TODO: typed tests for different types T (trivially copyable etc.)

using IntBuffer = lockfree::ExchangeBuffer<int>;

class TestExchangeBuffer : public ::testing::Test {
public:
  void SetUp() override {}

  void TearDown() override {}

  IntBuffer buffer;
};

// Some tests for (single-threaded) correctness (necessary condition for
// correctness).

TEST_F(TestExchangeBuffer, constructed_buffer_is_empty) {
  EXPECT_TRUE(buffer.empty());
}

TEST_F(TestExchangeBuffer, take_returns_nothing_if_empty) {
  auto result = buffer.take();
  EXPECT_FALSE(result.has_value());
}

TEST_F(TestExchangeBuffer, read_returns_nothing_if_empty) {
  auto result = buffer.read();
  EXPECT_FALSE(result.has_value());
}

TEST_F(TestExchangeBuffer, buffer_contains_value_after_write) {
  EXPECT_TRUE(buffer.write(73));
  EXPECT_FALSE(buffer.empty());
  auto result = buffer.take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 73);
}

TEST_F(TestExchangeBuffer, write_overwrites_previous_value) {
  EXPECT_TRUE(buffer.write(73));
  EXPECT_TRUE(buffer.write(37));
  auto result = buffer.take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 37);
}

// We cannot easily test the case where a concurrent write fails
// due to lack of memory (i.e. free indices/slots)
// We cannot provoke the problem since we cannot interrupt an
// ongoing write.
// Could be done by mocking the IndexPool but then we would have to expose
// it as e.g. template parameter to allow mocking.

TEST_F(TestExchangeBuffer, try_write_succeeds_if_empty) {
  EXPECT_TRUE(buffer.try_write(73));
  auto result = buffer.take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 73);
}

TEST_F(TestExchangeBuffer, try_write_fails_if_not_empty) {
  EXPECT_TRUE(buffer.try_write(37));
  EXPECT_FALSE(buffer.empty());
  EXPECT_FALSE(buffer.try_write(73));
  auto result = buffer.take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 37);
}

TEST_F(TestExchangeBuffer, take_removes_value) {
  EXPECT_TRUE(buffer.write(73));
  auto result = buffer.take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 73);
  EXPECT_TRUE(buffer.empty());
  result = buffer.take();
  EXPECT_FALSE(result.has_value());
}

TEST_F(TestExchangeBuffer, read_does_not_remove_value) {
  EXPECT_TRUE(buffer.write(73));
  auto result = buffer.read();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 73);
  EXPECT_FALSE(buffer.empty());
  result = buffer.read();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 73);
}

} // namespace