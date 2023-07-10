/** CurrentThread命名空间
 * 封装了c++11thread提供的this_thread
 */

#pragma once
#include <sys/types.h>
#include <unistd.h>
namespace myServer {
namespace currentThread {
// CurrentThread 命名空间表示当前线程
// 一些需要全局使用但不需要设置成全局变量的线程缓存变量, __thread GCC提供的线程局部存储设施，简单来说，用__thread修饰的变量在每个线程中都会存在
extern __thread int t_catchTid;           // 缓存的线程id
extern __thread char t_tidString[32];     // 缓存的线程id字符串
extern __thread int t_tidStringLength;    // 缓存的线程id字符串的长度
extern __thread const char *t_threadName; // 缓存的线程的名称
void catchTid();
inline int tid() {
    if (__builtin_expect((t_catchTid == 0), 0)) {
        // __builtin_expect：由GCC提供，作用是允许程序员将最有可能执行的分支告诉编译器
        // __builtin_expect(EXP, N) EXP==N的概率很大。
        // 此处表示，t_catchTid==0为假的可能性很大，在编译时就会把else后的语句提前（把if语句延后），
        // 提高编译效率，此处的依据是cpu进行流水线式的执行，汇编过程中将多个条件判断分支按需进行优化，最近的条件语句执行效率最高，其他的需要进行jmp跳转。因jmp跳转效率相对较低，因此我们在高级语言编程需要将最可能发生的条件判断分支写在最前侧。
        catchTid();
    }
    return t_catchTid;
} // 获取当前缓存的tid,如果当前没有缓存线程，则把当前线程加入缓存器
inline const char *tidString() { return t_tidString; }     // 返回缓存的线程id字符串
inline int tidStringLength() { return t_tidStringLength; } // 返回缓存的线程id字符串的长度
inline const char *threadName() { return t_threadName; }   // 返回缓存的线程的名称
inline bool isMainThread() { return tid() == ::getpid(); } // 判断当先线程id是否是进程id

} // namespace currentThread
} // namespace myServer