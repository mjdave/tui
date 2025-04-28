#ifndef TuiLog
#define TuiLog(fmt__, ...) printf(fmt__"\n", ##__VA_ARGS__)
#define TuiWarn(fmt__, ...) TuiLog("warning:" fmt__, ##__VA_ARGS__)
#define TuiError(fmt__, ...) TuiLog("error:" fmt__, ##__VA_ARGS__)
#endif
