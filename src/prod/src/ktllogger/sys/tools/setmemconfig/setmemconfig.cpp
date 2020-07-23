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
#include "InternalKtlLogger.h"
#include "KtlLogShimKernel.h"
#include <windows.h>

#define VERIFY_IS_TRUE(cond, msg) if (! (cond)) { printf("Failure %s in %s at line %d\n",  msg, __FILE__, __LINE__); ExitProcess(static_cast<UINT>(-1)); }

#define CONVERT_TO_ARGS(argc, cargs) \
    std::vector<WCHAR*> args_vec(argc);\
    WCHAR** args = (WCHAR**)args_vec.data();\
    std::vector<std::wstring> wargs(argc);\
    for (int iter = 0; iter < argc; iter++)\
    {\
        wargs[iter] = Utf8To16(cargs[iter]);\
        args[iter] = (WCHAR*)(wargs[iter].data());\
    }\

KAllocator* g_Allocator;

VOID SetupRawCoreLoggerTests(
    )
{
    NTSTATUS status;
    KtlSystem* System;

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
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAndRegister");
#endif

}

VOID CleanupRawCoreLoggerTests(
    )
{
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
#ifdef UDRIVER
    NTSTATUS status;
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Stopand Unregister");
#endif
    
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}



int
TheMain(__in int , __in wchar_t** )
{

    LONGLONG update = 1;

    SetupRawCoreLoggerTests();

    {
        NTSTATUS status;
        KServiceSynchronizer        serviceSync;
        KSynchronizer sync;
        KtlLogManager::SPtr logManager;
        KtlLogManager::MemoryThrottleLimits* outMemoryThrottleLimits;

#ifdef UPASSTHROUGH
        status = KtlLogManager::CreateInproc(KTL_TAG_TEST, *g_Allocator, logManager);
#else
        status = KtlLogManager::CreateDriver(KTL_TAG_TEST, *g_Allocator, logManager);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");

        status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");



        KtlLogManager::MemoryThrottleLimits* memoryThrottleLimits;
        KtlLogManager::AsyncConfigureContext::SPtr configureContext;
        KBuffer::SPtr inBuffer;
        KBuffer::SPtr outBuffer;
        ULONG result;

        status = logManager->CreateAsyncConfigureContext(configureContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");

        status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleLimits),
                                 inBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");



        {
            //
            // Verify Settings
            //
            configureContext->Reuse();
            configureContext->StartConfigure(KtlLogManager::QueryMemoryThrottleUsage,
                                             nullptr,
                                             result,
                                             outBuffer,
                                             NULL,
                                             sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");

            KtlLogManager::MemoryThrottleUsage* memoryThrottleUsage;
            memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
            outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

            printf("Old settings\n");
            printf("memoryThrottleUsage->TotalAllocationLimit 0x%llx\n", memoryThrottleUsage->TotalAllocationLimit);
            printf("memoryThrottleUsage->CurrentAllocations 0x%llx\n", memoryThrottleUsage->CurrentAllocations);
            printf("memoryThrottleUsage->IsUnderMemoryPressure %s\n", memoryThrottleUsage->IsUnderMemoryPressure ? "TRUE" : "FALSE");

            printf("outMemoryThrottleLimits->WriteBufferMemoryPoolMax 0x%llx\n", outMemoryThrottleLimits->WriteBufferMemoryPoolMax);
            printf("outMemoryThrottleLimits->WriteBufferMemoryPoolMin 0x%llx\n", outMemoryThrottleLimits->WriteBufferMemoryPoolMin);
            printf("outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream 0x%x\n", outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream);
            printf("outMemoryThrottleLimits->PinnedMemoryLimit 0x%llx\n", outMemoryThrottleLimits->PinnedMemoryLimit);
            printf("outMemoryThrottleLimits->PeriodicFlushTimeInSec 0x%x\n", outMemoryThrottleLimits->PeriodicFlushTimeInSec);
            printf("outMemoryThrottleLimits->PeriodicTimerIntervalInSec 0x%x\n", outMemoryThrottleLimits->PeriodicTimerIntervalInSec);
            printf("outMemoryThrottleLimits->AllocationTimeoutInMs 0x%x\n", outMemoryThrottleLimits->AllocationTimeoutInMs);
            printf("outMemoryThrottleLimits->MaximumDestagingWriteOutstanding 0x%llx\n", outMemoryThrottleLimits->MaximumDestagingWriteOutstanding);

            if (memoryThrottleUsage->CurrentAllocations > 0x1C0000000)
            {
                printf("ERROR current > new\n");
                update = 0;
            }

        }

        if (update == 1)
        {
            //
            // Update
            //
            memoryThrottleLimits = (KtlLogManager::MemoryThrottleLimits*)inBuffer->GetBuffer();
            memoryThrottleLimits->WriteBufferMemoryPoolMax = 0x1C0000000;
            memoryThrottleLimits->WriteBufferMemoryPoolMin = 0x1C0000000;
            memoryThrottleLimits->WriteBufferMemoryPoolPerStream = outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream;
            memoryThrottleLimits->PinnedMemoryLimit = outMemoryThrottleLimits->PinnedMemoryLimit;
            memoryThrottleLimits->PeriodicFlushTimeInSec = outMemoryThrottleLimits->PeriodicFlushTimeInSec;
            memoryThrottleLimits->PeriodicTimerIntervalInSec = outMemoryThrottleLimits->PeriodicTimerIntervalInSec;
            memoryThrottleLimits->AllocationTimeoutInMs = outMemoryThrottleLimits->AllocationTimeoutInMs;
            memoryThrottleLimits->MaximumDestagingWriteOutstanding = 768 * 1024 * 1024;


            configureContext->Reuse();
            configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                             inBuffer.RawPtr(),
                                             result,
                                             outBuffer,
                                             NULL,
                                             sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");

            printf("\n");

            {
                //
                // Verify Settings
                //
                configureContext->Reuse();
                configureContext->StartConfigure(KtlLogManager::QueryMemoryThrottleUsage,
                                                 nullptr,
                                                 result,
                                                 outBuffer,
                                                 NULL,
                                                 sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");

                KtlLogManager::MemoryThrottleUsage* memoryThrottleUsage;
                memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
                outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

                printf("New settings\n");
                printf("memoryThrottleUsage->TotalAllocationLimit 0x%llx\n", memoryThrottleUsage->TotalAllocationLimit);
                printf("memoryThrottleUsage->CurrentAllocations 0x%llx\n", memoryThrottleUsage->CurrentAllocations);
                printf("memoryThrottleUsage->IsUnderMemoryPressure %s\n", memoryThrottleUsage->IsUnderMemoryPressure ? "TRUE" : "FALSE");

                printf("outMemoryThrottleLimits->WriteBufferMemoryPoolMax 0x%llx\n", outMemoryThrottleLimits->WriteBufferMemoryPoolMax);
                printf("outMemoryThrottleLimits->WriteBufferMemoryPoolMin 0x%llx\n", outMemoryThrottleLimits->WriteBufferMemoryPoolMin);
                printf("outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream 0x%x\n", outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream);
                printf("outMemoryThrottleLimits->PinnedMemoryLimit 0x%llx\n", outMemoryThrottleLimits->PinnedMemoryLimit);
                printf("outMemoryThrottleLimits->PeriodicFlushTimeInSec 0x%x\n", outMemoryThrottleLimits->PeriodicFlushTimeInSec);
                printf("outMemoryThrottleLimits->PeriodicTimerIntervalInSec 0x%x\n", outMemoryThrottleLimits->PeriodicTimerIntervalInSec);
                printf("outMemoryThrottleLimits->AllocationTimeoutInMs 0x%x\n", outMemoryThrottleLimits->AllocationTimeoutInMs);
                printf("outMemoryThrottleLimits->MaximumDestagingWriteOutstanding 0x%llx\n", outMemoryThrottleLimits->MaximumDestagingWriteOutstanding);

            }
        }

        
        configureContext = nullptr;


        status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");

        logManager = nullptr;
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "FAIL");
    }

    CleanupRawCoreLoggerTests();
    
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
