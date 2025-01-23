#include <logger/logger.h>
#include <cstdint>
#include <etcd/Client.hpp>
#include <etcd/Response.hpp>
#include <iostream>
#include "common/vector_init.h"
#include "gtest/gtest.h"
// 需要先手动开启etcd server
namespace vectordb {
// NOLINTNEXTLINE
TEST(ETCDTest, SampleTest) {
  VdbServerInit(1);
  etcd::Client etcd("http://127.0.0.1:2379");

  // 设置键值对
  etcd::Response response = etcd.set("foo", "bar").get();
  if (response.is_ok()) {
    std::cout << "Key set successfully" << std::endl;
  } else {
    std::cerr << "Error: " << response.error_message() << std::endl;
  }
  EXPECT_EQ(response.is_ok(), true);

  // 获取键值对
  response = etcd.get("foo").get();
  if (response.is_ok()) {
    std::cout << "Value: " << response.value().as_string() << std::endl;
  } else {
    std::cerr << "Error: " << response.error_message() << std::endl;
  }
  EXPECT_EQ(response.value().as_string(), "bar");
}

}  // namespace vectordb