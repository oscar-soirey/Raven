/**
 * Raven - core/thread.h
 * Portable threads, locks, condition variables, atomics and clock.
 * Implemented by thread_posix.c and thread_win32.c.
 */

#ifndef RVN_THREAD_H
#define RVN_THREAD_H

#include "base.h"

#if defined(_WIN32)
	/* SRWLOCK and CONDITION_VARIABLE are both a single pointer: keeping them
	 * opaque avoids including <windows.h> from every source file. */
	typedef struct rvn_mutex_t { void* handle; } rvn_mutex_t;
	typedef struct rvn_rwlock_t { void* handle; } rvn_rwlock_t;
	typedef struct rvn_cond_t { void* handle; } rvn_cond_t;
	typedef struct rvn_thread_t { void* handle; } rvn_thread_t;
#else
	#include <pthread.h>
	typedef struct rvn_mutex_t { pthread_mutex_t handle; } rvn_mutex_t;
	typedef struct rvn_rwlock_t { pthread_rwlock_t handle; } rvn_rwlock_t;
	typedef struct rvn_cond_t { pthread_cond_t handle; } rvn_cond_t;
	typedef struct rvn_thread_t { pthread_t handle; } rvn_thread_t;
#endif

typedef void (*rvn_thread_fn)(void* arg);

int      rvn_thread_create(rvn_thread_t* thread, rvn_thread_fn function, void* arg, const char* name);
void     rvn_thread_join(rvn_thread_t* thread);
/** Identifier of the calling thread, never 0. */
uint64_t rvn_thread_id(void);
int      rvn_cpu_count(void);
void     rvn_sleep_ms(int milliseconds);
/** Monotonic clock, in seconds. */
double   rvn_clock_seconds(void);

void rvn_mutex_init(rvn_mutex_t* mutex);
void rvn_mutex_destroy(rvn_mutex_t* mutex);
void rvn_mutex_lock(rvn_mutex_t* mutex);
void rvn_mutex_unlock(rvn_mutex_t* mutex);

void rvn_rwlock_init(rvn_rwlock_t* lock);
void rvn_rwlock_destroy(rvn_rwlock_t* lock);
void rvn_rwlock_read_lock(rvn_rwlock_t* lock);
void rvn_rwlock_read_unlock(rvn_rwlock_t* lock);
void rvn_rwlock_write_lock(rvn_rwlock_t* lock);
void rvn_rwlock_write_unlock(rvn_rwlock_t* lock);

void rvn_cond_init(rvn_cond_t* cond);
void rvn_cond_destroy(rvn_cond_t* cond);
void rvn_cond_wait(rvn_cond_t* cond, rvn_mutex_t* mutex);
/** Returns R_FALSE when the wait timed out. */
int  rvn_cond_wait_timeout(rvn_cond_t* cond, rvn_mutex_t* mutex, double seconds);
void rvn_cond_signal(rvn_cond_t* cond);
void rvn_cond_broadcast(rvn_cond_t* cond);

/* ---------------------------------------------------------------------------
 *  Atomic 32-bit integer
 * ------------------------------------------------------------------------- */

#if defined(_MSC_VER) && !defined(__clang__)
	#include <intrin.h>
	typedef struct rvn_atomic_t { volatile long value; } rvn_atomic_t;
	RVN_INLINE int rvn_atomic_load(rvn_atomic_t* a) { return (int)_InterlockedCompareExchange(&a->value, 0, 0); }
	RVN_INLINE void rvn_atomic_store(rvn_atomic_t* a, int v) { _InterlockedExchange(&a->value, (long)v); }
	/** Adds and returns the new value. */
	RVN_INLINE int rvn_atomic_add(rvn_atomic_t* a, int v) { return (int)_InterlockedExchangeAdd(&a->value, (long)v) + v; }
	RVN_INLINE int rvn_atomic_exchange(rvn_atomic_t* a, int v) { return (int)_InterlockedExchange(&a->value, (long)v); }
	/** Raises the value to v when it is smaller. */
	RVN_INLINE void rvn_atomic_max(rvn_atomic_t* a, int v) {
		long seen = _InterlockedCompareExchange(&a->value, 0, 0);
		while (seen < (long)v) {
			long before = _InterlockedCompareExchange(&a->value, (long)v, seen);
			if (before == seen) break;
			seen = before;
		}
	}
	/** Atomic pointer. */
	typedef struct rvn_atomic_ptr_t { void* volatile value; } rvn_atomic_ptr_t;
	RVN_INLINE void* rvn_atomic_ptr_load(rvn_atomic_ptr_t* a) { return _InterlockedCompareExchangePointer(&a->value, NULL, NULL); }
	RVN_INLINE void rvn_atomic_ptr_store(rvn_atomic_ptr_t* a, void* v) { _InterlockedExchangePointer(&a->value, v); }
#else
	#include <stdatomic.h>
	typedef struct rvn_atomic_t { atomic_int value; } rvn_atomic_t;
	RVN_INLINE int rvn_atomic_load(rvn_atomic_t* a) { return atomic_load(&a->value); }
	RVN_INLINE void rvn_atomic_store(rvn_atomic_t* a, int v) { atomic_store(&a->value, v); }
	/** Adds and returns the new value. */
	RVN_INLINE int rvn_atomic_add(rvn_atomic_t* a, int v) { return atomic_fetch_add(&a->value, v) + v; }
	RVN_INLINE int rvn_atomic_exchange(rvn_atomic_t* a, int v) { return atomic_exchange(&a->value, v); }
	/** Raises the value to v when it is smaller. */
	RVN_INLINE void rvn_atomic_max(rvn_atomic_t* a, int v) {
		int seen = atomic_load(&a->value);
		while (seen < v && !atomic_compare_exchange_weak(&a->value, &seen, v)) {}
	}
	/** Atomic pointer. */
	typedef struct rvn_atomic_ptr_t { _Atomic(void*) value; } rvn_atomic_ptr_t;
	RVN_INLINE void* rvn_atomic_ptr_load(rvn_atomic_ptr_t* a) { return atomic_load(&a->value); }
	RVN_INLINE void rvn_atomic_ptr_store(rvn_atomic_ptr_t* a, void* v) { atomic_store(&a->value, v); }
#endif

#endif /* RVN_THREAD_H */
