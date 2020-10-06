#define VERBOSE
#include "threadsupp.h"
#undef VERBOSE

IF_PTHREAD(bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return crt_assert_impl(log, code_metric, res);
})

IFN_PTHREAD(bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return wapi_assert(log, code_metric, !res);
})

IF_PTHREAD(Errno_t thread_init(struct thread *thread, thread_callback callback, void *args)
{
    return pthread_create(&thread->thread, NULL, callback, args);
})

IFN_PTHREAD(Errno_t thread_init(struct thread *thread, thread_callback callback, void *args)
{
    struct thread res = (struct thread) { _beginthreadex(NULL, 0, callback, args, 0, NULL) };
    if (!res.thread || res.thread == (uintptr_t) INVALID_HANDLE_VALUE) return errno;
    *thread = res;
    return 0;
})

IF_PTHREAD(Errno_t thread_wait(struct thread *thread, thread_return *p_out)
{
    return pthread_join(thread->thread, p_out);
})

IFN_PTHREAD(Errno_t thread_wait(struct thread *thread, thread_return *p_out)
{
    return !(WaitForSingleObject((HANDLE) thread->thread, INFINITE) && GetExitCodeThread((HANDLE) thread->thread, (LPDWORD) p_out));
})

IF_PTHREAD(Errno_t thread_close(struct thread *thread)
{
    (void) thread;
    return 0;
})

IFN_PTHREAD(Errno_t thread_close(struct thread *thread)
{
    return thread && !CloseHandle((HANDLE) thread->thread);
})

IF_PTHREAD(Errno_t mutex_init(struct mutex *mutex)
{
    return pthread_mutex_init(&mutex->mutex, NULL);
})

IFN_PTHREAD(Errno_t mutex_init(struct mutex *mutex)
{
    return !InitializeCriticalSectionAndSpinCount(&mutex->mutex, 0x400);
})

IF_PTHREAD(Errno_t mutex_acquire(struct mutex *mutex)
{
    return pthread_mutex_lock(&mutex->mutex);
})

IFN_PTHREAD(Errno_t mutex_acquire(struct mutex *mutex)
{
    EnterCriticalSection(&mutex->mutex);
    return 0;
})

IF_PTHREAD(Errno_t mutex_release(struct mutex *mutex)
{
    return pthread_mutex_unlock(&mutex->mutex);
})

IFN_PTHREAD(Errno_t mutex_release(struct mutex *mutex)
{
    LeaveCriticalSection(&mutex->mutex);
    return 0;
})

IF_PTHREAD(Errno_t mutex_close(struct mutex *mutex)
{
    return mutex ? pthread_mutex_destroy(&mutex->mutex) : 0;
})

IFN_PTHREAD(Errno_t mutex_close(struct mutex *mutex)
{
    if (mutex) DeleteCriticalSection(&mutex->mutex);
    return 0;
})

IF_PTHREAD(Errno_t condition_init(struct condition *condition)
{
    return pthread_cond_init(&condition->condition, NULL);
})

IFN_PTHREAD(Errno_t condition_init(struct condition *condition)
{
    InitializeConditionVariable(&condition->condition);
    return 0;
})

IF_PTHREAD(Errno_t condition_signal(struct condition *condition)
{
    return pthread_cond_signal(&condition->condition);
})

IFN_PTHREAD(Errno_t condition_signal(struct condition *condition)
{
    WakeConditionVariable(&condition->condition);
    return 0;
})

IF_PTHREAD(Errno_t condition_broadcast(struct condition *condition)
{
    return pthread_cond_broadcast(&condition->condition);
})

IFN_PTHREAD(Errno_t condition_broadcast(struct condition *condition)
{
    WakeAllConditionVariable(&condition->condition);
    return 0;
})

IF_PTHREAD(Errno_t condition_sleep(struct condition *condition, struct mutex *mutex)
{
    return pthread_cond_wait(&condition->condition, &mutex->mutex);
})

IFN_PTHREAD(Errno_t condition_sleep(struct condition *condition, struct mutex *mutex)
{
    return !SleepConditionVariableCS(&condition->condition, &mutex->mutex, INFINITE);
})

IF_PTHREAD(Errno_t condition_close(struct condition *condition)
{
    return condition ? pthread_cond_destroy(&condition->condition) : 0;
})

IFN_PTHREAD(Errno_t condition_close(struct condition *condition)
{
    (void) condition;
    return 0;
})

IF_PTHREAD(Errno_t tls_init(struct tls *tls)
{
    return pthread_key_create(&tls->tls, NULL);
})

IFN_PTHREAD(Errno_t tls_init(struct tls *tls)
{
    struct tls res = (struct tls){ TlsAlloc() };
    if (res.tls == TLS_OUT_OF_INDEXES) return 1;
    *tls = res;
    return 0;
})

IF_PTHREAD(Errno_t tls_assign(struct tls *tls, void *ptr)
{
    return pthread_setspecific(tls->tls, ptr);
})

IFN_PTHREAD(Errno_t tls_assign(struct tls *tls, void *ptr)
{
    return !TlsSetValue(tls->tls, ptr);
})

IF_PTHREAD(void *tls_fetch(struct tls *tls)
{
    return pthread_getspecific(tls->tls);
})

IFN_PTHREAD(void *tls_fetch(struct tls *tls)
{
    return TlsGetValue(tls->tls);
})

IF_PTHREAD(Errno_t tls_close(struct tls *tls)
{
    return tls ? pthread_key_delete(tls->tls) : 0;
})

IFN_PTHREAD(Errno_t tls_close(struct tls *tls)
{
    return tls && !TlsFree(tls->tls);
})
