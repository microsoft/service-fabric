// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(PLATFORM_UNIX)
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include <ktllogger.h>
#include <KLogicalLog.h>

#include <bldver.h>

#include "KtlLoggerTests.h"

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"
#endif

#include "CloseSync.h"
#include "TestUtil.h"

#include "workerasync.h"

#if !defined(PLATFORM_UNIX)
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;
#endif

#include "VerifyQuiet.h"

#ifdef UDRIVER
#include "KtlLogShimUser.h"
#include "KtlLogShimKernel.h"

VOID
ForceCloseTest(
    KGuid diskId
)
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;

    //
    // Access the log manager
    //
    KtlLogManager::SPtr logManager;
    KServiceSynchronizer openSync;
#ifdef UPASSTHROUGH
    status = KtlLogManager::CreateInproc(KTL_TAG_TEST, *g_Allocator, logManager);
#else
    status = KtlLogManager::CreateDriver(KTL_TAG_TEST, *g_Allocator, logManager);
#endif
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                             openSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = openSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;
        KGuid logStreamGuid;
        KtlLogStreamId logStreamId;

        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
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
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();

        createStreamAsync->StartCreateLogStream(logStreamId,
            logStreamType,
            nullptr,           // Alias
            nullptr,
            securityDescriptor,
            metadataLength,
            DEFAULT_STREAM_SIZE,
            DEFAULT_MAX_RECORD_SIZE,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONGLONG asn = KtlLogAsn::Min().Get();
        ULONGLONG version = 0;
        KIoBuffer::SPtr iodata;
        PVOID iodataBuffer;
        KIoBuffer::SPtr metadata;
        PVOID metadataBuffer;
        KBuffer::SPtr workKBuffer;
        PUCHAR workBuffer;

        KIoBuffer::SPtr emptyIoBuffer;

        PUCHAR dataWritten;
        ULONG dataSizeWritten;

        ULONG offsetToStreamBlockHeaderM;

        offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

        status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(16 * 0x1000,
            iodata,
            iodataBuffer,
            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
            metadata,
            metadataBuffer,
            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KBuffer::Create(16 * 0x1000,
            workKBuffer,
            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        workBuffer = (PUCHAR)workKBuffer->GetBuffer();
        dataWritten = workBuffer;
        dataSizeWritten = 2 * 0x1000;

        for (ULONG i = 0; i < dataSizeWritten; i++)
        {
           dataWritten[i] = (i * 10) % 255;
        }

        //
        // Do a whole bunch of work
        //
        for (ULONG i = 0; i < 128; i++)
        {
            ULONG data = 7;
            PUCHAR dataWritten1 = (PUCHAR)&data;
            ULONG dataSizeWritten1 = sizeof(ULONG);
            version++;

            status = SetupAndWriteRecord(logStream,
                metadata,
                iodata,
                version,
                asn,
                dataWritten1,
                dataSizeWritten1,
                offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten1;
        }

        //
        // Force close the logger. This simulates the scenario in kernel mode where a process exit
        // occurs and the handle is cleaned up.
        //
        KtlLogManagerUser::SPtr logManagerUser;

        logManagerUser = down_cast<KtlLogManagerUser, KtlLogManager>(logManager);
        RequestMarshallerKernel::Cleanup(*logManagerUser->GetFileObjectTable(), *g_Allocator);
        logManagerUser->GetFileObjectTable()->ResetLogManager();
    }
}


class ForceCloseDeleteContainerWorkerAsync : public WorkerAsync
{
    K_FORCE_SHARED(ForceCloseDeleteContainerWorkerAsync);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out ForceCloseDeleteContainerWorkerAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            ForceCloseDeleteContainerWorkerAsync::SPtr context;
            
            context = _new(AllocationTag, Allocator) ForceCloseDeleteContainerWorkerAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (! NT_SUCCESS(status))
            {
                return(status);
            }

            Context = context.RawPtr();

            return(STATUS_SUCCESS);     
        }

        struct StartParameters
        {
            KtlLogContainer::SPtr LogContainer;
            KtlLogStreamId StreamId;            
        };
        
        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            StartParameters* startParameters = (StartParameters*)Parameters;

            _State = Initial;
            _LogContainer = startParameters->LogContainer;
            _StreamId = startParameters->StreamId;
            Start(ParentAsyncContext, CallbackPtr);
        }

    protected:
        VOID FSMContinue(
            __in NTSTATUS Status
            ) override
        {
            if ((! NT_SUCCESS(Status)) || (_IsCancelled))
            {
                Complete(Status);
                return;
            }
            
            switch(_State)
            {
                case Initial:
                {
                    Status = _LogContainer->CreateAsyncCreateLogStreamContext(_CreateStreamContext);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    
                    // Fall through
                }

                case CreateStream:
                {
                    KBuffer::SPtr securityDescriptor = nullptr;
                    ULONG metadataLength = 0x10000;
                    KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();

                    _CreateStreamContext->StartCreateLogStream(_StreamId,
                        logStreamType,
                        nullptr,           // Alias
                        nullptr,
                        securityDescriptor,
                        metadataLength,
                        DEFAULT_STREAM_SIZE,
                        DEFAULT_MAX_RECORD_SIZE,
                        _LogStream,
                        this,    // ParentAsync
                        _Completion);


                    _State = WriteStuff;
                    break;
                }

                case WriteStuff:
                {
                    if (_WriteContext)
                    {
                        _WriteContext->Reuse();
                    } else {
                        Status = _LogStream->CreateAsyncWriteContext(_WriteContext);
                        VERIFY_IS_TRUE(NT_SUCCESS(Status));

                        _StreamOffset = 1;
                        _Version = 1;
                    }


                    KIoBuffer::SPtr ioBuffer, metadataBuffer;
                    PVOID p;
                    Status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                        metadataBuffer,
                        p,
                        GetThisAllocator());
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    Status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator());
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));
                    
                    LLRecordObject llRecord;

                    ULONG headerSkipOffset = 0;                            
                    ULONG coreLoggerOverhead = _LogStream->QueryReservedMetadataSize() - sizeof(KtlLogVerify::KtlMetadataHeader);
                    llRecord.Initialize(headerSkipOffset,
                                        coreLoggerOverhead,
                                        *metadataBuffer,
                                        *ioBuffer);

                    llRecord.InitializeHeadersForNewRecord(_StreamId,
                                                          _StreamOffset,
                                                          _Version);
                    PUCHAR pp = (PUCHAR)&_StreamId;
                    ULONG offset = 0;
                    ULONG len = sizeof(KGuid);
                    llRecord.CopyFrom(pp, offset, len);
                    llRecord.UpdateHeaderInformation(_Version, 0, KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord);

                    ULONG metadataSize = llRecord.GetDataOffset() + llRecord.GetDataSize();
                    _WriteContext->StartWrite(_StreamOffset,
                                              _Version,
                                              metadataSize,
                                              metadataBuffer,
                                              ioBuffer,
                                              0,
                                              this,
                                              _Completion);
                    _Version++;
                    _StreamOffset += llRecord.GetDataSize();
                    break;
                }


                case Completed:
                {
                    Complete(STATUS_SUCCESS);
                    return;
                }

                default:
                {
                    VERIFY_IS_TRUE(FALSE);
                    Complete(STATUS_SUCCESS);
                    return;
                }
            }
        }

        VOID OnCancel() override
        {
            _IsCancelled = TRUE;
        }

        VOID OnReuse() override
        {
        }

        VOID OnCompleted() override
        {
            _LogContainer = nullptr;
            _LogStream = nullptr;
            _CreateStreamContext = nullptr;
            _WriteContext = nullptr;
        }
        
    private:
        enum  FSMState { Initial=0, CreateStream=1, WriteStuff=2, Completed=3 };
        FSMState _State;

        //
        // Parameters
        //
        KtlLogContainer::SPtr _LogContainer;
        KtlLogStreamId _StreamId;

        //
        // Internal
        //
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr _CreateStreamContext;
        KtlLogStream::AsyncWriteContext::SPtr _WriteContext;
        KtlLogStream::SPtr _LogStream;
        ULONGLONG _StreamOffset;
        ULONGLONG _Version;
        BOOLEAN _IsCancelled;
};

