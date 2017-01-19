#ifndef EXAMPLE2_H_
#define EXAMPLE2_H_
#include <memory>

namespace test2 {
struct HelloAgain {
  std::unique_ptr<HelloAgain> foo_again;
  int bar_again;
};
} // namespace test2

namespace test3 {
template <typename T>
struct ByeAgain {
  T foo_again;
  int bar_again;
  T method_foo(T);
};

template<>
struct ByeAgain<float> {
  float foo_again;
  float bar_Again;
  float method_foo(int);
};

ByeAgain<double> double_bye;

template <typename T1, typename T2>
bool Begin(T1 arg1, T2 arg2);
template <>
bool Begin<int, float>(int a, float b);
bool End ( float arg = 2.0) {
  bool ret = Begin(arg, 2);
  return ret;
}

} // namespace test3

#endif  // EXAMPLE2_H_
