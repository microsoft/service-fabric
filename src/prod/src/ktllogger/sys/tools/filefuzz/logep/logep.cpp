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
#include <KtlLogShimKernel.h>
#include <KLogicalLog.h>
#include <windows.h>

#define VERIFY_IS_TRUE(__condition, ...) if (! (__condition)) {  printf("[FAIL] File %s Line %d\n", __FILE__, __LINE__); KInvariant(FALSE); };

// {E5E18B9D-B4AE-4461-80D0-99E598A7EB81}
static const GUID WellKnownGuid = 
{ 0xe5e18b9d, 0xb4ae, 0x4461, { 0x80, 0xd0, 0x99, 0xe5, 0x98, 0xa7, 0xeb, 0x81 } };

// {5EC5F519-9D86-4d92-AC56-E232EC666789}
static const GUID WellKnownGuid2 = 
{ 0x5ec5f519, 0x9d86, 0x4d92, { 0xac, 0x56, 0xe2, 0x32, 0xec, 0x66, 0x67, 0x89 } };

KInvariantCalloutType PreviousKInvariantCallout;
BOOLEAN MyKInvariantCallout(
    __in LPCSTR Condition,
    __in LPCSTR File,
    __in ULONG Line
    )
{
    //
    // Remember the info passed
    //
    printf("KInvariant Failure %s at %s line %d\n", Condition, File, Line);

    return(TRUE);    // Force crash
}

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

class StreamCloseSynchronizer : public KObject<StreamCloseSynchronizer>
{
    K_DENY_COPY(StreamCloseSynchronizer);

public:

    FAILABLE
        StreamCloseSynchronizer(__in BOOLEAN IsManualReset = FALSE)
        : _Event(IsManualReset, FALSE),
        _CompletionStatus(STATUS_SUCCESS)
    {
            _Callback.Bind(this, &StreamCloseSynchronizer::AsyncCompletion);
            SetConstructorStatus(_Event.Status());
        }

    ~StreamCloseSynchronizer()
    {
    }

    KtlLogStream::CloseCompletionCallback CloseCompletionCallback()
    {
        return(_Callback);
    }

    NTSTATUS
        WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr)
    {
            KInvariant(!KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());

            BOOLEAN b = _Event.WaitUntilSet(TimeoutInMilliseconds);
            if (!b)
            {
                if (IsCompleted)
                {
                    *IsCompleted = FALSE;
                }

                return STATUS_IO_TIMEOUT;
            }
            else
            {
                if (IsCompleted)
                {
                    *IsCompleted = TRUE;
                }

                return _CompletionStatus;
            }
        }


    VOID
        Reset()
    {
            _Event.ResetEvent();
            _CompletionStatus = STATUS_SUCCESS;
        }


protected:
    VOID
        AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogStream& LogStream,
        __in NTSTATUS Status)
    {
            UNREFERENCED_PARAMETER(Parent);
            UNREFERENCED_PARAMETER(LogStream);

            _CompletionStatus = Status;

            _Event.SetEvent();
        }

private:
    KEvent                                      _Event;
    NTSTATUS                                    _CompletionStatus;
    KtlLogStream::CloseCompletionCallback       _Callback;
};


class ContainerCloseSynchronizer : public KObject<ContainerCloseSynchronizer>
{
    K_DENY_COPY(ContainerCloseSynchronizer);

public:

    FAILABLE
        ContainerCloseSynchronizer(__in BOOLEAN IsManualReset = FALSE)
        : _Event(IsManualReset, FALSE),
        _CompletionStatus(STATUS_SUCCESS)
    {
            _Callback.Bind(this, &ContainerCloseSynchronizer::AsyncCompletion);
            SetConstructorStatus(_Event.Status());
        }

    ~ContainerCloseSynchronizer()
    {
    }

    KtlLogContainer::CloseCompletionCallback CloseCompletionCallback()
    {
        return(_Callback);
    }

