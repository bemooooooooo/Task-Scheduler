#include <gtest/gtest.h>

#include <lib/scheduler.hpp>
#include <string>

TEST(scheduler_test, test_add_function) {
  TTaskScheduler scheduler;
  auto test1 = scheduler.add([]() { return std::string("test"); });
  auto test2 = scheduler.add([](float a) { return a * a; }, 1.25);
  auto test3 = scheduler.add([](float a, float b) { return a * b; }, 1.25, 5.0);
  ASSERT_EQ(scheduler.getResult<std::string>(test1), std::string("test"));
  ASSERT_EQ(scheduler.getResult<float>(test2), 1.5625);
  ASSERT_EQ(scheduler.getResult<float>(test3), 6.25);
}

TEST(scheduler_test, test_future_result) {
  TTaskScheduler scheduler;
  auto test1 = scheduler.add([]() { return 5 * 5; });
  auto test2 = scheduler.add([](int a, int b) { return a + b; },
                             scheduler.getFutureResult<int>(test1),
                             scheduler.getFutureResult<int>(test1));
  ASSERT_EQ(scheduler.getResult<int>(test2), 50);
}

TEST(scheduler_test, test_executeAll) {
  TTaskScheduler scheduler;
  auto test1 = scheduler.add([]() { return 6 * 7; });
  auto test2 = scheduler.add([]() { return 5 * 5; });
  auto test3 = scheduler.add([](int a, int b) { return a + b; },
                             scheduler.getFutureResult<int>(test1),
                             scheduler.getFutureResult<int>(test2));
  scheduler.executeAll();
  ASSERT_EQ(scheduler.getResult<int>(test1), 42);
  ASSERT_EQ(scheduler.getResult<int>(test2), 25);
  ASSERT_EQ(scheduler.getResult<int>(test3), 67);
}

std::string user_function(std::string& str, int x = 1) {
  std::string test_str;
  for (int i = 0; i < x; i++) {
    test_str += str;
  }
  return test_str;
}

TEST(scheduler_test, test_with_user_function) {
  TTaskScheduler scheduler;
  auto test1 =
      scheduler.add([](std::string a, int b) { return user_function(a, b); },
                    std::string("test"), 2);
  scheduler.executeAll();
  ASSERT_EQ(scheduler.getResult<std::string>(test1), std::string("testtest"));
}

struct user_struct {
  int a;
  std::string str;

  std::string user_function() {
    std::string test_str;
    for (int i = 0; i < a; i++) {
      test_str += str;
    }
    for (auto& i : test_str) {
      i = toupper(i);
    }
    return test_str;
  }
};

TEST(scheduler_test, test_with_user_structures) {
  TTaskScheduler scheduler;
  auto test1 =
      scheduler.add([](int a, std::string b) { return user_struct(a, b); }, 2,
                    std::string("test"));
  auto test2 = scheduler.add([](user_struct a) { return a.user_function(); },
                             scheduler.getFutureResult<user_struct>(test1));
  scheduler.executeAll();
  ASSERT_EQ(scheduler.getResult<std::string>(test2), std::string("TESTTEST"));
}

TEST(scheduler_test, default_test){
  float a = 1;
float b = -2;
float c = 0;

TTaskScheduler scheduler;

auto id1 = scheduler.add([](float a, float c){return -4 * a * c;}, a, c);

auto id2 = scheduler.add([](float b, float v){return b * b + v;}, b, scheduler.getFutureResult<float>(id1));

auto id3 = scheduler.add([](float b, float d){return -b + std::sqrt(d);}, b, scheduler.getFutureResult<float>(id2));

auto id4 = scheduler.add([](float b, float d){return -b - std::sqrt(d);}, b, scheduler.getFutureResult<float>(id2));

auto id5 = scheduler.add([](float a, float v){return v/(2*a);}, a, scheduler.getFutureResult<float>(id3));

auto id6 = scheduler.add([](float a, float v){return v/(2*a);},a, scheduler.getFutureResult<float>(id4));

scheduler.executeAll();

ASSERT_EQ(scheduler.getResult<float>(id5), 2.0);
ASSERT_EQ(scheduler.getResult<float>(id6), 0.0);
}

TEST(scheduler_test, test_of_the_importance_of_order) {
  float a = 1;
  float b = -2;
  float c = 0;

  TTaskScheduler scheduler;

  auto id1 = scheduler.add([](float a, float c) { return -4 * a * c; }, a, c);

  auto id2 = scheduler.add([](float b, float v) { return b * b + v; }, b, scheduler.getFutureResult<float>(id1));

  auto id4 = scheduler.add([](float b, float d) { return -b - std::sqrt(d); }, b, scheduler.getFutureResult<float>(id2));

  auto id6 = scheduler.add([](float a, float v) { return v / (2 * a); }, a, scheduler.getFutureResult<float>(id4));

  auto id3 = scheduler.add([](float b, float d) { return -b + std::sqrt(d); }, b, scheduler.getFutureResult<float>(id2));

  auto id5 = scheduler.add([](float a, float v) { return v / (2 * a); }, a, scheduler.getFutureResult<float>(id3));

  scheduler.executeAll();

  ASSERT_EQ(scheduler.getResult<float>(id5), 2.0);
  ASSERT_EQ(scheduler.getResult<float>(id6), 0.0);
}