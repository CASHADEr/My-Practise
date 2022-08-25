#pragma once
namespace Cshr::Sync {
// 内存屏障
static void inline store_barrier() { __asm__ __volatile__("sfence" ::: "memory"); }
static void inline load_barrier() { __asm__ __volatile__("lfence" ::: "memory"); }
}  // namespace Cshr::Sync