    NTSTATUS
        WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr)
    {
            KInvariant(!KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());

            BOOLEAN b = _Event.WaitUntilSet(TimeoutInMilliseconds);
            if (!b)
            {
                if (IsCompleted)
                {
                    *IsCompleted = FALSE;
                }

                return STATUS_IO_TIMEOUT;
            }
            else
            {
                if (IsCompleted)
                {
                    *IsCompleted = TRUE;
                }

                return _CompletionStatus;
            }
        }


    VOID
        Reset()
    {
            _Event.ResetEvent();
            _CompletionStatus = STATUS_SUCCESS;
        }


protected:
    VOID
        AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogContainer& LogContainer,
        __in NTSTATUS Status)
    {
            UNREFERENCED_PARAMETER(Parent);
            UNREFERENCED_PARAMETER(LogContainer);

            _CompletionStatus = Status;

            _Event.SetEvent();
        }

private:
    KEvent                                      _Event;
    NTSTATUS                                    _CompletionStatus;
    KtlLogContainer::CloseCompletionCallback       _Callback;
};


NTSTATUS
VerifyRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const,
    __in ULONG,
    __in const KIoBuffer::SPtr&
)
{
    return(STATUS_SUCCESS);
}


VOID SetupRawCoreLoggerTests(
    )
{
    NTSTATUS status;
    KtlSystem* System;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    PreviousKInvariantCallout = SetKInvariantCallout(MyKInvariantCallout);
    
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    KDbgMirrorToDebugger = TRUE;

    System->SetStrictAllocationChecks(TRUE);

}

