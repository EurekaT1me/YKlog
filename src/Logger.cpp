#include "Logger.h"
#include "CurrentThread.h"
#include "LogStream.h"
#include "TimeStamp.h"
#include <assert.h>
#include <string.h>
#include <thread>

#include <iostream>
namespace myServer {
// Impl私有类，实现日志信息缓冲格式化处理
class Logger::Impl {
  public:
    using LogLevel = Logger::LogLevel;
    Impl(LogLevel level, int savedErrno, const Logger::SourceFile &file, int line);
    void formatTime(); // 格式化时间
    void finish();     // 写入文件名，前端写日志完成时由析构函数调用

    TimeStamp time_;              // 日志创建时的时间戳
    LogStream stream_;            // 日志缓存
    LogLevel level_;              // 日志级别
    int line_;                    // 当前记录日式宏的 源代码行号
    Logger::SourceFile basename_; // 当前记录日式宏的 源代码名称
};

// 根据level获得levelname，长度都是6
const char *LogLevelName[Logger::NUM_LOG_LEVELS] =
    {
        "TRACE ",
        "DEBUG ",
        "INFO  ",
        "WARN  ",
        "ERROR ",
        "FATAL ",
};

// 获取errno的错误描述，并放入__thread 修饰的线程缓存变量t_errnobuf[512]中
__thread char t_errnobuf[512];
const char *strerror_tl(int savedErrno) {
    // 使用strerror_r是线程安全的，strerror不是线程安全的
    return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf)); // return the appropriate error description string,
}

// 用于写入给定大小数据的辅助类,可以保证每次格式化的长度相同
class T {
  public:
    explicit T(const char *data, unsigned int size) : data_(data), size_(size) {
        assert(strlen(data) == size);
    }
    const char *data_;
    const unsigned int size_;
};
// 重载对T类型的输入
inline LogStream &operator<<(LogStream &ts, const T &t) {
    ts.append(t.data_, t.size_);
    return ts;
}
// Impl类的构造函数
// 级别，错误(没有错误则传0),文件，行
// Impl类主要是负责日志的格式化, 格式：“时间 线程id 级别 错误”
Logger::Impl::Impl(LogLevel level, int savedErrno, const Logger::SourceFile &file, int line) : level_(level), basename_(file), line_(line), time_(TimeStamp::now()), stream_() {
    formatTime();
    currentThread::tid(); // 缓存当前线程

    // #ifdef DEBUG
    //     printf("%lu\n", strlen(currentThread::tidString()));
    //     printf("%s\n", currentThread::tidString());
    // #endif

    // stream_ << T(currentThread::tidString(), 6); // FIXME:线程号可能到7位数，不能用确定的数字作为输入
    stream_ << T(currentThread::tidString(), strlen(currentThread::tidString()));
    stream_ << T(LogLevelName[level], 6);
    if (savedErrno) {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ")";
    }
}

__thread char t_time[64];     // 线程缓存了当前时间字符串 “年:月:日 时:分:秒”
__thread time_t t_lastSecond; // 线程缓存了上一次日志记录的秒
void Logger::Impl::formatTime() {
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();        // 获得当前微秒时间戳
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / 1000000); // 获得秒
    int microSeconds = static_cast<int>(microSecondsSinceEpoch % 1000000);  // 获得微秒

    if (seconds != t_lastSecond) {
        // 和上一次日志记录不在同一秒
        // 需要更新t_time 和 t_lastsecond
        t_lastSecond = seconds;
        struct tm tm_time;
        // gmtime_r(&seconds, &tm_time);
        localtime_r(&seconds, &tm_time);
        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
    }
    char microBuf[32];
    snprintf(microBuf, sizeof(microBuf), ".%06dZ ", microSeconds); // Z表示UTC时区
    stream_ << T(t_time, 17) << T(microBuf, 9);
}

// 重载对SourceFile类型的<<
inline LogStream &operator<<(LogStream &ts, const Logger::SourceFile &file) {
    ts.append(file.data(), file.size());
    return ts;
}
void Logger::Impl::finish() {
    // 写入日志完成时的格式化，将文件名和行数写入缓存
    stream_ << " - " << basename_ << ":" << line_ << '\n';
}

void defaultOutput(const char *msg, int len) {
    fwrite(msg, 1, len, stdout);
}
void defaultFlush() {
    fflush(stdout);
}
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;
void Logger::setOutput(Logger::OutputFunc f) {
    g_output = f;
}
void Logger::setFlush(Logger::FlushFunc f) {
    g_flush = f;
}
Logger::~Logger() {
    // 使用fwrite写入缓冲区，默认为stdout
    impl_->finish();
    const LogStream::Buffer &buf(stream().buffer());

    g_output(buf.data(), buf.length());
    if (impl_->level_ == FATAL) {
        // 如果当前日志级别为FATAL，立刻刷新缓冲区并停止程序
        g_flush();
        abort();
    }
}
Logger::Logger(SourceFile file, int line) : impl_(new Impl(INFO, 0, file, line)) {
}
Logger::Logger(SourceFile file, int line, LogLevel level) : impl_(new Impl(level, 0, file, line)) {
}
Logger::Logger(SourceFile file, int line, LogLevel level, const char *func) : impl_(new Impl(level, 0, file, line)) {
    impl_->stream_ << func << ' ';
}
Logger::Logger(SourceFile file, int line, bool toAbort) : impl_(new Impl(toAbort ? FATAL : ERROR, errno, file, line)) {
}

Logger::LogLevel initLogLevel() {
    if (::getenv("myServer_LOG_TRACE"))
        return Logger::TRACE;
    else if (::getenv("myServer_LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}
Logger::LogLevel g_logLevel = initLogLevel();
void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

LogStream &Logger::stream() {
    return impl_->stream_;
}
} // namespace myServer
