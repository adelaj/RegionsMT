#pragma once

////////////////////////////////////////////////////////////////////////////////
//
//  Invariant wrappers for general thread operations and basic synchronization 
//  primitives
//

// Uncomment this to enable POSIX threads on Windows:
// #define FORCE_POSIX_THREADS

#include <stdbool.h>
#include <stdint.h>

#include "np.h"
#include "log.h"

#if (defined _WIN32 || defined _WIN64) && !defined FORCE_POSIX_THREADS

#   include <windows.h>
#   include <process.h>

#define thread_callback_convention __stdcall
typedef _beginthreadex_proc_type thread_callback;
typedef uintptr_t thread_handle;
typedef unsigned thread_return;
typedef CRITICAL_SECTION mutex_handle;
typedef CONDITION_VARIABLE condition_handle;
typedef DWORD tls_handle;

#elif defined __unix__ || defined __APPLE__ || ((defined _WIN32 || defined _WIN64) && defined FORCE_POSIX_THREADS)

#   include <pthread.h>

#define thread_callback_convention
typedef void *thread_return;
typedef thread_return (thread_callback_convention *thread_callback)(void *);
typedef pthread_t thread_handle;
typedef pthread_mutex_t mutex_handle;
typedef pthread_cond_t condition_handle;
typedef pthread_key_t tls_handle;

#endif

// To print an error status of the listed functions, except of 'thread_init', 'thread_assert' should be used
bool thread_assert(struct log *, struct code_metric, bool);

bool thread_init(thread_handle *, thread_callback, void *); // Use 'crt_assert' to get an error status 
//void thread_terminate(thread_handle *);
bool thread_wait(thread_handle *, thread_return *);
void thread_close(thread_handle *);

bool mutex_init(mutex_handle *);
void mutex_acquire(mutex_handle *);
void mutex_release(mutex_handle *);
void mutex_close(mutex_handle *);

bool condition_init(condition_handle *);
void condition_signal(condition_handle *);
void condition_broadcast(condition_handle *);
void condition_sleep(condition_handle *, mutex_handle *);
void condition_close(condition_handle *);

bool tls_init(tls_handle *);
void tls_assign(tls_handle *, void *);
void *tls_fetch(tls_handle *);
bool tls_close(tls_handle *);
