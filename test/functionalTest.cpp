#include "AsyncLogging.h"
#include "Logger.h"
using namespace myServer;

void testlog() {
    LOG_INFO << "test";

    Logger::setLogLevel(Logger::DEBUG);
    LOG_DEBUG << "debug test";

    LOG_ERROR << "error";
}

AsyncLogging *g_asyncLog = NULL;
void asyncOutput(const char *msg, int len) {
    g_asyncLog->append(msg, len);
}

void testAsynclog() {
    Logger::setOutput(asyncOutput);

    AsyncLogging log("asynclog", 100);
    log.start();
    g_asyncLog = &log;

    LOG_INFO << "123";
}

int main(int argc, char const *argv[]) {
    // testAsynclog();
    testlog();
    return 0;
}
