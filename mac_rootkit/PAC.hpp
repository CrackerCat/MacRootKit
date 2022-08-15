#ifndef __PAC_HPP_
#define __PAC_HPP_

#include <mach/mach_types.h>

#ifdef __arm64e__

namespace PAC
{
	uint64_t signPointerWithAKey(uint64_t pointer);

	uint64_t signPointerWithBKey(uint64_t pointer);
}

#define PACSignPointerWithAKey(ptr) PAC::signPointerWithAKey(ptr)
#define PACSignPointerWithBKey(ptr) PAC::signPointerWithBKey(ptr);

#else

#define PACSignPointerWithAKey(ptr) ptr
#define PACSignPointerWithBKey(ptr) ptr

#endif

#endif