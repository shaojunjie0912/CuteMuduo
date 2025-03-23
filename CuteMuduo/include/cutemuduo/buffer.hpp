#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace cutemuduo {

/*
+-------------------------+----------------------+---------------------+
|    prependable bytes    |    readable bytes    |    writable bytes   |
|                         |      (CONTENT)       |                     |
+-------------------------+----------------------+---------------------+
|                         |                      |                     |
0        <=           readerIndex     <=     writerIndex             size
*/

class Buffer {
public:
    static const size_t kCheapPrepend = 8;    // 前面预留的空间(prependable)
    static const size_t kInitialSize = 1024;  // 初始大小

    explicit Buffer(size_t initial_size = kInitialSize);

public:
    // 可读字节数
    size_t ReadableBytes() const;

    // 可写字节数
    size_t WritableBytes() const;

    // 可预留字节数
    size_t PrependableBytes() const;

    // 可读数据的起始地址
    char const* Peek() const;

    // 可写数据的起始地址
    char const* BeginWrite() const;
    char* BeginWrite();

    // 移动 reader_index_ 表示读出 len 字节数据
    void Retrieve(size_t len);

    // 移动 reader_index_ & writer_index_ 表示读出所有数据
    void RetrieveAll();

    // 读出 len 字节数据并以字符串返回
    std::string RetrieveAsString(size_t len);

    // 读出所有数据并以字符串返回
    std::string RetrieveAllAsString();

    // 确保有足够的空间写入 len 字节数据
    void EnsureWritableBytes(size_t len);

public:
    // 从 fd 上读取数据
    ssize_t ReadFd(int fd, int* saved_errno);

    // 通过 fd 发送数据
    ssize_t WriteFd(int fd, int* saved_errno);

private:
    // 调整可写空间
    void MakeSpace(size_t len);

    //
    void Append(char const* data, size_t len);

private:
    char* Begin() {
        return buffer_.data();
    }

    char const* Begin() const {
        return buffer_.data();
    }

private:
    std::vector<char> buffer_;  //
    size_t reader_index_;
    size_t writer_index_;
};

}  // namespace cutemuduo
