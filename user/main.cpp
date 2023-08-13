#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <getopt.h>

#include <sys/sysctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h> 

#include <mach/mach.h> 
#include <mach/exc.h>

#include <pthread.h>
#include <ptrauth.h>

#include <algorithm>
#include <array>
#include <bsm/libbsm.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dispatch/dispatch.h>
#include <functional>
#include <iostream>
#include <regex>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <vector>

#include <IOKit/IOKitLib.h>

#include <EndpointSecurity/EndpointSecurity.h>

#include "Kernel.hpp"
#include "Dwarf.hpp"
#include "UserMachO.hpp"
#include "Task.hpp"
#include "Dyld.hpp"
#include "PAC.hpp"

#include <arm64/PatchFinder_arm64.hpp>

using namespace std;

using namespace mrk;
using namespace dyld;

#define STACK_SIZE ((1024 * 1024) * 512)

#define ROP_ret "\xff\x0f\x5f\xd6"
#define ALIGNSIZE 8
#define align64(x) ( ((x) + ALIGNSIZE - 1) & ~(ALIGNSIZE - 1) )

char injectedCode[] =
	"\xff\xc3\x00\xd1" 		//    SUB         SP,  SP, 0x30
	"\xfd\x7b\x02\xa9" 		//    STP         X29, X30, [SP, 0x20]
	"\xe3\x13\x01\xa9" 		//    STP         X3,  X4,  [SP, 0x10]
	"\xe1\x0b\x00\xa9" 		//    STP         X1,  X2,  [SP, 0x0]
	"\xfd\x83\x00\x91" 		//    ADD         X29, SP, 0x20
	"\x00\x00\x00\x90" 		//    ADRP        X0,  0x0
	"\x00\x02\x00\x10"		//    ADR         X0,  0x40
	"\x41\x20\x80\xd2" 		//    MOV         X1,  0x102
	"\x03\x00\x00\x90"		//    ADRP        X3,  0x00
	"\x63\x06\x00\x10"		//    ADR         X3,  0xcc
	"\x64\x00\x40\xf9" 		//    LDR         X4,  [X3]
	"\x80\x00\x3f\xd6" 		//    BLR         X4
	"\x03\x00\x00\x90" 		//    ADRP        X3, 0x0
	"\x23\x06\x00\x10" 		//    ADR         X3, 0xc4
	"\x64\x00\x40\xf9" 		//    LDR         X4,  [X3]
	"\x80\x00\x3f\xd6" 		//    BLR         X4
	"\x1f\x20\x03\xd5" 		//    NOP
	"\xe1\x0b\x40\xa9" 		//    LDP         X1,  X2,  [SP, 0x0]
	"\xe3\x13\x41\xa9" 		//    LDP         X3,  X4,  [SP, 0x10]
	"\xfd\x7b\x42\xa9" 		//    LDP         X29, X30, [SP, 0x20]
	"\xff\xc3\x00\x91" 		//    ADD         SP,  SP, 0x30
	"\xc0\x03\x5f\xd6" 		//    RET


	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";

char injectedCode_x86_64[] =
	"\x55"										// 		push rbp
	"\x48\x89\xec"								// 		mov rsp, rbp
	"\x50"										// 		push rax
	"\x57"										//		push rdi
	"\x56"										//		push rsi
	"\x52"										// 		push rdx
	"\x51"										// 		push rcx
	"\x48\x8d\x3d\x40\x00\x00\x00"				// 		lea rdi, [rip + 0x40]
	"\x48\xc7\xc6\x02\x01\x00\x00"				//  	mov rsi, 0x102
	"\x48\xbb\xff\xff\xff\xff\xff\xff\xff\xff"  //      mov rax, 0xffffffffffffffff
	"\xff\xd0"									//		call rax
	"\x48\xbb\xff\xff\xff\xff\xff\xff\xff\xff"	// 		mov, rax 0xffffffffffffffff
	"\xff\xd0"									// 		call rax
	"\x59"										// 		pop rcx
	"\x5a"										// 		pop rdx
	"\x5e"										//		pop rsi
	"\x5f"										// 		pop rdi
	"\x58"										// 		pop rax
	"\x48\x89\xe5"								//		mov rbp, rsp
	"\x5d"										// 		pop rbp
	"\xc3";										// 		ret
	
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";


