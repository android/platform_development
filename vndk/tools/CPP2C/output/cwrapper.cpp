#include "generic/basics.h"
#include "cwrapper.h"
#include "runtime/uThread.h"
#include "runtime/uThreadPool.h"
#include "runtime/kThread.h"
#include "io/Network.h"
#ifdef __cplusplus
extern "C"{
#endif
WMutex* Mutex_create(){
    return reinterpret_cast<WMutex*>( new Mutex()); 
}
_Bool Mutex_acquire(WMutex* self){
    return reinterpret_cast<Mutex*>(self)->acquire(); 
}
_Bool Mutex_acquire_1(WMutex* self){
    return reinterpret_cast<Mutex*>(self)->acquire(); 
}
void Mutex_release(WMutex* self){
    reinterpret_cast<Mutex*>(self)->release(); 
}
void Mutex_unlock(WMutex* self){
    reinterpret_cast<Mutex*>(self)->unlock(); 
}
void Mutex_destroy(WMutex* self){
     delete reinterpret_cast<Mutex*>(self); 
}
WOwnerLock* OwnerLock_create(){
    return reinterpret_cast<WOwnerLock*>( new OwnerLock()); 
}
mword OwnerLock_acquire(WOwnerLock* self){
    return reinterpret_cast<OwnerLock*>(self)->acquire(); 
}
mword OwnerLock_release(WOwnerLock* self){
    return reinterpret_cast<OwnerLock*>(self)->release(); 
}
void ConditionVariable_wait(WConditionVariable* self, WMutex* mutex){
    reinterpret_cast<ConditionVariable*>(self)->wait(reinterpret_cast<Mutex*>(mutex)); 
}
void ConditionVariable_signal(WConditionVariable* self, WMutex* mutex){
    reinterpret_cast<ConditionVariable*>(self)->signal(reinterpret_cast<Mutex*>(mutex)); 
}
void ConditionVariable_signalAll(WConditionVariable* self, WMutex* mutex){
    reinterpret_cast<ConditionVariable*>(self)->signalAll(reinterpret_cast<Mutex*>(mutex)); 
}
_Bool ConditionVariable_empty(WConditionVariable* self){
    return reinterpret_cast<ConditionVariable*>(self)->empty(); 
}
void ConditionVariable_destroy(WConditionVariable* self){
     delete reinterpret_cast<ConditionVariable*>(self); 
}
WConditionVariable* ConditionVariable_create(){
    return reinterpret_cast<WConditionVariable*>( new ConditionVariable()); 
}
WSemaphore* Semaphore_create(mword c){
    return reinterpret_cast<WSemaphore*>( new Semaphore(c)); 
}
_Bool Semaphore_P(WSemaphore* self){
    return reinterpret_cast<Semaphore*>(self)->P(); 
}
void Semaphore_V(WSemaphore* self){
    reinterpret_cast<Semaphore*>(self)->V(); 
}
void Semaphore_destroy(WSemaphore* self){
     delete reinterpret_cast<Semaphore*>(self); 
}
WuThread* uThread_create(WuThread* self, size_t ss, _Bool joinable){
    return reinterpret_cast<WuThread*>(uThread::create(ss, joinable)); 
}
WuThread* uThread_create_1(WuThread* self, _Bool joinable){
    return reinterpret_cast<WuThread*>(uThread::create(joinable)); 
}
void uThread_start(WuThread* self, WCluster* cluster, ptr_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3){
    reinterpret_cast<uThread*>(self)->start(reinterpret_cast<Cluster*>(cluster), func, arg1, arg2, arg3); 
}
void uThread_yield(WuThread* self){
    uThread::yield(); 
}
void uThread_terminate(WuThread* self){
    uThread::terminate(); 
}
void uThread_migrate(WuThread* self, WCluster* ){
    uThread::migrate(reinterpret_cast<Cluster*>()); 
}
void uThread_resume(WuThread* self){
    reinterpret_cast<uThread*>(self)->resume(); 
}
_Bool uThread_join(WuThread* self){
    return reinterpret_cast<uThread*>(self)->join(); 
}
void uThread_detach(WuThread* self){
    reinterpret_cast<uThread*>(self)->detach(); 
}
WCluster* uThread_getCurrentCluster(WuThread* self){
    return reinterpret_cast<WCluster*>(reinterpret_cast<uThread*>(self)->getCurrentCluster()); 
}
uint64_t uThread_getTotalNumberofUTs(WuThread* self){
    return uThread::getTotalNumberofUTs(); 
}
uint64_t uThread_getID(WuThread* self){
    return reinterpret_cast<uThread*>(self)->getID(); 
}
WuThread* uThread_currentUThread(WuThread* self){
    return reinterpret_cast<WuThread*>(uThread::currentUThread()); 
}
void uThread_destroy(WuThread* self){
     delete reinterpret_cast<uThread*>(self); 
}
WCluster* Cluster_create(){
    return reinterpret_cast<WCluster*>( new Cluster()); 
}
void Cluster_destroy(WCluster* self){
     delete reinterpret_cast<Cluster*>(self); 
}
WCluster* Cluster_getDefaultCluster(WCluster* self){
    return reinterpret_cast<WCluster*>(Cluster::getDefaultCluster()); 
}
uint64_t Cluster_getID(WCluster* self){
    return reinterpret_cast<Cluster*>(self)->getID(); 
}
size_t Cluster_getNumberOfkThreads(WCluster* self){
    return reinterpret_cast<Cluster*>(self)->getNumberOfkThreads(); 
}
WkThread* kThread_create(WCluster* ){
    return reinterpret_cast<WkThread*>( new kThread(reinterpret_cast<Cluster*>())); 
}
void kThread_destroy(WkThread* self){
     delete reinterpret_cast<kThread*>(self); 
}
std::class thread::native_handle_type kThread_getThreadNativeHandle(WkThread* self){
    return reinterpret_cast<kThread*>(self)->getThreadNativeHandle(); 
}
Wid* kThread_getID(WkThread* self){
    return &reinterpret_cast<Wid*>(reinterpret_cast<kThread*>(self)->getID()); 
}
WkThread* kThread_currentkThread(WkThread* self){
    return reinterpret_cast<WkThread*>(kThread::currentkThread()); 
}
uint kThread_getTotalNumberOfkThreads(WkThread* self){
    return kThread::getTotalNumberOfkThreads(); 
}
WConnection* Connection_create(){
    return reinterpret_cast<WConnection*>( new Connection()); 
}
WConnection* Connection_create_1(int fd){
    return reinterpret_cast<WConnection*>( new Connection(fd)); 
}
WConnection* Connection_create_2(int domainint typeint protocol){
    return reinterpret_cast<WConnection*>( new Connection(domaintypeprotocol)); 
}
void Connection_destroy(WConnection* self){
     delete reinterpret_cast<Connection*>(self); 
}
int Connection_accept(WConnection* self, WConnection* conn, sockaddr* addr, socklen_t * addrlen){
    return reinterpret_cast<Connection*>(self)->accept(reinterpret_cast<Connection*>(conn), addr, addrlen); 
}
WConnection* Connection_accept(WConnection* self, sockaddr* addr, socklen_t * addrlen){
    return reinterpret_cast<WConnection*>(reinterpret_cast<Connection*>(self)->accept(addr, addrlen)); 
}
int Connection_socket(WConnection* self, int domain, int type, int protocol){
    return reinterpret_cast<Connection*>(self)->socket(domain, type, protocol); 
}
int Connection_listen(WConnection* self, int backlog){
    return reinterpret_cast<Connection*>(self)->listen(backlog); 
}
int Connection_bind(WConnection* self, sockaddr* addr, socklen_t addrlen){
    return reinterpret_cast<Connection*>(self)->bind(addr, addrlen); 
}
int Connection_connect(WConnection* self, sockaddr* addr, socklen_t addrlen){
    return reinterpret_cast<Connection*>(self)->connect(addr, addrlen); 
}
ssize_t Connection_recv(WConnection* self, void * buf, size_t len, int flags){
    return reinterpret_cast<Connection*>(self)->recv(buf, len, flags); 
}
ssize_t Connection_recvfrom(WConnection* self, void * buf, size_t len, int flags, sockaddr* src_addr, socklen_t * addrlen){
    return reinterpret_cast<Connection*>(self)->recvfrom(buf, len, flags, src_addr, addrlen); 
}
ssize_t Connection_recvmsg(WConnection* self, int sockfd, msghdr* msg, int flags){
    return reinterpret_cast<Connection*>(self)->recvmsg(sockfd, msg, flags); 
}
int Connection_recvmmsg(WConnection* self, int sockfd, mmsghdr* msgvec, unsigned int vlen, unsigned int flags, timespec* timeout){
    return reinterpret_cast<Connection*>(self)->recvmmsg(sockfd, msgvec, vlen, flags, timeout); 
}
ssize_t Connection_send(WConnection* self, const void * buf, size_t len, int flags){
    return reinterpret_cast<Connection*>(self)->send(buf, len, flags); 
}
ssize_t Connection_sendto(WConnection* self, int sockfd, const void * buf, size_t len, int flags, sockaddr* dest_addr, socklen_t addrlen){
    return reinterpret_cast<Connection*>(self)->sendto(sockfd, buf, len, flags, dest_addr, addrlen); 
}
ssize_t Connection_sendmsg(WConnection* self, msghdr* msg, int flags){
    return reinterpret_cast<Connection*>(self)->sendmsg(msg, flags); 
}
int Connection_sendmmsg(WConnection* self, int sockfd, mmsghdr* msgvec, unsigned int vlen, unsigned int flags){
    return reinterpret_cast<Connection*>(self)->sendmmsg(sockfd, msgvec, vlen, flags); 
}
ssize_t Connection_read(WConnection* self, void * buf, size_t count){
    return reinterpret_cast<Connection*>(self)->read(buf, count); 
}
ssize_t Connection_write(WConnection* self, const void * buf, size_t count){
    return reinterpret_cast<Connection*>(self)->write(buf, count); 
}
void Connection_blockOnRead(WConnection* self){
    reinterpret_cast<Connection*>(self)->blockOnRead(); 
}
void Connection_blockOnWrite(WConnection* self){
    reinterpret_cast<Connection*>(self)->blockOnWrite(); 
}
int Connection_close(WConnection* self){
    return reinterpret_cast<Connection*>(self)->close(); 
}
int Connection_getFd(WConnection* self){
    return reinterpret_cast<Connection*>(self)->getFd(); 
}
#ifdef __cplusplus
}
#endif

