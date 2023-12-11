#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __GNUC__
#define CORE_LIKELY(expr) __builtin_expect((expr), 1)
#define CORE_UNLIKELY(expr) __builtin_expect((expr), 0)
#else
#define CORE_LIKELY(expr) expr
#define CORE_UNLIKELY(expr) expr
#endif

///////////////////////////
//
//
// Number and Pointer Utilities
//
//
///////////////////////////

#define CORE_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CORE_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CORE_CLAMP(v, min, max) (((v) > (max)) ? (max) : ((v) < (min)) ? : (min) : (v))
#define CORE_UNUSED(expr) ((void)(expr))

// align must be a power of 2
#define CORE_INT_ROUND_UP_ALIGN(i, align) (((i) + ((align) - )) & ~((align) - 1))
// align must be a power of 2
#define CORE_INT_ROUND_DOWN_ALIGN(i, align) ((i) & ~((align) - 1))

#define CORE_ARRAY_COUNT(array) (sizeof(array) / sizeof(*(array)))
#define CORE_IS_POWER_OF_TWO_OR_ZERO(v) (((v) & ((v) - 1)) == 0)
#define CORE_IS_POWER_OF_TWO(v) (((v) != 0) && (((v) & ((v) - 1)) == 0))
#define CORE_PTR_ADD(ptr, by) (void*)((uintptr_t)(ptr) + (uintptr_t)(by))
#define CORE_PTR_SUB(ptr, by) (void*)((uintptr_t)(ptr) - (uintptr_t)(by))
#define CORE_PTR_DIFF(to, from) ((char*)(to) - (char*)(from))
// aling must be a power of 2
#define CORE_PTR_ROUND_UP_ALIGN(ptr, align) ((void*)CORE_INT_ROUND_UP_ALIGN((uintptr_t)(ptr), (uintptr_t)(align)))
// aling must be a power of 2
#define CORE_PTR_ROUND_DOWN_ALIGN(ptr, align) ((void*)CORE_INT_ROUND_DOWN_ALIGN((uintptr_t)(ptr), (uintptr_t)(align)))
#define CORE_ZERO_ELMT(ptr) memset(ptr, 0, sizeof(*(ptr)))
#define CORE_ONE_ELMT(ptr) memset(ptr, 0xff, sizeof(*(ptr)))
#define CORE_ZERO_ELMT_MANY(ptr, elmts_count) memset(ptr, 0, sizeof(*(ptr)) * (elmts_count))
#define CORE_ONE_ELMT_MANY(ptr, elmts_count) memset(ptr, 0xff, sizeof(*(ptr)) * (elmts_count))
#define CORE_ZERO_ARRAY(array) memset(array, 0, sizeof(array))
#define CORE_ONE_ARRAY(array) memset(array, 0xff, sizeof(array))
#define CORE_COPY_ARRAY(dst, src) memcpy(dst, src, sizeof(dst))
#define CORE_COPY_ELMT_MANY(dst, src, elmts_count) memcpy(dst, src, sizeof(*(dst)) * (elmts_count))
#define CORE_COPY_OVERLAP_ELMT_MANY(dst, src, elmts_count) memmove(dst, src, sizeof(*(dst)) * (elmts_count))
#define CORE_CMP_ARRAY(a, b) (memcmp(a, b, sizeof(a)) == 0)
#define CORE_CMP_ELMT(a, b) (memcmp(a, b, sizeof(*(a))) == 0)
#define CORE_CMP_ELMT_MANY(a, b, elmts_count) (memcmp(a, b, sizeof(*(a)) * elmts_count) == 0)

#define CORE_DIV_ROUND_UP(a, b) (((a) / (b)) + ((a) % (b) != 0))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint64_t uptr;
typedef int64_t sptr;

typedef float f32;
typedef double f64;

#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX
#define UPTR_MAX UINT64_MAX

#define I8_MIN INT8_MIN
#define I8_MAX INT8_MAX
#define I16_MIN INT16_MIN
#define I16_MAX INT16_MAX
#define I32_MIN INT32_MIN
#define I32_MAX INT32_MAX
#define I64_MIN INT64_MIN
#define I64_MAX INT64_MAX

///////////////////////////
//
//
// Abort and Assertions
//
//
///////////////////////////

#ifndef CORE_ENABLE_DEBUG_ASSERTIONS
#define CORE_ENABLE_DEBUG_ASSERTIONS 1
#endif
#ifndef CORE_ABORT
#define CORE_ABORT(...) _core_abort(__FILE__, __LINE__, __VA_ARGS__)
#endif

#ifndef CORE_ASSERT
#define CORE_ASSERT(cond, ...) if (CORE_UNLIKELY(!(cond))) _core_assert_failed(#cond, __FILE__, __LINE__, __VA_ARGS__)
#endif

#if CORE_ENABLE_DEBUG_ASSERTIONS
#define CORE_DEBUG_ASSERT CORE_ASSERT
#else
#define CORE_DEBUG_ASSERT(cond, ...) (void)(cond)
#endif

#define CORE_DEBUG_ASSERT_NON_ZERO(value) CORE_DEBUG_ASSERT(value, "'%s' must be a non zero value", #value)
#define CORE_DEBUG_ASSERT_ARRAY_BOUNDS(idx, count) (((idx) < (count)) ? (idx) : CORE_ABORT("idx %d out of bounds for array of size %d", idx, count))
#define CORE_DEBUG_ASSERT_ARRAY_RESIZE(count, cap) CORE_DEBUG_ASSERT((count) <= (cap), "cannot resize array to count '%zu' when it has a capacity of '%zu'", count, cap)
#define CORE_DEBUG_ASSERT_POWER_OF_TWO(v) CORE_DEBUG_ASSERT(CORE_IS_POWER_OF_TWO(v), #v "must be a power of two but got '%zu'", v)

#define CORE_BREAKPOINT() asm ("int $3") // x86 specific breakpoint instruction

void _core_assert_failed(const char* cond, const char* file, int line, const char* message, ...);
_Noreturn uintptr_t _core_abort(const char* file, int line, const char* message, ...);

/* static inline bool core_bitset_is_set(u64* bitset, uptr bit_idx) { */
/* 	return (bool)(bitset[bit_idx >> 6] & (1 << (bit_idx & 63))); */
/* } */

