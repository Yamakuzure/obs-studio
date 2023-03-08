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

#include "c17atomics.h"
#include "c99defs.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bmem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dynamic circular buffer */

struct circlebuf {
	void *data;
	a_size_t size;

	size_t start_pos;
	size_t end_pos;
	size_t capacity;
};

#define cb_get_size(CB_s_) atomic_load(&((CB_s_).size))
#define cb_get_size_p(CB_p_) atomic_load(&((CB_p_)->size))
#define cb_set_size_p(CB_p_, SIZE_) atomic_store(&((CB_p_)->size), (SIZE_))
#define cb_add_size_p(CB_p_, SIZE_) atomic_fetch_add(&((CB_p_)->size), (SIZE_))
#define cb_sub_size_p(CB_p_, SIZE_) atomic_fetch_sub(&((CB_p_)->size), (SIZE_))

static inline void circlebuf_init(struct circlebuf *cb)
{
#if defined(ATOMIC_LONG_LOCK_FREE) && (ATOMIC_LONG_LOCK_FREE == 2)
	static_assert(sizeof(a_size_t) == sizeof(size_t),
		      "ATOMIC_LONG_LOCK_FREE is greater than 1,"
		      " but atomic_size_t has a different size than size_t");
	memset(cb, 0, sizeof(struct circlebuf));
#else
	cb->data = nullptr;
	atomic_init(&cb->size, 0);
	cb->start_pos = 0;
	cb->end_pos = 0;
	cb->capacity = 0;
#endif // ATOMIC_LONG_LOCK_FREE
}

static inline void circlebuf_free(struct circlebuf *cb)
{
	bfree(cb->data);
	circlebuf_init(cb);
}

static inline void circlebuf_reorder_data(struct circlebuf *cb,
					  size_t new_capacity)
{
	size_t difference;
	uint8_t *data;

	if (!cb_get_size_p(cb) || !cb->start_pos || cb->end_pos > cb->start_pos)
		return;

	difference = new_capacity - cb->capacity;
	data = (uint8_t *)cb->data + cb->start_pos;
	memmove(data + difference, data, cb->capacity - cb->start_pos);
	cb->start_pos += difference;
}

static inline void circlebuf_ensure_capacity(struct circlebuf *cb)
{
	size_t new_capacity;
	if (cb_get_size_p(cb) <= cb->capacity)
		return;

	new_capacity = cb->capacity * 2;
	if (cb_get_size_p(cb) > new_capacity)
		new_capacity = cb_get_size_p(cb);

	cb->data = brealloc(cb->data, new_capacity);
	circlebuf_reorder_data(cb, new_capacity);
	cb->capacity = new_capacity;
}

static inline void circlebuf_reserve(struct circlebuf *cb, size_t capacity)
{
	if (capacity <= cb->capacity)
		return;

	cb->data = brealloc(cb->data, capacity);
	circlebuf_reorder_data(cb, capacity);
	cb->capacity = capacity;
}

static inline void circlebuf_upsize(struct circlebuf *cb, size_t size)
{
	size_t add_size = size - cb_get_size_p(cb);
	size_t new_end_pos = cb->end_pos + add_size;

	if (size <= cb_get_size_p(cb))
		return;

	cb_set_size_p(cb, size);
	circlebuf_ensure_capacity(cb);

	if (new_end_pos > cb->capacity) {
		size_t back_size = cb->capacity - cb->end_pos;
		size_t loop_size = add_size - back_size;

		if (back_size)
			memset((uint8_t *)cb->data + cb->end_pos, 0, back_size);

		memset(cb->data, 0, loop_size);
		new_end_pos -= cb->capacity;
	} else {
		memset((uint8_t *)cb->data + cb->end_pos, 0, add_size);
	}

	cb->end_pos = new_end_pos;
}

/** Overwrites data at a specific point in the buffer (relative).  */
static inline void circlebuf_place(struct circlebuf *cb, size_t position,
				   const void *data, size_t size)
{
	size_t end_point = position + size;
	size_t data_end_pos;

	if (end_point > cb_get_size_p(cb))
		circlebuf_upsize(cb, end_point);

	position += cb->start_pos;
	if (position >= cb->capacity)
		position -= cb->capacity;

	data_end_pos = position + size;
	if (data_end_pos > cb->capacity) {
		size_t back_size = data_end_pos - cb->capacity;
		size_t loop_size = size - back_size;

		memcpy((uint8_t *)cb->data + position, data, loop_size);
		memcpy(cb->data, (uint8_t *)data + loop_size, back_size);
	} else {
		memcpy((uint8_t *)cb->data + position, data, size);
	}
}

static inline void circlebuf_push_back(struct circlebuf *cb, const void *data,
				       size_t size)
{
	size_t new_end_pos = cb->end_pos + size;

	cb_add_size_p(cb, size);
	circlebuf_ensure_capacity(cb);

	if (new_end_pos > cb->capacity) {
		size_t back_size = cb->capacity - cb->end_pos;
		size_t loop_size = size - back_size;

		if (back_size)
			memcpy((uint8_t *)cb->data + cb->end_pos, data,
			       back_size);
		memcpy(cb->data, (uint8_t *)data + back_size, loop_size);

		new_end_pos -= cb->capacity;
	} else {
		memcpy((uint8_t *)cb->data + cb->end_pos, data, size);
	}

	cb->end_pos = new_end_pos;
}