Kernel *kernel;

Task *task;

MachO *libDyld;
MachO *libSystemPthread;

mach_vm_address_t dlopen;
mach_vm_address_t dlerror;

mach_vm_address_t pthread_create_from_mach_thread;
mach_vm_address_t gadget_address;

vm_address_t remote_stack;
vm_address_t remote_code;
vm_address_t remote_data;

thread_t thread;
pthread_t pthread;

arm_thread_state64_t state = {0};

mach_msg_type_number_t stateCount = ARM_THREAD_STATE64_COUNT;

static uint64_t ropcall(mach_vm_address_t function, char *argMap, uint64_t *arg1, uint64_t *arg2, uint64_t *arg3, uint64_t *arg4)
{
	kern_return_t kret;

#ifndef __arm64e__
	state.__pc = (uint64_t) ptrauth_sign_unauthenticated((void*) function, ptrauth_key_process_independent_code, ptrauth_string_discriminator("pc"));

	thread_convert_thread_state(thread, THREAD_CONVERT_THREAD_STATE_FROM_SELF, ARM_THREAD_STATE64, reinterpret_cast<thread_state_t>(&state), stateCount, reinterpret_cast<thread_state_t>(&state), &stateCount);

	state.__lr = gadget_address;
	state.__sp = ((remote_stack + STACK_SIZE) - (STACK_SIZE / 4));
	state.__fp = state.__sp;
#else
	state.__opaque_pc = (void*) ptrauth_sign_unauthenticated((void*) function, ptrauth_key_process_independent_code, ptrauth_string_discriminator("pc"));

	state.__opaque_lr = (void*) ptrauth_sign_unauthenticated((void*)gadget_address, 0, ptrauth_string_discriminator("lr"));
	state.__opaque_sp = (void*) ptrauth_sign_unauthenticated((void*)((remote_stack + STACK_SIZE) - (STACK_SIZE / 4)), ptrauth_key_asda, ptrauth_string_discriminator("sp"));
	state.__opaque_fp = (void*) ptrauth_sign_unauthenticated((void*)((remote_stack + STACK_SIZE) - (STACK_SIZE / 4)), ptrauth_key_asda, ptrauth_string_discriminator("fp"));;

	thread_convert_thread_state(thread, THREAD_CONVERT_THREAD_STATE_FROM_SELF, ARM_THREAD_STATE64, reinterpret_cast<thread_state_t>(&state), stateCount, reinterpret_cast<thread_state_t>(&state), &stateCount);
#endif

	char *local_fake_stack = (char *)malloc((size_t) STACK_SIZE);

	char *argp = (char *)argMap;

	char *stack_ptr = local_fake_stack;

	uint64_t paramLen = 0;

	for(int param = 0; param <= 4; param++)
	{
		if(!(*argp))
			break;

		switch(*argp)
		{
			case 's':
				;

				int num_digits;

				char tmp_buf[6];

				argp++;

				num_digits = 0;

				while(*argp >= '0' && *argp <= '9')
				{
					if(++num_digits == 6)
					{
						fprintf(stderr, "String too long, param=%d\n", param);

						return 0;
					}

					tmp_buf[num_digits-1] = *(argp++);
				}

				tmp_buf[num_digits] = 0;

				paramLen = strtoull(tmp_buf, NULL, 10);

				uint64_t *argPtr;

				if(param==0)
					argPtr = arg1;
				if(param==1)
					argPtr = arg2;
				if(param==2)
					argPtr = arg3;
				if(param==3)
					argPtr = arg4;

				memcpy(stack_ptr, argPtr, paramLen);

				state.__x[param] = (uint64_t)remote_stack + (stack_ptr - local_fake_stack);
				stack_ptr += 16;
				stack_ptr += paramLen;
				stack_ptr = (char *)align64((uint64_t)stack_ptr);

				break;

			case 'u':
				;

				state.__x[param] = (param==0) ? (uint64_t)arg1: (param==1) ? (uint64_t)arg2 : (param==2) ? (uint64_t)arg3 : (uint64_t)arg4;
				
				argp++;
				
				break;

			default:
				;

				fprintf(stderr, "Unknown argument type: '%c'\n", *argp);
				
				exit(-1);
		}
	}

	kret = vm_write(task->getTaskPort(), remote_stack, (vm_address_t)local_fake_stack, STACK_SIZE);
	
	free(local_fake_stack);

	if(kret != KERN_SUCCESS)
	{
		fprintf(stderr, "Unable to copy fake stack to target process! %s\n", mach_error_string(kret));

		exit(-1);
	}

	printf("Calling function at %p...\n", (void *)function);

	kret = thread_set_state(thread, ARM_THREAD_STATE64, (thread_state_t)&state, ARM_THREAD_STATE64_COUNT);

	if(kret != KERN_SUCCESS)
	{
		fprintf(stderr, "Could not set thread state! %s\n", mach_error_string(kret));

		exit(-1);
	}

	thread_resume(thread);

	while(1)
	{
		usleep(250000);

		thread_get_state(thread, ARM_THREAD_STATE64, (thread_state_t)&state, &stateCount);

	#ifdef __arm64e__
		if(ptrauth_strip(state.__opaque_pc, ptrauth_key_process_independent_code) == (void*) gadget_address)
	#else
		if(state.__pc == gadget_address)
	#endif
		{
			printf("Returned from function!\n");

			thread_suspend(thread);

			break;
		}
	}

	return (uint64_t)state.__x[0];
}

