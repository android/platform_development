#ifndef UTHREADS_CWRAPPER_H
#define UTHREADS_CWRAPPER_H_
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"{
#endif
#include <stdbool.h>
struct      WuThread; 
typedef     struct WuThread WuThread;
struct      WkThread; 
typedef     struct WkThread WkThread;
struct      WCluster; 
typedef     struct WCluster WCluster;
struct      WConnection; 
typedef     struct WConnection WConnection;
struct      WMutex; 
typedef     struct WMutex WMutex;
struct      WOwnerLock; 
typedef     struct WOwnerLock WOwnerLock;
struct      WConditionVariable; 
typedef     struct WConditionVariable WConditionVariable;
struct      WSemaphore; 
typedef     struct WSemaphore WSemaphore;
struct      WuThreadPool; 
typedef     struct WuThreadPool WuThreadPool;
WMutex* Mutex_create();
_Bool Mutex_acquire(WMutex* self);
_Bool Mutex_acquire_1(WMutex* self);
void Mutex_release(WMutex* self);
void Mutex_unlock(WMutex* self);
void Mutex_destroy(WMutex* self);
WOwnerLock* OwnerLock_create();
mword OwnerLock_acquire(WOwnerLock* self);
mword OwnerLock_release(WOwnerLock* self);
void ConditionVariable_wait(WConditionVariable* self, WMutex* mutex);
void ConditionVariable_signal(WConditionVariable* self, WMutex* mutex);
void ConditionVariable_signalAll(WConditionVariable* self, WMutex* mutex);
_Bool ConditionVariable_empty(WConditionVariable* self);
void ConditionVariable_destroy(WConditionVariable* self);
WConditionVariable* ConditionVariable_create();
WSemaphore* Semaphore_create(mword c);
_Bool Semaphore_P(WSemaphore* self);
void Semaphore_V(WSemaphore* self);
void Semaphore_destroy(WSemaphore* self);
WuThread* uThread_create(WuThread* self, size_t ss, _Bool joinable);
WuThread* uThread_create_1(WuThread* self, _Bool joinable);
void uThread_start(WuThread* self, WCluster* cluster, ptr_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3);
void uThread_yield(WuThread* self);
void uThread_terminate(WuThread* self);
void uThread_migrate(WuThread* self, WCluster* );
void uThread_resume(WuThread* self);
_Bool uThread_join(WuThread* self);
void uThread_detach(WuThread* self);
WCluster* uThread_getCurrentCluster(WuThread* self);
uint64_t uThread_getTotalNumberofUTs(WuThread* self);
uint64_t uThread_getID(WuThread* self);
WuThread* uThread_currentUThread(WuThread* self);
void uThread_destroy(WuThread* self);
WCluster* Cluster_create();
void Cluster_destroy(WCluster* self);
WCluster* Cluster_getDefaultCluster(WCluster* self);
uint64_t Cluster_getID(WCluster* self);
size_t Cluster_getNumberOfkThreads(WCluster* self);
WkThread* kThread_create(WCluster* );
void kThread_destroy(WkThread* self);
std::class thread::native_handle_type kThread_getThreadNativeHandle(WkThread* self);
Wid* kThread_getID(WkThread* self);
WkThread* kThread_currentkThread(WkThread* self);
uint kThread_getTotalNumberOfkThreads(WkThread* self);
WConnection* Connection_create();
WConnection* Connection_create_1(int fd);
WConnection* Connection_create_2(int domainint typeint protocol);
void Connection_destroy(WConnection* self);
int Connection_accept(WConnection* self, WConnection* conn, sockaddr* addr, socklen_t * addrlen);
WConnection* Connection_accept(WConnection* self, sockaddr* addr, socklen_t * addrlen);
int Connection_socket(WConnection* self, int domain, int type, int protocol);
int Connection_listen(WConnection* self, int backlog);
int Connection_bind(WConnection* self, sockaddr* addr, socklen_t addrlen);
int Connection_connect(WConnection* self, sockaddr* addr, socklen_t addrlen);
ssize_t Connection_recv(WConnection* self, void * buf, size_t len, int flags);
ssize_t Connection_recvfrom(WConnection* self, void * buf, size_t len, int flags, sockaddr* src_addr, socklen_t * addrlen);
ssize_t Connection_recvmsg(WConnection* self, int sockfd, msghdr* msg, int flags);
int Connection_recvmmsg(WConnection* self, int sockfd, mmsghdr* msgvec, unsigned int vlen, unsigned int flags, timespec* timeout);
ssize_t Connection_send(WConnection* self, const void * buf, size_t len, int flags);
ssize_t Connection_sendto(WConnection* self, int sockfd, const void * buf, size_t len, int flags, sockaddr* dest_addr, socklen_t addrlen);
ssize_t Connection_sendmsg(WConnection* self, msghdr* msg, int flags);
int Connection_sendmmsg(WConnection* self, int sockfd, mmsghdr* msgvec, unsigned int vlen, unsigned int flags);
ssize_t Connection_read(WConnection* self, void * buf, size_t count);
ssize_t Connection_write(WConnection* self, const void * buf, size_t count);
void Connection_blockOnRead(WConnection* self);
void Connection_blockOnWrite(WConnection* self);
int Connection_close(WConnection* self);
int Connection_getFd(WConnection* self);
#ifdef __cplusplus
}
#endif
#endif /* UTHREADS_CWRAPPER_H_ */

