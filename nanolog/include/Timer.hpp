#pragma once
#include <stddef.h>
#include <stdint.h>
namespace Cshr::Timer {
/**
 * Return the current value of the fine-grain CPU cycle counter
 * (accessed via the RDTSC instruction).
 */
inline __attribute__((__always_inline__)) uint64_t rdtsc() {
  size_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  //        __asm__ __volatile__("rdtscp" : "=a" (lo), "=d" (hi) : : "%rcx");
  return (((uint64_t)hi << 32) | lo);
}

// 获取cpu周期
inline __attribute__((__always_inline__)) uint64_t get_cpu_cycle() {
  return rdtsc();
}
};  // namespace Cshr::Timer