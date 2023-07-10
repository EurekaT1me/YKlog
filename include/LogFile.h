/**
 * LogFile: 后端日志管理
 * AppendFile: 实现类，写入缓冲，fflush
 * append(): 写入
 * flush(): 刷新缓冲，当短时间内日志长度较小时，不能将日志信息长时间放如缓存中，因此日志每记录1024次数就检查一次距前一次flush到文件的时间是否超过3s
 * rollFile(): 滚动日志，当日志消息记录长度达到设定值或日志记录时间超过当天进行日志滚动
 * getLogFileName(): 获得日志文件名字
 * m_mutex: 可选择是否对append和flush进行锁操作保证线程安全，因为append内部使用的是fwrite_unlocked()
 * 基本逻辑：
 *  AppendFile实现非线程安全的写入和刷新操作，在LogFile中来决定是否对写入和刷新加锁
 *  对于roll的时机：一是当前写入文件的字节数大于rollSize; 二是到了新的一天
 *  对于flush的时机：一是roll时关闭文件强制刷新；二是距离上一次刷新时间超过flushInterval秒
 *  对于进行roll和flush的检测时机：当append()后检测
 */
#pragma once
#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>
using boost::noncopyable;
using namespace std;
namespace myServer {

class LogFile : public noncopyable {
  public:
    LogFile(const string &basename, //  日志文件名，默认保存在当前工作目录下
            off_t rollSize,         //  日志文件超过设定值进行roll
            bool threadSafe = true, //  默认线程安全，使用互斥锁操作将消息写入缓冲区
            int flushInterval = 3,  //  flush刷新时间间隔
            int checkEveryN = 1024  //  每1024次日志操作，检查一个是否刷新、是否roll
    );
    ~LogFile(); // 不能将析构函数作为内联的，因为使用前置申明，不知道AppendFile的大小
    void append(const char *logline, int len);
    void flush();
    bool rollFile();

  private:
    class AppendFile;
    unique_ptr<AppendFile> file_; // PIMPL手法

    void append_unlocked(const char *logline, int len);                // 不加锁版本的append
    static string getLogFileName(const string &basename, time_t *now); // 获取roll时刻的文件名

    mutex mutex_; // 对append()操作加锁

    const string basename_;
    const off_t rollSize_;
    const bool threadSafe_;
    const int flushInterval_;
    const int checkEveryN_;

    time_t startOfPeriod_;                            // 用于标记同一天的时间戳(GMT的零点)
    time_t lastRoll_;                                 // 上一次roll的时间戳
    time_t lastFlush_;                                // 上一次flush的时间戳
    const static int kRollPerSeconds_ = 60 * 60 * 24; // 每过一天自动roll

    int count_; // 记录进行了多少次写入日志操作，当进行到checkEveryN次时检查是否需要roll
};

} // namespace myServer
