//
// Created by Sven Eden on 02.03.23.
//

// Clear old macro substitutions
#if defined(a_bool_t)
#undef a_bool_t
#endif // a_bool_t

#if defined(a_int64_t)
#undef a_int64_t
#endif // a_int64_t

#if defined(a_size_t)
#undef a_size_t
#endif // a_size_t


// Add atomic definitions and utilities
#if defined(__cplusplus)

#include <atomic>

#if !defined(HAVE_ATOMIC_BOOL_T_DEFINED)
typedef volatile std::atomic_bool a_bool_t;
#define HAVE_ATOMIC_BOOL_T_DEFINED
#endif // HAVE_ATOMIC_BOOL_T_DEFINED

#if !defined(HAVE_ATOMIC_INT64_T_DEFINED)
typedef std::atomic_int_fast64_t a_int64_t;
#define HAVE_ATOMIC_INT64_T_DEFINED
#endif // HAVE_ATOMIC_INT64_T_DEFINED

#if !defined(HAVE_ATOMIC_SIZE_T_DEFINED) && !defined(USE_NON_ATOMIC_SIZE_T)
typedef volatile std::atomic_size_t a_size_t;
using std::atomic_init;
#define HAVE_ATOMIC_SIZE_T_DEFINED
#endif // HAVE_ATOMIC_SIZE_T_DEFINED

#if !defined(HAVE_ATOMIC_UINT32_T_DEFINED)
typedef volatile std::atomic_uint_fast32_t a_uint32_t;
#define HAVE_ATOMIC_UINT32_T_DEFINED
#endif // HAVE_ATOMIC_UINT32_T_DEFINED

#if !defined(HAVE_ATOMIC_UINT64_T_DEFINED)
typedef volatile std::atomic_uint_fast64_t a_uint64_t;
#define HAVE_ATOMIC_UINT64_T_DEFINED
#endif // HAVE_ATOMIC_UINT64_T_DEFINED

#if defined(USE_NON_ATOMIC_SIZE_T)
/* Note: profiler.h needs to typedef DARRAY() into a type, which is not allowed
 *       in C++17. It would get answered by something like: (example)
 * error: member 'a_size_t da_union_profiler_time_entries_t::<unnamed struct>::num'
 *        with constructor not allowed in anonymous aggregate
 * In such cases we simply use a volatile size_t, the profiler should be happy
 * with that, too.
 */
#define a_size_t volatile size_t
#define atomic_init(verb_, val_) *(verb_)=(val_)
#endif // USE_NON_ATOMIC_SIZE_T

#else // __cplusplus

#include <stdatomic.h>

#if !defined(HAVE_ATOMIC_BOOL_T_DEFINED)
typedef volatile atomic_bool a_bool_t;
#define HAVE_ATOMIC_BOOL_T_DEFINED
#endif // HAVE_ATOMIC_BOOL_T_DEFINED

#if !defined(HAVE_ATOMIC_INT64_T_DEFINED)
typedef atomic_int_fast64_t a_int64_t;
#define HAVE_ATOMIC_INT64_T_DEFINED
#endif // HAVE_ATOMIC_INT64_T_DEFINED

#if !defined(HAVE_ATOMIC_SIZE_T_DEFINED)
typedef volatile atomic_size_t a_size_t;
#define HAVE_ATOMIC_SIZE_T_DEFINED
#endif // HAVE_ATOMIC_SIZE_T_DEFINED

#if !defined(HAVE_ATOMIC_UINT32_T_DEFINED)
typedef volatile atomic_uint_fast32_t a_uint32_t;
#define HAVE_ATOMIC_UINT32_T_DEFINED
#endif // HAVE_ATOMIC_UINT32_T_DEFINED

#if !defined(HAVE_ATOMIC_UINT64_T_DEFINED)
typedef volatile atomic_uint_fast64_t a_uint64_t;
#define HAVE_ATOMIC_UINT64_T_DEFINED
#endif // HAVE_ATOMIC_UINT64_T_DEFINED

#endif // __cplusplus
