#ifndef __HOOK_HPP_
#define __HOOK_HPP_

#include "Kernel.hpp"
#include "Patcher.hpp"

#include "Arch.hpp"

#include "Array.hpp"

#include <x86_64/Isa_x86_64.hpp>
#include <arm64/Isa_arm64.hpp>

class MacRootKit;

class Patcher;

class Payload;

class Kernel;
class Kext;

class Task;

using namespace Arch;

enum HookType
{
	kHookTypeNone,
	kHookTypeBreakpoint,
	kHookTypeCallback,
	kHookTypeInstrumentFunction,
	kHookTypeReplaceFunction,
};

struct HookPatch
{
	mach_vm_address_t to;
	mach_vm_address_t from;

	mach_vm_address_t trampoline;

	enum HookType type;

	union FunctionJmp patch;

	Payload *payload;

	uint8_t *original;
	uint8_t *replace;

	size_t patch_size;
};

class Patcher;

template<typename T, typename Y = enum HookType>
using HookCallbackPair = Pair<T, Y>;

template<typename T, typename Y = enum HookType>
using HookCallbackArray = Array<HookCallbackPair<T, Y>*>;

template<typename T = struct HookPatch*>
using HookArray = Array<T>;

class Hook
{
	public:
		Hook(Patcher *patcher, enum HookType hooktype);
		Hook(Patcher *patcher, enum HookType hooktype, Task *task, mach_vm_address_t from);

		void initWithHookParams(Task *task, mach_vm_address_t from);
		void initWithBreakpointParams(Task *task, mach_vm_address_t breakpoint);

		static Hook* hookForFunction(Task *task, Patcher *patcher, mach_vm_address_t address);
		static Hook* breakpointForAddress(Task *task, Patcher *patcher, mach_vm_address_t address);

		Patcher* getPatcher() { return patcher; }

		Task* getTask() { return task; }

		Architecture* getArchitecture() { return architecture; }

		Disassembler* getDisassembler() { return disassembler; }

		mach_vm_address_t getFrom() { return from; }

		struct HookPatch* getLatestRegisteredHook();

		mach_vm_address_t getTrampoline() { return trampoline; }

		mach_vm_address_t getTrampolineFromChain(mach_vm_address_t address);

		HookArray<struct HookPatch*>* getHooks() { return &hooks; }

		HookCallbackArray<mach_vm_address_t>* getCallbacks() { return &callbacks; }

		enum HookType getHookType() { return hooktype; }

		enum HookType getHookTypeForCallback(mach_vm_address_t callback);

		void setPatcher(Patcher *patcher) { this->patcher = patcher; }

		void setDisassembler(Disassembler *disassembler) { this->disassembler = disassembler; }

		void setTask(Task *task) { this->task = task; }

		void setFrom(mach_vm_address_t from) { this->from = from; }

		void setTrampoline(mach_vm_address_t trampoline) { this->trampoline = trampoline; }

		void setHookType(enum HookType hooktype) { this->hooktype = hooktype; }

		Payload* prepareTrampoline();

		void registerHook(struct HookPatch *patch);

		void registerCallback(mach_vm_address_t callback, enum HookType hooktype = kHookTypeCallback);

		void hookFunction(mach_vm_address_t to, enum HookType hooktype = kHookTypeInstrumentFunction);

		void uninstallHook();

		void addBreakpoint(mach_vm_address_t breakpoint_hook, enum HookType hooktype = kHookTypeBreakpoint);

		void removeBreakpoint();

	private:
		Patcher *patcher;

		Task *task;

		Architecture *architecture;

		Disassembler *disassembler;

		Payload *payload;

		bool kernelHook = false;

		mach_vm_address_t from;

		mach_vm_address_t trampoline;

		enum HookType hooktype;

		HookCallbackArray<mach_vm_address_t> callbacks;

		HookArray<struct HookPatch*> hooks;

};

#endif