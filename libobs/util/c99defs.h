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

/*
 * Contains hacks for getting some C99 stuff working in VC, things like
 * bool, stdint
 */

#define UNUSED_PARAMETER(param) (void)param

#ifdef _MSC_VER
#define _OBS_DEPRECATED __declspec(deprecated)
#define OBS_NORETURN __declspec(noreturn)
#define FORCE_INLINE __forceinline
#else
#define _OBS_DEPRECATED __attribute__((deprecated))
#define OBS_NORETURN __attribute__((noreturn))
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

#if defined(__cplusplus) && (__cplusplus >= 201703L)
#define WARN_UNUSED_RESULT [[nodiscard]]
#elif defined(_MSC_VER)
#if defined(_Check_return_)
#define WARN_UNUSED_RESULT _Check_return_ /* SAL */
#else
#define WARN_UNUSED_RESULT
#endif // _Check_return_
#else
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#endif // __cplusplus && C++17 and later

#if defined(SWIG_TYPE_TABLE)
#define OBS_DEPRECATED
#else
#define OBS_DEPRECATED _OBS_DEPRECATED
#endif

#if defined(IS_LIBOBS)
#define OBS_EXTERNAL_DEPRECATED
#else
#define OBS_EXTERNAL_DEPRECATED OBS_DEPRECATED
#endif

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

#ifdef _MSC_VER
#define PRAGMA_WARN_PUSH __pragma(warning(push))
#define PRAGMA_WARN_POP __pragma(warning(pop))
#define PRAGMA_WARN_DEPRECATION
#define PRAGMA_WARN_STRINGOP_OVERFLOW
#elif defined(__clang__)
#define PRAGMA_WARN_PUSH _Pragma("clang diagnostic push")
#define PRAGMA_WARN_POP _Pragma("clang diagnostic pop")
#define PRAGMA_WARN_DEPRECATION \
	_Pragma("clang diagnostic warning \"-Wdeprecated-declarations\"")
#define PRAGMA_WARN_STRINGOP_OVERFLOW
#elif defined(__GNUC__)
#define PRAGMA_WARN_PUSH _Pragma("GCC diagnostic push")
#define PRAGMA_WARN_POP _Pragma("GCC diagnostic pop")
#define PRAGMA_WARN_DEPRECATION \
	_Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
#define PRAGMA_WARN_STRINGOP_OVERFLOW \
	_Pragma("GCC diagnostic warning \"-Wstringop-overflow\"")
#else
#define PRAGMA_WARN_PUSH
#define PRAGMA_WARN_POP
#define PRAGMA_WARN_DEPRECATION
#define PRAGMA_WARN_STRINGOP_OVERFLOW
#endif

#if defined(__cplusplus)
#include <cstddef>
#include <cstdint>
#if __cplusplus < 201703L
#include <cstdbool>
#endif // < C++17
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define nullptr NULL
#endif // __cplusplus
#include <sys/types.h>
