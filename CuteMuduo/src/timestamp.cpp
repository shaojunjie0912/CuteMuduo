#include <time.h>
//
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

Timestamp::Timestamp() : micro_seconds_since_epoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch) : micro_seconds_since_epoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::Now() {
    return Timestamp(time(NULL));
}
std::string Timestamp::ToString() const {
    char buf[128] = {0};
    tm *tm_time = localtime(&micro_seconds_since_epoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
             tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    return buf;
}

}  // namespace cutemuduo
