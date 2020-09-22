#define VERBOSE
#include "threadsupp.h"
#undef VERBOSE

#if defined _WIN32 && !defined FORCE_POSIX_THREADS

bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return wapi_assert(log, code_metric, !res);
}

Errno_t thread_init(struct thread *thread, thread_callback callback, void *args)
{
    struct thread res = (struct thread) { _beginthreadex(NULL, 0, callback, args, 0, NULL) };
    if (!res.thread || res.thread == (uintptr_t) INVALID_HANDLE_VALUE) return errno;
    *thread = res;
    return 0;
}

Errno_t thread_wait(struct thread *thread, thread_return *p_out)
{
    return !(WaitForSingleObject((HANDLE) thread->thread, INFINITE) && GetExitCodeThread((HANDLE) thread->thread, (LPDWORD) p_out));
}

Errno_t thread_close(struct thread *thread)
{
    return thread && !CloseHandle((HANDLE) thread->thread);
}

Errno_t mutex_init(struct mutex *mutex)
{
    return !InitializeCriticalSectionAndSpinCount(&mutex->mutex, 0x400);
}

Errno_t mutex_acquire(struct mutex *mutex)
{
    EnterCriticalSection(&mutex->mutex);
    return 0;
}

Errno_t mutex_release(struct mutex *mutex)
{
    LeaveCriticalSection(&mutex->mutex);
    return 0;
}

Errno_t mutex_close(struct mutex *mutex)
{
    if (mutex) DeleteCriticalSection(&mutex->mutex);
    return 0;
}

Errno_t condition_init(struct condition *condition)
{
    InitializeConditionVariable(&condition->condition);
    return 0;
}

Errno_t condition_signal(struct condition *condition)
{
    WakeConditionVariable(&condition->condition);
    return 0;
}

Errno_t condition_broadcast(struct condition *condition)
{
    WakeAllConditionVariable(&condition->condition);
    return 0;
}

Errno_t condition_sleep(struct condition *condition, struct mutex *mutex)
{
    return !SleepConditionVariableCS(&condition->condition, &mutex->mutex, INFINITE);
}

Errno_t condition_close(struct condition *condition)
{
    (void) condition;
    return 0;
}

Errno_t tls_init(struct tls *tls)
{
    struct tls res = (struct tls) { TlsAlloc() };
    if (res.tls == TLS_OUT_OF_INDEXES) return 1;
    *tls = res;
    return 0;
}

Errno_t tls_assign(struct tls *tls, void *ptr)
{
    return !TlsSetValue(tls->tls, ptr);
}

void *tls_fetch(struct tls *tls)
{
    return TlsGetValue(tls->tls);
}

Errno_t tls_close(struct tls *tls)
{
    return !TlsFree(tls->tls);
}

#elif defined __unix__ || defined __APPLE__ || (defined _WIN32 && defined FORCE_POSIX_THREADS)

bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return crt_assert_impl(log, code_metric, res);
}

Errno_t thread_init(struct thread *thread, thread_callback callback, void *args)
{
    return pthread_create(&thread->thread, NULL, callback, args);
}

Errno_t thread_wait(struct thread *thread, thread_return *p_out)
{
    return pthread_join(thread->thread, p_out);
}

Errno_t thread_close(struct thread *thread)
{
    (void) thread;
    return 0;
}

Errno_t mutex_init(struct mutex *mutex)
{
    return pthread_mutex_init(&mutex->mutex, NULL);
}

Errno_t mutex_acquire(struct mutex *mutex)
{
    return pthread_mutex_lock(&mutex->mutex);
}

Errno_t mutex_release(struct mutex *mutex)
{
    return pthread_mutex_unlock(&mutex->mutex);
}

Errno_t mutex_close(struct mutex *mutex)
{
    return mutex ? pthread_mutex_destroy(&mutex->mutex) : 0;
}

Errno_t condition_init(struct condition *condition)
{
    return pthread_cond_init(&condition->condition, NULL);
}

Errno_t condition_signal(struct condition *condition)
{
    return pthread_cond_signal(&condition->condition);
}

Errno_t condition_broadcast(struct condition *condition)
{
    return pthread_cond_broadcast(&condition->condition);
}

Errno_t condition_sleep(struct condition *condition, struct mutex *mutex)
{
    return pthread_cond_wait(&condition->condition, &mutex->mutex);
}

Errno_t condition_close(struct condition *condition)
{
    return condition ? pthread_cond_destroy(&condition->condition) : 0;
}

Errno_t tls_init(struct tls *tls)
{
    return pthread_key_create(&tls->tls, NULL);
}

Errno_t tls_assign(struct tls *tls, void *ptr)
{
    return pthread_setspecific(tls->tls, ptr);
}

void *tls_fetch(struct tls *tls)
{
    return pthread_getspecific(tls->tls);
}

Errno_t tls_close(struct tls *tls)
{
    return pthread_key_delete(tls->tls);
}

#endif
