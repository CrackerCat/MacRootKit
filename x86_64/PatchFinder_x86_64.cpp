#include "PatchFinder_x86_64.hpp"
#include "Isa_x86_64.hpp"
#include "Disassembler_x86_64.hpp"

namespace Arch
{
	namespace x86_64
	{
		namespace PatchFinder
		{
			unsigned char* boyermoore_horspool_memmem(const unsigned char* haystack, size_t hlen,
													  const unsigned char* needle,   size_t nlen)
			{
				size_t last, scan = 0;
				size_t bad_char_skip[UCHAR_MAX + 1]; /* Officially called:
													 * bad character shift */

				/* Sanity checks on the parameters */
				if (nlen <= 0 || !haystack || !needle)
					return NULL;

				/* ---- Preprocess ---- */
				/* Initialize the table to default value */
				/* When a character is encountered that does not occur
				 * in the needle, we can safely skip ahead for the whole
				 * length of the needle.
				*/
				for (scan = 0; scan <= UCHAR_MAX; scan = scan + 1)
					bad_char_skip[scan] = nlen;

				/* C arrays have the first byte at [0], therefore:
				* [nlen - 1] is the last byte of the array. */
				last = nlen - 1;

				/* Then populate it with the analysis of the needle */
				for (scan = 0; scan < last; scan = scan + 1)
					bad_char_skip[needle[scan]] = last - scan;

				/* ---- Do the matching ---- */

				/* Search the haystack, while the needle can still be within it. */
				while (hlen >= nlen)
				{
					/* scan from the end of the needle */
					for (scan = last; haystack[scan] == needle[scan]; scan = scan - 1)
						if (scan == 0) /* If the first byte matches, we've found it. */
							return (unsigned char*) haystack;

					/* otherwise, we need to skip some bytes and start again.
					   Note that here we are getting the skip value based on the last byte
					   of needle, no matter where we didn't match. So if needle is: "abcd"
					   then we are skipping based on 'd' and that value will be 4, and
					   for "abcdd" we again skip on 'd' but the value will be only 1.
					   The alternative of pretending that the mismatched character was
					   the last character is slower in the normal case (E.g. finding
					   "abcd" in "...azcd..." gives 4 by using 'd' but only
					   4-2==2 using 'z'. */
					hlen     -= bad_char_skip[haystack[last]];
					haystack += bad_char_skip[haystack[last]];
				}

				return NULL;
			}

