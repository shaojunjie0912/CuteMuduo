#pragma once

namespace cutemuduo {

// 所有继承该类的子类都不可拷贝
class NonCopyable {
public:
    NonCopyable(NonCopyable const &) = delete;
    NonCopyable &operator=(NonCopyable const &) = delete;

protected:  // 防止从外部直接实例化该类
    NonCopyable() = default;
    ~NonCopyable() = default;
};

}  // namespace cutemuduo