static void* find_gadget(const char *gadget, int gadget_len)
{
	kern_return_t kr;

	vm_size_t size = 0;

	size = 65536;

	char *buf = (char*) malloc(size);

	char *orig_buf = buf;

	if(!buf)
	{
		fprintf(stderr, "Error allocating memory!\n");

		return NULL;
	}

	kr = vm_read_overwrite(task->getTaskPort(), dlopen, size, (vm_address_t)buf, &size);

	if(kr != KERN_SUCCESS)
	{
		fprintf(stderr, "Could not read RX pages!\n");

		free(orig_buf);

		return NULL;
	}

	while(buf < orig_buf + size)
	{
		char *ptr = (char *) memmem((const void *)buf, (size_t)size - (size_t)(buf - orig_buf), (const void *)gadget, (size_t)gadget_len);
	
		if(ptr)
		{
			vm_size_t offset = (vm_size_t) (ptr - orig_buf);

			vm_address_t gadget_addr_real = dlopen + offset;
			
			if(((uint64_t)gadget_addr_real % 8) == 0)
			{
				free(orig_buf);

				return (void *)gadget_addr_real;
			}
			else
			{
				buf = ptr + gadget_len;
			}
		}
	}

	free(orig_buf);

	return NULL;
}

int injectLibrary(char *dylib)
{
	kern_return_t kr;

	int err;

	libDyld = task->getDyld()->cacheDumpImage("libdyld.dylib");

	dlopen = libDyld->getSymbolAddressByName("_dlopen") + libDyld->getAslrSlide();

	dlerror = libDyld->getSymbolAddressByName("_dlerror") + libDyld->getAslrSlide();

	libSystemPthread = task->getDyld()->cacheDumpImage("libsystem_pthread.dylib");

	pthread_create_from_mach_thread = libSystemPthread->getSymbolAddressByName("_pthread_create_from_mach_thread") + libSystemPthread->getAslrSlide();

	printf("dlopen = 0x%llx\n", dlopen);

	printf("pthread_create_from_mach_thread = 0x%llx\n", pthread_create_from_mach_thread);

	if((kr = thread_create(task->getTaskPort(), &thread)) != KERN_SUCCESS)
	{
		fprintf(stderr, "Could not create new thread in task!\n");

		return -1;
	}

	gadget_address = reinterpret_cast<mach_vm_address_t>(find_gadget(ROP_ret, 4));

	if(!gadget_address)
	{
		fprintf(stderr, "Failed to find gadget address!\n");

		return 0;
	}

	fprintf(stdout, "Found gadget at 0x%llx\n", gadget_address);

	kr = vm_allocate(task->getTaskPort(), &remote_stack, STACK_SIZE, VM_FLAGS_ANYWHERE);

	if (kr != KERN_SUCCESS)
	{
		printf("Unable to allocate memory! %s\n", mach_error_string(kr));
		
		return -1;
	}

	kr = vm_protect(task->getTaskPort(), remote_stack, STACK_SIZE, FALSE, VM_PROT_READ| VM_PROT_WRITE);
	
	if(kr != KERN_SUCCESS)
	{
		printf("Unable to protect memory! %s\n", mach_error_string(kr));
		
		return -1;
	}

	kr = vm_allocate(task->getTaskPort(), (vm_address_t*) &remote_data, 16192, VM_FLAGS_ANYWHERE);

	if (kr != KERN_SUCCESS)
	{
		printf("Unable to allocate memory! %s\n", mach_error_string(kr));
		
		return -1;
	}

	kr = vm_protect(task->getTaskPort(), remote_data, 16192, FALSE, VM_PROT_READ| VM_PROT_WRITE);
	
	if(kr != KERN_SUCCESS)
	{
		printf("Unable to protect memory! %s\n", mach_error_string(kr));
		
		return -1;
	}

	kr = vm_allocate(task->getTaskPort(), (vm_address_t*) &remote_code, 16192, VM_FLAGS_ANYWHERE);

	if (kr != KERN_SUCCESS)
	{
		printf("Unable to allocate memory! %s\n", mach_error_string(kr));
		
		return -1;
	}

	kr = vm_protect(task->getTaskPort(), remote_code, 16192, FALSE, VM_PROT_READ| VM_PROT_WRITE);
	
	if(kr != KERN_SUCCESS)
	{
		printf("Unable to protect memory! %s\n", mach_error_string(kr));
		
		return -1;
	}

	bool lib = false;
	bool libaddr = false;

	char *injected_code = reinterpret_cast<char*>(malloc(sizeof(injectedCode)));

	memcpy(injected_code, injectedCode, sizeof(injectedCode));

	for(uint32_t i = 0; i < sizeof(injectedCode); i++)
	{
		char *zeros = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
		char *ones   = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";

		char *library = dylib;

		if(lib && libaddr)
			break;

		if(!lib && memcmp(&injectedCode[i], zeros, 20) == 0)
		{
			lib = true;

			strlcpy(&injected_code[i], library, strlen(library) + 1);

			i += strlen(library);
		}

		if(!libaddr && memcmp(&injectedCode[i], ones, sizeof(uint64_t)) == 0)
		{
			libaddr = true;

			memcpy(&injected_code[i], &dlopen, sizeof(uint64_t));
			memcpy(&injected_code[i] + sizeof(uint64_t), &dlerror, sizeof(uint64_t));

			i += sizeof(uint64_t);
		}
	}

	kr = vm_write(task->getTaskPort(), remote_code, (vm_address_t) injected_code, sizeof(injectedCode));

	if(kr != KERN_SUCCESS)
	{
		printf("Could not write injected code to remote code!\n");

		return -1;
	} 

	kr = vm_protect(task->getTaskPort(), remote_code, 16192, FALSE, VM_PROT_READ| VM_PROT_EXECUTE);
	
	if(kr != KERN_SUCCESS)
	{
		printf("Unable to protect memory! %s\n", mach_error_string(kr));
		
		return -1;
	}

	mach_vm_address_t pthread = remote_stack;

#ifdef __arm64e__
	state.__opaque_pc = ptrauth_sign_unauthenticated((void*) remote_code, ptrauth_key_process_independent_code, 0);

	remote_code = reinterpret_cast<mach_vm_address_t>(state.__opaque_pc);
#endif

	ropcall(pthread_create_from_mach_thread, (char *)"uuuu", (uint64_t*) pthread, NULL, (uint64_t*) remote_code, NULL );

	sleep(5);

	thread_suspend(thread);

	thread_terminate(thread);

	vm_deallocate(task->getTaskPort(), remote_stack, STACK_SIZE);

	free(injected_code);

	memset(&state, 0x0, sizeof(state));

	return 0;
}

