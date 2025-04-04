#include <sys/uio.h>
#include <unistd.h>
//
#include <cutemuduo/buffer.hpp>

namespace cutemuduo {

Buffer::Buffer(size_t initial_size)
    : buffer_(kCheapPrepend + initial_size), reader_index_(kCheapPrepend), writer_index_(kCheapPrepend) {}

size_t Buffer::ReadableBytes() const {
    return writer_index_ - reader_index_;
}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writer_index_;
}

size_t Buffer::PrependableBytes() const {
    return reader_index_;
}

char const* Buffer::Peek() const {
    return Begin() + reader_index_;
}

char const* Buffer::BeginWrite() const {
    return Begin() + writer_index_;
}

char* Buffer::BeginWrite() {
    return Begin() + writer_index_;
}

void Buffer::Retrieve(size_t len) {
    if (len < ReadableBytes()) {
        reader_index_ += len;
    } else {
        RetrieveAll();
    }
}

void Buffer::RetrieveUntil(char const* end) {
    Retrieve(end - Peek());  // 计算出 len
}

void Buffer::RetrieveAll() {
    reader_index_ = kCheapPrepend;
    writer_index_ = kCheapPrepend;
}

std::string Buffer::RetrieveAsString(size_t len) {
    std::string result{Peek(), len};
    Retrieve(len);  // 读出数据后, 右移 reader_index_ 表示读出
    return result;
}

std::string Buffer::RetrieveAllAsString() {
    return RetrieveAsString(ReadableBytes());
}

void Buffer::EnsureWritableBytes(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace(len);
    }
}

void Buffer::MakeSpace(size_t len) {
    // NOTE:
    // 如果当前预留空间(包括 kCheapPrepend + 被读出后空出来的空间) + 当前可写空间 - kCheapPrepend < len, 则扩容
    // 即扩容后还能保留一个 kCheapPrepend
    if (PrependableBytes() + WritableBytes() - kCheapPrepend < len) {
        buffer_.resize(writer_index_ + len);
    } else {
        // 移动可读数据到 kCheapPrepend 处, 腾出可写空间
        auto readable_bytes = ReadableBytes();
        std::copy(Begin() + reader_index_, Begin() + writer_index_, Begin() + kCheapPrepend);
        reader_index_ = kCheapPrepend;
        writer_index_ = reader_index_ + readable_bytes;
    }
}

void Buffer::Append(char const* data, size_t len) {
    EnsureWritableBytes(len);
    std::copy(data, data + len, BeginWrite());
    writer_index_ += len;
}

// readv 散布读: 将连续的数据读入内存分散的多个缓冲区
// writev 聚集写: 将内存分散的多个缓冲区的数据写入连续区域

ssize_t Buffer::ReadFd(int fd, int* saved_errno) {
    // 栈额外空间, 当从 fd 读但 buffer_ 空间不够时暂存数据
    // 之后再转移到buffer_
    char extrabuf[65536] = {0};  // 栈上内存空间 65536/1024 = 64KB
    iovec vec[2];                // 使用 iovec 指向两个缓冲区
    auto writable_bytes = WritableBytes();

    vec[0].iov_base = BeginWrite();  // 第一块缓冲区指向 buffer_ 可写空间
    vec[0].iov_len = writable_bytes;

    vec[1].iov_base = extrabuf;  // 第二块缓冲区指向栈上空间(buffer_ 满则用栈存)
    vec[1].iov_len = sizeof(extrabuf);

    int iovcnt = (writable_bytes < sizeof(extrabuf)) ? 2 : 1;  // buffer_ 可写空间 < extrabuf 则用两块缓冲区
    ssize_t n = readv(fd, vec, iovcnt);
    if (n < 0) {
        *saved_errno = errno;
    } else if (n <= (ssize_t)writable_bytes) {
        writer_index_ += n;
    } else {
        writer_index_ = buffer_.size();        // buffer_ 已满, 移动 writer_index_ 到最后
        Append(extrabuf, n - writable_bytes);  // 扩容 buffer_ 并将剩下一部分在 extrabuf 中的数据追加到 buffer_
    }
    return n;
}

ssize_t Buffer::WriteFd(int fd, int* saved_errno) {
    ssize_t n = write(fd, Peek(), ReadableBytes());  // 把 buffer_ 中的所有可读数据写入 fd
    if (n < 0) {
        *saved_errno = errno;
    } else {
        // TODO: (自己写的) 更新读游标 reader_index_
        Retrieve(n);
    }
    return n;
}

char const* Buffer::FindCRLF() const {
    // std::search 查找 \r\n 子序列是否在 buffer_ 中
    char const* crlf = std::search(Peek(), BeginWrite(), kCRLF.begin(), kCRLF.end());
    return crlf == BeginWrite() ? nullptr : crlf;
}

}  // namespace cutemuduo
