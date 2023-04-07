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

#include "c99defs.h"
#include "base.h"

#ifdef __cplusplus
#include <cwchar>
#include <cstring>
extern "C" {
#else
#include <wchar.h>
#include <string.h>
#endif

struct base_allocator {
	void *(*malloc)(size_t);
	void *(*realloc)(void *, size_t);
	void (*free)(void *);
};

OBS_DEPRECATED EXPORT void base_set_allocator(struct base_allocator *defs);

EXPORT void *bmalloc(size_t size);
EXPORT void *brealloc(void *ptr, size_t size);
EXPORT void bfree(void *ptr);

EXPORT int base_get_alignment(void);

EXPORT long bnum_allocs(void);

EXPORT void *bmemdup(const void *ptr, size_t size);
EXPORT void *bmemdup_ext(const void *ptr, size_t src_size, size_t tgt_size);

static inline void *bzalloc(size_t size)
{
	void *mem = bmalloc(size);
	if (mem)
		memset(mem, 0, size);
	return mem;
}

static inline char *bstrdup_n(const char *str, size_t n)
{
	char *dup;
	if (!str)
		return NULL;

	dup = (char *)bmemdup_ext(str, n, n + 1);

	return dup;
}

static inline wchar_t *bwstrdup_n(const wchar_t *str, size_t n)
{
	wchar_t *dup;
	if (!str)
		return NULL;

	dup = (wchar_t *)bmemdup_ext(str, n * sizeof(wchar_t),
				     (n + 1) * sizeof(wchar_t));

	return dup;
}

static inline char *bstrdup(const char *str)
{
	if (!str)
		return NULL;

	return bstrdup_n(str, strlen(str));
}

static inline wchar_t *bwstrdup(const wchar_t *str)
{
	if (!str)
		return NULL;

	return bwstrdup_n(str, wcslen(str));
}

#ifdef __cplusplus
}
#endif
