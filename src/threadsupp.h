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

#if defined _WIN32 && !defined FORCE_POSIX_THREADS

#define thread_callback_convention __stdcall
typedef unsigned thread_return;
typedef thread_return(thread_callback_convention *thread_callback)(void *);

#   ifdef VERBOSE
#       include <windows.h>
#       include <process.h>
struct thread { uintptr_t thread; };
struct mutex { CRITICAL_SECTION mutex; };
struct condition { CONDITION_VARIABLE condition; };
struct tls { DWORD tls; };
#   endif

#elif defined __unix__ || defined __APPLE__ || (defined _WIN32 && defined FORCE_POSIX_THREADS)

#define thread_callback_convention
typedef void *thread_return;
typedef thread_return (thread_callback_convention *thread_callback)(void *);

#   ifdef VERBOSE
#       include <pthread.h>
struct thread { pthread_t  thread; };
struct mutex { pthread_mutex_t mutex; };
struct condition { pthread_cond_t condition; };
struct tls { pthread_key_t tls; };
#   endif

#endif

#ifndef VERBOSE
struct thread;
struct mutex;
struct condition;
struct tls;
#endif

// To print an error status of the listed functions, except of 'thread_init', 'thread_assert' should be used
bool thread_assert(struct log *, struct code_metric, Errno_t);

Errno_t thread_init(struct thread *, thread_callback, void *); // Use 'crt_assert_impl' to get an error status 
Errno_t thread_wait(struct thread *, thread_return *);
Errno_t thread_close(struct thread *);

Errno_t mutex_init(struct mutex *);
Errno_t mutex_acquire(struct mutex *);
Errno_t mutex_release(struct mutex *);
Errno_t mutex_close(struct mutex *);

Errno_t condition_init(struct condition *);
Errno_t condition_signal(struct condition *);
Errno_t condition_broadcast(struct condition *);
Errno_t condition_sleep(struct condition *, struct mutex *);
Errno_t condition_close(struct condition *);

Errno_t tls_init(struct tls *);
Errno_t tls_assign(struct tls *, void *);
void *tls_fetch(struct tls *);
Errno_t tls_close(struct tls *);