			mach_vm_address_t xref64(MachO *macho, mach_vm_address_t start, mach_vm_address_t end, mach_vm_address_t what)
			{
				cs_insn *insns;

				size_t count;

				for(uint32_t i = 0; i < end; i++)
				{
					mach_vm_address_t address = start + i;

					uint64_t offset = macho->addressToOffset(start + i);

					if(offset)
					{
						size_t size = Arch::x86_64::Disassembler::disassemble(reinterpret_cast<mach_vm_address_t>((*macho)[offset]), 0x1000, &insns);

						for(uint32_t j = 0; j < size; j++)
						{
							mach_vm_address_t xref;

							cs_insn *insn = &insns[j];

							if(strcmp(insn->mnemonic, "lea") == 0)
							{
								if(insns[i].detail->x86.operands[1].type == X86_OP_MEM && insns[i].detail->x86.operands[1].reg == X86_REG_RIP)
								{
									xref = address + insns[i].detail->x86.operands[1].mem.disp;

									if(xref == what)
									{
										return address;
									}
								}

							} else if(strcmp(insn->mnemonic, "call") == 0)
							{
								if(insns[i].detail->x86.operands[0].type == X86_OP_IMM)
								{
									xref = address + insns[i].detail->x86.operands[0].imm;

									if(xref == what)
									{
										return address;
									}
								}

							} else if(strcmp(insn->mnemonic, "jmp")  == 0 ||
									  strcmp(insn->mnemonic, "jz")   == 0 ||
									  strcmp(insn->mnemonic, "je")   == 0 ||
									  strcmp(insn->mnemonic, "jnz")  == 0 ||
									  strcmp(insn->mnemonic, "jne")  == 0 ||
									  strcmp(insn->mnemonic, "js")   == 0 ||
									  strcmp(insn->mnemonic, "jns")  == 0 ||
									  strcmp(insn->mnemonic, "jo")   == 0 ||
									  strcmp(insn->mnemonic, "jno")  == 0 ||
									  strcmp(insn->mnemonic, "ja")   == 0 ||
									  strcmp(insn->mnemonic, "jnbe") == 0 ||
									  strcmp(insn->mnemonic, "jb")   == 0 ||
									  strcmp(insn->mnemonic, "jc")   == 0 ||
									  strcmp(insn->mnemonic, "jnae") == 0 ||
									  strcmp(insn->mnemonic, "jae")  == 0 ||
									  strcmp(insn->mnemonic, "jnb")  == 0 ||
									  strcmp(insn->mnemonic, "jnc")  == 0 ||
									  strcmp(insn->mnemonic, "jbe")  == 0 ||
									  strcmp(insn->mnemonic, "jna")  == 0)
							{
								if(insns[i].detail->x86.operands[0].type == X86_OP_IMM)
								{
									xref = address + insns[i].detail->x86.operands[0].imm;

									if(xref == what)
									{
										return address;
									}
								}

							} else if(strcmp(insn->mnemonic, "mov") == 0)
							{
								if(insns[i].detail->x86.operands[1].type == X86_OP_MEM && insns[i].detail->x86.operands[1].reg == X86_REG_RIP)
								{
									xref = address + insns[i].detail->x86.operands[1].mem.disp;

									if(xref == what)
									{
										return address;
									}
								}

								cs_regs read, write;

								uint8_t nread, nwrite;

								if(Arch::x86_64::Disassembler::registerAccess(insn, read, &nread, write, &nwrite))
								{
									if(nread)
									{
										x86_reg reg = static_cast<x86_reg>(read[0]);

										if(reg == X86_REG_CS)
										{

										}

										if(reg == X86_REG_DS)
										{

										}

										if(reg == X86_REG_ES)
										{

										}
									   	
									   	if(reg == X86_REG_FS)
									   	{

									   	}
									   
									 	if(reg == X86_REG_GS)
									 	{

									 	}
									   
									   	if(reg == X86_REG_SS)
									   	{

									   	}
									}
								}
							}

							address += insn->size;
						}
					} else
					{
						break;
					}

					i += 0x1000;
				}
				
				return 0;
			}
	
			mach_vm_address_t findInstruction64(MachO *macho, mach_vm_address_t start, size_t length, uint8_t *stream)
			{
				cs_insn *insn;

				size_t count;

				uint64_t offset = macho->addressToOffset(start);

				Arch::x86_64::Disassembler::disassemble(reinterpret_cast<mach_vm_address_t>(stream), Arch::x86_64::MaxInstructionSize, &insn);

				size_t size = insn->size;

				if(offset)
				{
					uint32_t j = 0;

					while(j < length)
					{
						if(memcmp((*macho)[offset + j], stream, size) == 0)
						{
							return start + j;
						}

						j += insn->size;
					}
				}

				return 0;
			}

			mach_vm_address_t findInstructionBack64(MachO *macho, mach_vm_address_t start, size_t length, uint8_t *stream)
			{
				cs_insn *insn;

				size_t count;

				uint64_t offset = macho->addressToOffset(start);
				
				Arch::x86_64::Disassembler::disassemble(reinterpret_cast<mach_vm_address_t>(stream), Arch::x86_64::MaxInstructionSize, &insn);

				size_t size = insn->size;

				while(offset)
				{
					size_t n = 0;

					uint32_t j = 0;

					do
					{
						n = Arch::x86_64::Disassembler::disassemble(reinterpret_cast<mach_vm_address_t>((*macho)[offset - ++j]), Arch::x86_64::MaxInstructionSize, &insn);
					
					} while(insn->size + (offset - j) != offset && n != 1);

					if(insn->size + (offset - j) != offset)
						return 0;

					if(memcmp((*macho)[offset - j], stream, size) == 0)
					{
						return start - j;
					}

					offset -= insn->size;
				}

				return 0;
			}

			mach_vm_address_t findInstructionNTimes64(MachO *macho, int n, mach_vm_address_t start, size_t length, uint8_t *stream, bool forward)
			{
				uint32_t n_insns = 0;

				while(n_insns < n && start)
				{
					if(forward)
					{
						start = findInstruction64(macho, start, length, stream);
					} else
					{
						start = findInstructionBack64(macho, start, length, stream);
					}
				}

				return start;
			}

