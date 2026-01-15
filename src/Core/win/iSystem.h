#ifndef ISYSTEM_H
#define ISYSTEM_H

#include <types.h>

void iVSync();

#define GET_MAKER_CODE() (*((U32*)0x80000004))
#define GET_BUS_FREQUENCY() (*((U32*)0x800000F8))

void** psGetMemoryFunctions();
U16 my_dsc(U16 dsc);

void iSystemInit(U32 options);
void iSystemExit();
void iSystemPollEvents();
//void MemoryProtectionErrorHandler(U16 last, OSContext* ctx, U64 unk1, U64 unk2);
//void FloatingPointErrorHandler(U16 last, OSContext* ctxt, U64 unk1, U64 unk2);
void TRCInit();
U32 iSystemShouldQuit();

void null_func();

#endif
