仿照muduo实现的日志库，支持同步写入和异步写入

默认引入logger头文件为同步模式写入，并且默认输出到标准输出
```c++
#include "logger.h"

LOG_INFO<<"xxxxx";
```

异步日志通常用在写入文件时使用，需要引入asynclogging，并且创建一个实例用于启动一个后台线程进行实际的写入操作。

```c++
#include "asynclogging.h"
AsyncLogging* g_asyncLog = NULL;
void asyncOutput(const char *msg,int lent){
    g_asyncLog->append(msg,len);
}

Logger::setOutput(asyncOutput); // 设置输出位置

```