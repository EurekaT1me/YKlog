/** TimeStamp: 时间戳类
 * 使用int64_t类型的微秒作为时间戳记录；
 * 提供格式化字符串函数返回时间
 * 静态成员函数获取当前时间戳
 * 静态成员函数获取time_t秒数或加偏移的时间戳。
 */

#pragma once
#include <algorithm>
#include <inttypes.h>
#include <stdint.h>
#include <string>
#include <sys/time.h>
#include <time.h>

namespace myServer {
class TimeStamp {
  public:
    static const int kMicroSecondPerSecond = 1000 * 1000; // 一秒有多少微秒
    TimeStamp() : microSecondsSinceEpoch_(0) {}
    explicit TimeStamp(int64_t x) : microSecondsSinceEpoch_(x) {}
    void swap(TimeStamp &rhs) {
        using std::swap;
        swap(microSecondsSinceEpoch_, rhs.microSecondsSinceEpoch_);
    }
    bool valid() { return microSecondsSinceEpoch_ > 0; }                                                              // 返回记录的微秒数是否大于0
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }                                        // 返回记录的微秒数
    time_t SecondsSinceEpoch() const { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSecond); } //  返回记录的秒数

    // 格式化时间戳字符串
    std::string toString() const;
    std::string toFormatString(bool showMicroSeconds = 0) const;

    // 静态成员函数获取当前时间戳
    static TimeStamp now();                            // 获得一个记录当前时间的时间戳
    static TimeStamp invalid() { return TimeStamp(); } // 获得一个空的非法的时间戳

    // 静态成员函数获取time_t秒数加偏移的时间戳
    static TimeStamp fromUnixTime(time_t t, int microSeconds) { return TimeStamp(static_cast<int64_t>(t) * kMicroSecondPerSecond + microSeconds); }
    static TimeStamp fromUnixTime(time_t t) { return fromUnixTime(t, 0); }

  private:
    int64_t microSecondsSinceEpoch_; // 1970至今的微秒数
};

// 重载一些比较操作符用于容器排序
// 此处不能申明为member函数，因为需要为所有参数提供隐式类型转换，具体见effective c++ 条款24
// member函数对 2<timeStamp 无法编译通过
inline bool operator<(const TimeStamp &lhs, const TimeStamp &rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}
inline bool operator>(const TimeStamp &lhs, const TimeStamp &rhs) {
    return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
}
inline bool operator==(const TimeStamp &lhs, const TimeStamp &rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

void swap(TimeStamp &lhs, TimeStamp &rhs); // 提供non-member的swap

inline double timeDifference(const TimeStamp &high, const TimeStamp &low) {
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / TimeStamp::kMicroSecondPerSecond;
}
// 构造一个延迟delay秒的时间戳
inline TimeStamp addTime(const TimeStamp &ts, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * TimeStamp::kMicroSecondPerSecond); // 精确到double小数点后6位
    return TimeStamp(ts.microSecondsSinceEpoch() + delta);
}
} // namespace myServer

namespace std {
template <>
void swap<myServer::TimeStamp>(myServer::TimeStamp &lhs, myServer::TimeStamp &rhs); // 提供std特化版本的swap,调用其成员函数
} // namespace std
