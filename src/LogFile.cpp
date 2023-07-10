#include "LogFile.h"
#include "Logger.h"
#include "TimeStamp.h"
#include <assert.h>

namespace myServer {
class LogFile::AppendFile {
  public:
    // 对文件资源采用RAII手法
    explicit AppendFile(const string &filename) : fp_(::fopen(filename.c_str(), "ae")), writtenBytes_(0) {
        init();
    } // 'e' for O_CLOEXEC
    explicit AppendFile(const char *filename) : fp_(::fopen(filename, "ae")), writtenBytes_(0) {
        init();
    }
    ~AppendFile() {
        fclose(fp_); // 关闭文件时会强制flush
    }

    void append(const char *logline, int len) {
        size_t n = write(logline, len); // 先写入缓冲区
        size_t remain = len - n;        // 缓冲区可能无法一次性写完
        while (remain > 0) {
            size_t x = write(logline, len);
            if (x == 0) {
                int err = ferror(fp_);
                if (err) {
                    fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
                }
                break;
            }
            n += x;
            remain = len - n;
        }
        writtenBytes_ += len;
    }                                                    // 调用fwrite_unlocked进行实际的写入动作，由于设置了缓冲区，会先将内容写入缓冲区
    void flush() { ::fflush(fp_); }                      // 立刻刷新缓冲区
    off_t writtenBytes() const { return writtenBytes_; } // 返回已写日志数据的总字节数

  private:
    void init() {
        assert(fp_);
        ::setbuffer(fp_, buffer_, sizeof(buffer_)); // 设置写入文件前的缓冲区，当缓冲区满时一次性写入，减少I/O调用次数
    }
    size_t write(const char *logline, int len) {
        return ::fwrite_unlocked(logline, 1, len, fp_);
    } // 调用fwrite_unlocked写入文件
    FILE *fp_;
    char buffer_[64 * 1024]; // 文件输出缓冲区，64kB大小
    off_t writtenBytes_;     // 已写日志数据的总字节数。off_t表示文件大小，不同位机器范围不同。
};

LogFile::LogFile(const string &basename, //  日志文件名，默认保存在当前工作目录下
                 off_t rollSize,         //  日志文件超过设定值进行roll
                 bool threadSafe,        //  默认线程安全，使用互斥锁操作将消息写入缓冲区
                 int flushInterval,      //  flush刷新时间间隔
                 int checkEveryN         //  每1024次日志操作，检查一个是否刷新、是否roll
                 ) : basename_(basename), rollSize_(rollSize), threadSafe_(threadSafe), flushInterval_(flushInterval), checkEveryN_(checkEveryN), startOfPeriod_(0), lastRoll_(0), lastFlush_(0) {
    assert(basename.find('/') == string::npos); // 保证basename不包含路径
    rollFile();
}

/**
 * 滚动日志
 * 相当于新创建日志文件，再向其中写入
 */
bool LogFile::rollFile() {
    time_t now = 0;
    string filename = getLogFileName(basename_, &now);

    // 注意，这里先除kRollPerSeconds_然后乘KPollPerSeconds表示对齐值kRollPerSeconds_的整数倍，
    // 也就是事件调整到当天零点(/除法会引发取整)
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start; // 在roll时更换每天的零点时间戳
        file_.reset(new AppendFile(filename));
    }
    return false;
}

// 返回主机名
string hostname() {
    char buf[256] = {0};
    if (::gethostname(buf, sizeof(buf)) == 0) {
        return buf;
    } else {
        return "unknownhost";
    }
}
/**
 * 构造一个日志文件名
 * 日志名由基本名字+时间戳+主机名+进程id+加上“.log”后缀
 */
string LogFile::getLogFileName(const string &basename, time_t *now) {
    string filename;
    filename.reserve(basename.size() + 64); // reserve()将字符串的容量设置为至少basename.size() + 64,因为后面要添加时间、主机名、进程id等内容

    filename = basename; // 加上文件基本名字

    TimeStamp ts(TimeStamp::now());
    char timeBuf[32] = {0};
    struct tm tm_time;
    *now = ts.SecondsSinceEpoch();
    gmtime_r(now, &tm_time);
    strftime(timeBuf, sizeof(timeBuf), ".%Y%m%d-%H%M%S.", &tm_time); // 格式化时间
    // snprintf(timeBuf, sizeof(timeBuf), ".%4d%02d%02d%-02d%02d%02d.", tm_time.tm_year + 1900, tm_time.tm_mon, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    filename += timeBuf; // 加上时间戳（UTC）

    filename += hostname(); // 加上主机名

    char pidbuf[32] = {0};
    pid_t pid = ::getpid();
    snprintf(pidbuf, sizeof(pidbuf), ".%d", pid);
    filename += pidbuf; // 加上进程号

    filename += ".log"; // 加上后缀

    return filename;
}

/**
 * 添加日志信息
 * 带锁添加和不带锁添加(此时只能单线程使用)
 * 带锁刷新和不带锁刷新（此时只能单线程使用）
 */
void LogFile::append_unlocked(const char *logline, int len) {
    file_->append(logline, len);
    // 文件中已写入的字节超过roll的限制，就roll,(但是不会把已经超过的部分写到新文件)
    if (file_->writtenBytes() > rollSize_) {
        rollFile();
    } else {
        count_++;
        if (count_ >= checkEveryN_) { // 检查是否需要roll和flush,roll的条件是是否到了新的一天，flush的条件是是否超过3秒没有flush
            count_ = 0;
            TimeStamp ts(TimeStamp::now());
            time_t now = ts.SecondsSinceEpoch();
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod_ != startOfPeriod_) {
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) {
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}
void LogFile::append(const char *logline, int len) {
    if (threadSafe_) {
        unique_lock<mutex> lck(mutex_);
        append_unlocked(logline, len);
    } else {
        append_unlocked(logline, len);
    }
}
void LogFile::flush() {
    if (threadSafe_) {
        unique_lock<mutex> lck(mutex_);
        file_->flush();
    } else {
        file_->flush();
    }
}

LogFile::~LogFile() = default;
} // namespace myServer