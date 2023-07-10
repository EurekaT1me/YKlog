#include "TimeStamp.h"

namespace myServer {
std::string TimeStamp::toString() const {
    // 将当前时间戳转化为“秒.微秒”的形式
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondPerSecond;
    // 使用PRId64来表示32位系统和64位系统的64位整数
    snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%.06" PRId64, seconds, microseconds);
    return std::string(buf);
}
std::string TimeStamp::toFormatString(bool showMicroSeconds) const {
    // 将当前时间转换为"年：月：日：时：分：秒"或者“年：月：日：时：分：秒.微秒”
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondPerSecond);
    struct tm *tm_time;
    // gmtime_r(&seconds, tm_time); // UTC时区
    tm_time = localtime(&seconds); // 计算机本地时区
    if (showMicroSeconds) {
        int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondPerSecond;
        snprintf(buf, sizeof(buf) - 1, "%4d%02d%02d% 02d:%02d:%02d.%06" PRId64, tm_time->tm_year + 1900, tm_time->tm_mon, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec, microseconds);
    } else {
        snprintf(buf, sizeof(buf) - 1, "%4d%02d%02d% 02d:%02d:%02d", tm_time->tm_year + 1900, tm_time->tm_mon, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    }
    return std::string(buf);
}

TimeStamp TimeStamp::now() {
    // 返回记录当前时间的时间戳
    struct timeval tv;
    gettimeofday(&tv, NULL); // 获取当前时间戳，NULL表示默认为UTC时区
    return TimeStamp(tv.tv_sec * kMicroSecondPerSecond + tv.tv_usec);
}

void swap(TimeStamp &lhs, TimeStamp &rhs) {
    lhs.swap(rhs);
} // 提供non-member的swap
} // namespace myServer

namespace std {
template <>
void swap<myServer::TimeStamp>(myServer::TimeStamp &lhs, myServer::TimeStamp &rhs) {
    lhs.swap(rhs);
} // 提供std特化版本的swap,调用其成员函数
} // namespace std
