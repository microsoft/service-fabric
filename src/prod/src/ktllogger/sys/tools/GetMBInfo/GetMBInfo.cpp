// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <KTpl.h>

#include <windows.h>

#define VERIFY_IS_TRUE(cond) if (! (cond)) { printf("Failure in file %s at line %d\n",  __FILE__, __LINE__); ExitProcess(static_cast<UINT>(-1)); }

#define CONVERT_TO_ARGS(argc, cargs) \
    std::vector<WCHAR*> args_vec(argc);\
    WCHAR** args = (WCHAR**)args_vec.data();\
    std::vector<std::wstring> wargs(argc);\
    for (int iter = 0; iter < argc; iter++)\
    {\
        wargs[iter] = Utf8To16(cargs[iter]);\
        args[iter] = (WCHAR*)(wargs[iter].data());\
    }\

static inline ULONG RoundUpTo4K(__in ULONG size)
{
    return(((size) + 0xFFF) &(~0xFFF));
}

static const ULONG oneMB = 1024 * 1024;
static const ULONGLONG oneMBL = static_cast<ULONGLONG>(oneMB);

KAllocator* g_Allocator;


void Usage()
{
    printf("GetMBInfo - Extract metadata info from ktllogger log\n");
    printf("\n");
    printf("       -f:<fully qualified input pathname>\n");
    printf("       -o:<fully qualified output pathname>\n");
    printf("\n");
}

typedef struct
{
    PWCHAR InFileName;
    PWCHAR OutFileName;
} COMMAND_OPTIONS, *PCOMMAND_OPTIONS;

ULONG ParseCommandLine(__in int argc, __in wchar_t** args, __out PCOMMAND_OPTIONS Options)
{
    Options->InFileName = NULL;
    Options->OutFileName = NULL;

    WCHAR flag;
    size_t len;
    PWCHAR arg;

    for (int i = 1; i < argc; i++)
    {
        arg = args[i];

        len = wcslen(arg);
        if (len < 3)
        {
            Usage();
            return(ERROR_INVALID_PARAMETER);
        }

        if ((arg[0] != L'-') || (arg[2] != L':'))
        {
            Usage();
            return(ERROR_INVALID_PARAMETER);
        }

        flag = (WCHAR)tolower(arg[1]);

        switch (flag)
        {
            case L'f':
            {
                Options->InFileName = arg + 3;
                break;
            }

            case L'o':
            {
                Options->OutFileName = arg + 3;
                break;
            }

            default:
            {
                Usage();
                return(ERROR_INVALID_PARAMETER);
            }
        }

    }
    
    if ((Options->InFileName == NULL) ||
        (Options->OutFileName == NULL))
    {
        Usage();
        return(ERROR_INVALID_PARAMETER);
    }
        
    return(ERROR_SUCCESS);
}

