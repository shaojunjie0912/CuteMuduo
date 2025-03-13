#pragma once

#include <string>

namespace cutemuduo {

class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp Now();
    std::string ToString() const;

private:
    int64_t micro_seconds_since_epoch_;
};

}  // namespace cutemuduo
