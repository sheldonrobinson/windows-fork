#include <phnt_windows.h>
#include <phnt.h>
#include <setjmp.h>
#include "fork.h"

/* setjmp env for the jump back into the fork() function */
static jmp_buf jenv;

/* entry point for our child thread process - just longjmp into fork */
static int child_entry(void)
{
    longjmp(jenv, 1);
    return 0;
}

int fork(void)
{
    HANDLE hProcess = 0, hThread = 0;
    OBJECT_ATTRIBUTES oa = { sizeof(OBJECT_ATTRIBUTES) };
    MEMORY_BASIC_INFORMATION mbi;
    CLIENT_ID cid;
	INITIAL_TEB stack;
    PNT_TIB tib;
    THREAD_BASIC_INFORMATION tbi;
 
    CONTEXT context = {CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS | CONTEXT_FLOATING_POINT};
 
    if (setjmp(jenv) != 0)
    {
        return 0;    /* return as a child */
    }

    /* create forked process */
    if(!NT_SUCCESS((NTSTATUS) ZwCreateProcess(&hProcess, PROCESS_ALL_ACCESS, &oa, NtCurrentProcess(), TRUE, 0, 0, 0)))
	{
		ZwClose(hProcess);
		return -1;
	}
 
    /* set the Rip for the child process to our child function */
    if((BOOL) ZwGetContextThread(NtCurrentThread(), &context))
	{
		#if defined(_ARM64_)
			context.Pc = (DWORD64)child_entry;
		#else	
			context.Rip = (DWORD64)child_entry;
		#endif
	} else {
		ZwClose(hProcess);
		return -1;
	}

	/* get thread using the virtual memory */
	#if defined(_ARM64_)
	if(NT_SUCCESS((NTSTATUS) ZwQueryVirtualMemory(NtCurrentProcess(), (PVOID)context.Sp, MemoryBasicInformation, &mbi, sizeof mbi, 0)))
	#else
    if(NT_SUCCESS((NTSTATUS) ZwQueryVirtualMemory(NtCurrentProcess(), (PVOID)context.Rsp, MemoryBasicInformation, &mbi, sizeof mbi, 0)))
	#endif
	{
		// stack.OldInitialTeb = { .OldStackBase = 0, .OldStackLimit = 0 };
		stack.OldInitialTeb.OldStackBase = 0; stack.OldInitialTeb.OldStackLimit = 0;
		stack.StackBase = (PCHAR)mbi.BaseAddress + mbi.RegionSize;
		stack.StackLimit = mbi.BaseAddress;
		stack.StackAllocationBase = mbi.AllocationBase;
	} else {
		ZwClose(hProcess);
		return -1;
	}
	
    /* create thread using the modified context and stack */
    if(!NT_SUCCESS((NTSTATUS) ZwCreateThread(&hThread, THREAD_ALL_ACCESS, &oa, hProcess, &cid, &context, &stack, TRUE)))
	{
		/* clean up */
		ZwClose(hThread);
		ZwClose(hProcess);
		return -1;
	}
 
    /* copy exception table */
    if(NT_SUCCESS((NTSTATUS) ZwQueryInformationThread(NtCurrentThread(), ThreadPriority, &tbi, sizeof tbi, 0)))
	{
		tib = (PNT_TIB)tbi.TebBaseAddress;
	} else {
		/* clean up */
		ZwClose(hThread);
		ZwClose(hProcess);
		return -1;
	}
    
    if(!NT_SUCCESS((NTSTATUS) ZwQueryInformationThread(hThread, ThreadPriority, &tbi, sizeof tbi, 0)))
	{
		/* clean up */
		ZwClose(hThread);
		ZwClose(hProcess);
		return -1;
	}
    if(!NT_SUCCESS((NTSTATUS) ZwWriteVirtualMemory(hProcess, tbi.TebBaseAddress, &tib->ExceptionList, sizeof tib->ExceptionList, 0)))
	{
		/* clean up */
		ZwClose(hThread);
		ZwClose(hProcess);
		return -1;
	}
 
    /* start (resume really) the child */
     if(!NT_SUCCESS((NTSTATUS) ZwResumeThread(hThread, 0)))
	 {
		/* clean up */
		ZwClose(hThread);
		ZwClose(hProcess);
		return -1;
	 }
 
    /* clean up */
    ZwClose(hThread);
    ZwClose(hProcess);
 
    /* exit with child's pid */
    return (int)cid.UniqueProcess;
}
