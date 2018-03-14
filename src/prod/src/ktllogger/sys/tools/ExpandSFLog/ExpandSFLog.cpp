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

static const ULONGLONG SIGNATURE = 0x1234abcd9876;  
typedef struct
{
    ULONGLONG Signature;
    ULONGLONG FileSize;                         // This is the size of the consolidated file
    ULONGLONG NominalFileSize;                  // This is the original sparse file size
    ULONG HeaderSize;
    ULONG AllocationCount;
    KBlockFile::AllocatedRange Allocations[1];  // Dynamically size at AllocationCount

} ConsolidatedHeader;


void Usage()
{
    printf("ExpandSFLog - consolidate and expand ktllogger log files\n");
    printf("\n");
    printf("       -f:<fully qualified input pathname>\n");
    printf("       -o:<fully qualified output pathname>\n");
    printf("       -p:<operation to perform: Consolidate, Expand or View>\n");
    printf("\n");
}

typedef enum
{
    Undefined = 0,
    Consolidate = 1,
    Expand = 2,
    ViewOnly = 3
} TaskOperation;

typedef struct
{
    PWCHAR InFileName;
    PWCHAR OutFileName;
    TaskOperation Operation;
} COMMAND_OPTIONS, *PCOMMAND_OPTIONS;

ULONG ParseCommandLine(__in int argc, __in wchar_t** args, __out PCOMMAND_OPTIONS Options)
{
    Options->InFileName = NULL;
    Options->OutFileName = NULL;
    Options->Operation = TaskOperation::Undefined;

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

            case L'p':
            {
                WCHAR operation = (WCHAR)tolower(arg[3]);

                if (operation == L'c')
                {
                    Options->Operation = TaskOperation::Consolidate;
                } else if (operation == L'e') {
                    Options->Operation = TaskOperation::Expand;
                } else if (operation == L'v') {
                    Options->Operation = TaskOperation::ViewOnly;
                } else {
                    Usage();
                    return(ERROR_INVALID_PARAMETER);
                }
                break;
            }

            default:
            {
                Usage();
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    if (Options->Operation == TaskOperation::Undefined)
    {
        Usage();
        return(ERROR_INVALID_PARAMETER);        
    }
    
    if ((Options->Operation == TaskOperation::ViewOnly) &&
        (Options->InFileName == NULL))
    {
        printf("%d: ", __LINE__);
        Usage();
        return(ERROR_INVALID_PARAMETER);        
    }

    if ((! (Options->Operation == TaskOperation::ViewOnly)) && 
        ((Options->InFileName == NULL) || (Options->OutFileName == NULL)))
    {
        printf("%d: ", __LINE__);
        Usage();
        return(ERROR_INVALID_PARAMETER);        
    }
    
    return(ERROR_SUCCESS);
}

ktl::Task ViewOnlyTask(
    __in COMMAND_OPTIONS& Options,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    KFinally([&](){  Acs.SetResult(STATUS_SUCCESS); });

    NTSTATUS status;
    KBlockFile::SPtr file;
    ULONGLONG fileSize;
    ULONGLONG dataSize;
    KArray<KBlockFile::AllocatedRange> allocations(*g_Allocator);
    
    KWString inFileName(*g_Allocator);
#if !defined(PLATFORM_UNIX)
    inFileName = L"\\??\\";
#endif
    inFileName += Options.InFileName;   
    VERIFY_IS_TRUE(NT_SUCCESS(inFileName.Status()));

    ULONG createOptions = static_cast<ULONG>(KBlockFile::eReadOnly) |
                          static_cast<ULONG>(KBlockFile::eShareRead) |
                          static_cast<ULONG>(KBlockFile::eShareWrite);
        
    status = co_await KBlockFile::CreateAsync(inFileName,
                                                        FALSE,          // WriteThrough
                                                        KBlockFile::eOpenExisting,
                                                        static_cast<KBlockFile::CreateOptions>(createOptions),
                                                        file,
                                                        nullptr,       // Parent
                                                        *g_Allocator,
                                                        KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
#if !defined(PLATFORM_UNIX)
        printf("Failed to open %ws due to error 0x%x\n", Options.InFileName, status);
#else
        printf("Failed to open %ws due to error 0x%x\n", Utf16To8(Options.InFileName).c_str(), status);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
#if !defined(PLATFORM_UNIX)
    if (! file->IsSparseFile())
    {
        printf("Expect %ws to be a sparse file\n", Options.InFileName);
        co_return;
    }
#else
	status = file->SetSparseFile(TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif

    fileSize = file->QueryFileSize();
    status = co_await file->QueryAllocationsAsync(0,
                                                  fileSize,
                                                  allocations,
                                                  nullptr);           // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dataSize = 0;
    for (ULONG i = 0; i < allocations.Count(); i++)
    {
        dataSize += allocations[i].Length;
    }

#if !defined(PLATFORM_UNIX)
    printf("%ws is nominal size %lld and data size %lld in %d allocations\n",
           Options.InFileName, fileSize, dataSize, allocations.Count());
#else
    printf("%s is nominal size %lld and data size %lld in %d allocations\n",
           Utf16To8(Options.InFileName).c_str(), fileSize, dataSize, allocations.Count());
#endif    
    for (ULONG i = 0; i < allocations.Count(); i++)
    {
        printf("    Allocation %d: Offset 0x%16llx Length 0x%16llx\n", i, allocations[i].Offset, allocations[i].Length);
    }
    printf("\n");   
}

VOID ViewItOnly(
    __in COMMAND_OPTIONS& Options
    )
{
    NTSTATUS status;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs;
    
    status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ViewOnlyTask(Options, *acs);
    status = SyncAwait(acs->GetAwaitable());    
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
    ULONGLONG dataSize;
    ConsolidatedHeader* outHeader;
    ULONG outHeaderSizeNeeded;
    ULONGLONG outFileSizeNeeded;
    KIoBuffer::SPtr headerIoBuffer;
    PVOID p;
    KArray<KBlockFile::AllocatedRange> allocations(*g_Allocator);
    
    KWString inFileName(*g_Allocator);
#if !defined(PLATFORM_UNIX)
    inFileName = L"\\??\\";
#endif
    inFileName += Options.InFileName;   
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
    if (! NT_SUCCESS(status))
    {
#if !defined(PLATFORM_UNIX)
        printf("Failed to open %ws due to error 0x%x\n", Options.InFileName, status);
#else
        printf("Failed to open %s due to error 0x%x\n", Utf16To8(Options.InFileName).c_str(), status);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

#if !defined(PLATFORM_UNIX)
    if (! fileIn->IsSparseFile())
    {
        printf("Expect %ws to be a sparse file\n", Options.InFileName);
        co_return;
    }
#else
	status = fileIn->SetSparseFile(TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
    fileSize = fileIn->QueryFileSize();
    status = co_await fileIn->QueryAllocationsAsync(0,
                                                    fileSize,
                                                    allocations,
                                                    nullptr);           // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dataSize = 0;
    for (ULONG i = 0; i < allocations.Count(); i++)
    {
        dataSize += allocations[i].Length;
    }

#if !defined(PLATFORM_UNIX)
    printf("%ws is nominal size %lld and data size %lld in %d allocations\n",
           Options.InFileName, fileSize, dataSize, allocations.Count());
#else
    printf("%s is nominal size %lld and data size %lld in %d allocations\n",
           Utf16To8(Options.InFileName).c_str(), fileSize, dataSize, allocations.Count());
#endif

    //
    // Build the header in the consolidated file
    //      
    outHeaderSizeNeeded = RoundUpTo4K(sizeof(ConsolidatedHeader) + (allocations.Count() * sizeof(KBlockFile::AllocatedRange)));
    status = KIoBuffer::CreateSimple(outHeaderSizeNeeded, headerIoBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    outFileSizeNeeded = outHeaderSizeNeeded + dataSize;
    
    outHeader = (ConsolidatedHeader*)p;
    outHeader->Signature = SIGNATURE;
    outHeader->FileSize = outFileSizeNeeded;
    outHeader->NominalFileSize = fileSize;
    outHeader->AllocationCount = allocations.Count();
    outHeader->HeaderSize = outHeaderSizeNeeded;
    
    KMemCpySafe(outHeader->Allocations, (allocations.Count() * sizeof(KBlockFile::AllocatedRange)),
                allocations.Data(), (allocations.Count() * sizeof(KBlockFile::AllocatedRange)));
        
    //
    // Create the outfile and write the header
    //
    KWString outFileName(*g_Allocator);
#if !defined(PLATFORM_UNIX)
    outFileName = L"\\??\\";
#endif
    outFileName += Options.OutFileName; 
    VERIFY_IS_TRUE(NT_SUCCESS(outFileName.Status()));
    
    status = co_await KBlockFile::CreateAsync(outFileName,
                                              FALSE,            // IsWriteThrough
                                              KBlockFile::eCreateAlways,
                                              static_cast<KBlockFile::CreateOptions>(
                                                      static_cast<ULONG>(KBlockFile::eAlwaysUseFileSystem) +
                                                      static_cast<ULONG>(KBlockFile::eInheritFileSecurity)),
                                              fileOut,
                                              nullptr,          // Parent
                                              *g_Allocator);
    if (! NT_SUCCESS(status))
    {
#if !defined(PLATFORM_UNIX)
        printf("Failed to open %ws due to error 0x%x\n", Options.OutFileName, status);
#else
        printf("Failed to open %ws due to error 0x%x\n", Utf16To8(Options.OutFileName).c_str(), status);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    status = co_await fileOut->SetFileSizeAsync(outFileSizeNeeded);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = co_await fileOut->TransferAsync(KBlockFile::eForeground,
                                             KBlockFile::SystemIoPriorityHint::eNormal,
                                             KBlockFile::eWrite,
                                             0,                       // Offset
                                             *headerIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Copy the contents from the sparse file into the consolidated
    // file
    //
    KIoBuffer::SPtr oneMBIoBuffer;  
    ULONGLONG destinationOffset = headerIoBuffer->QuerySize();
    ULONGLONG sourceOffset;
    ULONG oneMBWriteCount;
    ULONG lastWriteCount;
    ULONGLONG length;
    
    headerIoBuffer = nullptr;

    status = KIoBuffer::CreateSimple(oneMB, oneMBIoBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        
    for (ULONG i = 0; i < allocations.Count(); i++)
    {
        printf("    Allocation %d: Offset 0x%16llx Length 0x%16llx ....", i, allocations[i].Offset, allocations[i].Length);
        length = allocations[i].Length;
        sourceOffset = allocations[i].Offset;

        oneMBWriteCount = (ULONG)(length / oneMBL);
        lastWriteCount = (ULONG)(length - (ULONGLONG)(oneMBWriteCount * oneMB));
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
        printf(" Written\n");
    }
    printf("\n");   
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

ktl::Task ExpandTask(
    __in COMMAND_OPTIONS& Options,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{   
    KFinally([&](){  Acs.SetResult(STATUS_SUCCESS); });

    NTSTATUS status;
    KBlockFile::SPtr fileIn;
    KBlockFile::SPtr fileOut;

    ULONGLONG fileSize;
    ULONGLONG dataSize;
    ULONGLONG computedFileSize;
    ULONG expectedHeaderSize;
    ConsolidatedHeader* header;
    ULONG headerSizeNeeded;
    KIoBuffer::SPtr headerIoBuffer;
    PVOID p;
    KBlockFile::AllocatedRange* allocations;
    
    KWString inFileName(*g_Allocator);
#if !defined(PLATFORM_UNIX)
    inFileName = L"\\??\\";
#endif
    inFileName += Options.InFileName;   
    VERIFY_IS_TRUE(NT_SUCCESS(inFileName.Status()));
    
    KWString outFileName(*g_Allocator);
#if !defined(PLATFORM_UNIX)
    outFileName = L"\\??\\";
#endif
    outFileName += Options.OutFileName; 
    VERIFY_IS_TRUE(NT_SUCCESS(outFileName.Status()));
    
    status = co_await KBlockFile::CreateAsync(inFileName,
                                              FALSE,          // WriteThrough
                                              KBlockFile::eOpenExisting,
                                              fileIn,
                                              nullptr,       // Parent
                                              *g_Allocator,
                                              KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
#if !defined(PLATFORM_UNIX)
        printf("Failed to open %ws due to error 0x%x\n", Options.InFileName, status);
#else
        printf("Failed to open %s due to error 0x%x\n", Utf16To8(Options.InFileName).c_str(), status);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    headerSizeNeeded = RoundUpTo4K(sizeof(ConsolidatedHeader));
    status = KIoBuffer::CreateSimple(headerSizeNeeded, headerIoBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = co_await fileIn->TransferAsync(KBlockFile::eForeground,
                                   KBlockFile::SystemIoPriorityHint::eNormal,
                                   KBlockFile::eRead,
                                   0,                       // Offset
                                   *headerIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    header = static_cast<ConsolidatedHeader*>(p);

    //
    // Do some validations on the header
    //
    if (header->Signature != SIGNATURE)
    {
#if !defined(PLATFORM_UNIX)
        printf("Error - %ws has invalid signature 0x%llx\n", Options.InFileName, header->Signature);
#else
        printf("Error - %s has invalid signature 0x%llx\n", Utf16To8(Options.InFileName).c_str(), header->Signature);
#endif
        co_return;
    }
    
    fileSize = fileIn->QueryFileSize();
    if (fileSize != header->FileSize)
    {
#if !defined(PLATFORM_UNIX)
        printf("Error - %ws has invalid file size in header 0x%llx expecting 0x%llx\n",
               Options.InFileName, header->FileSize, fileSize);
#else
        printf("Error - %s has invalid file size in header 0x%llx expecting 0x%llx\n",
               Utf16To8(Options.InFileName).c_str(), header->FileSize, fileSize);
#endif
        co_return;
    }

    expectedHeaderSize = RoundUpTo4K( sizeof(ConsolidatedHeader) + (header->AllocationCount * sizeof(KBlockFile::AllocatedRange)) );
    if (expectedHeaderSize != header->HeaderSize)
    {
#if !defined(PLATFORM_UNIX)
        printf("Error - %ws has invalid header size 0x%x expecting 0x%x from allocation count %d\n",
               Options.InFileName, header->HeaderSize, expectedHeaderSize, header->AllocationCount);
#else
        printf("Error - %s has invalid header size 0x%x expecting 0x%x from allocation count %d\n",
               Utf16To8(Options.InFileName).c_str(), header->HeaderSize, expectedHeaderSize, header->AllocationCount);
#endif
        co_return;
    }

    //
    // If there are more allocations than will fit in first read, read them in
    // here
    //
    if (header->HeaderSize > headerSizeNeeded)
    {
        headerSizeNeeded = header->HeaderSize;
        headerIoBuffer = nullptr;
        
        status = KIoBuffer::CreateSimple(headerSizeNeeded, headerIoBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = co_await fileIn->TransferAsync(KBlockFile::eForeground,
                                       KBlockFile::SystemIoPriorityHint::eNormal,
                                       KBlockFile::eRead,
                                       0,                       // Offset
                                       *headerIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        header = static_cast<ConsolidatedHeader*>(p);
    }

    allocations = &header->Allocations[0];
    dataSize = 0;
    for (ULONG i = 0; i < header->AllocationCount; i++)
    {
        dataSize += allocations[i].Length;
    }

    computedFileSize = headerSizeNeeded + dataSize;
    if (computedFileSize != fileSize)
    {
#if !defined(PLATFORM_UNIX)
        printf("Error - %ws has invalid computed file size 0x%llx expecting 0x%llx from allocation count %d\n",
               Options.InFileName, computedFileSize, fileSize, header->AllocationCount);
#else
        printf("Error - %s has invalid computed file size 0x%x expecting 0x%x from allocation count %d\n",
               Utf16To8(Options.InFileName).c_str(), computedFileSize, fileSize, header->AllocationCount);
#endif
        co_return;
    }

#if !defined(PLATFORM_UNIX)
    printf("%ws is 0x%llx in size and has %d allocations with 0x%llx bytes of data\n",
           Options.InFileName, fileSize, header->AllocationCount, dataSize);
#else
    printf("%s is 0x%llx in size and has %d allocations with 0x%llx bytes of data\n",
           Utf16To8(Options.InFileName).c_str(), fileSize, header->AllocationCount, dataSize);
#endif

    //
    // Copy data from the consolidated file to the expanded file
    //
    status = co_await KBlockFile::CreateSparseFileAsync(outFileName,
                                                        FALSE,          // WriteThrough
                                                        KBlockFile::eCreateAlways,
                                                        fileOut,
                                                        nullptr,       // Parent
                                                        *g_Allocator,
                                                        KTL_TAG_TEST);
    if (! NT_SUCCESS(status))
    {
#if !defined(PLATFORM_UNIX)
        printf("Failed to open %ws due to error 0x%x\n", Options.OutFileName, status);
#else
        printf("Failed to open %ws due to error 0x%x\n", Utf16To8(Options.OutFileName).c_str(), status);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    status = co_await fileOut->SetFileSizeAsync(header->NominalFileSize);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Copy data from consolidated file to expanded file
    //
    KIoBuffer::SPtr oneMBIoBuffer;  
    ULONGLONG sourceOffset = headerIoBuffer->QuerySize();
    ULONGLONG destinationOffset;
    ULONG oneMBWriteCount;
    ULONG lastWriteCount;
    ULONGLONG length;
    
    status = KIoBuffer::CreateSimple(oneMB, oneMBIoBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 
        
    for (ULONG i = 0; i < header->AllocationCount; i++)
    {
        printf("    Allocation %d: Offset 0x%16llx Length 0x%16llx ....", i, allocations[i].Offset, allocations[i].Length);
        length = allocations[i].Length;
        destinationOffset = allocations[i].Offset;

        oneMBWriteCount = static_cast<ULONG>(length / oneMBL);
        lastWriteCount = static_cast<ULONG>(length - static_cast<ULONGLONG>(oneMBWriteCount * oneMB));
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
        printf(" Written\n");
    }

    
}

VOID ExpandIt(
    __in COMMAND_OPTIONS& Options
    )
{
    NTSTATUS status;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs;
    
    status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ExpandTask(Options, *acs);
    status = SyncAwait(acs->GetAwaitable());    
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

    switch(options.Operation)
    {
        case TaskOperation::ViewOnly:
        {
            ViewItOnly(options);
            break;
        }

        case TaskOperation::Consolidate:
        {
            ConsolidateIt(options);
            break;
        }

        case TaskOperation::Expand:
        {
            ExpandIt(options);
            break;
        }
        
        default:
        {
            break;
        }
    }

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
