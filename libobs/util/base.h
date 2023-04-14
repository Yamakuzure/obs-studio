/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <stdarg.h>

#include "c99defs.h"

/*
 * Just contains logging/crash related stuff
 */

#ifdef __cplusplus
extern "C" {
#endif

#define STRINGIFY(x) #x
#define STRINGIFY_(x) STRINGIFY(x)
#define S__LINE__ STRINGIFY_(__LINE__)

#define INT_CUR_LINE __LINE__
#define FILE_LINE __FILE__ " (" S__LINE__ "): "

#define OBS_COUNTOF(x) (sizeof(x) / sizeof(x[0]))

enum {
	/**
	 * Use if there's a problem that can potentially affect the program,
	 * but isn't enough to require termination of the program.
	 *
	 * Use in creation functions and core subsystem functions.  Places that
	 * should definitely not fail.
	 */
	LOG_ERROR = 100,

	/**
	 * Use if a problem occurs that doesn't affect the program and is
	 * recoverable.
	 *
	 * Use in places where failure isn't entirely unexpected, and can
	 * be handled safely.
	 */
	LOG_WARNING = 200,

	/**
	 * Informative message to be displayed in the log.
	 */
	LOG_INFO = 300,

	/**
	 * Debug message to be used mostly by developers.
	 */
	LOG_DEBUG = 400
};

typedef void (*log_handler_t)(int lvl, const char *msg, va_list args, void *p);

EXPORT void base_get_log_handler(log_handler_t *handler, void **param);
EXPORT void base_set_log_handler(log_handler_t handler, void *param);

EXPORT void base_set_crash_handler(void (*handler)(const char *, va_list,
						   void *),
				   void *param);

EXPORT void blogva(int log_level, const char *format, va_list args);

#if !defined(_MSC_VER) && !defined(SWIG)
#define PRINTFATTR(f, a) __attribute__((__format__(__printf__, f, a)))
#else
#define PRINTFATTR(f, a)
#endif

PRINTFATTR(2, 3)
EXPORT void blog_internal(int log_level, const char *format, ...);

#if defined(_DEBUG)
EXPORT char const *obs_internal_location_info(char const *path, size_t line,
					      char const *func);
#define blog(L_LEVEL_, FMT_, ...)                                    \
	blog_internal(L_LEVEL_, "%s: " FMT_,                         \
		      obs_internal_location_info(__FILE__, __LINE__, \
						 __func__),          \
		      ##__VA_ARGS__)
#define debug_log(FMT_, ...) blog(LOG_DEBUG, "[debug] " FMT_, ##__VA_ARGS__)
//--- let's make mutex handling transparent ---
#if defined(PTHREAD_H) || defined(_PTHREAD_H)
#if defined(PTHREAD_H)
#define CURRENT_THREAD_ID ((size_t)(pthread_self().p))
#else
#define CURRENT_THREAD_ID ((size_t)(pthread_self()))
#endif // PTHREAD_H vs _PTHREAD_H
#if defined(ENABLE_MUTEX_LOGGING) && !defined(NO_MUTEX_DEBUG_LOGGING)
// === Without GNU extensions we need a more complex path...
#define debug_log_there(FILE_, LINE_, FUNC_, FMT_, ...)                \
	blog_internal(LOG_DEBUG, "%s: [debug] " FMT_,                  \
		      obs_internal_location_info(FILE_, LINE_, FUNC_), \
		      ##__VA_ARGS__)
static int pthread_mutex_init_debug_(char const *path, size_t line,
				     char const *func, char const *what,
				     pthread_mutex_t *mutex,
				     const pthread_mutexattr_t *attr)
{
	debug_log_there(path, line, func, "[MUTEX] Initialize '%s': %zu", what,
			CURRENT_THREAD_ID);
	return pthread_mutex_init(mutex, attr);
}
#define pthread_mutex_init(MUTEX_, MUTEXATTR_)                           \
	pthread_mutex_init_debug_(__FILE__, __LINE__, __func__, #MUTEX_, \
				  MUTEX_, MUTEXATTR_)
static int pthread_mutex_destroy_debug_(char const *path, size_t line,
					char const *func, char const *what,
					pthread_mutex_t *mutex)
{
	debug_log_there(path, line, func, "[MUTEX] Destroy '%s': %zu", what,
			CURRENT_THREAD_ID);
	return pthread_mutex_destroy(mutex);
}
#define pthread_mutex_destroy(MUTEX_)                                       \
	pthread_mutex_destroy_debug_(__FILE__, __LINE__, __func__, #MUTEX_, \
				     MUTEX_)
static int pthread_mutex_trylock_debug_(char const *path, size_t line,
					char const *func, char const *what,
					pthread_mutex_t *mutex)
{
	debug_log_there(path, line, func, "[MUTEX] Try To Lock '%s': %zu", what,
			CURRENT_THREAD_ID);
	return pthread_mutex_trylock(mutex);
}
#define pthread_mutex_trylock(MUTEX_)                                       \
	pthread_mutex_trylock_debug_(__FILE__, __LINE__, __func__, #MUTEX_, \
				     MUTEX_)
static int pthread_mutex_lock_debug_(char const *path, size_t line,
				     char const *func, char const *what,
				     pthread_mutex_t *mutex)
{
	debug_log_there(path, line, func, "[MUTEX] Lock '%s': %zu", what,
			CURRENT_THREAD_ID);
	return pthread_mutex_lock(mutex);
}
#define pthread_mutex_lock(MUTEX_) \
	pthread_mutex_lock_debug_(__FILE__, __LINE__, __func__, #MUTEX_, MUTEX_)
static int pthread_mutex_unlock_debug_(char const *path, size_t line,
				       char const *func, char const *what,
				       pthread_mutex_t *mutex)
{
	debug_log_there(path, line, func, "[MUTEX] Unlock '%s': %zu", what,
			CURRENT_THREAD_ID);
	return pthread_mutex_unlock(mutex);
}
#define pthread_mutex_unlock(MUTEX_)                                       \
	pthread_mutex_unlock_debug_(__FILE__, __LINE__, __func__, #MUTEX_, \
				    MUTEX_)
#endif // ENABLE_MUTEX_LOGGING && !NO_MUTEX_DEBUG_LOGGING
#endif // PTHREAD_H || _PTHREAD_H
//---------------------------------------------
#else
#define blog(L_LEVEL_, FMT_, ...) blog_internal(L_LEVEL_, FMT_, ##__VA_ARGS__)
#define debug_log(FMT_, ...) \
	do {                 \
	} while (0)
#endif

PRINTFATTR(1, 2)
#ifndef SWIG
OBS_NORETURN
#endif
EXPORT void bcrash(const char *format, ...);

#undef PRINTFATTR

#ifdef __cplusplus
}
#endif
