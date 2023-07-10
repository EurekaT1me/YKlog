#include "AsyncLogging.h"
#include "LogFile.h"
#include "TimeStamp.h"
#include <assert.h>
#include <chrono>
namespace myServer {
AsyncLogging::AsyncLogging(const char *basename, off_t rollSize, int flushInterval) : basename_(basename),
                                                                                      rollSize_(rollSize),
                                                                                      flushInterval_(flushInterval),
                                                                                      latch_(1),
                                                                                      currentBuffer_(new Buffer),
                                                                                      nextBuffer_(new Buffer),
                                                                                      buffers_(),
                                                                                      running_(true)

{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}
/** 前端写入
 * 如果当前缓冲区大小不足，则将nextBuffer移动给当前缓冲区
 * 如果nextBUffer不存在，则创建一块新的缓存，（只有当日志量很大时会出现）
 */
void AsyncLogging::append(const char *msg, int len) {
    unique_lock<mutex> lck(mutex_);
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(msg, len);
    } else {
        // 当前缓冲区空间不足
        buffers_.push_back(move(currentBuffer_)); // 右值添加内部调用emplace_back
        if (nextBuffer_) {
            currentBuffer_ = move(nextBuffer_);
        } else {
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(msg, len);
        cond_.notify_one();
    }
}

/** 后端线程创建函数
 * 创建后端用缓存，用于和前端缓存交换
 * 临界区内进行缓存数组的交换，临界区触发条件：超时（超过刷新时间），前端写满一个或多个buffer
 * 非临界区执行文件写入
 * (1) 日志待写入文件buffersToWrite队列超限（短时间堆积，多为异常情况）
 * (2) buffersToWrited队列中的日志消息交给后端写入
 * (3) 将buffersToWrited队列中的buffer重新填充newBuffer1、newBuffer2
 */
void AsyncLogging::threadFunc() {
    assert(running_ == true);

    LogFile output(basename_, rollSize_, false); // 单线程使用非线程安全的写入

    // 创建后端用缓冲和缓冲数组
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector bufferToWrite; // 待写入本地文件的缓冲数组
    bufferToWrite.reserve(16);

    while (running_.load()) {
        latch_.countDown(); // 保证线程进入到循环，因为可能有一种情况，开始异步日志又马上结束，线程没来得及进入循环，后端无法拿到前端的数据进行写入
        // 单线程，不必考虑cond唤醒之后条件是否满足
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(bufferToWrite.empty());

        // 临界区操作：交换缓存数组，移动前后端缓存
        {
            unique_lock<mutex> lck(mutex_);
            if (buffers_.empty()) {
                // 日志量很少时，buffers_为空，等待一次刷新间隔进行唤醒
                cond_.wait_for(lck, chrono::duration<int>{flushInterval_});
            }
            buffers_.push_back(move(currentBuffer_));
            currentBuffer_ = move(newBuffer1); // 移动buffer
            bufferToWrite.swap(buffers_);
            if (!nextBuffer_) {
                // nextBuffer不存在时进行补充
                nextBuffer_ = move(newBuffer2);
            }
        }

        // 非临界区操作：写入本地文件
        // 1. 日志待写入文件buffersToWrite队列超限,只保留两块缓存区，其余的丢弃
        if (bufferToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Dropped log messages at %s, %zd larger buffers\n", TimeStamp::now().toFormatString().c_str(), bufferToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            bufferToWrite.erase(bufferToWrite.begin() + 2, bufferToWrite.end()); // unique_ptr会自动释放内存
        }
        // 2.buffersToWrited队列中的日志消息交给后端写入
        for (const auto &buffer : bufferToWrite) {
            output.append(buffer->data(), buffer->length());
        }
        // 3.将buffersToWrited队列中的buffer重新填充newBuffer1、newBuffer2
        if (bufferToWrite.size() > 2) {
            bufferToWrite.resize(2); // resize 会删除掉多余的元素
            bufferToWrite.reserve(16);
        }
        if (!newBuffer1) {
            assert(!bufferToWrite.empty());
            newBuffer1 = move(bufferToWrite.back());
            bufferToWrite.pop_back();
            newBuffer1->reset();
        }
        if (!newBuffer2) {
            assert(!bufferToWrite.empty());
            newBuffer2 = move(bufferToWrite.back());
            bufferToWrite.pop_back();
            newBuffer2->reset();
        }

        bufferToWrite.clear();
        output.flush();
    }

    output.flush();
}

} // namespace myServer