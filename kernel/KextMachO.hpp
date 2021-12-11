#ifndef __KEXT_MACHO_HPP_
#define __KEXT_MACHO_HPP_

#include <mach/mach_types.h>
#include <mach/kmod.h>

#include "MachO.hpp"

#include "Array.hpp"
#include "SymbolTable.hpp"

#include "Kernel.hpp"
#include "Kext.hpp"

class Kernel;

class KextMachO : public MachO
{
	public:
		KextMachO(Kernel *kernel, char *name, kmod_info_t *kmod_info);

		~KextMachO();

		Kernel* getKernel() { return kernel; }

		char* getKextName() { return name; }

		mach_vm_address_t getAddress() { return address; }

		size_t getSize() { return kmod_info->size; }

		kmod_start_func_t* getKmodStart() { return kmod_info->start; }
		kmod_stop_func_t* getKmodStop() { return kmod_info->stop; }

		void setKernelCollection(mach_vm_address_t kc) { this->kernel_collection = kc; }

		virtual void parseLinkedit();

		virtual bool parseLoadCommands();

		virtual void parseHeader();

		virtual void parseMachO();

	private:
		Kernel *kernel;

		mach_vm_address_t address;

		char *name;

		off_t base_offset;

		mach_vm_address_t kernel_collection;

		kmod_info_t *kmod_info;

		uint8_t *linkedit;

		off_t linkedit_off;

		size_t linkedit_size;
};

#endif