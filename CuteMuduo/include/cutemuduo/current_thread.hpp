#pragma once

#include <sys/syscall.h>
#include <unistd.h>

namespace cutemuduo {

namespace current_thread {

extern __thread int t_cached_tid;  // 保存tid缓存 因为系统调用非常耗时 拿到tid后将其保存

void CacheTid();

inline int Tid()  // 内联函数只在当前文件中起作用
{
    // __builtin_expect 是一种底层优化, 意思是如果还未获取tid 进入if 通过cacheTid()系统调用获取tid
    if (__builtin_expect(t_cached_tid == 0, 0)) {
        CacheTid();
    }
    return t_cached_tid;
}

}  // namespace current_thread

}  // namespace cutemuduo
