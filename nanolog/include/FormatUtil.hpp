#pragma once
#include "LoggerInfo.hpp"
namespace Cshr::Logger::Format {
using namespace Cshr::LoggerInternal;
constexpr inline bool is_legal_marker(const char &c) {
  return c == '%' || c == 'd' || c == 's' || c == 'f' || c == 'c';
}

constexpr inline ParamType marker_to_param_type(const char c) {
  if (c == 'd')
    return NON_STRING;
  else if (c == 'f')
    return NON_STRING;
  else if (c == 'c')
    return NON_STRING;
  else if (c == 's')
    return STRING;
  else
    return INVALID;
}
// 解析字符串中的变量标记
// 设计为printf()风格, 但是目前仅实现%d, %f, %s
template <size_t N>
constexpr size_t count_param_types(const char (&stringFormat)[N]) {
  size_t ret = 0;
  for (size_t i = 0; i < N; ++i) {
    if (stringFormat[i] == '%') {
      ++i;
      // static_assert(is_legal_marker(stringFormat[i]));
      ret += stringFormat[i] != '%';
    }
  }
  return ret;
}

template <size_t NTypes, size_t N>
constexpr std::array<ParamType, NTypes> analyze_params_format(
    const char (&stringFormat)[N]) {
  std::array<ParamType, NTypes> ret{};
  for (size_t cur = 0, i = 0; i < N; ++i) {
    if (stringFormat[i] == '%') {
      ++i;
      // static_assert(is_legal_marker(stringFormat[i]));
      if (stringFormat[i] != '%')
        ret[cur++] = marker_to_param_type(stringFormat[i]);
    }
  }
  return ret;
}

template <size_t NTypes, typename... Types>
constexpr bool check_number_match(Types... args) {
  return NTypes == sizeof...(Types);
}

template <size_t N, size_t NTypes, typename... Types>
constexpr bool check_param_match(std::array<ParamType, NTypes> paramsArray,
                                 Types... args) {
  // TODO: 字符串参数和输入参数格式匹配
  return true;
}

// if(false) 目的是编译器就优化掉这部分代码，只做编译期检查
template <size_t N, typename... Types>
constexpr inline bool static_check_format(const char (&stringFormat)[N],
                                          Types... args) {
  if (true) std::printf(stringFormat, args...);
  return true;
}
}  // namespace Cshr::Logger::Format