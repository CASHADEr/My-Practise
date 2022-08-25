#pragma once
#include "FormatUtil.hpp"
#include "Logger.hpp"
/**
 * @brief chsr_log 提供给用户实际使用log功能的接口
 * 通过宏初始化每条语句的static唯一所在对象
 *
 */
#define cshr_log(logLevel, stringFormat, ...)\
do {\
  constexpr size_t NTypes =\
      Cshr::Logger::Format::count_param_types(stringFormat);\
  static_assert(\
      Cshr::Logger::Format::check_number_match<NTypes>(__VA_ARGS__),\
      "无效的Log参数数量");\
  static uint32_t logId = Cshr::Logger::loggerIdUnsigned;\
  static std::array<Cshr::LoggerInternal::ParamType, NTypes> paramsArray =\
      Cshr::Logger::Format::analyze_params_format<NTypes>(stringFormat);\
  if (logLevel > Cshr::LoggerInternal::Logger::get_log_level()) break;\
  Cshr::Logger::Format::static_check_format(stringFormat, __VA_ARGS__);\
} while (0)