VOID CleanupRawCoreLoggerTests(
    )
{    
    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

void OpenAndReadStream(
    __in RvdLogManager& LogManager,
    __in RvdLogStreamId LogStreamId,
    __in KString& ContainerFullPath
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KServiceSynchronizer        serviceSync;
    RvdLogManager::SPtr coreLogManager = &LogManager;
    KString::SPtr containerFullPathName = &ContainerFullPath;
    RvdLog::SPtr coreLog;
    KGuid diskId;
    diskId.CreateNew();
    RvdLogId rvdLogId = WellKnownGuid2;
    KString::SPtr containerPath;
    BOOLEAN b;

    containerPath = KString::Create(*g_Allocator,
                                            KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerPath != nullptr) ? true : false);
    
    b = containerPath->CopyFrom(KStringView(L"\\??\\c:\\app\\Container.log"));
    VERIFY_IS_TRUE(b ? true : false);   


    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogId logId = WellKnownGuid;
    
    
    //
    // Reopen the streams via the overlay log
    //
    OverlayLog::SPtr overlayLog;
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(KTL_TAG_TEST, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               containerPath.RawPtr(),        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    // Ignore status

    coreOpenLog->StartOpenLog(*containerPath,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenLog = nullptr;  
    
    ULONG maxRecordSize = 16 * 1024;
    LONGLONG streamSize = 256 * 1024;
    ULONG metadataLength = 0x10000;
    
    overlayStream = _new(KTL_TAG_TEST, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     LogStreamId,
                                                                     diskId,
                                                                     containerFullPathName.RawPtr(),
                                                                     maxRecordSize,
                                                                     streamSize,
                                                                     metadataLength,
                                                                     *throttledAllocator);

    VERIFY_IS_TRUE(overlayStream ? TRUE : FALSE);
    status = overlayStream->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayStream->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE((status == STATUS_CRC_ERROR) || (status == K_STATUS_LOG_STRUCTURE_FAULT) || (NT_SUCCESS(status)));
    if (! NT_SUCCESS(status))
    {
        printf("%x at overlayStream->StartServiceOpen\n", status);
        ExitProcess(status);
    }

    OverlayStream::AsyncIoctlContextOverlay::SPtr ioctlContext;
    status = overlayStream->CreateAsyncIoctlContext(ioctlContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG result;
    KBuffer::SPtr tailAndVersionBuffer;
    KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation* tailAndVersion;
    ioctlContext->StartIoctl(KLogicalLogInformation::QueryLogicalLogTailAsnAndHighestOperationControl, NULL, result,
                             tailAndVersionBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    ioctlContext = nullptr;
    tailAndVersion = (KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation*)tailAndVersionBuffer->GetBuffer();              

    OverlayStream::AsyncMultiRecordReadContextOverlay::SPtr multiRecordRead;
    status = overlayStream->CreateAsyncMultiRecordReadContextOverlay(multiRecordRead);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogAsn lowAsn;
    KtlLogAsn highAsn;
    KtlLogAsn truncationAsn;

    status = overlayStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 
    
    KtlLogAsn asn = lowAsn;
    KIoBuffer::SPtr metadata;
    KIoBuffer::SPtr ioBuffer;
    const ULONG ioBufferSize = 12 * 1024;
    PVOID p;

    status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                     metadata,
                                     p,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KIoBuffer::CreateSimple(ioBufferSize, ioBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    do
    {
        multiRecordRead->Reuse();
        multiRecordRead->StartRead(asn,
                                   *metadata,
                                   *ioBuffer,
                                   NULL,
                                   sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE((status == STATUS_CRC_ERROR) ||
                       (status == K_STATUS_LOG_STRUCTURE_FAULT) ||
                       (status == STATUS_NOT_FOUND) ||
                       NT_SUCCESS(status));
        if ((status == K_STATUS_LOG_STRUCTURE_FAULT) || (status == STATUS_CRC_ERROR))
        {
            printf("%x at StartRead\n", status);
            ExitProcess(status);
        }

        asn = asn.Get() + ioBufferSize;
    } while (status != STATUS_NOT_FOUND);
    multiRecordRead = nullptr;

    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreLog = nullptr;
    printf("Successful read\n");
}



void ReadListOfStreams(
    __in RvdLogManager& LogManager,
    __in KString& ContainerFullPath,
    __out KArray<RvdLogStreamId>& StreamIds
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager = &LogManager;
    StreamIds.Clear();
    
    RvdLogManager::AsyncOpenLog::SPtr openLogOp;
    RvdLog::SPtr logContainer;
    status = logManager->CreateAsyncOpenLogContext(openLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    openLogOp->StartOpenLog(ContainerFullPath, logContainer, nullptr, sync);
    status = sync.WaitForCompletion();

    openLogOp = nullptr;
    VERIFY_IS_TRUE((status == STATUS_CRC_ERROR) || (status == K_STATUS_LOG_STRUCTURE_FAULT) || (NT_SUCCESS(status)));
    if (! NT_SUCCESS(status))
    {
        printf("%x at StartOpenLog\n", status);
        ExitProcess(status);
    }

    ULONG streamsCount;
    streamsCount = logContainer->GetNumberOfStreams();

    status = StreamIds.Reserve(streamsCount);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    ULONG sizeNeeded = streamsCount * sizeof(RvdLogStreamId);
    KBuffer::SPtr buffer;
    status = KBuffer::Create(sizeNeeded, buffer, *g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    RvdLogStreamId* streams = (RvdLogStreamId*)buffer->GetBuffer();
    
    logContainer->GetStreams(streamsCount, streams);

    for (ULONG i = 0; i < streamsCount; i++)
    {
        StreamIds.Append(streams[i]);
    }
    logContainer = nullptr;
}
    
void ReadAllRecordsInContainer(
    LPCWSTR ContainerPath
    )
{
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;
    KArray<RvdLogStreamId> streamIds(*g_Allocator);
    BOOLEAN b;

    KString::SPtr containerFullPathName;
    containerFullPathName = KString::Create(*g_Allocator,
                                            KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerFullPathName != nullptr) ? true : false);
    
    b = containerFullPathName->CopyFrom(KStringView(L"\\??\\"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(KStringView(ContainerPath));
    VERIFY_IS_TRUE(b ? true : false);


    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    status = logManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                      &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    ReadListOfStreams(*logManager, *containerFullPathName, streamIds);

    //
    // Open up each stream and do multirecord read
    //
    for (ULONG i = 0; i < streamIds.Count(); i++)
    {
        if (streamIds[i] != RvdDiskLogConstants::CheckpointStreamId())
        {
            OpenAndReadStream(*logManager, streamIds[i], *containerFullPathName );
        }
    }

    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();     
}



int
TheMain(__in int , __in wchar_t** args)
{
    SetupRawCoreLoggerTests();
    KFinally([&] { CleanupRawCoreLoggerTests(); });

    ReadAllRecordsInContainer(args[1]);

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
