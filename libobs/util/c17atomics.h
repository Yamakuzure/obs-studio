//
// Created by Sven Eden on 02.03.23.
//

// Clear old macro substitution
#if defined(a_size_t)
#undef a_size_t
#endif // a_size_t

#if defined(__cplusplus)

#if !defined(HAVE_ATOMIC_SIZE_T_DEFINED) && !defined(USE_NON_ATOMIC_SIZE_T)
#include <atomic>
typedef std::atomic_size_t a_size_t;
#define HAVE_ATOMIC_SIZE_T_DEFINED
#endif // HAVE_ATOMIC_SIZE_T_DEFINED

#if defined(USE_NON_ATOMIC_SIZE_T)
/* Note: profiler.h needs to typedef DARRAY() into a type, which is not allowed
 *       in C++17. It would get answered by something like: (example)
 * error: member 'a_size_t da_union_profiler_time_entries_t::<unnamed struct>::num'
 *        with constructor not allowed in anonymous aggregate
 * In such cases we simply use a volatile size_t, the profiler should be happy
 * with that, too.
 */
#define a_size_t volatile size_t
#endif // USE_NON_ATOMIC_SIZE_T

#else // __cplusplus

#if !defined(HAVE_ATOMIC_SIZE_T_DEFINED)
#include <stdatomic.h>
typedef atomic_size_t a_size_t;
#define HAVE_ATOMIC_SIZE_T_DEFINED
#endif // HAVE_ATOMIC_SIZE_T_DEFINED

#endif // __cplusplus