			mach_vm_address_t step64(MachO *macho, mach_vm_address_t start, size_t length, char *mnemonic, char *op_string)
			{
				cs_insn *insn;

				size_t count;

				uint64_t offset = macho->addressToOffset(start);

				if(offset)
				{
					uint32_t j = 0;

					while(j < length)
					{
						Arch::x86_64::Disassembler::disassemble(reinterpret_cast<mach_vm_address_t>((*macho)[offset + j]), Arch::x86_64::MaxInstructionSize, &insn);

						if(strcmp(insn->mnemonic, mnemonic) == 0)
						{
							if(op_string && strcmp(insn->op_str, op_string) == 0)
							{
								return start + j;
							} else
							{
								return start + j;
							}
						}

						j += insn->size;
					}
				}

				return 0;
			}

			mach_vm_address_t stepBack64(MachO *macho, mach_vm_address_t start, size_t length, char *mnemonic, char *op_string)
			{
				cs_insn *insn;

				size_t count;

				uint64_t offset = macho->addressToOffset(start);

				if(offset)
				{
					uint32_t j = 0;

					while(j < length)
					{
						size_t n = 0;

						while(n != 1)
							n = Arch::x86_64::Disassembler::disassemble(reinterpret_cast<mach_vm_address_t>((*macho)[offset - ++j]), Arch::x86_64::MaxInstructionSize, &insn);

						if(insn->size + (offset - j) != offset)
							return 0;

						if(strcmp(insn->mnemonic, mnemonic) == 0)
						{
							if(op_string && strcmp(insn->op_str, op_string) == 0)
							{
								return start + j;
							} else
							{
								return start + j;
							}
						}

						offset -= insn->size;
					}
				}

				return 0;
			}

			mach_vm_address_t findFunctionBegin(MachO *macho, mach_vm_address_t start, mach_vm_address_t where)
			{
				return stepBack64(macho, start, 0x400, "push", "rsp");
			}

			mach_vm_address_t findReference(MachO *macho, mach_vm_address_t to, int n, enum text which_text)
			{
				Segment *segment;

				mach_vm_address_t ref;

				mach_vm_address_t text_base = 0;
				mach_vm_address_t text_size = 0;

				mach_vm_address_t text_end;

				if((segment = macho->getSegment("__TEXT_EXEC")))
				{
					struct segment_command_64 *segment_command = segment->getSegmentCommand();

					text_base = segment_command->vmaddr;
					text_size = segment_command->vmsize;
				}

				switch(which_text)
				{
					case __TEXT_XNU_BASE:
						break;

					case __TEXT_PRELINK_BASE:
						
						if((segment = macho->getSegment("__PRELINK_TEXT")))
						{
							struct segment_command_64 *segment_command = segment->getSegmentCommand();

							text_base = segment_command->vmaddr;
							text_size = segment_command->vmsize;
						}

						break;
					case __TEXT_PPL_BASE:
						
						if((segment = macho->getSegment("__PPLTEXT")))
						{
							struct segment_command_64 *segment_command = macho->getSegment("__PPLTEXT")->getSegmentCommand();

							text_base = segment_command->vmaddr;
							text_size = segment_command->vmsize;
						}

						break;
					default:
						return 0;
				}

				if(n <= 0)
				{
					n = 1;
				}

				text_end = text_base + text_size;

				do
				{
					ref = xref64(macho, text_base, text_end, to);

					if(!ref)
						return 0;

					text_base = ref + sizeof(uint32_t);

				} while(--n > 0);

				return ref;
			}

