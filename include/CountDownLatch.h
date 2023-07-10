/** CountDownLatch:倒计时类，当倒计时结束执行下一步操作
 * 设定一个需要准备就绪的事件数量
 * 该变量被互斥锁保护，在其他多个线程中被修改（各线程准备就绪后，数目变量进行减一）
 * 主线程阻塞等待，一旦数目变量减到值为0，条件变量通知唤醒主线程。
 */
#pragma once
#include <boost/noncopyable.hpp>
#include <condition_variable>
#include <mutex>

using boost::noncopyable;
using std::condition_variable;
using std::lock_guard;
using std::mutex;
using std::unique_lock;
namespace myServer {
class CountDownLatch : public noncopyable {
  public:
    explicit CountDownLatch(int cnt) : cnt_(cnt) {}
    void wait() {
        unique_lock<mutex> lck(mutex_);
        while (cnt_ > 0) {
            condition_.wait(lck);
        }
    } // 等待就绪事件完成
    void countDown() {
        unique_lock<mutex> lck(mutex_);
        cnt_--;
        if (cnt_ == 0) {
            condition_.notify_all();
        }
    } // 完成了一项就绪事件，当所有就绪事件完成，通知所有等待的线程

    int getCount() const {
        lock_guard<mutex> lck(mutex_);
        return cnt_;
    } // 返回需要等待就绪的事件数量

  private:
    mutable mutex mutex_;          // 互斥锁
    condition_variable condition_; // 条件变量
    int cnt_;                      // 等待就绪的事件数量
};

} // namespace myServer
