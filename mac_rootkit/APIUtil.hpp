#ifndef __API_UTIL_HPP_
#define __API_UTIL_HPP_

#include "Log.hpp"

#ifdef __USER__

#include <IOKit/IOKitLib.h>

#endif

#ifdef __KERNEL__

#include <IOKit/IOLib.h>

#endif

#ifdef __cplusplus

	extern "C"
	{
#endif
		#include "API.h"
#ifdef __cplusplus
	}

#endif

#include <mach/mach_types.h>
#include <mach/vm_types.h>

namespace xnu
{
    class Kernel;
}

using namespace xnu;

/**
 *  Export function or symbol for linking
 */
#define EXPORT __attribute__((visibility("default")))

/**
 *  Ensure the symbol is not exported
 */
#define PRIVATE __attribute__((visibility("hidden")))

/**
 *  For private fallback symbol definition
 */
#define WEAKFUNC __attribute__((weak))

/**
 *  Remove padding between fields
 */
#define PACKED __attribute__((packed))

/**
 *  Deprecate the interface
 */
#define DEPRECATE(x) __attribute__((deprecated(x)))

/**
 *  Non-null argument
 */
#define NONNULL __attribute__((nonnull))

/**
 *  Access struct member by its offset
 *
 *  @param T     pointer to the field you need
 *  @param that  pointer to struct
 *  @param off   offset in bytes to the member
 *
 *  @return reference to the struct member
 */
template <typename T>
inline T &getMember(void *that, size_t off)
{
    return *reinterpret_cast<T *>(static_cast<uint8_t *>(that) + off);
}

/**
 *  Align value by align (page size by default)
 *
 *  @param size  value
 *
 *  @return algined value
 */
template <typename T>
inline T alignValue(T size, T align = 4096)
{
    return (size + align - 1) & (~(align - 1));
}

/**
 *  Check pointer alignment for type T
 *
 *  @param p  pointer
 *
 *  @return true if properly aligned
 */
template<typename T>
inline bool isAligned(T *p)
{
    return reinterpret_cast<uintptr_t>(p) % alignof(T) == 0;
}

/**
 *  Obtain bit value of size sizeof(T)
 *  Warning, you are suggested to always pass the type explicitly!
 *
 *  @param n  bit no
 *
 *  @return bit value
 */
template <typename T>
constexpr T getBit(T n)
{
    return static_cast<T>(1U) << n;
}

/**
 *  Obtain bit mask of size sizeof(T)
 *  Warning, you are suggested to always pass the type explicitly!
 *
 *  @param hi  starting high bit
 *  @param lo  ending low bit
 *
 *  @return bit mask
 */
template <typename T>
constexpr T getBitMask(T hi, T lo)
{
    return (getBit(hi)|(getBit(hi)-1U)) & ~(getBit(lo)-1U);
}

/**
 *  Obtain bit field of size sizeof(T)
 *  Warning, you are suggested to always pass the type explicitly!
 *
 *  @param so  source
 *  @param hi  starting high bit
 *  @param lo  ending low bit
 *
 *  @return bit field value
 */
template <typename T>
constexpr T getBitField(T so, T hi, T lo)
{
    return (so & getBitMask(hi, lo)) >> lo;
}

/**
 *  Set bit field of size sizeof(T)
 *  Warning, you are suggested to always pass the type explicitly!
 *
 *  @param va  value
 *  @param hi  starting high bit
 *  @param lo  ending low bit
 *
 *  @return bit field value
 */
template <typename T>
constexpr T setBitField(T so, T hi, T lo)
{
    return (so << lo) & getBitMask(hi, lo);
}

/**
 *  This is an ugly replacement to std::find_if, allowing you
 *  to check whether a container consists only of value values.
 *
 *  @param in     container
 *  @param size   container size
 *  @param value  value to look for
 *
 *  @return true if an element different from value was found
 */
template <typename T, typename Y>
inline bool findNotEquals(T &in, size_t size, Y value)
{
    for (size_t i = 0; i < size; i++)
        if (in[i] != value)
            return true;
        
    return false;
}

/**
 *  Returns non-null string when they can be null
 *
 *  @param str  original string
 *
 *  @return non-null string
 */
inline const char *safeString(const char *str)
{
    return str ? str : "(null)";
}

/**
 *  A shorter form of writing reinterpret_cast<decltype(&org)>(ptr)
 */
template <typename T>
inline T FunctionCast(T org, mach_vm_address_t ptr)
{
    return reinterpret_cast<T>(ptr);
}

namespace API
{
	void dump(Kernel *kernel, mach_vm_address_t address);

	void hexdump(unsigned char *data, size_t size, bool rev);
};

#endif