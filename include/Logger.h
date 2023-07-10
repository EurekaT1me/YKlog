/** Logger类：、
 * Logger负责全局的日志级别、输出目的地设置（静态成员函数），实际的数据流处理由Impl内部类实现
 * Impl的成员变量LogSteam对象是实际的缓存处理对象，包含了日志信息的加工，通过Logger的stream()函数取得实现各种日志宏功能
 * 当Logger对象析构时，将LogStream的日志数据flush到输出目的地，默认是stdout。
 * 每个线程使用使用会先创建一个Logger内部有Logstrem,然后将内容写入stream缓冲区，由于这是单线程操作，所以不需要锁，并且是非阻塞的，当写入完成后，析构时会使用fwrite将这块内存写入到指定缓冲区，fwrite是线程安全的。所以对用户来说是完全非阻塞的，异步的 */
#pragma once
#include "LogStream.h"
#include <functional>
#include <memory>
#include <string.h>
namespace myServer {
using namespace std;
class Logger {
  public:
    enum LogLevel { TRACE,
                    DEBUG,
                    INFO,
                    WARN,
                    ERROR,
                    FATAL,
                    NUM_LOG_LEVELS };
    // 计算源文件名

    class SourceFile {
      public:
        const char *data() const { return data_; }
        int size() const { return size_ - 1; } // 返回不包括'\0'的长度
        template <int N>
        SourceFile(const char (&arr)[N]) : data_(arr), size_(N) {
            // The strrchr() function returns a pointer to the last occurrence of the character c in the string s.
            const char *slash = strrchr(data_, '/');
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }
        explicit SourceFile(const char *arr) : data_(arr) {
            const char *slash = strrchr(data_, '/');
            if (slash) {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

      private:
        const char *data_;
        int size_;
    };
    ~Logger();

    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char *func);
    Logger(SourceFile file, int line, bool toAbort);

    LogStream &stream(); // 返回impl实现类中的Logstream,主要用于日志宏

    static LogLevel logLevel();              // 全局方法，返回日志级别list
    static void setLogLevel(LogLevel level); // 全局方法，设置日志级别list

    using OutputFunc = function<void(const char *msg, int len)>; // 用户传递的调用fwrtie的函数，通常会自己选择输出位置
    using FlushFunc = function<void()>;                          // 用户传递的调用fflush的函数，通常会自己选择输出位置
    static void setOutput(OutputFunc);                           // 全局方法，设置ffwrite
    static void setFlush(FlushFunc);                             // 全局方法，设置flush
  private:
    class Impl;
    unique_ptr<Impl> impl_; // 内部实现类
};
extern Logger::LogLevel g_logLevel;
inline Logger::LogLevel Logger::logLevel() { return g_logLevel; }

// 定义日志宏，创建并返回一个Logstream
/** 预定义标识符
 * __FILE__ 当前源文件名
 * __LINE__ 当前源代码行号
 * __func__ 包含封闭函数的未限定和未修饰名称的字符串(函数名)
 */
// 如果当前Logger内的全局g_logLevel等级小于TRACE,则创建stream
#define LOG_TRACE                                                \
    if (myServer::Logger::logLevel() <= myServer::Logger::TRACE) \
    myServer::Logger(__FILE__, __LINE__, myServer::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                                \
    if (myServer::Logger::logLevel() <= myServer::Logger::DEBUG) \
    myServer::Logger(__FILE__, __LINE__, myServer::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                                \
    if (myServer::Logger::logLevel() <= myServer::Logger::INFO) \
    myServer::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN myServer::Logger(__FILE__, __LINE__, myServer::Logger::WARN).stream()
#define LOG_ERROR myServer::Logger(__FILE__, __LINE__, myServer::Logger::ERROR).stream()
#define LOG_FATAL myServer::Logger(__FILE__, __LINE__, myServer::Logger::FATAL).stream()
#define LOG_SYSERR myServer::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL myServer::Logger(__FILE__, __LINE__, true).stream()

const char *strerror_tl(int savedErrno);
} // namespace myServer
