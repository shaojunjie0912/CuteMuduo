#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace cutemuduo {

class Buffer {
public:
    // 从 fd 上读取数据
    ssize_t ReadFd(int fd, int* saved_errno);

    // 通过 fd 发送数据
    ssize_t WriteFd(int fd, int* saved_errno);

private:
    char* Begin() {
        return buffer_.data();
    }

    char const* Begin() const {
        return buffer_.data();
    }

private:
    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;
};

}  // namespace cutemuduo
