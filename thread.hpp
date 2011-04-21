#ifndef MCP_BASE_THREAD_HEAD
#define MCP_BASE_THREAD_HEAD

#include <pthread.h>
#include <tr1/functional>

namespace base {

typedef std::tr1::function<void()> ThreadBody;
pthread_t makeThread(ThreadBody body);

}  // namespace base

#endif // MCP_BASE_THREAD_HEAD
