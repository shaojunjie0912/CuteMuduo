#pragma once

#include <unistd.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
//
#include <cutemuduo/noncopyable.hpp>

namespace cutemuduo {

class Thread : NonCopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = "");

    ~Thread();

    // 创建线程并开始执行线程函数
    void Start();

    // 阻塞等待线程执行完毕
    void Join();

    // 返回线程是否已经开始执行
    bool Started() const;

    // 返回线程 ID
    pid_t Tid() const;

    // 返回线程名称
    std::string const& Name() const;

    // 返回已经创建的线程数
    static int NumCreated();

private:
    // 设置线程默认名称
    void SetDefaultName();

private:
    bool started_;                         // 是否已经开始执行
    bool joined_;                          // 是否已经执行完毕
    std::shared_ptr<std::thread> thread_;  // 线程对象
    pid_t tid_;                            // 线程 ID
    ThreadFunc func_;                      // 线程执行的函数
    std::string name_;                     // 线程名称
    static std::atomic_int num_created_;   // 已经创建的线程数
};

}  // namespace cutemuduo
