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

#include "common.h"
#include "np.h"
#include "log.h"

#if defined FORCE_PTHREAD || TEST(IF_UNIX_APPLE)
#   define IF_PTHREAD(...) __VA_ARGS__
#   define IFN_PTHREAD(...)
#else
#   define IF_PTHREAD(...)
#   define IFN_PTHREAD(...) __VA_ARGS__
#endif

#define thread_callback_conv IF_WIN(__stdcall)

typedef IF_PTHREAD(void *) IFN_PTHREAD(unsigned) thread_return;
typedef thread_return (thread_callback_conv *thread_callback)(void *);

#if TEST(IF_VERBOSE)
#   if TEST(IF_PTHREAD)
#       include <pthread.h>
#   else
#       include <windows.h>
#       include <process.h>
#   endif
#endif

struct thread IF_VERBOSE({ IF_PTHREAD(pthread_t) IFN_PTHREAD(uintptr_t) thread; });
struct mutex IF_VERBOSE({ IF_PTHREAD(pthread_mutex_t) IFN_PTHREAD(CRITICAL_SECTION) mutex; });
struct condition IF_VERBOSE({ IF_PTHREAD(pthread_cond_t) IFN_PTHREAD(CONDITION_VARIABLE) condition; });
struct tls IF_VERBOSE({ IF_PTHREAD(pthread_key_t) IFN_PTHREAD(DWORD) tls; });

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
