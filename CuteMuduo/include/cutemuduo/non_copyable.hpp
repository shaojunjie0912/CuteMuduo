#pragma once

namespace cutemuduo {

class NonCopyable {
public:
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;

protected:  // 防止直接实例化该类
    NonCopyable() = default;
    ~NonCopyable() = default;
};

}  // namespace cutemuduo
