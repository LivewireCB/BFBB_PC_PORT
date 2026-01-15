#include "iMemMgr.h"

#include "xMemMgr.h"
#include "xDebug.h"

#include <stdlib.h>
#include <windows.h>

#define HEAP_SIZE 0x384000

static HANDLE TheHeap;

void iMemInit()
{
    ULONG_PTR StackLo;
    ULONG_PTR StackHi;
    GetCurrentThreadStackLimits(&StackLo, &StackHi);

    TheHeap = HeapCreate(0, HEAP_SIZE, 0);
    if (!TheHeap) {
        xASSERTFAILMSG("Failed to create main heap!");
        exit(-5);
    }

    xMemAddr HeapBase = (xMemAddr)HeapAlloc(TheHeap, 0, HEAP_SIZE);
    if (!HeapBase) {
        xASSERTFAILMSG("Failed to allocate main heap memory!");
        exit(-5);
    }

    gMemInfo.system.addr = 0;
    gMemInfo.system.size = 16;
    gMemInfo.system.flags = XMEMAREA_0x20;
    gMemInfo.stack.addr = (xMemAddr)StackLo;
    gMemInfo.stack.size = (U32)(StackHi - StackLo);
    gMemInfo.stack.flags = XMEMAREA_0x20 | XMEMAREA_0x800;
    gMemInfo.DRAM.addr = HeapBase;
    gMemInfo.DRAM.size = HEAP_SIZE;
    gMemInfo.DRAM.flags = XMEMAREA_0x20 | XMEMAREA_0x800;
    gMemInfo.SRAM.addr = 0;
    gMemInfo.SRAM.size = 32;
    gMemInfo.SRAM.flags = XMEMAREA_0x20 | XMEMAREA_0x40 | XMEMAREA_0x200 | XMEMAREA_0x400;
}

void iMemExit()
{
    HeapFree(TheHeap, 0, (void*)gMemInfo.DRAM.addr);
    gMemInfo.DRAM.addr = 0;

    HeapDestroy(TheHeap);
}


//#include "iMemMgr.h"
//#include "iSystem.h"
//#include "xMemMgr.h"
//
//#include <types.h>
//#include <PowerPC_EABI_Support/MSL_C/MSL_Common/stdlib.h>
//#include <dolphin.h>
//
//U32 mem_top_alloc;
//U32 mem_base_alloc;
//volatile OSHeapHandle the_heap;
//OSHeapHandle hs;
//OSHeapHandle he;
//U32 HeapSize;
//extern xMemInfo_tag gMemInfo;
//extern unsigned char _stack_addr[];
//
//// Starts going wrong after the if and else statement, everything else before looks fine.
//void iMemInit()
//{
//    OSHeapHandle hi = (OSHeapHandle)OSGetArenaHi();
//    he = hi & 0xffffffe0;
//    hs = (OSHeapHandle)OSGetArenaLo() + 0x1f & 0xffffffe0;
//    the_heap = OSCreateHeap(OSInitAlloc((void*)hs, (void*)(hi & 0xffffffe0), 1), (void*)he);
//    OSHeapHandle currHeap = the_heap;
//    if (currHeap >= 0)
//    {
//        OSSetCurrentHeap(currHeap);
//    }
//    else
//    {
//        exit(-5);
//    }
//    gMemInfo.system.addr = 0;
//    gMemInfo.system.size = 0x100000;
//    gMemInfo.system.flags = 0x20;
//    gMemInfo.stack.addr = (U32)&_stack_addr;
//    gMemInfo.stack.size = 0xffff8000;
//    gMemInfo.stack.flags = gMemInfo.DRAM.flags = 0x820;
//    HeapSize = 0x384000;
//    gMemInfo.DRAM.addr = (U32)OSAllocFromHeap(__OSCurrHeap, 0x384000);
//    gMemInfo.DRAM.size = HeapSize;
//    gMemInfo.DRAM.flags = 0x820;
//    gMemInfo.SRAM.addr = 0;
//    gMemInfo.SRAM.size = 0x200000;
//    gMemInfo.SRAM.flags = 0x660;
//    mem_top_alloc = gMemInfo.DRAM.addr + HeapSize;
//    mem_base_alloc = gMemInfo.DRAM.addr;
//}
//
//void iMemExit()
//{
//    free((void*)gMemInfo.DRAM.addr);
//    gMemInfo.DRAM.addr = 0;
//}