			mach_vm_address_t findDataReference(MachO *macho, mach_vm_address_t to, enum data which_data, int n)
			{
				Segment *segment;

				struct segment_command_64 *segment_command;

				mach_vm_address_t start;
				mach_vm_address_t end;

				segment = NULL;
				segment_command = NULL;

				switch(which_data)
				{
					case __DATA_CONST:

						if((segment = macho->getSegment("__DATA_CONST")))
						{
							segment_command = segment->getSegmentCommand();
						}

						break;
					case __PPLDATA_CONST:

						if((segment = macho->getSegment("__PPLDATA_CONST")))
						{
							segment_command = segment->getSegmentCommand();
						}

						break;
					case __PPLDATA:
						
						if((segment = macho->getSegment("__PPLDATA")))
						{
							segment_command = segment->getSegmentCommand();
						}

						break;
					case __DATA:
						
						if((segment = macho->getSegment("__DATA")))
						{
							segment_command = segment->getSegmentCommand();
						}

						break;
					case __BOOTDATA:
						
						if((segment = macho->getSegment("__BOOTDATA")))
						{
							segment_command = segment->getSegmentCommand();
						}

						break;
					case __PRELINK_DATA:
						
						if((segment = macho->getSegment("__PRELINK_DATA")))
						{
							segment_command = segment->getSegmentCommand();
						}

						break;
					case __PLK_DATA_CONST:
						
						if((segment = macho->getSegment("__PLK_DATA_CONST")))
						{
							segment_command = segment->getSegmentCommand();
						}

						break;
					default:
						segment = NULL;

						segment_command = NULL;

						return 0;
				}

				if(!segment || !segment_command)
					return 0;

				start = segment_command->vmaddr;
				end = segment_command->vmaddr + segment_command->vmsize;

				for(mach_vm_address_t i = start; i <= end; i += sizeof(uint16_t))
				{
					mach_vm_address_t ref = *reinterpret_cast<mach_vm_address_t*>(i);

					if(ref == to)
					{
						return i;
					}
				}

				return 0;
			}

			uint8_t* findString(MachO *macho, char *string, mach_vm_address_t base, mach_vm_address_t size, bool full_match)
			{
				uint8_t *find;

				mach_vm_address_t offset = 0;

				while((find = boyermoore_horspool_memmem(reinterpret_cast<unsigned char*>(base + offset), size - offset, (uint8_t *)string, strlen(string))))
				{
					if((find == reinterpret_cast<unsigned char*>(base)|| *(string - 1) == '\0') && (!full_match || strcmp((char*)find, string) == 0))
						break;

					offset = (uint64_t) (find -  base + 1);
				}

				return find;
			}

			mach_vm_address_t findStringReference(MachO *macho, char *string, int n, enum string which_string, enum text which_text, bool full_match)
			{
				Segment *segment;
				Section *section;

				uint8_t *find;

				mach_vm_address_t base;

				size_t size = 0;

				switch(which_string)
				{
					case __const_:

						segment = macho->getSegment("__TEXT");

						if(segment)
						{
							section = macho->getSection("__TEXT", "__const");

							if(section)
							{
								struct section_64 *sect = section->getSection();

								base = sect->addr;
								size = sect->size;
							}
						}

						break;
					case __data_:

						segment = macho->getSegment("__DATA");

						if(segment)
						{
							section = macho->getSection("__DATA", "__data");

							if(section)
							{
								struct section_64 *sect = section->getSection();

								base = sect->addr;
								size = sect->size;
							}
						}

						break;
					case __oslstring_:

						segment = macho->getSegment("__TEXT");

						if(segment)
						{
							section = macho->getSection("__TEXT", "__os_log");

							if(section)
							{
								struct section_64 *sect = section->getSection();

								base = sect->addr;
								size = sect->size;
							}
						}

						break;
					case __pstring_:

						segment = macho->getSegment("__TEXT");

						if(segment)
						{
							section = macho->getSection("__TEXT", "__text");

							if(section)
							{
								struct section_64 *sect = section->getSection();

								base = sect->addr;
								size = sect->size;
							}
						}

						break;
					case __cstring_:

						segment = macho->getSegment("__TEXT");

						if(segment)
						{
							section = macho->getSection("__TEXT", "__cstring");

							if(section)
							{
								struct section_64 *sect = section->getSection();

								base = sect->addr;
								size = sect->size;
							}
						}

						break;
					default:
						break;
				}

				if(!base && !size)
					return 0;

				find = findString(macho, string, base, size, full_match);


				if(!find)
					return 0;

				return Arch::x86_64::PatchFinder::findReference(macho, (mach_vm_address_t) find, n, which_text);
			}

			void printInstruction64(MachO *macho, mach_vm_address_t start, uint32_t length, char *mnemonic, char *op_string)
			{
				
			}

		}
	}
}