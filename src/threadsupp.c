#include "threadsupp.h"

#if (defined _WIN32 || defined _WIN64) && !defined FORCE_POSIX_THREADS

bool thread_assert(struct log *log, struct code_metric code_metric, bool res)
{
    return wapi_assert(log, code_metric, res);
}

bool thread_init(thread_handle *p_thread, thread_callback callback, void *args)
{
    thread_handle res = _beginthreadex(NULL, 0, callback, args, 0, NULL);
    if (!res || res == (thread_handle) INVALID_HANDLE_VALUE) return 0;
    *p_thread = res;
    return 1;
}

/*void thread_terminate(thread_handle *p_thread)
{
    TerminateThread(*p_thread, 0);
}*/

bool thread_wait(thread_handle *p_thread, thread_return *p_out)
{
    return WaitForSingleObject((HANDLE) *p_thread, INFINITE) && GetExitCodeThread((HANDLE) *p_thread, (LPDWORD) p_out);
}

void thread_close(thread_handle *p_thread)
{
    if (p_thread) CloseHandle((HANDLE) *p_thread);
}

bool mutex_init(mutex_handle *p_mutex)
{
    return InitializeCriticalSectionAndSpinCount(p_mutex, 16);
}

void mutex_acquire(mutex_handle *p_mutex)
{
    EnterCriticalSection(p_mutex);
}

void mutex_release(mutex_handle *p_mutex)
{
    LeaveCriticalSection(p_mutex);
}

void mutex_close(mutex_handle *p_mutex)
{
    if (p_mutex) DeleteCriticalSection(p_mutex);
}

bool condition_init(condition_handle *p_condition)
{
    InitializeConditionVariable(p_condition);
    return 1;
}

void condition_signal(condition_handle *p_condition)
{
    WakeConditionVariable(p_condition);
}

void condition_broadcast(condition_handle *p_condition)
{
    WakeAllConditionVariable(p_condition);
}

void condition_sleep(condition_handle *p_condition, mutex_handle *pmutex)
{
    SleepConditionVariableCS(p_condition, pmutex, INFINITE);
}

void condition_close(condition_handle *p_condition)
{
    (void) p_condition;
}

bool tls_init(tls_handle *p_tls)
{
    tls_handle res = TlsAlloc();
    if (res == TLS_OUT_OF_INDEXES) return 0;
    *p_tls = res;
    return 1;
}

void tls_assign(tls_handle *p_tls, void *ptr)
{
    TlsSetValue(*p_tls, ptr);
}

void *tls_fetch(tls_handle *p_tls)
{
    return TlsGetValue(*p_tls);
}

bool tls_close(tls_handle *p_tls)
{
    return TlsFree(*p_tls);
}

#elif defined __unix__ || defined __APPLE__ || ((defined _WIN32 || defined _WIN64) && defined FORCE_POSIX_THREADS)

bool thread_assert(struct log *log, struct code_metric code_metric, bool res)
{
    return crt_assert(log, code_metric, res);
}

bool thread_init(thread_handle *p_thread, thread_callback callback, void *args)
{
    return !pthread_create(p_thread, NULL, callback, args);
}

/*void thread_terminate(thread_handle *p_thread)
{
    pthread_cancel(*p_thread);
    pthread_join(*p_thread, NULL);
}*/

bool thread_wait(thread_handle *p_thread, thread_return *p_out)
{
    return !pthread_join(*p_thread, p_out);
}

void thread_close(thread_handle *p_thread)
{
    (void) p_thread;
}

bool mutex_init(mutex_handle *p_mutex)
{
    return !pthread_mutex_init(p_mutex, NULL);
}

void mutex_acquire(mutex_handle *p_mutex)
{
    pthread_mutex_lock(p_mutex);
}

void mutex_release(mutex_handle *p_mutex)
{
    pthread_mutex_unlock(p_mutex);
}

void mutex_close(mutex_handle *p_mutex)
{
    if (p_mutex) (void) pthread_mutex_destroy(p_mutex);
}

bool condition_init(condition_handle *p_condition)
{
    return !pthread_cond_init(p_condition, NULL);
}

void condition_signal(condition_handle *p_condition)
{
    pthread_cond_signal(p_condition);
}

void condition_broadcast(condition_handle *p_condition)
{
    pthread_cond_broadcast(p_condition);
}

void condition_sleep(condition_handle *p_condition, mutex_handle *pmutex)
{
    pthread_cond_wait(p_condition, pmutex);
}

void condition_close(condition_handle *p_condition)
{
    if (p_condition) (void) pthread_cond_destroy(p_condition);
}

bool tls_init(tls_handle *p_tls)
{
    return !pthread_key_create(p_tls, NULL);
}

void tls_assign(tls_handle *p_tls, void *ptr)
{
    pthread_setspecific(*p_tls, ptr);
}

void *tls_fetch(tls_handle *p_tls)
{
    return pthread_getspecific(*p_tls);
}

void tls_close(tls_handle *p_tls)
{
    pthread_key_delete(*p_tls);
}

#endif