ktl::Task ConsolidateTask(
    __in COMMAND_OPTIONS& Options,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{   
    KFinally([&](){  Acs.SetResult(STATUS_SUCCESS); });

    NTSTATUS status;
    KBlockFile::SPtr fileIn;
    KBlockFile::SPtr fileOut;
    ULONGLONG fileSize;
    PVOID p;
    
    KWString inFileName(*g_Allocator);
#if !defined(PLATFORM_UNIX)
    inFileName = L"\\??\\";
#endif
    inFileName += Options.InFileName;
    inFileName += L":MBInfo";
    VERIFY_IS_TRUE(NT_SUCCESS(inFileName.Status()));
    
    ULONG createOptions = static_cast<ULONG>(KBlockFile::eReadOnly) |
                          static_cast<ULONG>(KBlockFile::eShareRead) |
                          static_cast<ULONG>(KBlockFile::eShareWrite);
    
    status = co_await KBlockFile::CreateAsync(inFileName,
                                                        FALSE,          // WriteThrough
                                                        KBlockFile::eOpenExisting,
                                                        static_cast<KBlockFile::CreateOptions>(createOptions),
                                                        fileIn,
                                                        nullptr,       // Parent
                                                        *g_Allocator,
                                                        KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    fileSize = fileIn->QueryFileSize();
    
    //
    // Create the outfile and write the header
    //
    KWString outFileName(*g_Allocator);
#if !defined(PLATFORM_UNIX)
    outFileName = L"\\??\\";
#endif
    outFileName += Options.OutFileName; 
    VERIFY_IS_TRUE(NT_SUCCESS(outFileName.Status()));

    createOptions =       static_cast<ULONG>(KBlockFile::eShareRead) |
                          static_cast<ULONG>(KBlockFile::eAlwaysUseFileSystem) |
                          static_cast<ULONG>(KBlockFile::eInheritFileSecurity);    
	
    status = co_await KBlockFile::CreateAsync(outFileName,
                                              FALSE,            // IsWriteThrough
                                              KBlockFile::eCreateAlways,
                                              static_cast<KBlockFile::CreateOptions>(createOptions),
                                              fileOut,
                                              nullptr,          // Parent
                                              *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = co_await fileOut->SetFileSizeAsync(fileSize);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Copy the contents from the sparse file into the consolidated
    // file
    //
    KIoBuffer::SPtr oneMBIoBuffer;  
    ULONGLONG destinationOffset;
    ULONGLONG sourceOffset;
    ULONG oneMBWriteCount;
    ULONG lastWriteCount;
    
    status = KIoBuffer::CreateSimple(oneMB, oneMBIoBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 
    
    oneMBWriteCount = (ULONG)(fileSize / oneMBL);
    lastWriteCount = (ULONG)(fileSize - (ULONGLONG)(oneMBWriteCount * oneMB));

    sourceOffset = 0;
    destinationOffset = 0;
    
    for (ULONG j = 0; j < oneMBWriteCount; j++)
    {
        status = co_await fileIn->TransferAsync(KBlockFile::eForeground,
                                       KBlockFile::SystemIoPriorityHint::eNormal,
                                       KBlockFile::eRead,
                                       sourceOffset,
                                       *oneMBIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = co_await fileOut->TransferAsync(KBlockFile::eForeground,
                                        KBlockFile::SystemIoPriorityHint::eNormal,
                                        KBlockFile::eWrite,
                                        destinationOffset,
                                        *oneMBIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        sourceOffset += oneMBL;
        destinationOffset += oneMBL;
    }

    if (lastWriteCount > 0)
    {
        status = co_await fileIn->TransferAsync(KBlockFile::eForeground,
                                       KBlockFile::SystemIoPriorityHint::eNormal,
                                       KBlockFile::eRead,
                                       sourceOffset,
                                       p,
                                       lastWriteCount);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = co_await fileOut->TransferAsync(KBlockFile::eForeground,
                                        KBlockFile::SystemIoPriorityHint::eNormal,
                                        KBlockFile::eWrite,
                                        destinationOffset,
                                        p,
                                        lastWriteCount);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        sourceOffset += (ULONGLONG)lastWriteCount;
        destinationOffset += (ULONGLONG)lastWriteCount;
    }
}

VOID ConsolidateIt(
    __in COMMAND_OPTIONS& Options
    )
{
    NTSTATUS status;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs1;
    
    status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs1);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    ConsolidateTask(Options, *acs1);
    status = SyncAwait(acs1->GetAwaitable());   
}


VOID Setup(
    )
{
    NTSTATUS status;
    KtlSystem* System;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    KDbgMirrorToDebugger = TRUE;

    System->SetStrictAllocationChecks(TRUE);

#ifdef UDRIVER
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif

}

VOID Cleanup(
    )
{
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
#ifdef UDRIVER
    NTSTATUS status;
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

int
TheMain(__in int argc, __in wchar_t** args)
{
    ULONG error;
    COMMAND_OPTIONS options;

    Setup();
    KFinally([&](){  Cleanup(); });
    
    error = ParseCommandLine(argc, args, &options);
    if (error != ERROR_SUCCESS)
    {
        return(error);
    }

    ConsolidateIt(options);

    return(0);
}

#if !defined(PLATFORM_UNIX)
int
wmain(int argc, WCHAR* args[])
{
    return TheMain(argc, args);
}
#else
#include <vector>
int main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs);
    return TheMain(argc, args); 
}
#endif