ForceCloseDeleteContainerWorkerAsync::ForceCloseDeleteContainerWorkerAsync()
{
}

ForceCloseDeleteContainerWorkerAsync::~ForceCloseDeleteContainerWorkerAsync()
{
}


VOID
ForceCloseOnDeleteContainerWaitTest(
    KGuid diskId
)
{
    NTSTATUS status;
    
    for (ULONG i = 0; i < 32; i++)
    {
        KGuid logContainerGuid;
        KtlLogContainerId logContainerId;
        StreamCloseSynchronizer closeStreamSync;
        ContainerCloseSynchronizer closeContainerSync;

        //
        // Access the log manager
        //
        KtlLogManager::SPtr logManager;
        KServiceSynchronizer openSync;
#ifdef UPASSTHROUGH
        status = KtlLogManager::CreateInproc(KTL_TAG_TEST, *g_Allocator, logManager);
#else
        status = KtlLogManager::CreateDriver(KTL_TAG_TEST, *g_Allocator, logManager);
#endif
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                                 openSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = openSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        logContainerGuid.CreateNew();
        logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

        //
        // Test 1: While asyncs are busy writing to streams in the log,
        //         Delete the log container and then simulate process crash
        //
        {
            KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
            LONGLONG logSize = DefaultTestLogFileSize;
            KtlLogContainer::SPtr logContainer;
            KSynchronizer sync;
            KGuid logStreamGuid;
            KtlLogStreamId logStreamId;

            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

            status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            createContainerAsync->StartCreateLogContainer(diskId,
                logContainerId,
                logSize,
                0,            // Max Number Streams
                0,            // Max Record Size
                logContainer,
                NULL,         // ParentAsync
                sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            static const int count = 8;
            ForceCloseDeleteContainerWorkerAsync::SPtr async[count];
            for (ULONG i1 = 0; i1 < 8; i1++)
            {

                status = ForceCloseDeleteContainerWorkerAsync::Create(*g_Allocator, KTL_TAG_TEST, async[i1]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                ForceCloseDeleteContainerWorkerAsync::StartParameters parameters;

                parameters.LogContainer = logContainer;

                KGuid logStreamGuid1;
                logStreamGuid1.CreateNew();
                parameters.StreamId = logStreamGuid1;
                async[i1]->StartIt(&parameters, NULL, NULL);
            }

            KNt::Sleep(10 * 1000);

            for (ULONG i1 = 0; i1 < count; i1++)
            {
                async[i1]->Cancel();
            }

            KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

            status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            deleteContainerAsync->StartDeleteLogContainer(diskId,
                                                 logContainerId,
                                                 NULL,         // ParentAsync
                                                 sync);

            // Do not close container or stream

            //
            // Force close the logger. This simulates the scenario in kernel mode where a process exit
            // occurs and the handle is cleaned up.
            //
            KtlLogManagerUser::SPtr logManagerUser;

            logManagerUser = down_cast<KtlLogManagerUser, KtlLogManager>(logManager);
            RequestMarshallerKernel::Cleanup(*logManagerUser->GetFileObjectTable(), *g_Allocator);
            logManagerUser->GetFileObjectTable()->ResetLogManager();

            //
            // Expect delete to complete
            //
            status = sync.WaitForCompletion(10 * 1000);        
            VERIFY_IS_TRUE(NT_SUCCESS(status) || (status == STATUS_NOT_FOUND) || (status == STATUS_FILE_CLOSED));
        }

        //
        // Clean up the log container
        //
        {
            KtlLogManager::SPtr logManager1;
            KServiceSynchronizer openSync1;
            KSynchronizer sync;
#ifdef UPASSTHROUGH
            status = KtlLogManager::CreateInproc(KTL_TAG_TEST, *g_Allocator, logManager1);
#else
            status = KtlLogManager::CreateDriver(KTL_TAG_TEST, *g_Allocator, logManager1);
#endif
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = logManager1->StartOpenLogManager(NULL, // ParentAsync
                                                     openSync1.OpenCompletionCallback());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = openSync1.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

            status = logManager1->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            deleteContainerAsync->StartDeleteLogContainer(diskId,
                                                 logContainerId,
                                                 NULL,         // ParentAsync
                                                 sync);
            status = sync.WaitForCompletion();                  
        }
    }
}

VOID SetupMiscTests(
    KGuid& DiskId,
    UCHAR& DriveLetter,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    NTSTATUS status;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
        &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    //
    // Do not enable strict allocation checks as this test does not cleanup properly by design
    //
    System->SetStrictAllocationChecks(FALSE);  

    StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

#if !defined(PLATFORM_UNIX)
    DriveLetter = 0;
    status = FindDiskIdForDriveLetter(DriveLetter, DiskId);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#else
    // {204F01E4-6DEC-48fe-8483-B2C6B79ABECB}
    GUID randomGuid = 
        { 0x204f01e4, 0x6dec, 0x48fe, { 0x84, 0x83, 0xb2, 0xc6, 0xb7, 0x9a, 0xbe, 0xcb } };
    
    DiskId = randomGuid;    
#endif

    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}


VOID CleanupMiscTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    UNREFERENCED_PARAMETER(DiskId);
    UNREFERENCED_PARAMETER(System);
    UNREFERENCED_PARAMETER(StartingAllocs);

    NTSTATUS status;
    KServiceSynchronizer        sync;

    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    KInvariant(NT_SUCCESS(status));

    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

#endif