static struct option long_options[] =
{
	{"pid", required_argument, 0, 'p'},
	{"wait_for_process", required_argument, 0, 'w'}
};

void print_usage()
{
	printf("mrk_inject -p <pid> -w <process_name>libs\n");

	exit(-1);
}

int main(int argc, char **argv)
{
	int err;

	char *wait_for_process_name = NULL;

	char *process_name;

	int pid = -1;

	int c;

	kernel = new Kernel();

	printf("Kernel base = 0x%llx slide = 0x%llx\n", kernel->getBase(), kernel->getSlide());

	/*
	task = new Task(kernel, 614);

	mach_vm_address_t ASD = task->getDyld()->getImageLoadedAt("AppStoreDaemon", NULL);

	printf("AppStoreDaemon loaded at 0x%llx\n", ASD);

	MachO *AppStoreDaemon = task->getDyld()->cacheDumpImage("AppStoreDaemon");

	return 0;

	UserMachO *AS = new UserMachO("/Users/ilhanraja/Downloads/Files/Work/AppStore.app/Contents/MacOS/AppStore_");

	return 0;
	*/

	/*
	using namespace Debug;

	Dwarf *dwarf = new Dwarf("/Library/Developer/KDKs/KDK_12.5_21G72.kdk/System/Library/Kernels/kernel.development.t8020.dSYM/Contents/Resources/DWARF/kernel.development.t8020");


	return 0;
	*/

	while(1)
	{
		int option_index = 0;

		c = getopt_long (argc, argv, "p:w:", long_options, &option_index);

		if (c == -1)
			break; 

		switch(c)
		{
			case 'p':
				pid = atoi(optarg);

				break;
			case 'w':
				wait_for_process_name = optarg;

				break;

			default:
				break;
		}
	}

	if(pid <= 0)
	{
		print_usage();
	}

	if(pid)
	{
		task = new Task(kernel, pid);
	}

	if(!task)
	{
		print_usage();
	}

	int argi = optind;

	printf("PID = %d task = 0x%llx proc = 0x%llx\n", task->getPid(), task->getTask(), task->getProc());

	while(argi < argc)
	{
		char *library = argv[argi];

		mach_vm_address_t libraryLoadedAt = task->getDyld()->getImageLoadedAt(library, NULL);
	
		if(!libraryLoadedAt)
		{
			if(wait_for_process_name)
			{
				es_client_t *client = NULL;

				ensure(es_new_client(&client, ^(es_client_t *client, const es_message_t *message)
				{
					switch (message->event_type)
					{
						case ES_EVENT_TYPE_AUTH_EXEC:
						{
							const char *name = message->event.exec.target->executable->path.data;

							for (const auto &process : processes)
							{
								pid_t pid = audit_token_to_pid(message->process->audit_token);
								
								if (std::regex_search(name, process) && is_translated(getpid()) == is_translated(pid))
								{
									if (is_cs_enforced(pid))
									{
										ensure(!ptrace(PT_ATTACHEXC, pid, nullptr, 0));
										// Work around FB9786809
										dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1'000'000'000), dispatch_get_main_queue(), ^
										{
											ensure(!ptrace(PT_DETACH, pid, nullptr, 0));
										});
									}

									task = new Task(kernel, pid);

									inject(library);
								}
							}
							es_respond_auth_result(client, message, ES_AUTH_RESULT_ALLOW, false);
							break;
						}
						default:
							ensure(false && "Unexpected event type!");
					}
				}) == ES_NEW_CLIENT_RESULT_SUCCESS);
				
				es_event_type_t events[] = {ES_EVENT_TYPE_AUTH_EXEC};
				
				ensure(es_subscribe(client, events, sizeof(events) / sizeof(*events)) == ES_RETURN_SUCCESS);
				
				dispatch_main();
			} else
			{
				err = injectLibrary(library);

				if(err != 0)
				{
					return err;
				}

				libraryLoadedAt = task->getDyld()->getImageLoadedAt(library, NULL);
			}
		}

		printf("%s loaded at 0x%llx\n", library, libraryLoadedAt);

		argi++;
	}

	delete task;

	delete kernel;

	return err;
}