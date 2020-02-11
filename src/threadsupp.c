#include "threadsupp.h"

#if (defined _WIN32 || defined _WIN64) && !defined FORCE_POSIX_THREADS

bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return wapi_assert(log, code_metric, !res);
}

Errno_t thread_init(thread_handle *p_thread, thread_callback callback, void *args)
{
    thread_handle res = _beginthreadex(NULL, 0, callback, args, 0, NULL);
    if (!res || res == (thread_handle) INVALID_HANDLE_VALUE) return errno;
    *p_thread = res;
    return 0;
}

Errno_t thread_wait(thread_handle *p_thread, thread_return *p_out)
{
    return !(WaitForSingleObject((HANDLE) *p_thread, INFINITE) && GetExitCodeThread((HANDLE) *p_thread, (LPDWORD) p_out));
}

Errno_t thread_close(thread_handle *p_thread)
{
    return p_thread && !CloseHandle((HANDLE) *p_thread);
}

Errno_t mutex_init(mutex_handle *p_mutex)
{
    return !InitializeCriticalSectionAndSpinCount(p_mutex, 0x400);
}

Errno_t mutex_acquire(mutex_handle *p_mutex)
{
    EnterCriticalSection(p_mutex);
    return 0;
}

Errno_t mutex_release(mutex_handle *p_mutex)
{
    LeaveCriticalSection(p_mutex);
    return 0;
}

Errno_t mutex_close(mutex_handle *p_mutex)
{
    if (p_mutex) DeleteCriticalSection(p_mutex);
    return 0;
}

Errno_t condition_init(condition_handle *p_condition)
{
    InitializeConditionVariable(p_condition);
    return 0;
}

Errno_t condition_signal(condition_handle *p_condition)
{
    WakeConditionVariable(p_condition);
    return 0;
}

Errno_t condition_broadcast(condition_handle *p_condition)
{
    WakeAllConditionVariable(p_condition);
    return 0;
}

Errno_t condition_sleep(condition_handle *p_condition, mutex_handle *pmutex)
{
    return !SleepConditionVariableCS(p_condition, pmutex, INFINITE);
}

Errno_t condition_close(condition_handle *p_condition)
{
    (void) p_condition;
    return 0;
}

Errno_t tls_init(tls_handle *p_tls)
{
    tls_handle res = TlsAlloc();
    if (res == TLS_OUT_OF_INDEXES) return 1;
    *p_tls = res;
    return 0;
}

Errno_t tls_assign(tls_handle *p_tls, void *ptr)
{
    return !TlsSetValue(*p_tls, ptr);
}

void *tls_fetch(tls_handle *p_tls)
{
    return TlsGetValue(*p_tls);
}

Errno_t tls_close(tls_handle *p_tls)
{
    return !TlsFree(*p_tls);
}

#elif defined __unix__ || defined __APPLE__ || ((defined _WIN32 || defined _WIN64) && defined FORCE_POSIX_THREADS)

bool thread_assert(struct log *log, struct code_metric code_metric, Errno_t res)
{
    return crt_assert_impl(log, code_metric, res);
}

Errno_t thread_init(thread_handle *p_thread, thread_callback callback, void *args)
{
    return pthread_create(p_thread, NULL, callback, args);
}

Errno_t thread_wait(thread_handle *p_thread, thread_return *p_out)
{
    return pthread_join(*p_thread, p_out);
}

Errno_t thread_close(thread_handle *p_thread)
{
    (void) p_thread;
    return 0;
}

Errno_t mutex_init(mutex_handle *p_mutex)
{
    return pthread_mutex_init(p_mutex, NULL);
}

Errno_t mutex_acquire(mutex_handle *p_mutex)
{
    return pthread_mutex_lock(p_mutex);
}

Errno_t mutex_release(mutex_handle *p_mutex)
{
    return pthread_mutex_unlock(p_mutex);
}

Errno_t mutex_close(mutex_handle *p_mutex)
{
    return p_mutex ? pthread_mutex_destroy(p_mutex) : 0;
}

Errno_t condition_init(condition_handle *p_condition)
{
    return pthread_cond_init(p_condition, NULL);
}

Errno_t condition_signal(condition_handle *p_condition)
{
    return pthread_cond_signal(p_condition);
}

Errno_t condition_broadcast(condition_handle *p_condition)
{
    return pthread_cond_broadcast(p_condition);
}

Errno_t condition_sleep(condition_handle *p_condition, mutex_handle *pmutex)
{
    return pthread_cond_wait(p_condition, pmutex);
}

Errno_t condition_close(condition_handle *p_condition)
{
    return p_condition ? pthread_cond_destroy(p_condition) : 0;
}

Errno_t tls_init(tls_handle *p_tls)
{
    return pthread_key_create(p_tls, NULL);
}

Errno_t tls_assign(tls_handle *p_tls, void *ptr)
{
    return pthread_setspecific(*p_tls, ptr);
}

void *tls_fetch(tls_handle *p_tls)
{
    return pthread_getspecific(*p_tls);
}

Errno_t tls_close(tls_handle *p_tls)
{
    return pthread_key_delete(*p_tls);
}

#endif
