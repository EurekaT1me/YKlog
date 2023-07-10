/** AsyncLogging
 * 异步日志类，用于管理日志前端和后端，实现用户的异步日志输入
 * append(),将前端的日志输出重定向为AsyncLogging的append()，append将前端缓存中的日志信息添加到异步日志类的缓存中
 * LogFile，异步日志调用非线程安全的LogFile提升效率,后端需要使用单线程
 * 后端总共使用4块4MB的缓存，前端用两块，后端用两块，在前后端之间进行流动
 * FIXME:当日志量突然增大时，前端nextbuffer不存在时会不断创建新缓存，导致占用内存不断增大，且无法主动释放。
 * FIXME:解决方法：限制最大缓存数量，当数量超过时，丢弃掉多余的日志buffer，只留两个; 使用内存池防止内存碎片
 */

#pragma once
#include "CountDownLatch.h"
#include "Logger.h"
#include <atomic>
#include <boost/noncopyable.hpp>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
namespace myServer {
using namespace std;
using boost::noncopyable;
class AsyncLogging : public noncopyable {
  public:
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = vector<unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    AsyncLogging(const char *basename, off_t rollSize, int flushInterval_ = 3);
    ~AsyncLogging() {
        if (running_.load()) {
            stop();
        }
    }

    void start() {
        running_.store(true);
        thread_.push_back(unique_ptr<thread>(new thread(bind(&AsyncLogging::threadFunc, this))));
        latch_.wait();
    }                                      // 启动异步日志类，主要是启动后端写入线程
    void append(const char *msg, int len); // 将前端数据写入前端缓存
    void stop() {
        running_.store(false);
        cond_.notify_one();
        if (thread_[0]->joinable()) {
            thread_[0]->join();
        }
    } // 结束异步日志类，阻塞等待后端线程结束

  private:
    mutex mutex_;             // 用户保护后端存放的缓存，因为前端操作缓存时涉及多线程
    condition_variable cond_; // 通知后端处理缓存，写入本地文件
    CountDownLatch latch_;    // 倒计时器，用于等待后端线程创建

    BufferVector buffers_;    // 前端使用的缓存数组，当待写入的缓存满时，放入数组，用以和后端的缓存数组进行交换，swap操作只交换元素指针，速度很快
    BufferPtr currentBuffer_; // 前端待写入的缓存
    BufferPtr nextBuffer_;    // 前端下一块待写入的缓存

    vector<unique_ptr<thread>> thread_; // 用于创建后端线程

    const char *basename_;    // 本地文件基本名，初始化AppendFile类
    const int flushInterval_; // 刷新缓存时间间隔，初始化AppendFile类
    const off_t rollSize_;    // 本地文件最大字节数，初始化AppendFile类

    atomic<bool> running_; // 异步日志类是否运行

    void threadFunc();
};

} // namespace myServer
