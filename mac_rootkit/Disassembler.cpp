#include "Disassembler.hpp"

#include "Task.hpp"

#include <x86_64/x86_64.hpp>
#include <arm64/arm64.hpp>

#include <x86_64/Disassembler_x86_64.hpp>
#include <arm64/Disassembler_arm64.hpp>

Disassembler::Disassembler(Task *task)
{
	this->task = task;
	this->architecture = Arch::getArchitecture();
}

void Disassembler::initDisassembler()
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			Arch::x86_64::Disassembler::init();

			break;
		case ARCH_arm64:
			Arch::arm64::Disassembler::init();

			break;
		default:
			break;
	}
}

void Disassembler::deinitDisassembler()
{
}

size_t Disassembler::disassemble(mach_vm_address_t address, size_t size, cs_insn **result)
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			return Arch::x86_64::Disassembler::disassemble(address, size, result);

			break;
		case ARCH_arm64:
			return Arch::arm64::Disassembler::disassemble(address, size, result);

			break;
		default:
			break;
	}

	return 0;
}

size_t Disassembler::quickInstructionSize(mach_vm_address_t address, size_t min)
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			return Arch::x86_64::Disassembler::quickInstructionSize(address, min);

			break;
		case ARCH_arm64:
			return Arch::arm64::Disassembler::quickInstructionSize(address, min);

			break;
		default:
			break;
	}

	return 0;
}

size_t Disassembler::instructionSize(mach_vm_address_t address, size_t min)
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			return Arch::x86_64::Disassembler::instructionSize(address, min);

			break;
		case ARCH_arm64:
			return Arch::arm64::Disassembler::instructionSize(address, min);

			break;
		default:
			break;
	}

	return 0;
}

mach_vm_address_t Disassembler::disassembleNthCall(mach_vm_address_t address, size_t num, size_t lookup_size)
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			return Arch::x86_64::Disassembler::disassembleNthCall(address, num, lookup_size);

			break;
		case ARCH_arm64:
			return Arch::arm64::Disassembler::disassembleNthBranchLink(address, num, lookup_size);

			break;
		default:
			break;
	}

	return 0;
}

mach_vm_address_t Disassembler::disassembleNthJmp(mach_vm_address_t address, size_t num, size_t lookup_size)
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			return Arch::x86_64::Disassembler::disassembleNthJmp(address, num, lookup_size);

			break;
		case ARCH_arm64:
			return Arch::arm64::Disassembler::disassembleNthBranch(address, num, lookup_size);

			break;
		default:
			break;
	}

	return 0;
}

mach_vm_address_t Disassembler::disassembleNthInstruction(mach_vm_address_t address, uint32_t insn, size_t num, size_t lookup_size)
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			return Arch::x86_64::Disassembler::disassembleNthInstruction(address, (x86_insn) insn, num, lookup_size);

			break;
		case ARCH_arm64:
			return Arch::arm64::Disassembler::disassembleNthInstruction(address, (arm64_insn) insn, num, lookup_size);

			break;
		default:
			break;
	}

	return 0;
}

mach_vm_address_t Disassembler::disassembleSignature(mach_vm_address_t address, Array<struct DisasmSig*> *signature, size_t num, size_t lookup_size)
{
	switch(this->architecture)
	{
		case ARCH_x86_64:
			return Arch::x86_64::Disassembler::disassembleSignature(address, signature, num, lookup_size);

			break;
		case ARCH_arm64:
			return Arch::arm64::Disassembler::disassembleSignature(address, signature, num, lookup_size);

			break;
		default:
			break;
	}

	return 0;
}