#pragma once
#include <fcntl.h>
#include <stdint.h>

#include <chrono>

// A macro to disallow the copy constructor and operator= functions
#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  TypeName& operator=(const TypeName&) = delete;
#endif

namespace Cshr::Logger {
enum LogLevel {
  SILENT = 0,
  ERROR,
  WARNING,
  NOTICE,
  DEBUG,
  LOG_TYPE_NUM  // 特殊值，用于标记目前的Level类型个数
};
// logger线程使用的循环队列buffer
constexpr uint32_t defaultLoggerBufferSize = 1 << 20;
// logger buffer可用低水位标记
constexpr uint32_t loggerBufferLowWaterMark = defaultLoggerBufferSize >> 1;
// IO线程用于swap给异步队列的Buffer大小
constexpr uint32_t swapBufferSize = 1 << 26;
// IO线程为了节省CPU资源在没有任务的时候会睡眠
constexpr uint32_t ioThreadSleepTimeIntervalMicroseconds = 1;
constexpr uint32_t ioThreadBlockedByAIOMicroseconds = 1;
// 理论上应该远远小于
constexpr float ratioOfBufferLoggerIo = 3.0 / 2;
static_assert(
    ratioOfBufferLoggerIo * defaultLoggerBufferSize <= swapBufferSize,
    "The logger buffer used by background logger IO thread should have much "
    "more spaces than the one that logger threads use.");
// Cacheline对齐用到的参数，用于避免读写指针出现的假更新
constexpr uint32_t x86CacheLineBytesSize = 64;
constexpr uint32_t cacheLineBytesSize = x86CacheLineBytesSize;
// 每个线程第一次写log前都没有注册
constexpr int loggerIdUnsigned = -1;
constexpr char defaultLogFilePath[] = "./cshr_log.clg";
constexpr int defaultLogOpenSetting =
    O_APPEND | O_RDWR | O_CREAT | O_NOATIME | O_DSYNC;  // Log File的打开参数
constexpr int defaultLogOpenMode = 0666;
}  // namespace Cshr::Logger