static inline void circlebuf_push_front(struct circlebuf *cb, const void *data,
					size_t size)
{
	cb_add_size_p(cb, size);
	circlebuf_ensure_capacity(cb);

	if ((cb_get_size_p(cb) == size)) {
		cb->start_pos = 0;
		cb->end_pos = size;
		memcpy((uint8_t *)cb->data, data, size);

	} else if (cb->start_pos < size) {
		size_t back_size = size - cb->start_pos;

		if (cb->start_pos)
			memcpy(cb->data, (uint8_t *)data + back_size,
			       cb->start_pos);

		cb->start_pos = cb->capacity - back_size;
		memcpy((uint8_t *)cb->data + cb->start_pos, data, back_size);
	} else {
		cb->start_pos -= size;
		memcpy((uint8_t *)cb->data + cb->start_pos, data, size);
	}
}

static inline void circlebuf_push_back_zero(struct circlebuf *cb, size_t size)
{
	size_t new_end_pos = cb->end_pos + size;

	cb_add_size_p(cb, size);
	circlebuf_ensure_capacity(cb);

	if (new_end_pos > cb->capacity) {
		size_t back_size = cb->capacity - cb->end_pos;
		size_t loop_size = size - back_size;

		if (back_size)
			memset((uint8_t *)cb->data + cb->end_pos, 0, back_size);
		memset(cb->data, 0, loop_size);

		new_end_pos -= cb->capacity;
	} else {
		memset((uint8_t *)cb->data + cb->end_pos, 0, size);
	}

	cb->end_pos = new_end_pos;
}

static inline void circlebuf_push_front_zero(struct circlebuf *cb, size_t size)
{
	cb_add_size_p(cb, size);
	circlebuf_ensure_capacity(cb);

	if (cb_get_size_p(cb) == size) {
		cb->start_pos = 0;
		cb->end_pos = size;
		memset((uint8_t *)cb->data, 0, size);

	} else if (cb->start_pos < size) {
		size_t back_size = size - cb->start_pos;

		if (cb->start_pos)
			memset(cb->data, 0, cb->start_pos);

		cb->start_pos = cb->capacity - back_size;
		memset((uint8_t *)cb->data + cb->start_pos, 0, back_size);
	} else {
		cb->start_pos -= size;
		memset((uint8_t *)cb->data + cb->start_pos, 0, size);
	}
}

static inline void circlebuf_peek_front(struct circlebuf *cb, void *data,
					size_t size)
{
	assert(size <= cb_get_size_p(cb));

	if (data) {
		size_t start_size = cb->capacity - cb->start_pos;

		if (start_size < size) {
			memcpy(data, (uint8_t *)cb->data + cb->start_pos,
			       start_size);
			memcpy((uint8_t *)data + start_size, cb->data,
			       size - start_size);
		} else {
			memcpy(data, (uint8_t *)cb->data + cb->start_pos, size);
		}
	}
}

static inline void circlebuf_peek_back(struct circlebuf *cb, void *data,
				       size_t size)
{
	assert(size <= cb_get_size_p(cb));

	if (data) {
		size_t back_size = (cb->end_pos ? cb->end_pos : cb->capacity);

		if (back_size < size) {
			size_t front_size = size - back_size;
			size_t new_end_pos = cb->capacity - front_size;

			memcpy((uint8_t *)data + (size - back_size), cb->data,
			       back_size);
			memcpy(data, (uint8_t *)cb->data + new_end_pos,
			       front_size);
		} else {
			memcpy(data, (uint8_t *)cb->data + cb->end_pos - size,
			       size);
		}
	}
}

static inline void circlebuf_pop_front(struct circlebuf *cb, void *data,
				       size_t size)
{
	circlebuf_peek_front(cb, data, size);

	cb_sub_size_p(cb, size);
	if (!cb_get_size_p(cb)) {
		cb->start_pos = cb->end_pos = 0;
		return;
	}

	cb->start_pos += size;
	if (cb->start_pos >= cb->capacity)
		cb->start_pos -= cb->capacity;
}

static inline void circlebuf_pop_back(struct circlebuf *cb, void *data,
				      size_t size)
{
	circlebuf_peek_back(cb, data, size);

	cb_sub_size_p(cb, size);
	if (!cb_get_size_p(cb)) {
		cb->start_pos = cb->end_pos = 0;
		return;
	}

	if (cb->end_pos <= size)
		cb->end_pos = cb->capacity - (size - cb->end_pos);
	else
		cb->end_pos -= size;
}

static inline void *circlebuf_data(struct circlebuf *cb, size_t idx)
{
	uint8_t *ptr = (uint8_t *)cb->data;
	size_t offset = cb->start_pos + idx;

	if (idx >= cb_get_size_p(cb))
		return NULL;

	if (offset >= cb->capacity)
		offset -= cb->capacity;

	return ptr + offset;
}

#ifdef __cplusplus
}
#endif
