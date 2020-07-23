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

#include <ktllogger.h>
#include <klogicallog.h>

#include <bldver.h>

#include "ktlloggertests.h"
#include "LWTtests.h"

#include "rwtstress.h"

#include "CloseSync.h"

#include "ControllerHost.h"
#include "llworkload.h"

#include "workerasync.h"

# define VERIFY_IS_TRUE(__condition, ...) if (! (__condition)) {  DebugBreak(); }

#ifdef KInvariant
#undef KInvariant
#endif
#define KInvariant(x) VERIFY_IS_TRUE( (x) ? TRUE : FALSE )

#include "TestUtil.h"

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

#define ALLOCATION_TAG 'LLKT'


typedef struct
{
    ULONG RecordSize;
    BOOLEAN WriteDedicatedOnly;
    ULONG NumberStreams;
    ULONG TimeToRunInSeconds;
    ULONG WaitTimerInMs;
    WCHAR LogContainerDirectory[MAX_PATH];
    WCHAR LogStreamDirectory[MAX_PATH]; 
} LWTParameters;

VOID CreateGuidPathname(
    __in PWCHAR Path,
    __out KString::SPtr& FullPathname
)
{
    BOOL b;
    KGuid guid;
    KString::SPtr path;
    KString::SPtr filename;
    path = KString::Create(*g_Allocator,
                           MAX_PATH);
    VERIFY_IS_TRUE((path != nullptr) ? true : false);
    filename = KString::Create(*g_Allocator,
                           MAX_PATH);
    VERIFY_IS_TRUE((filename != nullptr) ? true : false);

    
    b = path->CopyFrom(Path, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    guid. CreateNew();
    b = filename->FromGUID(guid);
    VERIFY_IS_TRUE(b ? true : false);
    b = filename->Concat(KStringView(L".log"));
    VERIFY_IS_TRUE(b ? true : false);
    
    b = path->Concat(KStringView(L"\\"));
    VERIFY_IS_TRUE(b ? true : false);
        
    b = path->Concat(*filename, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    FullPathname = Ktl::Move(path);
}

//
// This test will allow any number of streams to perform a long running
// workload. The workload will write fixed or random size data and truncate
// periodically.
//
// Future iterations may check the size of the log to ensure it is not
// too large, perform record reads while writing, etc.
//

#include "LWTAsync.h"

VOID LongRunningWriteTest(
    LWTParameters* params,
    KtlLogManager::SPtr logManager
    )
{
    ULONG NumberStreams = params->NumberStreams;
    
    const ULONG NumberWriteStress = params->NumberStreams;
    const ULONG MaxWriteRecordSize = params->RecordSize;
    const ULONGLONG AllowedLogSpace = 25165824;
    const ULONGLONG MaxWriteAsn = AllowedLogSpace * 64;
    const LONGLONG StreamSize = MaxWriteAsn / 2;
    const ULONG WaitTimerInMs = params->WaitTimerInMs;

    static const NumberWriteStressMax = 512;
    KInvariant(NumberWriteStress <= NumberWriteStressMax);
    LongRunningWriteStress::SPtr writeStress[NumberWriteStressMax];
    KSynchronizer writeSyncs[NumberWriteStressMax];
    
    KtlLogStreamId logStreamId[NumberWriteStressMax];
    KtlLogStream::SPtr logStream[NumberWriteStressMax];

    NTSTATUS status;
    KSynchronizer sync;

    //
    // Configure logger with no throttling limits
    //
    KtlLogManager::MemoryThrottleLimits* memoryThrottleLimits;
    KtlLogManager::AsyncConfigureContext::SPtr configureContext;
    KBuffer::SPtr inBuffer;
    KBuffer::SPtr outBuffer;
    ULONG result;

    status = logManager->CreateAsyncConfigureContext(configureContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleLimits),
                             inBuffer,
                             *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Configure defaults
    //
    memoryThrottleLimits = (KtlLogManager::MemoryThrottleLimits*)inBuffer->GetBuffer();
    memoryThrottleLimits->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits->WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
    memoryThrottleLimits->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits->PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;
    memoryThrottleLimits->PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
    memoryThrottleLimits->AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;        
    memoryThrottleLimits->MaximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    
    configureContext->Reuse();
    configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                     inBuffer.RawPtr(),
                                     result,
                                     outBuffer,
                                     NULL,
                                     sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    
    KGuid logStreamGuid;
    ULONG metadataLength = 0x10000;
    StreamCloseSynchronizer closeStreamSync;
    
    //
    // Create container
    //
    KString::SPtr containerPath;
    CreateGuidPathname(params->LogContainerDirectory, containerPath);
        
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = 0x10000000; // 0x200000000;  // 8GB

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(*containerPath,
                                                  logContainerId,
                                                  logSize,
                                                  0,            // Max Number Streams
                                                  0,            // Max Record Size
                                                  logContainer,
                                                  NULL,         // ParentAsync
                                                  sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KBuffer::SPtr securityDescriptor = nullptr;

    for (ULONG i = 0; i < NumberStreams; i++)
    {
        KString::SPtr streamPath;
        CreateGuidPathname(params->LogStreamDirectory, streamPath);
        
        logStreamGuid.CreateNew();
        logStreamId[i] = static_cast<KtlLogStreamId>(logStreamGuid);

        createStreamAsync->Reuse();
        createStreamAsync->StartCreateLogStream(logStreamId[i],
                                                KLogicalLogInformation::GetLogicalLogStreamType(),
                                                nullptr,           // Alias
                                                KString::CSPtr(streamPath.RawPtr()),
                                                securityDescriptor,                                             
                                                metadataLength,
                                                StreamSize,
                                                1024*1024,  // 1MB
                                                KtlLogManager::FlagSparseFile,
                                                logStream[i],
                                                NULL,    // ParentAsync
                                                sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    printf("Starting Asyncs\n");
    KDbgPrintf("Starting Asyncs\n");
    for (ULONG i = 0; i < NumberWriteStress; i++)
    {
        LongRunningWriteStress::StartParameters params1;

        status = LongRunningWriteStress::Create(*g_Allocator,
                                                     KTL_TAG_TEST,
                                                     writeStress[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        params1.LogStream = logStream[i];
        params1.MaxRandomRecordSize = MaxWriteRecordSize;
        params1.LogSpaceAllowed = AllowedLogSpace;
        params1.HighestAsn = MaxWriteAsn;
        params1.UseFixedRecordSize = TRUE;
        params1.WaitTimerInMs = WaitTimerInMs;
        writeStress[i]->StartIt(&params1,
                                NULL,
                                writeSyncs[i]);
    }


    //
    // Time bound how long this test runs
    //
    KTimer::SPtr timer;

    status = KTimer::Create(timer, *g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    timer->StartTimer(params->TimeToRunInSeconds * 1000, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    printf("Shutting down Asyncs\n");
    KDbgPrintf("Shutting down Asyncs\n");
    
    ULONGLONG bytes = 0;
    ULONGLONG a;
    for (ULONG i = 0; i < NumberWriteStress; i++)
    {       
        a = writeStress[i]->ForceCompletion();
        bytes += a;
    }

    ULONGLONG bytesPerSec = bytes / params->TimeToRunInSeconds;
    printf("%I64d bytes or %I64d per second\n", bytes, bytesPerSec);
    KDbgPrintf("%I64d bytes or %I64d per second\n", bytes, bytesPerSec);
    
    ULONG completed = NumberWriteStress;

    while (completed > 0)
    {
        for (ULONG i = 0; i < NumberWriteStress; i++)
        {
            status = writeSyncs[i].WaitForCompletion(60 * 1000);

            if (status == STATUS_IO_TIMEOUT)
            {
                printf("writeStress[%d]: %I64x Version, %I64x Asn, %I64x TruncationAsn\n", i,
                       writeStress[i]->GetVersion(), writeStress[i]->GetAsn(), writeStress[i]->GetTruncationAsn());
            } else {                
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                completed--;
            }
        }
    }
    
    for (ULONG i = 0; i < NumberStreams; i++)
    {
        logStream[i]->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    
    logContainer->StartClose(NULL,
        closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
    
    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteContainerAsync->StartDeleteLogContainer(*containerPath,
                                                  logContainerId,
                                                  NULL,         // ParentAsync
                                                  sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));         
}

VOID SetupTests(
    )
{
    NTSTATUS status;
    KtlSystem* System;

    EventRegisterMicrosoft_Windows_KTL();
    
    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KtlSystemInitialize");
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    KDbgMirrorToDebugger = TRUE;

    System->SetStrictAllocationChecks(TRUE);

#ifdef UDRIVER
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAndRegisterOverlayManager");
#endif
#if defined(KDRIVER) || defined(UPASSTHROUGH) || defined(DAEMON)
    //
    // For kernel, assume driver already installed by InstallForCITs
    //
#endif  
}

VOID CleanupTests(
    )
{
#ifdef UDRIVER
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    NTSTATUS status;
    
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    KInvariant(NT_SUCCESS(status));
#endif
#if defined(KDRIVER) || defined(UPASSTHROUGH) || defined(DAEMON)
    //
    // For kernel, assume driver already installed by InstallForCITs
    //
#endif
    
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}


VOID Usage()
{
    printf("LWTPerf - long write test performance\n");
    printf("    -r:<record size in bytes>\n");
    printf("    -f:<directory to log container file>\n");
    printf("    -s:<directory to log stream files>\n");
    printf("    -d:<true to write to dedicated log only>\n");
    printf("    -n:<Number streams>\n");
    printf("    -t:<TimeToRunInSeconds>\n");
    printf("    -w:<WaitTimerInMs between writes>\n");
    printf("\n");
    printf("    Note that path for log directories must be an absolute path in the form of \\??\\c:\\temp\\\n");
    printf("\n");
}


NTSTATUS ParseLWTParameters(
    int argc,
    wchar_t** args,
    LWTParameters* params
)
{
    //
    // Setup defaults
    //
    params->RecordSize = 512 * 1024;
    params->WriteDedicatedOnly = FALSE;
    params->NumberStreams = 1;
    params->TimeToRunInSeconds = 120;
    *params->LogContainerDirectory = 0;
    *params->LogStreamDirectory = 0;


    //
    // Parse Parameters
    //
    for (int i = 1; i < argc; i++)
    {
        PWCHAR a = args[i];
        WCHAR option;
        PWCHAR value;
        size_t len;

        len  = wcslen(a);
        if ((len < 3) || (a[0] != L'-') || (a[2] != L':'))
        {
            printf("Invalid Parameter %ws\n\n", a);
            return(STATUS_INVALID_PARAMETER);
        }

        option = towlower(a[1]);
        value = &a[3];

        switch(option)
        {
            case L'r':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n\n", a);
                    return(STATUS_INVALID_PARAMETER);
                }

                params->RecordSize = x;

                break;
            }

            case L'f':
            {
                StringCchCopy(params->LogContainerDirectory, MAX_PATH, value);
                break;
            }

            case L's':
            {
                StringCchCopy(params->LogStreamDirectory, MAX_PATH, value);
                break;
            }

            case L'd':
            {
                if (_wcsnicmp(value, L"false", (sizeof(L"false") / sizeof(WCHAR))) == 0)
                {
                    params->WriteDedicatedOnly = FALSE;
                } 
                else if (_wcsnicmp(value, L"true", (sizeof(L"true") / sizeof(WCHAR))) == 0)
                {
                    params->WriteDedicatedOnly = TRUE;
                } 
                else 
                {
                    printf("Invalid parameter value %ws\n\n", a);
                    return(STATUS_INVALID_PARAMETER);
                }

                break;
            }

            case L'n':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n\n", a);
                    return(STATUS_INVALID_PARAMETER);
                }

                params->NumberStreams = x;
                break;
            }

            case L't':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n\n", a);
                    return(STATUS_INVALID_PARAMETER);
                }

                params->TimeToRunInSeconds = x;
                break;
            }

            case L'w':
            {
                ULONG x;
                x = _wtoi(value);
                if (x == 0)
                {
                    printf("Invalid parameter value %ws\n\n", a);
                    return(STATUS_INVALID_PARAMETER);
                }

                params->WaitTimerInMs = x;
                break;
            }

            default:
            {
                printf("Invalid Parameter %ws\n\n", a);
                return(STATUS_INVALID_PARAMETER);
            }
        }
    }

    if ((*params->LogContainerDirectory == 0) || (*params->LogStreamDirectory == 0))
    {
        printf("Invalid Parameters - must specify fully qualified log container path and log stream path of the form \\??\\x:\\Directory\\\n\n");
        return(STATUS_INVALID_PARAMETER);
    }
    
    return(STATUS_SUCCESS);
}

int
wmain(int argc, wchar_t** args)
{
    NTSTATUS status = STATUS_SUCCESS;
    LWTParameters params;
    
    printf("LongWriteTest Logger Performance test\n");

    status = ParseLWTParameters(argc, args, &params);
    if (! NT_SUCCESS(status))
    {
        Usage();
        return(status);
    }
    
    printf("Log Container Path   : %ws\n", params.LogContainerDirectory);
    printf("Log Stream Path      : %ws\n", params.LogStreamDirectory);
    printf("RecordSize           : %d 0x%x\n", params.RecordSize, params.RecordSize);
    printf("WriteToDedicatedOnly : %s\n", params.WriteDedicatedOnly ? "true" : "false");
    printf("Number Streams       : %d\n", params.NumberStreams);
    printf("TimeToRunInSeconds   : %d\n", params.TimeToRunInSeconds);
    printf("WaitTimerInMs        : %d\n", params.WaitTimerInMs);
    printf("\n");

    SetupTests();

    {
        KtlLogManager::SPtr logManager;
        KServiceSynchronizer        serviceSync;

#ifdef UPASSTHROUGH
        status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, logManager);
#else
        status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, logManager);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        LongRunningWriteTest(&params, logManager);


        //
        // Now close the log manager and verify that it closes promptly.
        //
        status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logManager= nullptr;        
    }
    
    CleanupTests();
    
    return(status);
}



