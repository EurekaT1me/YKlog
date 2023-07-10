#include "LogStream.h"
#include <iostream>
/** Efficient Integer to String Conversions, by Matthew Wilson.
 * 1.设计一个查询表 "9876543210123456789"
 * 2.从给定数据的最低位开始取余，并找到对应字符放入buf中，就算余数是负数也能正确找到
 * 3.最后在buf中放入负数符号（如果需要）
 * 4.将buf reverse即可
 */
namespace myServer {
const char digits[] = "9876543210123456789";
const char *zero = digits + 9;
const char digitsHex[] = "0123456789ABCDEF";
template <typename T>
size_t convert(char *buf, T value) {
    char *p = buf;
    T i = value;
    do {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd]; // lsd是负数也可以正确查找
    } while (i != 0);
    if (value < 0) {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);
    return p - buf;
}
template <typename T>
void LogStream::formatInterge(T val) {
    if (buffer_.avail() >= kMaxNumericSize) {
        size_t len = convert(buffer_.current(), val);
        buffer_.add(len);
    }
}

LogStream &LogStream::operator<<(int val) {
    formatInterge(val);
    return *this;
}
LogStream &LogStream::operator<<(unsigned int val) {
    formatInterge(val);
    return *this;
}
LogStream &LogStream::operator<<(double val) {
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", val);
        buffer_.add(len);
    }
    return *this;
}
LogStream &LogStream::operator<<(long val) {
    formatInterge(val);
    return *this;
}
LogStream &LogStream::operator<<(unsigned long val) {
    formatInterge(val);
    return *this;
}
LogStream &LogStream::operator<<(long long val) {
    formatInterge(val);
    return *this;
}
LogStream &LogStream::operator<<(unsigned long long val) {
    formatInterge(val);
    return *this;
}
LogStream &LogStream::operator<<(unsigned short val) {
    formatInterge(val);
    return *this;
}
LogStream &LogStream::operator<<(short val) {
    formatInterge(val);
    return *this;
}

size_t convertHex(char buf[], uintptr_t value) {
    uintptr_t i = value;
    char *p = buf;

    do {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}
LogStream &LogStream::operator<<(const void *p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize) {
        char *buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }
    return *this;
}

} // namespace myServer
