#ifndef MJLog
#define MJLog(fmt__, ...) printf(fmt__"\n", ##__VA_ARGS__)
#define MJWarn(fmt__, ...) MJLog("warning:" fmt__, ##__VA_ARGS__)
#define MJError(fmt__, ...) MJLog("error:" fmt__, ##__VA_ARGS__)
#endif
