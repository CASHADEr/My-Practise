#pragma once
#include <functional>

#include "./LoggerConfig.hpp"

namespace Cshr::LoggerInternal {
/**
 * @brief ParamType 为Logger的解析符号，表示了不同的输入类型
 * 实际的logger输出数值为NON_STRING和STRING
 *
 */
enum ParamType : int32_t {
  INVALID = -6,               // 无效参数
  DYNAMIC_WIDTH = -5,         // 动态变长数值, 并不是一个log参数值
  DYNAMIC_PRECISION = -4,     // 精度值, 并不是一个log参数值
  NON_STRING = -3,            // 非字符串
  STRING_DYNAMIC_WIDTH = -5,  // 动态变长数值, 并不是一个log参数值
  STRING_DYNAMIC_PRECISION = -4,  // 精度值, 并不是一个log参数值
  STRING = 0                      // 字符串
};
/**
 * @brief LoggerInfo在编译期时被确定，标记了每个线程的配置参数
 *
 */
struct LoggerInfo {
  // 没有选择std::function包装的原因是 function没办法实现constepxr
  typedef void (*CompressionFunc)(int, const ParamType *, char **, char **);
  constexpr LoggerInfo(const char *fileName, const uint32_t lineNum,
                       const uint8_t serverity, const char *formatString,
                       const int numParams, const int numNibbles,
                       const ParamType *paramTypes, CompressionFunc compressor)
      : mFileName(fileName),
        mLineNum(lineNum),
        mServerity(serverity),
        mFormatString(formatString),
        mNumParams(numParams),
        mNumNibbles(numNibbles),
        mParamTypes(paramTypes),
        mCompressor(compressor) {}
  const char *mFileName;
  const uint32_t mLineNum;
  const uint8_t mServerity;
  const char *mFormatString;
  const int mNumParams;
  const int mNumNibbles;
  const ParamType *mParamTypes;
  CompressionFunc mCompressor;
};

}  // namespace Cshr::LoggerInternal