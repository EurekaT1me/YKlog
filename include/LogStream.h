#pragma once
#include <algorithm>
#include <boost/implicit_cast.hpp>
#include <boost/noncopyable.hpp>
#include <cstring>
#include <string>

namespace myServer {
using boost::noncopyable;
using namespace std;
template <int SIZE>
class FixedBuffer : noncopyable {
  private:
    char data_[SIZE];                                         // SIZE大小的缓冲区
    char *cur_;                                               // 指向缓冲区可以添加字符的位置
    const char *end() const { return data_ + sizeof(data_); } // 返回缓冲区末尾位置

  public:
    FixedBuffer() : cur_(data_) {} // 构造函数，初始化当前指针位置至头指针
    ~FixedBuffer() {}              // 析构函数
    void append(const char *buf, size_t len) {
        if (boost::implicit_cast<size_t>(avail()) > len) {
            // implicit_cast 让编译器检查隐式转换，增加代码安全
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }                                                             // 添加一段长度为len的字符串到缓冲区
    const char *data() const { return data_; }                    // 返回缓冲区头的指针
    int length() const { return static_cast<int>(cur_ - data_); } // 返回缓冲区已使用的长度
    char *current() { return cur_; };                             // 返回缓冲区当前可写入的指针
    int avail() const { return static_cast<int>(end() - cur_); }  // 返回缓冲区剩余可以使用的长度
    void add(size_t len) { cur_ += len; };                        // 将可写入位置后移len长度
    void reset() { cur_ = data_; }                                // 将缓冲区重置
    void bzero() { memset(data_, 0, sizeof(data_)); }             // 将缓冲区置为0
};
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;
class LogStream : noncopyable {
  public:
    using Buffer = FixedBuffer<kSmallBuffer>;

  public:
    void append(const char *buf, size_t len) {
        buffer_.append(buf, len);
    }                                       // 向缓冲区添加c风格字符串
    void resetBuffer() { buffer_.reset(); } // 重置缓冲区
    Buffer &buffer() { return buffer_; }    // 返回缓冲区指针
    LogStream &operator<<(char v) {
        buffer_.append(&v, 1);
        return *this;
    } // 重载<<运算符，添加字符
    LogStream &operator<<(const char *str) {
        if (str) {
            // 这里只能用strlen,不能用sizeof,因为不能包含末尾的结束符
            buffer_.append(str, strlen(str));
        } else {
            buffer_.append("(NULL)", 6);
        }
        return *this;
    } // 重载<<运算符，添加C风格字符
    LogStream &operator<<(const std::string &str) {
        buffer_.append(str.c_str(), str.size());
        return *this;
    }                                    // 重载<<运算符，添加string容器
    LogStream &operator<<(int);          // 重载<<运算符，添加int类型
    LogStream &operator<<(unsigned int); // 重载<<运算符，添加unsigned int类型
    LogStream &operator<<(double);
    LogStream &operator<<(float x) {
        this->operator<<(static_cast<double>(x));
        return *this;
    }
    LogStream &operator<<(long);
    LogStream &operator<<(unsigned long);
    LogStream &operator<<(long long);
    LogStream &operator<<(unsigned long long);
    LogStream &operator<<(unsigned short);
    LogStream &operator<<(short);

    LogStream &operator<<(const void *);

  private:
    Buffer buffer_;                        // 4000字节的缓冲区
    static const int kMaxNumericSize = 32; // 数字转化为字符串的最大长度
    template <typename T>
    void formatInterge(T); // 用于将int类型转化为c风格字符串类型
};
} // namespace myServer
