#include "CurrentThread.h"
#include <string>
namespace myServer {
namespace currentThread {
__thread int t_catchTid = 0;                   // 缓存的线程id
__thread char t_tidString[32];                 // 缓存的线程id字符串
__thread int t_tidStringLength = 6;            // 缓存的线程id字符串的长度
__thread const char *t_threadName = "unknown"; // 缓存的线程的名称
void catchTid() {
    if (t_catchTid == 0) {
        t_catchTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_catchTid);
    }
}
void afterFork() {
    // 供子进程调用
    t_threadName = "main";
    t_catchTid = 0;
    tid();
}
class ThreadNameInitializer {
  public:
    ThreadNameInitializer() {
        t_threadName = "main";
        tid();
        pthread_atfork(NULL, NULL, &afterFork); // 对fork函数调用进行注册，当fork时子进程会调用afterfork
    }
};
ThreadNameInitializer init;
} // namespace currentThread
} // namespace myServer