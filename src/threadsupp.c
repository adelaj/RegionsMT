#define VERBOSE
#include "threadsupp.h"
#undef VERBOSE

#if (defined _WIN32 || defined _WIN64) && !defined FORCE_POSIX_THREADS

bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return wapi_assert(log, code_metric, !res);
}

Errno_t thread_init(struct thread *p_thread, thread_callback callback, void *args)
{
    struct thread res = (struct thread) { _beginthreadex(NULL, 0, callback, args, 0, NULL) };
    if (!res.thread || res.thread == (uintptr_t) INVALID_HANDLE_VALUE) return errno;
    *p_thread = res;
    return 0;
}

Errno_t thread_wait(struct thread *p_thread, thread_return *p_out)
{
    return !(WaitForSingleObject((HANDLE) p_thread->thread, INFINITE) && GetExitCodeThread((HANDLE) p_thread->thread, (LPDWORD) p_out));
}

Errno_t thread_close(struct thread *p_thread)
{
    return p_thread && !CloseHandle((HANDLE) p_thread->thread);
}

Errno_t mutex_init(struct mutex *p_mutex)
{
    return !InitializeCriticalSectionAndSpinCount(&p_mutex->mutex, 0x400);
}

Errno_t mutex_acquire(struct mutex *p_mutex)
{
    EnterCriticalSection(&p_mutex->mutex);
    return 0;
}

Errno_t mutex_release(struct mutex *p_mutex)
{
    LeaveCriticalSection(&p_mutex->mutex);
    return 0;
}

Errno_t mutex_close(struct mutex *p_mutex)
{
    if (p_mutex) DeleteCriticalSection(&p_mutex->mutex);
    return 0;
}

Errno_t condition_init(struct condition *p_condition)
{
    InitializeConditionVariable(&p_condition->condition);
    return 0;
}

Errno_t condition_signal(struct condition *p_condition)
{
    WakeConditionVariable(&p_condition->condition);
    return 0;
}

Errno_t condition_broadcast(struct condition *p_condition)
{
    WakeAllConditionVariable(&p_condition->condition);
    return 0;
}

Errno_t condition_sleep(struct condition *p_condition, struct mutex *p_mutex)
{
    return !SleepConditionVariableCS(&p_condition->condition, &p_mutex->mutex, INFINITE);
}

Errno_t condition_close(struct condition *p_condition)
{
    (void) p_condition;
    return 0;
}

Errno_t tls_init(struct tls *p_tls)
{
    struct tls res = (struct tls) { TlsAlloc() };
    if (res.tls == TLS_OUT_OF_INDEXES) return 1;
    *p_tls = res;
    return 0;
}

Errno_t tls_assign(struct tls *p_tls, void *ptr)
{
    return !TlsSetValue(p_tls->tls, ptr);
}

void *tls_fetch(struct tls *p_tls)
{
    return TlsGetValue(p_tls->tls);
}

Errno_t tls_close(struct tls *p_tls)
{
    return !TlsFree(p_tls->tls);
}

#elif defined __unix__ || defined __APPLE__ || ((defined _WIN32 || defined _WIN64) && defined FORCE_POSIX_THREADS)

bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return crt_assert_impl(log, code_metric, res);
}

Errno_t thread_init(struct thread *p_thread, thread_callback callback, void *args)
{
    return pthread_create(&p_thread->thread, NULL, callback, args);
}

Errno_t thread_wait(struct thread *p_thread, thread_return *p_out)
{
    return pthread_join(p_thread->thread, p_out);
}

Errno_t thread_close(struct thread *p_thread)
{
    (void) p_thread;
    return 0;
}

Errno_t mutex_init(struct mutex *p_mutex)
{
    return pthread_mutex_init(&p_mutex->mutex, NULL);
}

Errno_t mutex_acquire(struct mutex *p_mutex)
{
    return pthread_mutex_lock(&p_mutex->mutex);
}

Errno_t mutex_release(struct mutex *p_mutex)
{
    return pthread_mutex_unlock(&p_mutex->mutex);
}

Errno_t mutex_close(struct mutex *p_mutex)
{
    return p_mutex ? pthread_mutex_destroy(&p_mutex->mutex) : 0;
}

Errno_t condition_init(struct condition *p_condition)
{
    return pthread_cond_init(&p_condition->condition, NULL);
}

Errno_t condition_signal(struct condition *p_condition)
{
    return pthread_cond_signal(&p_condition->condition);
}

Errno_t condition_broadcast(struct condition *p_condition)
{
    return pthread_cond_broadcast(&p_condition->condition);
}

Errno_t condition_sleep(struct condition *p_condition, struct mutex *p_mutex)
{
    return pthread_cond_wait(&p_condition->condition, &p_mutex->mutex);
}

Errno_t condition_close(struct condition *p_condition)
{
    return p_condition ? pthread_cond_destroy(&p_condition->condition) : 0;
}

Errno_t tls_init(struct tls *p_tls)
{
    return pthread_key_create(&p_tls->tls, NULL);
}

Errno_t tls_assign(struct tls *p_tls, void *ptr)
{
    return pthread_setspecific(p_tls->tls, ptr);
}

void *tls_fetch(struct tls *p_tls)
{
    return pthread_getspecific(p_tls->tls);
}

Errno_t tls_close(struct tls *p_tls)
{
    return pthread_key_delete(p_tls->tls);
}

#endif
