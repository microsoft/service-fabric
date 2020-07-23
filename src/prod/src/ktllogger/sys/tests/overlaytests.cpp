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
#include <KtlLogShimKernel.h>
#include <KLogicalLog.h>

#include "MockRvdLogStream.h"

#include "KtlLoggerTests.h"

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"
#endif

#include "CloseSync.h"

#include "AsyncInterceptor.h"

#include "workerasync.h"

#define ALLOCATION_TAG 'LLKT'

extern KAllocator* g_Allocator;

#include "VerifyQuiet.h"

#define StreamOffsetToExpectedData(s)  (((s) % 255) + (((s) / 255) % 255))

NTSTATUS SetupToWriteRecord(
    __out KBuffer::SPtr& MetadataKBuffer,
    __out KIoBuffer::SPtr& DataIoBuffer,
    __in KtlLogStreamId logStreamId,
    __in ULONGLONG version,
    __in ULONGLONG asn,
    __in PUCHAR data,
    __in ULONG dataSize,
    __in ULONG coreLoggerOffset,
    __in ULONG reservedMetadataSize,
    __in BOOLEAN IsBarrierRecord,
    __in ULONG BarrierRecord = 0,
    __in ULONG MetadataSize = KLogicalLogInformation::FixedMetadataSize
    )
{
    //
    // Build metadata as if offsets start at the beginning of the core logger metadata but buffer begins at the KtlShim
    // metadata.
    //
    NTSTATUS status;
    PVOID metadataBuffer;
    PVOID dataBuffer = NULL;
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
    ULONG offsetToMetadataBlockHeader = coreLoggerOffset + reservedMetadataSize;
    ULONG offsetToStreamBlockHeader = offsetToMetadataBlockHeader + sizeof(KLogicalLogInformation::MetadataBlockHeader);
    ULONG offsetToData = offsetToStreamBlockHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);
    ULONG dataPtrToMetadataBlockHeader = reservedMetadataSize;
    ULONG dataPtrToStreamBlockHeader = dataPtrToMetadataBlockHeader + sizeof(KLogicalLogInformation::MetadataBlockHeader);
    ULONG dataPtrToData = dataPtrToStreamBlockHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);

    //
    // Copy data into exclusive buffers
    //
    ULONG dataSizeInMetadata;
    ULONG dataSizeInIoBuffer;
    ULONG maxDataSizeInMetadata = KLogicalLogInformation::FixedMetadataSize - offsetToData;

    if (dataSize <= maxDataSizeInMetadata)
    {
        dataSizeInMetadata = dataSize;
        dataSizeInIoBuffer = 0;
    }
    else
    {
        dataSizeInMetadata = maxDataSizeInMetadata;
        dataSizeInIoBuffer = dataSize - maxDataSizeInMetadata;
    }

    PVOID src, dest;

    status = KBuffer::Create(MetadataSize, MetadataKBuffer, *g_Allocator);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    metadataBuffer = MetadataKBuffer->GetBuffer();

    dest = (PVOID)((PUCHAR)metadataBuffer + dataPtrToData);
    src = data;

    KMemCpySafe(dest, dataSizeInMetadata, src, dataSizeInMetadata);

    if (dataSizeInIoBuffer > 0)
    {
        status = KIoBuffer::CreateSimple(KLoggerUtilities::RoundUpTo4K(dataSizeInIoBuffer), DataIoBuffer, dataBuffer, *g_Allocator);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        dest = (PVOID)DataIoBuffer->First()->GetBuffer();
        src = (PVOID)((PUCHAR)data + dataSizeInMetadata);
        KMemCpySafe(dest, dataSizeInIoBuffer, src, dataSizeInIoBuffer);
    }
    else
    {
        status = KIoBuffer::CreateEmpty(DataIoBuffer, *g_Allocator);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }
    }

    metadataBlockHeader = (KLogicalLogInformation::MetadataBlockHeader*)((PUCHAR)metadataBuffer + dataPtrToMetadataBlockHeader);
    metadataBlockHeader->OffsetToStreamHeader = offsetToStreamBlockHeader;

    if (BarrierRecord == 0)
    {
        if (IsBarrierRecord)
        {
            metadataBlockHeader->Flags = KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord;
        } else {
            metadataBlockHeader->Flags = 0;
        }
    } else {
        metadataBlockHeader->Flags = BarrierRecord;
    }

    //
    // Stream block header may not cross metadata/iodata boundary
    //
    streamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)((PUCHAR)metadataBuffer + dataPtrToStreamBlockHeader);

    streamBlockHeader->Signature = KLogicalLogInformation::StreamBlockHeader::Sig;
    streamBlockHeader->StreamId = logStreamId;
    streamBlockHeader->StreamOffset = asn;
    streamBlockHeader->HighestOperationId = version;
    streamBlockHeader->DataSize = dataSize;

    streamBlockHeader->DataCRC64 = KLogicalLogInformation::ComputeDataBlockCRC(streamBlockHeader,
                                                                               *DataIoBuffer,
                                                                               offsetToData);
    streamBlockHeader->HeaderCRC64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(streamBlockHeader);

    return(STATUS_SUCCESS);
}

NTSTATUS WriteALogicalLogRecord(
    RvdLogStream::SPtr& logStream,
    ULONGLONG version,
    RvdLogAsn recordAsn,
    ULONG CoreLoggerMetadataOverhead,
    UCHAR contents
)
{
    NTSTATUS status;
    KSynchronizer sync;
    ULONG offsetToStreamBlockHeader;
    ULONG offsetToMetadataBlockHeader;
    ULONG offsetToDataLocation;
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
    PUCHAR dataPtr;

    PUCHAR metadataBuffer;
    KBuffer::SPtr metadataKBuffer;
    RvdLogStreamId logStreamId;

    status = KBuffer::Create(0x1000,
                             metadataKBuffer,
                                *g_Allocator,
                                ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    metadataBuffer = (PUCHAR)metadataKBuffer->GetBuffer();

    logStream->QueryLogStreamId(logStreamId);

    offsetToMetadataBlockHeader = sizeof(KtlLogVerify::KtlMetadataHeader);
    metadataBlockHeader = (KLogicalLogInformation::MetadataBlockHeader*)((PUCHAR)metadataBuffer + offsetToMetadataBlockHeader);

    offsetToStreamBlockHeader = offsetToMetadataBlockHeader + sizeof(KLogicalLogInformation::MetadataBlockHeader);
    streamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)((PUCHAR)metadataBuffer + offsetToStreamBlockHeader);

    offsetToDataLocation = offsetToStreamBlockHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);
    dataPtr = (PUCHAR)metadataBuffer + offsetToDataLocation;

    RtlZeroMemory(metadataBuffer, KLogicalLogInformation::FixedMetadataSize);

    KIoBuffer::SPtr emptyKIoBuffer;
    status = KIoBuffer::CreateEmpty(emptyKIoBuffer, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader =
                                (KtlLogVerify::KtlMetadataHeader*)(metadataBuffer);

    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            recordAsn,
                                            version,
                                            0x1000,
                                            emptyKIoBuffer,
                                            emptyKIoBuffer);

    metadataBlockHeader->OffsetToStreamHeader = offsetToStreamBlockHeader + CoreLoggerMetadataOverhead;
    metadataBlockHeader->Flags = KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord;

    streamBlockHeader->Signature = KLogicalLogInformation::StreamBlockHeader::Sig;
    streamBlockHeader->StreamId = logStreamId;
    streamBlockHeader->StreamOffset = recordAsn.Get();
    streamBlockHeader->HighestOperationId = version;
    streamBlockHeader->DataSize = 128;

    memset(dataPtr, 128, contents);

    //
    // Write record
    //
    RvdLogStream::AsyncWriteContext::SPtr writeContext;

    status = logStream->CreateAsyncWriteContext(writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    writeContext->StartWrite(recordAsn,
                                version,
                                metadataKBuffer,
                                emptyKIoBuffer,
                                NULL,    // ParentAsync
                                sync);

    status = sync.WaitForCompletion();

    return(status);
}

NTSTATUS WriteARecord(
    RvdLogStream::SPtr& logStream,
    ULONGLONG version,
    RvdLogAsn recordAsn,
    UCHAR contents
)
{
    NTSTATUS status;

    //
    // Write records
    //
    {
        KSynchronizer sync;
        KIoBuffer::SPtr dataIoBuffer;
        ULONG myMetadataSize = 0x1000;

        status = KIoBuffer::CreateEmpty(dataIoBuffer,
                                        *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KIoBufferElement::SPtr dataElement;
        VOID* dataBuffer;

        status = KIoBufferElement::CreateNew(0x2000,        // Size
                                                  dataElement,
                                                  dataBuffer,
                                                  *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (int j = 0; j < 0x2000; j++)
        {
            ((PUCHAR)dataBuffer)[j] = (UCHAR)contents;
        }

        dataIoBuffer->AddIoBufferElement(*dataElement);


        ULONG metadataBufferSize = myMetadataSize;
        PUCHAR myMetadataBuffer;
        KBuffer::SPtr metadataKBuffer;

        status = KBuffer::Create(metadataBufferSize,
                                 metadataKBuffer,
                                 *g_Allocator,
                                 ALLOCATION_TAG);

        VERIFY_IS_TRUE(NT_SUCCESS(status));

        myMetadataBuffer = (PUCHAR)metadataKBuffer->GetBuffer();

        KIoBuffer::SPtr emptyKIoBuffer = nullptr;
        struct KtlLogVerify::KtlMetadataHeader* metaDataHeader =
                                 (KtlLogVerify::KtlMetadataHeader*)(myMetadataBuffer);

        KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                recordAsn,
                                                version,
                                                metadataBufferSize,
                                                emptyKIoBuffer,
                                                dataIoBuffer);
        //
        // Now write the record
        //
        RvdLogStream::AsyncWriteContext::SPtr writeContext;

        status = logStream->CreateAsyncWriteContext(writeContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeContext->StartWrite(recordAsn,
                                 version,
                                 metadataKBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);

        status = sync.WaitForCompletion();
    }

    return(status);
}

NTSTATUS WriteARawRecord(
    RvdLogStream::SPtr& logStream,
    ULONGLONG version,
    RvdLogAsn recordAsn,
    KBuffer::SPtr metadataKBuffer,
    ULONG metadataBufferSize,
    KIoBuffer::SPtr dataIoBuffer
    )
{
    NTSTATUS status;

    //
    // Write records
    //
    {
        KSynchronizer sync;
        PUCHAR myMetadataBuffer;

        myMetadataBuffer = (PUCHAR)metadataKBuffer->GetBuffer();

        KIoBuffer::SPtr emptyKIoBuffer = nullptr;
        struct KtlLogVerify::KtlMetadataHeader* metaDataHeader =
            (KtlLogVerify::KtlMetadataHeader*)(myMetadataBuffer);

        KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
            recordAsn,
            version,
            metadataBufferSize,
            emptyKIoBuffer,
            dataIoBuffer);
        //
        // Now write the record
        //
        RvdLogStream::AsyncWriteContext::SPtr writeContext;

        status = logStream->CreateAsyncWriteContext(writeContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeContext->StartWrite(recordAsn,
            version,
            metadataKBuffer,
            dataIoBuffer,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
    }

    return(status);
}


NTSTATUS ReadARecord(
    __in OverlayStream::SPtr overlayStream,
    __in RvdLogAsn RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt RvdLogAsn* const ActualRecordAsn,
    __out_opt ULONGLONG* const Version,
    __out UCHAR& contents
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    OverlayStream::AsyncReadContext::SPtr context;
    KIoBuffer::SPtr ioBuffer;
    KBuffer::SPtr metadataBuffer;

    status = overlayStream->CreateAsyncReadContext(context);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    context->StartRead(RecordAsn,
                         Type,
                         ActualRecordAsn,
                         Version,
                         metadataBuffer,
                         ioBuffer,
                         NULL,
                         sync);

    status = sync.WaitForCompletion();

    if (NT_SUCCESS(status))
    {
        PUCHAR p = (PUCHAR)ioBuffer->First()->GetBuffer();
        contents = *p;
    }

    return(status);
}

NTSTATUS TruncateAllStream(
    RvdLogStream::SPtr& logStream
)
{
    NTSTATUS status;
    KtlLogAsn recordAsn;
    KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
    KSynchronizer sync;

    status = logStream->QueryRecordRange(NULL,
                                         &recordAsn,
                                         NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream->Truncate(recordAsn,
                        recordAsn);

    return(STATUS_SUCCESS);
}

NTSTATUS DeleteRvdlogFile(
    KGuid& diskId,
    KGuid& guid
    )
{
    NTSTATUS status;
    KSynchronizer sync;

#if !defined(PLATFORM_UNIX)
    KWString rootPath(*g_Allocator, L"\\Global??\\Volume");
    rootPath += diskId;
    rootPath += L"\\RvdLog\\";
    VERIFY_IS_TRUE(NT_SUCCESS(rootPath.Status()));
#else
    KWString rootPath(*g_Allocator, L"/RvdLog/");
#endif
    
    KWString filePath(rootPath);
    filePath += L"Log";
    filePath += guid;
    filePath += L".log";
    VERIFY_IS_TRUE(NT_SUCCESS(filePath.Status()));

    for (ULONG i = 0; i < 5; i++)
    {
        status = KVolumeNamespace::DeleteFileOrDirectory(filePath, *g_Allocator, sync);
        VERIFY_IS_TRUE((K_ASYNC_SUCCESS(status)));
        status = sync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            break;
        } else {
            KNt::Sleep(2000);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS DeleteInitialKtlLogStream(
    KGuid& diskId,
    KGuid& logContainerGuid
    )
{
    NTSTATUS status;
    KtlLogManager::SPtr logManager;
    KServiceSynchronizer        serviceSync;
    KSynchronizer sync;

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

    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainer;
    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG i = 0; i < 5; i++)
    {
        deleteContainer->Reuse();
        deleteContainer->StartDeleteLogContainer(diskId,
                                                logContainerGuid,
                                                NULL,
                                                sync);
        status = sync.WaitForCompletion();
        if (NT_SUCCESS(status))
        {
            break;
        } else {
            KNt::Sleep(2000);
        }
    }

    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager = nullptr;

    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    return(STATUS_SUCCESS);
}

NTSTATUS CreateInitialKtlLogStream(
    KGuid& diskId,
    KGuid& logContainerGuid,
    ULONG  logStreamCount,
    KGuid* logStreamGuid,
    const RvdLogStreamType& logStreamType,
    ULONG MaxContainerRecordSize = DEFAULT_MAX_RECORD_SIZE,
    ULONG MaxStreamRecordSize = DEFAULT_MAX_RECORD_SIZE,
    KString::CSPtr Alias = nullptr,
    LONGLONG streamSize = DEFAULT_STREAM_SIZE,
    LONGLONG logSize = (LONGLONG)((LONGLONG)2 * (LONGLONG)1024 * (LONGLONG)0x100000)   // 2GB
    )
{
    NTSTATUS status;
    KtlLogManager::SPtr logManager;
    KtlLogContainer::SPtr logContainer;
    KtlLogContainerId logContainerId;
    KtlLogStreamId logStreamId;
    KSynchronizer sync;
    KServiceSynchronizer        serviceSync;
    ContainerCloseSynchronizer closeContainerSync;
    StreamCloseSynchronizer closeStreamSync;

    //
    // Setup the test by create a log container and a stream in it
    //
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

    //
    // create a log container
    //
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             MaxContainerRecordSize,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    for (ULONG i = 0; i < logStreamCount; i++)
    {
        KtlLogStream::SPtr logStream;
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        ULONG metadataLength = 0x10000;

        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid[i]);

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
                                                logStreamType,
                                                Alias,           // Alias
                                                nullptr,
                                                securityDescriptor,
                                                metadataLength,
                                                streamSize,
                                                MaxStreamRecordSize,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_SUCCESS);
        logStream = nullptr;
    }

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS);

    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager = nullptr;
    logContainer = nullptr;

    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    return(STATUS_SUCCESS);
}

NTSTATUS AddNewKtlLogStream(
    KGuid& diskId,
    KGuid& logContainerGuid,
    KGuid& logStreamGuid,
    ULONGLONG streamSize
    )
{
    NTSTATUS status;
    KtlLogManager::SPtr logManager;
    KtlLogContainer::SPtr logContainer;
    KtlLogContainerId logContainerId;
    KtlLogStreamId logStreamId;
    KSynchronizer sync;
    KServiceSynchronizer        serviceSync;
    ContainerCloseSynchronizer closeContainerSync;
    StreamCloseSynchronizer closeStreamSync;

    //
    // Setup the test by create a log container and a stream in it
    //
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

    //
    // create a log container
    //
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openContainerAsync;

        status = logManager->CreateAsyncOpenLogContainerContext(openContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openContainerAsync->StartOpenLogContainer(diskId,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    KtlLogStream::SPtr logStream;
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
    ULONG metadataLength = 0x10000;

    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG MaxStreamRecordSize = DEFAULT_MAX_RECORD_SIZE;
    KBuffer::SPtr securityDescriptor = nullptr;
    createStreamAsync->StartCreateLogStream(logStreamId,
                                            nullptr,           // Alias
                                            nullptr,
                                            securityDescriptor,
                                            metadataLength,
                                            streamSize,
                                            MaxStreamRecordSize,
                                            logStream,
                                            NULL,    // ParentAsync
                                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream->StartClose(NULL,
                     closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS);
    logStream = nullptr;

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS);

    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager = nullptr;
    logContainer = nullptr;
    logStream = nullptr;

    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    return(STATUS_SUCCESS);
}

VOID ReadTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KGuid logStreamGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;

    //
    // Build the initial structures for the OverlayStream
    //
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KtlLogInformation::KtlLogDefaultStreamType());

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId logId;
    RvdLogStreamId logStreamId;
    ULONG metadataLength = 0x10000;
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logId = logContainerGuid;
    logStreamId = logStreamGuid;
    coreOpenLog->StartOpenLog(diskId,
                              logId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

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
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogId rvdLogId = logContainerGuid;
    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Now the test is all setup, let's start testing
    //
    ULONGLONG asn = 1;
    UCHAR content;
    UCHAR expectedContent;
    RvdLogAsn actualAsn;
    RvdLogAsn expectedAsn;
    ULONGLONG actualVersion;
    ULONG expectedVersion;

    //
    // Test 1: Same ASN and version in both streams, expect record from
    // dedicated
    //
    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(sharedStream,
                             1,                 // Version
                             asn + i*10,
                             'S');
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARecord(dedicatedStream,
                             1,                 // Version
                             asn+ i*10,
                             'D');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    // Read Exact
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         expectedAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    // Read Next
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 4*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Previous
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 4*10,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Containing
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 + 3,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);


    //
    // Test 2: Record only in dedicated, expect read from dedicated
    // (all types)
    //
    asn = 2000;

    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(dedicatedStream,
                             1,                 // Version
                             asn+ i*10,
                             'D');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Read Exact
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         expectedAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    // Read Next
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 4*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Previous
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 4*10,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Containing
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 + 3,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);

    //
    // Test 3: Record only in shared, expect read from shared
    // (all types)
    //
    asn = 3000;

    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(sharedStream,
                             1,                 // Version
                             asn+ i*10,
                             'S');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Read Exact
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         expectedAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    // Read Next
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 4*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Previous
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 4*10,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Containing
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 + 3,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);

    //
    // Test 4: Record in neither stream, expect error
    // (all types)
    //
    asn = 4000;

    // Read Exact
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         expectedAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE((status == STATUS_NOT_FOUND) ? TRUE : FALSE);


    // Read Next
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 4*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE((status == STATUS_NOT_FOUND) ? TRUE : FALSE);

    // Read Previous
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 4*10,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE((status == STATUS_NOT_FOUND) ? TRUE : FALSE);

    // Read Containing
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 + 3,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE((status == STATUS_NOT_FOUND) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);

    //
    // Test 5: Higher version record in dedicated, expect read from
    // dedicated
    // (all types)
    //
    asn = 5000;

    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(sharedStream,
                             1,                 // Version
                             asn + i*10,
                             'S');
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARecord(dedicatedStream,
                             10,                 // Version
                             asn+ i*10,
                             'D');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Read Exact
    expectedContent = 'D';
    expectedVersion = 10;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         expectedAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    // Read Next
    expectedContent = 'D';
    expectedVersion = 10;
    expectedAsn = asn + 4*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Previous
    expectedContent = 'D';
    expectedVersion = 10;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 4*10,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Containing
    expectedContent = 'D';
    expectedVersion = 10;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 + 3,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);


    //
    // Test 6: Higher version record in shared, expect read from
    // shared
    // (all types)
    //
    asn = 6000;

    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(sharedStream,
                             100,                 // Version
                             asn + i*10,
                             'S');
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARecord(dedicatedStream,
                             10,                 // Version
                             asn+ i*10,
                             'D');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Read Exact
    expectedContent = 'S';
    expectedVersion = 100;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         expectedAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    // Read Next
    expectedContent = 'S';
    expectedVersion = 100;
    expectedAsn = asn + 4*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Previous
    expectedContent = 'S';
    expectedVersion = 100;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 4*10,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    // Read Containing
    expectedContent = 'S';
    expectedVersion = 100;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 + 3,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);

    //
    // Test 7: Record is before shared truncation point, expect read
    // from dedicated
    //

    // TODO:

    //
    // Test 8: For ReadNext, closest record is in dedicated, expect
    // read from dedicated
    //
    asn = 8000;

    //
    //  S  0, 10,  20,   30,   40,   50,   60,  70,   80,   90,
    //  D    5,  15,  25,   35,   45,   55,  65,   75,   85,   95
    //
    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(sharedStream,
                             1,                 // Version
                             asn + i*10,
                             'S');
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARecord(dedicatedStream,
                             1,                 // Version
                             asn+ i*10+5,
                             'D');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10+5;
    status = ReadARecord(overlayStream,
                         asn + 3*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    //
    // Test 9: For ReadNext, closes record is in shared, expect read
    // from shared
    //
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 4*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 +5,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    //
    // Test 8a: For ReadNext, next record is in dedicated, no next
    // record in shared expect read from dedicated
    //
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 9*10+5;
    status = ReadARecord(overlayStream,
                         asn + 9*10,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    //
    // Test 9a: For ReadNext, next record is in shared, no next record
    // in dedicated, expect read from shared
    //
    status = WriteARecord(sharedStream,
                         1,                 // Version
                         asn + 10*10,
                         'S');
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 10*10;
    status = ReadARecord(overlayStream,
                         asn + 9*10 +5,
                         RvdLogStream::AsyncReadContext::ReadNextRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    //
    // Test 10: For ReadPrev, closest record is in dedicated, expect
    // read from dedicated
    //
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 3*10+5;
    status = ReadARecord(overlayStream,
                         asn + 4*10,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    //
    // Test 11: For ReadPrev, closest record is in shared, expect read
    // from shared
    //
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 +5,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    //
    // Test 11a: For ReadPrev, closes record is in shared, expect read
    // from shared. Note: 11a and 10a are out of order due to the
    // record layout in the streams
    //
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 3*10;
    status = ReadARecord(overlayStream,
                         asn + 3*10 +5,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    //
    // Test 10a: For ReadPrev, prev record is in dedicated, no previous
    // record in shared, expect read from dedicated
    //

    // Slip in a record on the dedicated before the rest
    status = WriteARecord(dedicatedStream,
                         1,                 // Version
                         asn - 5,
                         'D');
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn - 5;
    status = ReadARecord(overlayStream,
                         asn,
                         RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);


    //  Streams at this point
    //
    //  S     0, 10,  20,   30,   40,   50,   60,  70,   80,   90,  ,100
    //  D  -5,  5,  15,  25,   35,   45,   55,  65,   75,   85,   95
    //

    //
    // Test 12: For ReadContaining, both records with same version,
    // closest (highest) record is in shared, expect read from shared
    //
    expectedContent = 'S';
    expectedVersion = 1;
    expectedAsn = asn + 40;
    status = ReadARecord(overlayStream,
                         asn + 42,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    //
    // Test 13: For ReadContaining, both records with same version,
    // closest (highest) record is in dedicaed, expect read from
    // dedicated
    //
    expectedContent = 'D';
    expectedVersion = 1;
    expectedAsn = asn + 55;
    status = ReadARecord(overlayStream,
                         asn + 57,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);

    //
    // Test 14: For ReadContaining, different ASNs, dedicated with
    // highest version, expect read from dedicated
    asn = 14000;

    //
    //  S  0, 10,  20,   30,   40,   50,   60,  70,   80,   90,    Version(1)
    //  D    5,  15,  25,   35,   45,   55,  65,   75,   85,   95  Version(2)
    //
    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(sharedStream,
                             1,                 // Version
                             asn + i*10,
                             'S');
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARecord(dedicatedStream,
                             2,                 // Version
                             asn+ i*10+5,
                             'D');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    expectedContent = 'D';
    expectedVersion = 2;
    expectedAsn = asn + 55;
    status = ReadARecord(overlayStream,
                         asn + 62,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);


    //
    // Test 15: For ReadContaining, different ASNs, shared with
    // highest version, expect read from shared
    asn = 15000;

    //
    //  S  0, 10,  20,   30,   40,   50,   60,  70,   80,   90,   Version(2)
    //  D    5,  15,  25,   35,   45,   55,  65,   75,   85,   95 Version(1)
    //
    for (ULONG i = 0; i < 10; i++)
    {
        status = WriteARecord(sharedStream,
                             2,                 // Version
                             asn + i*10,
                             'S');
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARecord(dedicatedStream,
                             1,                 // Version
                             asn+ i*10+5,
                             'D');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    expectedContent = 'S';
    expectedVersion = 2;
    expectedAsn = asn + 60;
    status = ReadARecord(overlayStream,
                         asn + 62,
                         RvdLogStream::AsyncReadContext::ReadContainingRecord,
                         &actualAsn,
                         &actualVersion,
                         content);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(content == expectedContent);
    VERIFY_IS_TRUE(actualVersion == expectedVersion);
    VERIFY_IS_TRUE((actualAsn == expectedAsn) ? TRUE : FALSE);

    TruncateAllStream(dedicatedStream);
    TruncateAllStream(sharedStream);

    //
    // Final Cleanup
    //  
    dedicatedStream = nullptr;
    sharedStream = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreLog = nullptr;
    coreLogManager->Deactivate();
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);
}


VOID MultiRecordCornerCaseTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType());
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;
    RvdLogStreamId logStreamId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    logStreamId = logStreamGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Open the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    OverlayLog::SPtr overlayLog;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Test 1: There is a specific case where MultiRecordRead will fail
    //
    // Dedicated Log records [ASN(version) - size]
    //    [1(3) - beef0]
    //
    // Shared Log Records
    //         [bccd1(4) - 2200]
    //
    //
    // Allocate buffers and stuff for writing data
    //
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;

    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();


    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    //
    // Write dedicated log records
    //
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                                logStreamId, 3, 1,
                                dataBufferPtr, 0xbeef0,
                                coreLoggerOffset, reservedMetadataSize, TRUE, 0xbeef0,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(dedicatedStream,
                             3, 1, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Now write shared log record
    //
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                                logStreamId, 4, 0xbccd1,
                                dataBufferPtr, 0x2200,
                                coreLoggerOffset, reservedMetadataSize, TRUE, 0x2200,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    status = WriteARawRecord(sharedStream,
                             4, 0xbccd1, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayStream->SetLogicalLogTailOffset(0xbeef1 + 0xbeef0);

    //
    // Perform multirecord read from asn 0x81 for metadatabuffer 0x1000
    // and ioBuffer 0x100000
    //

    OverlayStream::AsyncMultiRecordReadContextOverlay::SPtr context;
    KIoBuffer::SPtr ioBuffer;
    KIoBuffer::SPtr metadataBuffer;
    PVOID p;

    status = KIoBuffer::CreateSimple(0x1000, metadataBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KIoBuffer::CreateSimple(0x100000, ioBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayStream->CreateAsyncMultiRecordReadContextOverlay(context);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    context->StartRead(0x81,
                       *metadataBuffer,
                       *ioBuffer,
                       NULL,
                       sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // verify read size
    //


    //
    // Now write a follow on record [beef1(6) - beef0] and verify that read
    //
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
        logStreamId, 6, 0xbeef1,
        dataBufferPtr, 0xbeef0,
        coreLoggerOffset, reservedMetadataSize, TRUE, 0xbeef0,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(dedicatedStream,
        6, 0xbeef1, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    context->Reuse();
    context->StartRead(0x81,
        *metadataBuffer,
        *ioBuffer,
        NULL,
        sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    // verify read size

    context = nullptr;

    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    
    dedicatedStream = nullptr;
    sharedStream = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);

}


VOID MultiRecordCornerCase2Test(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType());
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;
    RvdLogStreamId logStreamId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    logStreamId = logStreamGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Open the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    OverlayLog::SPtr overlayLog;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Test 1: There is a specific case where MultiRecordRead will read
    //         invalid data
    //
    // Dedicated Log records [ASN(version) - size - record marker]
    //    [1(3) - beef0 - beef0]
    //    [beef1(6) - beef0 - a0000]
    //
    // Shared Log Records
    //         [15EEF0(7) - 2000]
    //         [160EF0(8) - 2000]
    //
    // Read from b0000
    // Allocate buffers and stuff for writing data
    //
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;

    status = KBuffer::Create(0x101000, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();

    for (ULONG i = 0; i < dataBuffer->QuerySize(); i++)
    {
        dataBufferPtr[i] = 'A';
    }

    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);


    //
    // Write dedicated log records
    //
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                                logStreamId, 3, 1,
                                dataBufferPtr, 0xbeef0,
                                coreLoggerOffset, reservedMetadataSize, TRUE, 0xbeef0,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(dedicatedStream,
                             3, 1, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG i = 0xA0000; i < dataBuffer->QuerySize(); i++)
    {
        dataBufferPtr[i] = 'B';
    }

    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                                logStreamId, 6, 0xbeef1,
                                dataBufferPtr, 0xbeef0,
                                coreLoggerOffset, reservedMetadataSize, TRUE, 0xa0000,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(dedicatedStream,
                             6, 0xbeef1, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Now write shared log record
    //
    for (ULONG i = 0; i < dataBuffer->QuerySize(); i++)
    {
        dataBufferPtr[i] = 'A';
    }

    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                                logStreamId, 7, 0xbeef1 + 0xa0000,
                                dataBufferPtr, 0x100,
                                coreLoggerOffset, reservedMetadataSize, TRUE, 0x100,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(sharedStream,
                             7, 0xbeef1 + 0xa0000, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(dedicatedStream,
                             7, 0xbeef1 + 0xa0000, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                                logStreamId, 8, 0xbeef1 + 0xa0100,
                                dataBufferPtr, 0x100,
                                coreLoggerOffset, reservedMetadataSize, TRUE, 0x100,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(sharedStream,
                             8, 0xbeef1 + 0xa0100, metadataKBuffer, 0, dataIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    ULONG tail = 0xbeef1 + 0xa0100 + 0x100;
    overlayStream->SetLogicalLogTailOffset(tail);

    //
    // Perform multirecord read from asn 0x81 for metadatabuffer 0x1000
    // and ioBuffer 0x100000
    //

    OverlayStream::AsyncMultiRecordReadContextOverlay::SPtr context;
    KIoBuffer::SPtr ioBuffer;
    KIoBuffer::SPtr metadataBuffer;
    PVOID p;

    status = KIoBuffer::CreateSimple(0x1000, metadataBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KIoBuffer::CreateSimple(0x100000, ioBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayStream->CreateAsyncMultiRecordReadContextOverlay(context);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    context->StartRead(0xb0000,
                       *metadataBuffer,
                       *ioBuffer,
                       NULL,
                       sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG dataSizeExpected = tail - 0xb0000;
    ULONGLONG versionExpected = 1;
    ULONG metadataSizeRead = 0;

    status = ValidateRecordData(0xb0000,
                                dataSizeExpected,
                                dataBufferPtr,
                                versionExpected,
                                logStreamId,
                                metadataSizeRead, // not used
                                metadataBuffer,
                                ioBuffer,
                                coreLoggerOffset + reservedMetadataSize,
                                TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    context = nullptr;

    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);

    dedicatedStream = nullptr;
    sharedStream = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);

}


NTSTATUS
VerifyRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const,
    __in ULONG,
    __in const KIoBuffer::SPtr&
)
{
    return(STATUS_SUCCESS);
}

VOID RecoveryTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KtlLogInformation::KtlLogDefaultStreamType());
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                          &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                           &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Test 1: Open shared stream and write a set of records to it and
    // then open the overlay container for that stream. The overlay
    // container is expected to copy all records from the shared stream
    // to the dedicated stream and truncate the dedicated stream
    //
    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreSharedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONGLONG asn = 2000;
    ULONG recordCount = 25;
    RvdLogAsn highWriteAsn = RvdLogAsn::Min();

    for (ULONG i = 0; i < recordCount; i++)
    {
        status = WriteARecord(coreSharedLogStream,
                             1,                 // Version
                             asn + i*10,
                             'X');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        highWriteAsn.SetIfLarger(asn + i*10);
    }

    //
    // Include a few out of order records
    //
    for (ULONG i = 0; i < recordCount / 3; i++)
    {
        status = WriteARecord(coreSharedLogStream,
                             1,                 // Version
                             asn + i*10 +5,
                             'X');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        highWriteAsn.SetIfLarger(asn + i*10 +5);
    }

    coreSharedLogStream = nullptr;

    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    OverlayLog::SPtr overlayLog;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Verify records are in dedicated and not shared
    //
    RvdLogAsn lowAsn;
    RvdLogAsn highAsn;
    RvdLogAsn truncationAsn;

    sharedStream->QueryRecordRange(&lowAsn,
                                   &highAsn,
                                   &truncationAsn);
    VERIFY_IS_TRUE(OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn) ? TRUE : FALSE);

    dedicatedStream->QueryRecordRange(&lowAsn,
                                      &highAsn,
                                      NULL);
    VERIFY_IS_TRUE((lowAsn == asn) ? TRUE : FALSE);
    VERIFY_IS_TRUE((highAsn == asn + (recordCount-1)*10) ? TRUE : FALSE);

    //
    // Final Cleanup
    //
    dedicatedStream = nullptr;
    sharedStream = nullptr;

    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;
    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;   
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);

}



typedef struct
{
    ULONGLONG asn;
    ULONGLONG version;
    ULONG dataOffset;
    ULONG length;
    ULONG barrier;
} RecordToWrite;


VOID Recovery2Test(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType());

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreSharedLog;
    RvdLog::SPtr coreDedicatedLog;
    RvdLogId rvdLogSharedId;
    RvdLogId rvdLogDedicatedId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                          &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    rvdLogSharedId = logContainerGuid;
    rvdLogDedicatedId = logStreamGuid;
    RvdLogStreamId logStreamId = logStreamGuid;
    
    RvdLog::AsyncOpenLogStreamContext::SPtr coreSharedOpenStream;
    RvdLog::AsyncOpenLogStreamContext::SPtr coreDedicatedOpenStream;
    RvdLogStream::SPtr coreSharedLogStream;
    RvdLogStream::SPtr coreDedicatedLogStream;

    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreOpenLog->StartOpenLog(diskId,
                              rvdLogSharedId,
                              coreSharedLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    ULONG coreLoggerOffset = coreSharedLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);   

    coreSharedLog = nullptr;
    coreOpenLog = nullptr;
    
    OverlayLog::SPtr overlayLog;
    RvdLogId rvdLogId = logContainerGuid;
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    srand((ULONG)GetTickCount());   

    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    ULONGLONG asnBase = KtlLogAsn::Min().Get();
    ULONGLONG versionBase = 0;

    //
    // Test 1: Write a set of smaller records in the shared log and a
    //         set of larger records in the dedicated log and then
    //         cause destaging at open to occur. Both shared and
    //         dedicated have exactly the same data but have
    //         interleaving records. The record marker is at the end
    //         of each shared record. Read the data and verify correctness.
    //
    {
        printf("Test 1\n");
        ULONGLONG lastBarrier = 0;
        
        status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogSharedId,
                                  coreSharedLog,
                                  NULL,
                                  sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    
        coreOpenLog->Reuse();
        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogDedicatedId,
                                  coreDedicatedLog,
                                  NULL,
                                  sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLog->CreateAsyncOpenLogStreamContext(coreSharedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLog->CreateAsyncOpenLogStreamContext(coreDedicatedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        
        static const ULONG fourMB = 4 * (1024 * 1024);
        KBuffer::SPtr dataBuffer;
        PUCHAR data;
        UCHAR entropy = 99;

        status = KBuffer::Create(fourMB, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        data = (PUCHAR)dataBuffer->GetBuffer();
        for (ULONG i = 0; i < fourMB; i++)
        {
            data[i] = (static_cast<UCHAR>(i) * entropy) % 255;
        }

        //
        // Randomly create 512 shared records of different sizes and a
        // the same time map the data into the dedicated records
        //
        static const ULONG dedicatedRecordsSize = 768 * 1024;
        static const ULONG dedicatedRecordsCount = (fourMB / dedicatedRecordsSize) + 1;
        static const ULONG sharedRecordsCount = 512;
        static const ULONG randomSize = (fourMB / sharedRecordsCount) - 64;
        RecordToWrite dedicatedRecords[dedicatedRecordsCount];
        RecordToWrite sharedRecords[sharedRecordsCount];

        ULONGLONG asn = 0;
        ULONGLONG version = 0;
        
        ULONG size;
        ULONG dataOffset;

        ULONG iDedicated;
        ULONG dedicatedSize;
        
        iDedicated = 0;
        dedicatedSize = 0;
        
        dataOffset = 0;

        dedicatedRecords[iDedicated].asn = asnBase;
        dedicatedRecords[iDedicated].dataOffset = dataOffset;
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            size = 64 + (rand() % randomSize);
            
            sharedRecords[i].asn = asn + asnBase;
            sharedRecords[i].version = version + versionBase;
            sharedRecords[i].dataOffset = dataOffset;
            sharedRecords[i].length = size;         
            sharedRecords[i].barrier = size;
            lastBarrier = asn + size;

            printf("Set Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d %s Barrier\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier ? "Is" : "Is Not");
            
            dedicatedRecords[iDedicated].version = sharedRecords[i].version;
            
            ULONG leftover = 0;
            if ((dedicatedSize + size) >= dedicatedRecordsSize)
            {
                //
                // In this case, the new shared record will fill the
                // dedicatated record and so we note this in the
                // dedicated records list
                //
                leftover = (dedicatedSize + size) - dedicatedRecordsSize;
                if (leftover == 0)
                {
                    if (sharedRecords[i].barrier != 0)
                    {
                        dedicatedRecords[iDedicated].barrier = dedicatedRecordsSize;
                    }
                }
                                
                dedicatedRecords[iDedicated].length = dedicatedRecordsSize;
                
                iDedicated++;
                dedicatedRecords[iDedicated].asn = dedicatedRecords[iDedicated-1].asn + dedicatedRecordsSize;
                dedicatedRecords[iDedicated].dataOffset = dedicatedRecords[iDedicated-1].dataOffset + dedicatedRecordsSize;
                
                dedicatedSize = leftover;
                dedicatedRecords[iDedicated].barrier = leftover;
                dedicatedRecords[iDedicated].length = leftover;
            } else {
                dedicatedRecords[iDedicated].length += size;
                if (sharedRecords[i].barrier != 0)
                {
                    dedicatedRecords[iDedicated].barrier = dedicatedRecords[iDedicated].length;
                }
                dedicatedSize += size;
            }

            printf("Set dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Leftover %d Barrier %d\n",
                                        iDedicated,
                                        dedicatedRecords[iDedicated].asn,
                                        dedicatedRecords[iDedicated].version,
                                        dedicatedRecords[iDedicated].length,
                                        dedicatedRecords[iDedicated].dataOffset,
                                        leftover,
                                        dedicatedRecords[iDedicated].barrier);
                        
            asn += size;
            version++;
            dataOffset += size; 
        }

        if (dedicatedSize > 0)
        {
            dedicatedRecords[iDedicated].length = dedicatedSize;
            dedicatedRecords[iDedicated].barrier = dedicatedSize;
            iDedicated++;
        }        
        
        //
        // gain access to the shared and dedicated log streams
        //
        RvdLogStream::AsyncWriteContext::SPtr writeSharedContext;
        RvdLogStream::AsyncWriteContext::SPtr writeDedicatedContext;
        
        coreSharedOpenStream->Reuse();
        coreSharedOpenStream->StartOpenLogStream(logStreamId,
                                           coreSharedLogStream,
                                           NULL,
                                           sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreDedicatedOpenStream->Reuse();
        coreDedicatedOpenStream->StartOpenLogStream(logStreamId,
                                           coreDedicatedLogStream,
                                           NULL,
                                           sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLogStream->CreateAsyncWriteContext(writeSharedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLogStream->CreateAsyncWriteContext(writeDedicatedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Write large records to shared and dedicated logs
        //
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            printf("Write Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier);
            
            status = SetupToWriteRecord(metadataKBuffer,
                                        dataIoBuffer,
                                        logStreamId,
                                        sharedRecords[i].version,
                                        sharedRecords[i].asn,
                                        data + sharedRecords[i].dataOffset,
                                        sharedRecords[i].length,
                                        coreLoggerOffset,
                                        reservedMetadataSize,
                                        FALSE,
                                        sharedRecords[i].barrier);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogVerify::KtlMetadataHeader* metadataHeader;
            metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeSharedContext->Reuse();
            writeSharedContext->StartWrite(sharedRecords[i].asn, sharedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));                 
        }
        printf("\n");
        
        
        for (ULONG i = 0; i < iDedicated; i++)
        {
            printf("Write Dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                        i,
                                        dedicatedRecords[i].asn,
                                        dedicatedRecords[i].version,
                                        dedicatedRecords[i].length,
                                        dedicatedRecords[i].dataOffset,
                                        dedicatedRecords[i].barrier);
            status = SetupToWriteRecord(metadataKBuffer,
                                        dataIoBuffer,
                                        logStreamId,
                                        dedicatedRecords[i].version,
                                        dedicatedRecords[i].asn,
                                        data + dedicatedRecords[i].dataOffset,
                                        dedicatedRecords[i].length,
                                        coreLoggerOffset,
                                        reservedMetadataSize,
                                        FALSE,
                                        dedicatedRecords[i].barrier);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogVerify::KtlMetadataHeader* metadataHeader;
            metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeDedicatedContext->Reuse();
            writeDedicatedContext->StartWrite(dedicatedRecords[i].asn, dedicatedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        printf("\n");
                
        writeSharedContext = nullptr;
        writeDedicatedContext = nullptr;
        coreSharedLogStream = nullptr;
        coreDedicatedLogStream = nullptr;
        
        coreDedicatedOpenStream = nullptr;
        coreSharedOpenStream = nullptr;

        //
        // Reopen the streams via the overlay log. This will move records
        // from shared to dedicated
        //
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreSharedLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        printf("Opened log - Tail Asn %llx, Highest Version %llx\n",
               overlayStream->GetLogicalLogTailOffset().Get(),
               overlayStream->GetLogicalLogLastVersion());

        VERIFY_IS_TRUE(overlayStream->GetLogicalLogTailOffset().Get() == (asnBase + lastBarrier) - 1);
        VERIFY_IS_TRUE(overlayStream->GetLogicalLogLastVersion() == (versionBase + version) - 1);
        
        //
        // Verify records are in dedicated and not shared.
        //
        OverlayStream::AsyncReadContext::SPtr readContext1;
        OverlayStream::AsyncReadContextOverlay::SPtr readContext;
        
        status = overlayStream->CreateAsyncReadContext(readContext1);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
        readContext1 = nullptr;

        ULONGLONG readAsn;
        RvdLogAsn actualReadAsn;
        ULONGLONG actualReadVersion;
        KBuffer::SPtr readMetadata;
        KIoBuffer::SPtr readIoBuffer;
        KIoBuffer::SPtr verifyIoBuffer;
        BOOLEAN readFromDedicated;
        ULONG readDataSize;
        ULONG readDataOffset;
        PUCHAR expectedData;
        KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;

        
        readAsn = asnBase;
        do
        {
            printf("Read: ReadAsn %llx ... ",
                   readAsn);
            
            readContext->Reuse();
            readContext->StartRead(readAsn,
                                   RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                                   &actualReadAsn,
                                   &actualReadVersion,
                                   readMetadata,
                                   readIoBuffer,
                                   &readFromDedicated,
                                   NULL,
                                   sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            printf("ActualAsn %llx ActualVersion %llx\n",
                   actualReadAsn.Get(),
                   actualReadVersion);
            
            expectedData = data + (actualReadAsn.Get() - asnBase);
            
            status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(readMetadata,
                                                                                       coreLoggerOffset,
                                                                                       *readIoBuffer,
                                                                                       reservedMetadataSize,
                                                                                       metadataBlockHeader,
                                                                                       streamBlockHeader,
                                                                                       readDataSize,
                                                                                       readDataOffset);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBuffer::SPtr metadataIoBuffer;
            PVOID metadataIoBufferPtr;

            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize, metadataIoBuffer, metadataIoBufferPtr, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KMemCpySafe((PVOID)((PUCHAR)metadataIoBufferPtr + coreLoggerOffset),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset,
                        readMetadata->GetBuffer(),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
            
            status = KIoBuffer::CreateEmpty(verifyIoBuffer,
                                            *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*metadataIoBuffer,
                                                          0,
                                                          metadataIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*readIoBuffer,
                                                          0,
                                                          readIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBufferStream stream(*verifyIoBuffer,
                                    readDataOffset);

            for (ULONG i = 0; i < readDataSize; i++)
            {
                UCHAR byte;
                BOOLEAN b = stream.Pull(byte);
                if (!b) VERIFY_IS_TRUE(b ? true : false);
                if (byte != expectedData[i]) VERIFY_IS_TRUE((byte == expectedData[i]) ? true : false);
            }

            readAsn = readAsn + (rand() % (512 * 1024));
            
        } while (readAsn < (asnBase + lastBarrier));

        readContext = nullptr;
        
        //
        // Advance ASN and version for next test
        //
        asnBase += lastBarrier;
        versionBase += version;
        
        //
        // Cleanup
        //
        coreOpenLog = nullptr;
        coreSharedLog = nullptr;
        coreDedicatedLog = nullptr;
        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;
    }

    //
    // Test 2: Write a set of smaller records in the shared log and a
    //         set of larger records in the dedicated log and then
    //         cause destaging at open to occur. Both shared and
    //         dedicated have exactly the same data and the record
    //         marker is in the middle of the data. Read the data and
    //         verify correctness.
    //
    {
        printf("Test 2\n");
        
        ULONGLONG lastBarrier  = 0;
        KSynchronizer sync2;
        status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogSharedId,
                                  coreSharedLog,
                                  NULL,
                                  sync2);
        status = sync2.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    
        coreOpenLog->Reuse();
        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogDedicatedId,
                                  coreDedicatedLog,
                                  NULL,
                                  sync2);
        status = sync2.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLog->CreateAsyncOpenLogStreamContext(coreSharedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLog->CreateAsyncOpenLogStreamContext(coreDedicatedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        static const ULONG fourMB = 4 * (1024 * 1024);
        KBuffer::SPtr dataBuffer;
        PUCHAR data;
        UCHAR entropy = 77;

        status = KBuffer::Create(fourMB, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        data = (PUCHAR)dataBuffer->GetBuffer();
        for (ULONG i = 0; i < fourMB; i++)
        {
            data[i] = (static_cast<UCHAR>(i) * entropy) % 255;
        }

        //
        // Randomly create 512 shared records of different sizes and a
        // the same time map the data into the dedicated records
        //
        static const ULONG dedicatedRecordsSize = 768 * 1024;
        static const ULONG dedicatedRecordsCount = (fourMB / dedicatedRecordsSize) + 1;
        static const ULONG sharedRecordsCount = 512;
        static const ULONG randomSize = (fourMB / sharedRecordsCount) - 64;
        RecordToWrite dedicatedRecords[dedicatedRecordsCount];
        RecordToWrite sharedRecords[sharedRecordsCount];

        ULONGLONG asn = 0;
        ULONGLONG version = 0;
        
        ULONG size;
        ULONG dataOffset;

        ULONG iDedicated;
        ULONG dedicatedSize;
        
        iDedicated = 0;
        dedicatedSize = 0;
        
        dataOffset = 0;

        dedicatedRecords[iDedicated].asn = asnBase;
        dedicatedRecords[iDedicated].dataOffset = dataOffset;
        dedicatedRecords[iDedicated].barrier = 0;
        
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            size = 64 + (rand() % randomSize);
            
            sharedRecords[i].asn = asn + asnBase;
            sharedRecords[i].version = version + versionBase;
            sharedRecords[i].dataOffset = dataOffset;
            sharedRecords[i].length = size;

            if ((size & 2) == 0)
            {
                sharedRecords[i].barrier = size;
                lastBarrier = asn + size;
            } else {
                sharedRecords[i].barrier = 0;
            }

            printf("Set Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d %s Barrier\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier ? "Is" : "Is Not");
            
            dedicatedRecords[iDedicated].version = sharedRecords[i].version;
            
            ULONG leftover = 0;
            if ((dedicatedSize + size) >= dedicatedRecordsSize)
            {
                //
                // In this case, the new shared record will fill the
                // dedicatated record and so we note this in the
                // dedicated records list
                //
                leftover = (dedicatedSize + size) - dedicatedRecordsSize;
                if (leftover == 0)
                {
                    if (sharedRecords[i].barrier != 0)
                    {
                        dedicatedRecords[iDedicated].barrier = dedicatedRecordsSize;
                    }
                }
                                
                dedicatedRecords[iDedicated].length = dedicatedRecordsSize;
                
                iDedicated++;
                dedicatedRecords[iDedicated].asn = dedicatedRecords[iDedicated-1].asn + dedicatedRecordsSize;
                dedicatedRecords[iDedicated].dataOffset = dedicatedRecords[iDedicated-1].dataOffset + dedicatedRecordsSize;
                
                dedicatedSize = leftover;
                dedicatedRecords[iDedicated].barrier = leftover;
                dedicatedRecords[iDedicated].length = leftover;
            } else {
                dedicatedRecords[iDedicated].length += size;
                if (sharedRecords[i].barrier != 0)
                {
                    dedicatedRecords[iDedicated].barrier = dedicatedRecords[iDedicated].length;
                }
                dedicatedSize += size;
            }

            printf("Set dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Leftover %d Barrier %d\n",
                                        iDedicated,
                                        dedicatedRecords[iDedicated].asn,
                                        dedicatedRecords[iDedicated].version,
                                        dedicatedRecords[iDedicated].length,
                                        dedicatedRecords[iDedicated].dataOffset,
                                        leftover,
                                        dedicatedRecords[iDedicated].barrier);
            
            
            asn += size;
            version++;
            dataOffset += size; 
        }

        if (dedicatedSize > 0)
        {
            iDedicated++;
        }        
        
        //
        // gain access to the shared and dedicated log streams
        //
        RvdLogStream::AsyncWriteContext::SPtr writeSharedContext;
        RvdLogStream::AsyncWriteContext::SPtr writeDedicatedContext;
        
        coreSharedOpenStream->Reuse();
        coreSharedOpenStream->StartOpenLogStream(logStreamId,
                                           coreSharedLogStream,
                                           NULL,
                                           sync2);
        status = sync2.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreDedicatedOpenStream->Reuse();
        coreDedicatedOpenStream->StartOpenLogStream(logStreamId,
                                           coreDedicatedLogStream,
                                           NULL,
                                           sync2);
        status = sync2.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLogStream->CreateAsyncWriteContext(writeSharedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLogStream->CreateAsyncWriteContext(writeDedicatedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Write large records to shared and dedicated logs
        //
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            printf("Write Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier);
            
            status = SetupToWriteRecord(metadataKBuffer,
                                        dataIoBuffer,
                                        logStreamId,
                                        sharedRecords[i].version,
                                        sharedRecords[i].asn,
                                        data + sharedRecords[i].dataOffset,
                                        sharedRecords[i].length,
                                        coreLoggerOffset,
                                        reservedMetadataSize,
                                        FALSE,
                                        sharedRecords[i].barrier);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogVerify::KtlMetadataHeader* metadataHeader;
            metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeSharedContext->Reuse();
            writeSharedContext->StartWrite(sharedRecords[i].asn, sharedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync2);
            status = sync2.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));                 
        }
        printf("\n");
        
        
        for (ULONG i = 0; i < iDedicated; i++)
        {
            printf("Write Dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                        i,
                                        dedicatedRecords[i].asn,
                                        dedicatedRecords[i].version,
                                        dedicatedRecords[i].length,
                                        dedicatedRecords[i].dataOffset,
                                        dedicatedRecords[i].barrier);
            status = SetupToWriteRecord(metadataKBuffer,
                                        dataIoBuffer,
                                        logStreamId,
                                        dedicatedRecords[i].version,
                                        dedicatedRecords[i].asn,
                                        data + dedicatedRecords[i].dataOffset,
                                        dedicatedRecords[i].length,
                                        coreLoggerOffset,
                                        reservedMetadataSize,
                                        FALSE,
                                        dedicatedRecords[i].barrier);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogVerify::KtlMetadataHeader* metadataHeader;
            metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeDedicatedContext->Reuse();
            writeDedicatedContext->StartWrite(dedicatedRecords[i].asn, dedicatedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync2);
            status = sync2.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        printf("\n");
                
        writeSharedContext = nullptr;
        writeDedicatedContext = nullptr;
        coreSharedLogStream = nullptr;
        coreDedicatedLogStream = nullptr;
        
        coreDedicatedOpenStream = nullptr;
        coreSharedOpenStream = nullptr;

        //
        // Reopen the streams via the overlay log. This will move records
        // from shared to dedicated
        //        
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreSharedLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        printf("Opened log - Tail Asn %llx, Highest Version %llx\n",
               overlayStream->GetLogicalLogTailOffset().Get(),
               overlayStream->GetLogicalLogLastVersion());

        VERIFY_IS_TRUE(overlayStream->GetLogicalLogTailOffset().Get() == (asnBase + lastBarrier) - 1);
        VERIFY_IS_TRUE(overlayStream->GetLogicalLogLastVersion() == (versionBase + version) - 1);
        
        //
        // Verify records are in dedicated and not shared.
        //
        OverlayStream::AsyncReadContext::SPtr readContext1;
        OverlayStream::AsyncReadContextOverlay::SPtr readContext;
        
        status = overlayStream->CreateAsyncReadContext(readContext1);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
        readContext1 = nullptr;

        ULONGLONG readAsn;
        RvdLogAsn actualReadAsn;
        ULONGLONG actualReadVersion;
        KBuffer::SPtr readMetadata;
        KIoBuffer::SPtr readIoBuffer;
        KIoBuffer::SPtr verifyIoBuffer;
        BOOLEAN readFromDedicated;
        ULONG readDataSize;
        ULONG readDataOffset;
        PUCHAR expectedData;
        KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
        
        readAsn = asnBase;
        do
        {
            printf("Read: ReadAsn %llx ... ",
                   readAsn);
            
            readContext->Reuse();
            readContext->StartRead(readAsn,
                                   RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                                   &actualReadAsn,
                                   &actualReadVersion,
                                   readMetadata,
                                   readIoBuffer,
                                   &readFromDedicated,
                                   NULL,
                                   sync2);
            status = sync2.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            printf("ActualAsn %llx ActualVersion %llx\n",
                   actualReadAsn.Get(),
                   actualReadVersion);
            
            expectedData = data + (actualReadAsn.Get() - asnBase);
            
            status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(readMetadata,
                                                                                       coreLoggerOffset,
                                                                                       *readIoBuffer,
                                                                                       reservedMetadataSize,
                                                                                       metadataBlockHeader,
                                                                                       streamBlockHeader,
                                                                                       readDataSize,
                                                                                       readDataOffset);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBuffer::SPtr metadataIoBuffer;
            PVOID metadataIoBufferPtr;

            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize, metadataIoBuffer, metadataIoBufferPtr, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KMemCpySafe((PVOID)((PUCHAR)metadataIoBufferPtr + coreLoggerOffset),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset,
                        readMetadata->GetBuffer(),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
            
            status = KIoBuffer::CreateEmpty(verifyIoBuffer,
                                            *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*metadataIoBuffer,
                                                          0,
                                                          metadataIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*readIoBuffer,
                                                          0,
                                                          readIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBufferStream stream(*verifyIoBuffer,
                                    readDataOffset);

            for (ULONG i = 0; i < readDataSize; i++)
            {
                UCHAR byte;
                BOOLEAN b = stream.Pull(byte);
                if (!b) VERIFY_IS_TRUE(b ? true : false);
                if (byte != expectedData[i]) VERIFY_IS_TRUE((byte == expectedData[i]) ? true : false);
            }

            readAsn = readAsn + (rand() % (512 * 1024));
            
        } while (readAsn < (asnBase + lastBarrier));

        readContext = nullptr;
        
        //
        // Advance ASN and version for next test
        //
        asnBase += lastBarrier;
        versionBase += version;
        
        //
        // Cleanup
        //        
        coreOpenLog = nullptr;
        coreSharedLog = nullptr;
        coreDedicatedLog = nullptr;
        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync2);
        status = sync2.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;
    }

    //
    // Test 3: Write a set of smaller records in the shared log and a
    //         set of larger records in the dedicated log and then
    //         cause destaging at open to occur. The shared log has
    //         more data than the dedicated log and the record
    //         marker is at the end of each record. Read the data and
    //         verify correctness.
    //  
    {
        printf("Test 3\n");
        
        ULONGLONG lastBarrier  = 0;
        KSynchronizer sync3;
        
        status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogSharedId,
                                  coreSharedLog,
                                  NULL,
                                  sync3);
        status = sync3.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    
        coreOpenLog->Reuse();
        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogDedicatedId,
                                  coreDedicatedLog,
                                  NULL,
                                  sync3);
        status = sync3.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLog->CreateAsyncOpenLogStreamContext(coreSharedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLog->CreateAsyncOpenLogStreamContext(coreDedicatedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        static const ULONG fourMB = 4 * (1024 * 1024);
        KBuffer::SPtr dataBuffer;
        PUCHAR data;
        UCHAR entropy = 111;

        status = KBuffer::Create(fourMB, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        data = (PUCHAR)dataBuffer->GetBuffer();
        for (ULONG i = 0; i < fourMB; i++)
        {
            data[i] = (static_cast<UCHAR>(i) * entropy) % 255;
        }

        //
        // Randomly create 512 shared records of different sizes and a
        // the same time map the data into the dedicated records
        //
        static const ULONG dedicatedRecordsSize = 768 * 1024;
        static const ULONG dedicatedRecordsCount = (fourMB / dedicatedRecordsSize) + 1;
        static const ULONG sharedRecordsCount = 512;
        static const ULONG randomSize = (fourMB / sharedRecordsCount) - 64;
        RecordToWrite dedicatedRecords[dedicatedRecordsCount];
        RecordToWrite sharedRecords[sharedRecordsCount];

        ULONGLONG asn = 0;
        ULONGLONG version = 0;
        
        ULONG size;
        ULONG dataOffset;

        ULONG iDedicated;
        ULONG dedicatedSize;
        
        iDedicated = 0;
        dedicatedSize = 0;
        
        dataOffset = 0;

        dedicatedRecords[iDedicated].asn = asnBase;
        dedicatedRecords[iDedicated].dataOffset = dataOffset;
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            size = 64 + (rand() % randomSize);
            
            sharedRecords[i].asn = asn + asnBase;
            sharedRecords[i].version = version + versionBase;
            sharedRecords[i].dataOffset = dataOffset;
            sharedRecords[i].length = size;         
            sharedRecords[i].barrier = size;
            lastBarrier = asn + size;

            printf("Set Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d %s Barrier\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier ? "Is" : "Is Not");
            
            dedicatedRecords[iDedicated].version = sharedRecords[i].version;
            
            ULONG leftover = 0;
            if ((dedicatedSize + size) >= dedicatedRecordsSize)
            {
                //
                // In this case, the new shared record will fill the
                // dedicatated record and so we note this in the
                // dedicated records list
                //
                leftover = (dedicatedSize + size) - dedicatedRecordsSize;
                if (leftover == 0)
                {
                    if (sharedRecords[i].barrier != 0)
                    {
                        dedicatedRecords[iDedicated].barrier = dedicatedRecordsSize;
                    }
                }
                                
                dedicatedRecords[iDedicated].length = dedicatedRecordsSize;
                
                iDedicated++;
                dedicatedRecords[iDedicated].asn = dedicatedRecords[iDedicated-1].asn + dedicatedRecordsSize;
                dedicatedRecords[iDedicated].dataOffset = dedicatedRecords[iDedicated-1].dataOffset + dedicatedRecordsSize;
                
                dedicatedSize = leftover;
                dedicatedRecords[iDedicated].barrier = leftover;
                dedicatedRecords[iDedicated].length = leftover;
            } else {
                dedicatedRecords[iDedicated].length += size;
                if (sharedRecords[i].barrier != 0)
                {
                    dedicatedRecords[iDedicated].barrier = dedicatedRecords[iDedicated].length;
                }
                dedicatedSize += size;
            }

            printf("Set dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Leftover %d Barrier %d\n",
                                        iDedicated,
                                        dedicatedRecords[iDedicated].asn,
                                        dedicatedRecords[iDedicated].version,
                                        dedicatedRecords[iDedicated].length,
                                        dedicatedRecords[iDedicated].dataOffset,
                                        leftover,
                                        dedicatedRecords[iDedicated].barrier);
                        
            asn += size;
            version++;
            dataOffset += size; 
        }

#if 0  // Do not write to cause more data in shared than in dedicated
        if (dedicatedSize > 0)
        {
            dedicatedRecords[iDedicated].length = dedicatedSize;
            dedicatedRecords[iDedicated].barrier = dedicatedSize;
            iDedicated++;
        }        
#endif
        
        //
        // gain access to the shared and dedicated log streams
        //
        RvdLogStream::AsyncWriteContext::SPtr writeSharedContext;
        RvdLogStream::AsyncWriteContext::SPtr writeDedicatedContext;
        
        coreSharedOpenStream->Reuse();
        coreSharedOpenStream->StartOpenLogStream(logStreamId,
                                           coreSharedLogStream,
                                           NULL,
                                           sync3);
        status = sync3.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreDedicatedOpenStream->Reuse();
        coreDedicatedOpenStream->StartOpenLogStream(logStreamId,
                                           coreDedicatedLogStream,
                                           NULL,
                                           sync3);
        status = sync3.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLogStream->CreateAsyncWriteContext(writeSharedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLogStream->CreateAsyncWriteContext(writeDedicatedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Write large records to shared and dedicated logs
        //
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            printf("Write Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier);
            
            status = SetupToWriteRecord(metadataKBuffer,
                                        dataIoBuffer,
                                        logStreamId,
                                        sharedRecords[i].version,
                                        sharedRecords[i].asn,
                                        data + sharedRecords[i].dataOffset,
                                        sharedRecords[i].length,
                                        coreLoggerOffset,
                                        reservedMetadataSize,
                                        FALSE,
                                        sharedRecords[i].barrier);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogVerify::KtlMetadataHeader* metadataHeader;
            metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeSharedContext->Reuse();
            writeSharedContext->StartWrite(sharedRecords[i].asn, sharedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync3);
            status = sync3.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));                 
        }
        printf("\n");
        
        
        for (ULONG i = 0; i < iDedicated; i++)
        {
            printf("Write Dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                        i,
                                        dedicatedRecords[i].asn,
                                        dedicatedRecords[i].version,
                                        dedicatedRecords[i].length,
                                        dedicatedRecords[i].dataOffset,
                                        dedicatedRecords[i].barrier);
            status = SetupToWriteRecord(metadataKBuffer,
                                        dataIoBuffer,
                                        logStreamId,
                                        dedicatedRecords[i].version,
                                        dedicatedRecords[i].asn,
                                        data + dedicatedRecords[i].dataOffset,
                                        dedicatedRecords[i].length,
                                        coreLoggerOffset,
                                        reservedMetadataSize,
                                        FALSE,
                                        dedicatedRecords[i].barrier);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogVerify::KtlMetadataHeader* metadataHeader;
            metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeDedicatedContext->Reuse();
            writeDedicatedContext->StartWrite(dedicatedRecords[i].asn, dedicatedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync3);
            status = sync3.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        printf("\n");
                
        writeSharedContext = nullptr;
        writeDedicatedContext = nullptr;
        coreSharedLogStream = nullptr;
        coreDedicatedLogStream = nullptr;
        
        coreDedicatedOpenStream = nullptr;
        coreSharedOpenStream = nullptr;

        //
        // Reopen the streams via the overlay log. This will move records
        // from shared to dedicated
        //
        
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreSharedLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        printf("Opened log - Tail Asn %llx, Highest Version %llx\n",
               overlayStream->GetLogicalLogTailOffset().Get(),
               overlayStream->GetLogicalLogLastVersion());

        VERIFY_IS_TRUE(overlayStream->GetLogicalLogTailOffset().Get() == (asnBase + lastBarrier) - 1);
        VERIFY_IS_TRUE(overlayStream->GetLogicalLogLastVersion() == (versionBase + version) - 1);
        
        //
        // Verify records are in dedicated and not shared.
        //
        OverlayStream::AsyncReadContext::SPtr readContext1;
        OverlayStream::AsyncReadContextOverlay::SPtr readContext;
        
        status = overlayStream->CreateAsyncReadContext(readContext1);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
        readContext1 = nullptr;

        ULONGLONG readAsn;
        RvdLogAsn actualReadAsn;
        ULONGLONG actualReadVersion;
        KBuffer::SPtr readMetadata;
        KIoBuffer::SPtr readIoBuffer;
        KIoBuffer::SPtr verifyIoBuffer;
        BOOLEAN readFromDedicated;
        ULONG readDataSize;
        ULONG readDataOffset;
        PUCHAR expectedData;
        KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;

        
        readAsn = asnBase;
        do
        {
            printf("Read: ReadAsn %llx ... ",
                   readAsn);
            
            readContext->Reuse();
            readContext->StartRead(readAsn,
                                   RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                                   &actualReadAsn,
                                   &actualReadVersion,
                                   readMetadata,
                                   readIoBuffer,
                                   &readFromDedicated,
                                   NULL,
                                   sync3);
            status = sync3.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            printf("ActualAsn %llx ActualVersion %llx\n",
                   actualReadAsn.Get(),
                   actualReadVersion);
            
            expectedData = data + (actualReadAsn.Get() - asnBase);
            
            status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(readMetadata,
                                                                                       coreLoggerOffset,
                                                                                       *readIoBuffer,
                                                                                       reservedMetadataSize,
                                                                                       metadataBlockHeader,
                                                                                       streamBlockHeader,
                                                                                       readDataSize,
                                                                                       readDataOffset);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBuffer::SPtr metadataIoBuffer;
            PVOID metadataIoBufferPtr;

            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize, metadataIoBuffer, metadataIoBufferPtr, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KMemCpySafe((PVOID)((PUCHAR)metadataIoBufferPtr + coreLoggerOffset),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset,
                        readMetadata->GetBuffer(),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
            
            status = KIoBuffer::CreateEmpty(verifyIoBuffer,
                                            *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*metadataIoBuffer,
                                                          0,
                                                          metadataIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*readIoBuffer,
                                                          0,
                                                          readIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBufferStream stream(*verifyIoBuffer,
                                    readDataOffset);

            for (ULONG i = 0; i < readDataSize; i++)
            {
                UCHAR byte;
                BOOLEAN b = stream.Pull(byte);
                if (!b) VERIFY_IS_TRUE(b ? true : false);
                if (byte != expectedData[i]) VERIFY_IS_TRUE((byte == expectedData[i]) ? true : false);
            }

            readAsn = readAsn + (rand() % (512 * 1024));
            
        } while (readAsn < (asnBase + lastBarrier));

        readContext = nullptr;
        
        //
        // Advance ASN and version for next test
        //
        asnBase += lastBarrier;
        versionBase += version;
        
        //
        // Cleanup
        //        
        coreOpenLog = nullptr;
        coreSharedLog = nullptr;
        coreDedicatedLog = nullptr;
        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync3);
        status = sync3.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;
    }

    //
    // Test 5: Write a set of smaller records in the shared log and a
    //         set of larger records in the dedicated log and then
    //         cause destaging at open to occur. The dedicated logis
    //         missing records in the middle that are in the shared log and 
    //         the record marker is in the middle of the data. Read the data and
    //         verify correctness.
    //  
    {
        printf("Test 5\n");
        
        ULONGLONG lastBarrier  = 0;
        KSynchronizer sync5;
        
        status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogSharedId,
                                  coreSharedLog,
                                  NULL,
                                  sync5);
        status = sync5.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    
        coreOpenLog->Reuse();
        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogDedicatedId,
                                  coreDedicatedLog,
                                  NULL,
                                  sync5);
        status = sync5.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLog->CreateAsyncOpenLogStreamContext(coreSharedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLog->CreateAsyncOpenLogStreamContext(coreDedicatedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        static const ULONG fourMB = 4 * (1024 * 1024);
        KBuffer::SPtr dataBuffer;
        PUCHAR data;
        UCHAR entropy = 23;

        status = KBuffer::Create(fourMB, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        data = (PUCHAR)dataBuffer->GetBuffer();
        for (ULONG i = 0; i < fourMB; i++)
        {
            data[i] = (static_cast<UCHAR>(i) * entropy) % 255;
        }

        //
        // Randomly create 512 shared records of different sizes and a
        // the same time map the data into the dedicated records
        //
        static const ULONG dedicatedRecordsSize = 768 * 1024;
        static const ULONG dedicatedRecordsCount = (fourMB / dedicatedRecordsSize) + 1;
        static const ULONG sharedRecordsCount = 512;
        static const ULONG randomSize = (fourMB / sharedRecordsCount) - 64;
        RecordToWrite dedicatedRecords[dedicatedRecordsCount];
        RecordToWrite sharedRecords[sharedRecordsCount];

        ULONGLONG asn = 0;
        ULONGLONG version = 0;
        
        ULONG size;
        ULONG dataOffset;

        ULONG iDedicated;
        ULONG dedicatedSize;
        
        iDedicated = 0;
        dedicatedSize = 0;
        
        dataOffset = 0;

        dedicatedRecords[iDedicated].asn = asnBase;
        dedicatedRecords[iDedicated].dataOffset = dataOffset;
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            size = 64 + (rand() % randomSize);
            
            sharedRecords[i].asn = asn + asnBase;
            sharedRecords[i].version = version + versionBase;
            sharedRecords[i].dataOffset = dataOffset;
            sharedRecords[i].length = size;

            if ((size & 2) == 0)
            {
                sharedRecords[i].barrier = size;
                lastBarrier = asn + size;
            } else {
                sharedRecords[i].barrier = 0;
            }

            printf("Set Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d %s Barrier\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier ? "Is" : "Is Not");
            
            dedicatedRecords[iDedicated].version = sharedRecords[i].version;
            
            ULONG leftover = 0;
            if ((dedicatedSize + size) >= dedicatedRecordsSize)
            {
                //
                // In this case, the new shared record will fill the
                // dedicatated record and so we note this in the
                // dedicated records list
                //
                leftover = (dedicatedSize + size) - dedicatedRecordsSize;
                if (leftover == 0)
                {
                    if (sharedRecords[i].barrier != 0)
                    {
                        dedicatedRecords[iDedicated].barrier = dedicatedRecordsSize;
                    }
                }
                                
                dedicatedRecords[iDedicated].length = dedicatedRecordsSize;
                
                iDedicated++;
                dedicatedRecords[iDedicated].asn = dedicatedRecords[iDedicated-1].asn + dedicatedRecordsSize;
                dedicatedRecords[iDedicated].dataOffset = dedicatedRecords[iDedicated-1].dataOffset + dedicatedRecordsSize;
                
                dedicatedSize = leftover;
                dedicatedRecords[iDedicated].barrier = leftover;
                dedicatedRecords[iDedicated].length = leftover;
            } else {
                dedicatedRecords[iDedicated].length += size;
                if (sharedRecords[i].barrier != 0)
                {
                    dedicatedRecords[iDedicated].barrier = dedicatedRecords[iDedicated].length;
                }
                dedicatedSize += size;
            }

            printf("Set dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Leftover %d Barrier %d\n",
                                        iDedicated,
                                        dedicatedRecords[iDedicated].asn,
                                        dedicatedRecords[iDedicated].version,
                                        dedicatedRecords[iDedicated].length,
                                        dedicatedRecords[iDedicated].dataOffset,
                                        leftover,
                                        dedicatedRecords[iDedicated].barrier);
                        
            asn += size;
            version++;
            dataOffset += size; 
        }

        if (dedicatedSize > 0)
        {
            dedicatedRecords[iDedicated].length = dedicatedSize;
            dedicatedRecords[iDedicated].barrier = dedicatedSize;
            iDedicated++;
        }        
        
        //
        // gain access to the shared and dedicated log streams
        //
        RvdLogStream::AsyncWriteContext::SPtr writeSharedContext;
        RvdLogStream::AsyncWriteContext::SPtr writeDedicatedContext;
        
        coreSharedOpenStream->Reuse();
        coreSharedOpenStream->StartOpenLogStream(logStreamId,
                                           coreSharedLogStream,
                                           NULL,
                                           sync5);
        status = sync5.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        coreDedicatedOpenStream->Reuse();
        coreDedicatedOpenStream->StartOpenLogStream(logStreamId,
                                           coreDedicatedLogStream,
                                           NULL,
                                           sync5);
        status = sync5.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = coreSharedLogStream->CreateAsyncWriteContext(writeSharedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = coreDedicatedLogStream->CreateAsyncWriteContext(writeDedicatedContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Write large records to shared and dedicated logs
        //
        for (ULONG i = 0; i < sharedRecordsCount; i++)
        {
            printf("Write Shared: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                        i,
                                        sharedRecords[i].asn,
                                        sharedRecords[i].version,
                                        sharedRecords[i].length,
                                        sharedRecords[i].dataOffset,
                                        sharedRecords[i].barrier);

            status = SetupToWriteRecord(metadataKBuffer,
                                        dataIoBuffer,
                                        logStreamId,
                                        sharedRecords[i].version,
                                        sharedRecords[i].asn,
                                        data + sharedRecords[i].dataOffset,
                                        sharedRecords[i].length,
                                        coreLoggerOffset,
                                        reservedMetadataSize,
                                        FALSE,
                                        sharedRecords[i].barrier);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KtlLogVerify::KtlMetadataHeader* metadataHeader;
            metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  

            writeSharedContext->Reuse();
            writeSharedContext->StartWrite(sharedRecords[i].asn, sharedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync5);
            status = sync5.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        printf("\n");
        
        
        for (ULONG i = 0; i < iDedicated; i++)
        {
            ULONG s = rand();
            if ((s & 0x11) == 0)
            {
                printf("Write Dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d SKIPPED\n",
                                            i,
                                            dedicatedRecords[i].asn,
                                            dedicatedRecords[i].version,
                                            dedicatedRecords[i].length,
                                            dedicatedRecords[i].dataOffset,
                                            dedicatedRecords[i].barrier);
            } else {
                printf("Write Dedicated: %d: Asn: %llx Version: %llx Length: %d Offset %d Barrier %d\n",
                                            i,
                                            dedicatedRecords[i].asn,
                                            dedicatedRecords[i].version,
                                            dedicatedRecords[i].length,
                                            dedicatedRecords[i].dataOffset,
                                            dedicatedRecords[i].barrier);
                status = SetupToWriteRecord(metadataKBuffer,
                                            dataIoBuffer,
                                            logStreamId,
                                            dedicatedRecords[i].version,
                                            dedicatedRecords[i].asn,
                                            data + dedicatedRecords[i].dataOffset,
                                            dedicatedRecords[i].length,
                                            coreLoggerOffset,
                                            reservedMetadataSize,
                                            FALSE,
                                            dedicatedRecords[i].barrier);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KtlLogVerify::KtlMetadataHeader* metadataHeader;
                metadataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
                KtlLogVerify::ComputeRecordVerification(metadataHeader->VerificationData,
                                                        asn,
                                                        version,
                                                        0x1000,
                                                        NULL,
                                                        dataIoBuffer);                                  

                writeDedicatedContext->Reuse();
                writeDedicatedContext->StartWrite(dedicatedRecords[i].asn, dedicatedRecords[i].version, metadataKBuffer, dataIoBuffer, NULL, sync5);
                status = sync5.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
        }
        printf("\n");
                
        writeSharedContext = nullptr;
        writeDedicatedContext = nullptr;
        coreSharedLogStream = nullptr;
        coreDedicatedLogStream = nullptr;
        
        coreDedicatedOpenStream = nullptr;
        coreSharedOpenStream = nullptr;

        //
        // Reopen the streams via the overlay log. This will move records
        // from shared to dedicated
        //
        
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreSharedLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        printf("Opened log - Tail Asn %llx, Highest Version %llx\n",
               overlayStream->GetLogicalLogTailOffset().Get(),
               overlayStream->GetLogicalLogLastVersion());

        VERIFY_IS_TRUE(overlayStream->GetLogicalLogTailOffset().Get() == (asnBase + lastBarrier) - 1);
        VERIFY_IS_TRUE(overlayStream->GetLogicalLogLastVersion() == (versionBase + version) - 1);
        
        //
        // Verify records are in dedicated and not shared.
        //
        OverlayStream::AsyncReadContext::SPtr readContext1;
        OverlayStream::AsyncReadContextOverlay::SPtr readContext;
        
        status = overlayStream->CreateAsyncReadContext(readContext1);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
        readContext1 = nullptr;

        ULONGLONG readAsn;
        RvdLogAsn actualReadAsn;
        ULONGLONG actualReadVersion;
        KBuffer::SPtr readMetadata;
        KIoBuffer::SPtr readIoBuffer;
        KIoBuffer::SPtr verifyIoBuffer;
        BOOLEAN readFromDedicated;
        ULONG readDataSize;
        ULONG readDataOffset;
        PUCHAR expectedData;
        KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;

        
        readAsn = asnBase;
        do
        {
            printf("Read: ReadAsn %llx ... ",
                   readAsn);
            
            readContext->Reuse();
            readContext->StartRead(readAsn,
                                   RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                                   &actualReadAsn,
                                   &actualReadVersion,
                                   readMetadata,
                                   readIoBuffer,
                                   &readFromDedicated,
                                   NULL,
                                   sync5);
            status = sync5.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            printf("ActualAsn %llx ActualVersion %llx\n",
                   actualReadAsn.Get(),
                   actualReadVersion);
            
            expectedData = data + (actualReadAsn.Get() - asnBase);
            
            status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(readMetadata,
                                                                                       coreLoggerOffset,
                                                                                       *readIoBuffer,
                                                                                       reservedMetadataSize,
                                                                                       metadataBlockHeader,
                                                                                       streamBlockHeader,
                                                                                       readDataSize,
                                                                                       readDataOffset);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBuffer::SPtr metadataIoBuffer;
            PVOID metadataIoBufferPtr;

            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize, metadataIoBuffer, metadataIoBufferPtr, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KMemCpySafe((PVOID)((PUCHAR)metadataIoBufferPtr + coreLoggerOffset),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset,
                        readMetadata->GetBuffer(),
                        KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
            
            status = KIoBuffer::CreateEmpty(verifyIoBuffer,
                                            *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*metadataIoBuffer,
                                                          0,
                                                          metadataIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = verifyIoBuffer->AddIoBufferReference(*readIoBuffer,
                                                          0,
                                                          readIoBuffer->QuerySize());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KIoBufferStream stream(*verifyIoBuffer,
                                    readDataOffset);

            for (ULONG i = 0; i < readDataSize; i++)
            {
                UCHAR byte;
                BOOLEAN b = stream.Pull(byte);
                if (!b) VERIFY_IS_TRUE(b ? true : false);
                if (byte != expectedData[i]) VERIFY_IS_TRUE((byte == expectedData[i]) ? true : false);
            }

            readAsn = readAsn + (rand() % (512 * 1024));
            
        } while (readAsn < (asnBase + lastBarrier));

        readContext = nullptr;
        
        //
        // Advance ASN and version for next test
        //
        asnBase += lastBarrier;
        versionBase += version;
        
        //
        // Cleanup
        //        
        coreOpenLog = nullptr;
        coreSharedLog = nullptr;
        coreDedicatedLog = nullptr;
        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync5);
        status = sync5.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;
    }


    //
    // Cleanup
    //
    coreSharedOpenStream = nullptr;
    coreSharedLog = nullptr;
    coreOpenLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;   
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);
}

VOID RecoveryViaOpenContainerTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KtlLogInformation::KtlLogDefaultStreamType());

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                           &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Test 1: Open shared stream and write a set of records to it and
    // then open the KTL shim container that contains the stream. The overlay
    // container is expected to copy all records from the shared stream
    // to the dedicated stream and truncate the dedicated stream
    //
    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;
    RvdLogStream::SPtr coreDedicatedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreSharedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream = nullptr;

    ULONGLONG asn = 2000;
    ULONG recordCount = 25;

    for (ULONG i = 0; i < recordCount; i++)
    {
        status = WriteARecord(coreSharedLogStream,
                             1,                 // Version
                             asn + i*10,
                             'X');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Include a few out of order records
    //
    for (ULONG i = 0; i < recordCount / 3; i++)
    {
        status = WriteARecord(coreSharedLogStream,
                             1,                 // Version
                             asn + i*10 +5,
                             'X');
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    coreSharedLogStream = nullptr;
    coreLog = nullptr;

    //
    // Wait for the logs to close
    //
    KNt::Sleep(5 * 1000);

    //
    // Reopen the container containing the stream via the overlay
    // container. This will move records from shared to dedicated
    //
    OverlayLog::SPtr overlayLog;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;

    //
    // Wait for the logs to close
    //
    KNt::Sleep(5 * 1000);

    //
    // Reopen the stream and verify records are in dedicated container
    // and not shared. First step is to open the shared stream and
    // verify it is empty
    //
    coreOpenLog->Reuse();

    rvdLogId = logContainerGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    logStreamId = logStreamGuid;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreSharedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream = nullptr;

    RvdLogAsn lowAsn;
    RvdLogAsn highAsn;
    RvdLogAsn truncationAsn;

    coreSharedLogStream->QueryRecordRange(&lowAsn,
                                   &highAsn,
                                   &truncationAsn);
    VERIFY_IS_TRUE(OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn) ? TRUE : FALSE);

    coreSharedLogStream = nullptr;
    coreLog = nullptr;

    //
    // Wait for the logs to close
    //
    KNt::Sleep(5 * 1000);

    //
    // Next step is to open the dedicated stream and verify that all
    // records are in it
    //
    coreOpenLog->Reuse();

    rvdLogId = logStreamGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    logStreamId = logStreamGuid;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreDedicatedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream = nullptr;

    {
        RvdLogAsn expectedHighAsn = asn + (recordCount-1)*10;
        ULONGLONG version;
        RvdLogStream::RecordDisposition disposition;
        ULONG ioBufferSize;
        ULONGLONG debugInfo1;

        status = coreDedicatedLogStream->QueryRecord(expectedHighAsn,
                             &version,
                             &disposition,
                             &ioBufferSize,
                             &debugInfo1);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE((disposition == KtlLogStream::eDispositionPersisted) ? true : false);
    }




    coreDedicatedLogStream->QueryRecordRange(&lowAsn,
                                   &highAsn,
                                   &truncationAsn);
    VERIFY_IS_TRUE((lowAsn == asn) ? TRUE : FALSE);
    VERIFY_IS_TRUE((highAsn == asn + (recordCount-1)*10) ? TRUE : FALSE);

    coreDedicatedLogStream = nullptr;
    coreLog = nullptr;


    //
    // Cleanup
    //
    coreOpenLog = nullptr;

    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                            logContainerGuid);

}

VOID RecoveryPartlyCreatedStreamTestWorker(
    KGuid& diskId,
    SharedLCMBInfoAccess::DispositionFlags dispositionFlags
    )
{
    const LPCWSTR StreamAliasText = L"StreamAliasText";
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KString::SPtr alias;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    alias = KString::Create(StreamAliasText, *g_Allocator, TRUE);
    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KtlLogInformation::KtlLogDefaultStreamType(),
                              DEFAULT_MAX_RECORD_SIZE,
                              DEFAULT_MAX_RECORD_SIZE,
                              KString::CSPtr(alias.RawPtr())
                              );

    //
    // Access the shared metadata for the container and change the
    // creation disposition
    //
    SharedLCMBInfoAccess::SPtr sharedLCMBInfo;

    RvdLogId rvdLogId;
    RvdLogStreamId logStreamId;

    rvdLogId = logContainerGuid;
    logStreamId = logStreamGuid;
    status = SharedLCMBInfoAccess::Create(sharedLCMBInfo,
                                          diskId,
                                          nullptr,
                                          rvdLogId,
                                          1024,             // Should match RvdLogOnDiskConfig::_DefaultMaxNumberOfStreams
                                          *g_Allocator,
                                          ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    sharedLCMBInfo->StartOpenMetadataBlock(NULL,
                                  nullptr,
                                  sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    SharedLCMBInfoAccess::AsyncUpdateEntryContext::SPtr updateContext;

    status = sharedLCMBInfo->CreateAsyncUpdateEntryContext(updateContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    updateContext->StartUpdateEntry(logStreamId,
                                    NULL,              // Path
                                    NULL,              // Alias
                                    dispositionFlags,
                                    FALSE,             // RemoveEntry
                                    0,                 // Assume that the stream is created at index 0
                                    NULL,              // ParentAsync
                                    sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    updateContext = nullptr;

    sharedLCMBInfo->Reuse();
    sharedLCMBInfo->StartCloseMetadataBlock(nullptr,
                                   sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedLCMBInfo = nullptr;

    //
    // Now open up the container to kick off recovery
    //
    RvdLogManager::SPtr coreLogManager;
    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                           &KtlLogVerify::VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    OverlayLog::SPtr overlayLog;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Verify that the alias has been removed and there is no OverlayStreamFreeService for the stream
    //
    RvdLogStreamId aliasLogStreamId;
    status = overlayLog->LookupAlias(*alias, aliasLogStreamId);
    VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

    OverlayStreamFreeService::SPtr osfs;
    status = overlayLog->_StreamsTable.FindOverlayStream(logStreamId, osfs);
    VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;

    //
    // Corrupted log stream should have been removed from shared log
    //
    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLogStream::SPtr coreSharedLogStream;
    RvdLog::SPtr coreLog;

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStreamId = logStreamGuid;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreSharedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);
    coreOpenStream = nullptr;

    //
    // Dedicated log container should have also been removed
    //
    rvdLogId = logStreamGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);

    //
    // Cleanup
    //
    coreOpenLog = nullptr;

    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                            logContainerGuid);


    // TODO: A good enhancement for the test is to create a corrupted
    // stream with records in the S log and verify that the S log gets
    // cleaned up. Think about various corruption scenarios.
}


VOID RecoveryPartlyCreatedStreamTest(
    KGuid& diskId
    )
{
    RecoveryPartlyCreatedStreamTestWorker(diskId,
                                    SharedLCMBInfoAccess::FlagCreating);

    RecoveryPartlyCreatedStreamTestWorker(diskId,
                                    SharedLCMBInfoAccess::FlagDeleting);
}

VOID DestagedWriteTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxContainerRecordSize = (0x1000*0x1000) / 2;
    ULONG maxStreamRecordSize =     0x1000*0x1000;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KtlLogInformation::KtlLogDefaultStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreSharedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayLog::SPtr overlayLog;
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // For this test, disable behavior where first write is
    // automatically sent to both shared and dedicated as we are
    // testing the coalescing
    //
    overlayStream->ResetIsNextRecordFirstWrite();

    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Create a set of useful contexts
    //
    OverlayStream::AsyncWriteContext::SPtr writeContext1;
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;

    status = overlayStream->CreateAsyncWriteContext(writeContext1);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    writeContext.Attach(static_cast<OverlayStream::AsyncWriteContextOverlay*>(writeContext1.Detach()));

    RvdLogStream::AsyncReadContext::SPtr coreSharedReadContext;
    RvdLogStream::AsyncReadContext::SPtr coreDedicatedReadContext;

    status = sharedStream->CreateAsyncReadContext(coreSharedReadContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = dedicatedStream->CreateAsyncReadContext(coreDedicatedReadContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    KIoBuffer::SPtr ioBufferSmall;
    KIoBuffer::SPtr ioBufferBig;
    KBuffer::SPtr metadataBuffer;
    VOID* p;
    BOOLEAN sendToShared;

    status = KIoBuffer::CreateSimple(maxContainerRecordSize / 2,
                                     ioBufferSmall,
                                     p,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KIoBuffer::CreateSimple(maxContainerRecordSize + 0x1000,
                                     ioBufferBig,
                                     p,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    status = KBuffer::Create(0x1024,
                             metadataBuffer,
                             *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    // Test 1: Write records verify written to S and D log and wait to see
    //         that they get migrated to D log only
    {
        ULONGLONG readVersion;
        KBuffer::SPtr readMetadataBuffer;
        KIoBuffer::SPtr readIoBuffer;

        ULONGLONG asn = 1000;

        writeContext->Reuse();
        writeContext->StartReservedWrite(0,
                                         asn,
                                         1,       // Version
                                         metadataBuffer,
                                         ioBufferSmall,
                                         FALSE,
                                         &sendToShared,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(sendToShared ? TRUE : FALSE);

        //
        // Now wait a few seconds for the write to make it to the D log
        //

        KNt::Sleep(2000);

        RvdLogAsn lowAsn;
        RvdLogAsn highAsn;
        RvdLogAsn truncationAsn;

        dedicatedStream->QueryRecordRange(&lowAsn,
                                          &highAsn,
                                          &truncationAsn);
        VERIFY_IS_TRUE((lowAsn == asn) ? TRUE : FALSE);
        VERIFY_IS_TRUE((highAsn == asn) ? TRUE : FALSE);
        VERIFY_IS_TRUE((truncationAsn < asn) ? TRUE : FALSE);

        sharedStream->QueryRecordRange(&lowAsn,
                                       &highAsn,
                                       &truncationAsn);
        VERIFY_IS_TRUE(OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn) ? TRUE : FALSE);

        coreDedicatedReadContext->Reuse();
        coreDedicatedReadContext->StartRead(asn,
                                     &readVersion,
                                     readMetadataBuffer,
                                     readIoBuffer,
                                     NULL,
                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    // Test 2: Write record larger than base container and verify written
    //         to D log only
    {
        ULONGLONG readVersion;
        KBuffer::SPtr readMetadataBuffer;
        KIoBuffer::SPtr readIoBuffer;

        ULONGLONG asn = 2000;

        writeContext->Reuse();
        writeContext->StartReservedWrite(0,
                                         asn,
                                         1,       // Version
                                         metadataBuffer,
                                         ioBufferBig,
                                         FALSE,
                                         &sendToShared,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(! sendToShared);

        coreSharedReadContext->Reuse();
        coreSharedReadContext->StartRead(asn,
                                     &readVersion,
                                     readMetadataBuffer,
                                     readIoBuffer,
                                     NULL,
                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

        coreDedicatedReadContext->Reuse();
        coreDedicatedReadContext->StartRead(asn,
                                     &readVersion,
                                     readMetadataBuffer,
                                     readIoBuffer,
                                     NULL,
                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        RvdLogAsn lowAsn;
        RvdLogAsn highAsn;
        RvdLogAsn truncationAsn;

        dedicatedStream->QueryRecordRange(&lowAsn,
                                          &highAsn,
                                          &truncationAsn);
        VERIFY_IS_TRUE((highAsn == asn) ? TRUE : FALSE);
        VERIFY_IS_TRUE((truncationAsn < asn) ? TRUE : FALSE);

        sharedStream->QueryRecordRange(&lowAsn,
                                       &highAsn,
                                       &truncationAsn);
        VERIFY_IS_TRUE(OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn) ? TRUE : FALSE);

    }

    // Test 3: Write below S truncation point and verify written to D log
    //         only
    {
        //
        // At this point the D truncation should be at 0 and the S
        // truncation point at 1000.
        //
        ULONGLONG readVersion;
        KBuffer::SPtr readMetadataBuffer;
        KIoBuffer::SPtr readIoBuffer;

        ULONGLONG asn = 500;

        writeContext->Reuse();
        writeContext->StartReservedWrite(0,
                                         asn,
                                         1,       // Version
                                         metadataBuffer,
                                         ioBufferSmall,
                                         FALSE,
                                         &sendToShared,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(! sendToShared);

        coreSharedReadContext->Reuse();
        coreSharedReadContext->StartRead(asn,
                                     &readVersion,
                                     readMetadataBuffer,
                                     readIoBuffer,
                                     NULL,
                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

        coreDedicatedReadContext->Reuse();
        coreDedicatedReadContext->StartRead(asn,
                                     &readVersion,
                                     readMetadataBuffer,
                                     readIoBuffer,
                                     NULL,
                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Now manually truncate at 750 and try to write at 800. This
        // should still go to D only
        //
        asn = 800;
        overlayStream->Truncate(750,
                                750);


        writeContext->Reuse();
        writeContext->StartReservedWrite(0,
                                         asn,
                                         1,       // Version
                                         metadataBuffer,
                                         ioBufferSmall,
                                         FALSE,
                                         &sendToShared,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(! sendToShared);

        coreSharedReadContext->Reuse();
        coreSharedReadContext->StartRead(asn,
                                     &readVersion,
                                     readMetadataBuffer,
                                     readIoBuffer,
                                     NULL,
                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

        coreDedicatedReadContext->Reuse();
        coreDedicatedReadContext->StartRead(asn,
                                     &readVersion,
                                     readMetadataBuffer,
                                     readIoBuffer,
                                     NULL,
                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    // Test 4: Write when above S quota and verify written to D log
    // only
    {
        // TODO:
    }


    //
    // Verify all records are in dedicated and not shared
    //
    RvdLogAsn lowAsn;
    RvdLogAsn highAsn;
    RvdLogAsn truncationAsn;

    sharedStream->QueryRecordRange(&lowAsn,
                                   &highAsn,
                                   &truncationAsn);
    VERIFY_IS_TRUE(OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn) ? TRUE : FALSE);


    //
    // Final Cleanup
    //
    dedicatedStream = nullptr;
    sharedStream = nullptr;
    coreDedicatedReadContext = nullptr;
    coreSharedReadContext = nullptr;

    writeContext = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;
    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
        
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
                     logContainerGuid);
    DeleteRvdlogFile(diskId,
                     logStreamGuid);

}

VOID OfflineDestageContainerTest(
    KGuid& diskId
    )
{
    const ULONG StreamCount = 16;

    NTSTATUS status;
    KGuid logStreamGuid[StreamCount];
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG maxContainerRecordSize = (0x1000*0x1000) / 2;
    ULONG maxStreamRecordSize =     0x1000*0x1000;

    for (ULONG i = 0; i < StreamCount; i++)
    {
        logStreamGuid[i].CreateNew();
    }


    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              StreamCount,
                              logStreamGuid,
                              KtlLogInformation::KtlLogDefaultStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                           &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    ULONGLONG asn;
    ULONG recordCount;

    for (ULONG i = 0; i < StreamCount; i++)
    {
        RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

        status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogStreamId logStreamId = logStreamGuid[i];
        RvdLogStream::SPtr coreSharedLogStream;

        coreOpenStream->StartOpenLogStream(logStreamId,
                                           coreSharedLogStream,
                                           NULL,
                                           sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn = 2000;
        recordCount = 25;
        RvdLogAsn highWriteAsn = RvdLogAsn::Min();

        for (ULONG i1 = 0; i1 < recordCount; i1++)
        {
            status = WriteARecord(coreSharedLogStream,
                                 1,                 // Version
                                 asn + i1*10,
                                 'X');
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            highWriteAsn.SetIfLarger(asn + i1*10);
        }

        //
        // Include a few out of order records
        //
        for (ULONG i1 = 0; i1 < recordCount / 3; i1++)
        {
            status = WriteARecord(coreSharedLogStream,
                                 1,                 // Version
                                 asn + i1*10 +5,
                                 'X');
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            highWriteAsn.SetIfLarger(asn + i1*10 +5);
        }

        coreSharedLogStream = nullptr;
    }


    //
    // Now kick off the offline destage
    //
    KtlLogManager::SPtr logManager;

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

    coreOpenLog = nullptr;
    coreLog = nullptr;
    KNt::Sleep(1000);


    //
    // Open log container and expect that it will cause the destagign
    // to occur for all streams
    //
    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;
        ContainerCloseSynchronizer closeSync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(diskId,
                                             rvdLogId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_SUCCESS);
    }

    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager = nullptr;

    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));



    //
    // Reopen the streams via the overlay log to verify records moved
    // from shared to dedicated
    //
    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG i = 0; i < StreamCount; i++)
    {
        RvdLogStream::SPtr dedicatedStream;
        RvdLogStream::SPtr sharedStream;

        //
        // Open shared stream
        //
        RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;
        status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLogStreamId logStreamId = logStreamGuid[i];
        RvdLogStream::SPtr coreSharedLogStream;

        coreOpenStream->StartOpenLogStream(logStreamId,
                                           sharedStream,
                                           NULL,
                                           sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Open dedicated container
        //
        RvdLog::SPtr dedicatedContainer;

        rvdLogId = logStreamGuid[i];
        coreOpenLog->Reuse();
        coreOpenLog->StartOpenLog(diskId,
                                  rvdLogId,
                                  dedicatedContainer,
                                  NULL,
                                  sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        RvdLog::AsyncOpenLogStreamContext::SPtr dedicatedOpenStream;
        status = dedicatedContainer->CreateAsyncOpenLogStreamContext(dedicatedOpenStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStreamId = logStreamGuid[i];

        dedicatedOpenStream->StartOpenLogStream(logStreamId,
                                           dedicatedStream,
                                           NULL,
                                           sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));



        //
        // Verify records are in dedicated and not shared
        //
        RvdLogAsn lowAsn;
        RvdLogAsn highAsn;
        RvdLogAsn truncationAsn;

        sharedStream->QueryRecordRange(&lowAsn,
                                       &highAsn,
                                       &truncationAsn);
        VERIFY_IS_TRUE(OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn) ? TRUE : FALSE);

        dedicatedStream->QueryRecordRange(&lowAsn,
                                          &highAsn,
                                          NULL);
        VERIFY_IS_TRUE((lowAsn == asn) ? TRUE : FALSE);
        VERIFY_IS_TRUE((highAsn == asn + (recordCount-1)*10) ? TRUE : FALSE);

        //
        // Final Cleanup
        //
        dedicatedContainer = nullptr;
        dedicatedStream = nullptr;
        sharedStream = nullptr;

    }

    coreOpenLog = nullptr;
    coreLog = nullptr;
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);

}



VOID CreateAsyncWriteContextOverlay(
    __in OverlayStream& OS,
    __out OverlayStream::AsyncWriteContextOverlay::SPtr& WriteContext
    )
{
    NTSTATUS status;
    OverlayStream::SPtr overlayStream = &OS;
    OverlayStream::AsyncWriteContext::SPtr writeContext1;
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;

    status = overlayStream->CreateAsyncWriteContext(writeContext1);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    WriteContext.Attach(static_cast<OverlayStream::AsyncWriteContextOverlay*>(writeContext1.Detach()));
}

VOID CreateAsyncIoctlContextOverlay(
    __in OverlayStream& OS,
    __out OverlayStream::AsyncIoctlContextOverlay::SPtr& IoctlContext
    )
{
    NTSTATUS status;
    OverlayStream::SPtr overlayStream = &OS;
    OverlayStream::AsyncIoctlContextOverlay::SPtr ioctlContext;

    status = overlayStream->CreateAsyncIoctlContext(ioctlContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    IoctlContext = ioctlContext;
}



VOID ReadAndVerifyRecord(
    __in RvdLogStream::AsyncReadContext::SPtr coreDedicatedReadContext,
    __in ULONG reservedMetadataSize,
    __in ULONG coreLoggerOffset,
    __in RvdLogAsn readAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __in NTSTATUS expectedStatus,
    __in RvdLogAsn expectedAsn,
    __in ULONGLONG expectedVersion,
    __in ULONG expectedDataSize,
    __in PUCHAR expectedData,
    __in RvdLogStreamId& expectedStreamId
)
{
    //
    // Metadata has offsets based on the buffer beginning with the core logger overhead but the buffer begins
    // after the core logger overhead so offsets need to be fixed up by coreLoggerOffset
    //
    NTSTATUS status;
    KSynchronizer sync;
    RvdLogAsn oldAsn;
    ULONGLONG oldVersion;
    KBuffer::SPtr oldMetadata;
    KIoBuffer::SPtr oldData;

    coreDedicatedReadContext->Reuse();
    coreDedicatedReadContext->StartRead(readAsn, Type, &oldAsn, &oldVersion, oldMetadata, oldData, NULL, sync);
    status = sync.WaitForCompletion();

    VERIFY_IS_TRUE(status == expectedStatus);

    if (NT_SUCCESS(status))
    {
        KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
        ULONG dataSize;
        ULONG dataOffset;
        KIoBuffer::SPtr streamIoBuffer;

        status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(oldMetadata,
            coreLoggerOffset,
            *oldData,
            reservedMetadataSize,
            metadataBlockHeader,
            streamBlockHeader,
            dataSize,
            dataOffset);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(oldAsn == expectedAsn ? true : false);
        VERIFY_IS_TRUE(oldVersion == expectedVersion);

        VERIFY_IS_TRUE((streamBlockHeader->Signature == KLogicalLogInformation::StreamBlockHeader::Sig) ? true : false);

        VERIFY_IS_TRUE((streamBlockHeader->StreamOffset == expectedAsn.Get()) ? true : false);
#if DBG
        ULONGLONG crc64;

        //  crc64 = KChecksum::Crc64(dataRead, dataSizeRead, 0);
        //    VERIFY_IS_TRUE(crc64 == streamBlockHeader->DataCRC64);

        ULONGLONG headerCrc64 = streamBlockHeader->HeaderCRC64;
        streamBlockHeader->HeaderCRC64 = 0;
        crc64 = KChecksum::Crc64(streamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader), 0);
        VERIFY_IS_TRUE((crc64 == headerCrc64) ? true : false);
#endif
        VERIFY_IS_TRUE((expectedDataSize == dataSize) ? true : false);
        VERIFY_IS_TRUE((expectedVersion == streamBlockHeader->HighestOperationId) ? true : false);


        //
        // Build KIoBufferStream that spans the metadata and the data
        //
        KIoBuffer::SPtr metadataIoBuffer;
        PVOID metadataIoBufferPtr;

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize, metadataIoBuffer, metadataIoBufferPtr, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KMemCpySafe((PVOID)((PUCHAR)metadataIoBufferPtr + coreLoggerOffset),
                    KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset,
                    oldMetadata->GetBuffer(),
                    KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);

        status = KIoBuffer::CreateEmpty(streamIoBuffer,
            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = streamIoBuffer->AddIoBufferReference(*metadataIoBuffer,
            0,
            metadataIoBuffer->QuerySize());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = streamIoBuffer->AddIoBufferReference(*oldData,
            0,
            oldData->QuerySize());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KIoBufferStream stream(*streamIoBuffer,
                                dataOffset);

        for (ULONG i = 0; i < dataSize; i++)
        {
            UCHAR byte;
            BOOLEAN b = stream.Pull(byte);
            if (!b) VERIFY_IS_TRUE(b ? true : false);
            if (byte != expectedData[i]) VERIFY_IS_TRUE((byte == expectedData[i]) ? true : false);
        }

        VERIFY_IS_TRUE((expectedStreamId.Get() == streamBlockHeader->StreamId.Get()) ? true : false);
    }
}


NTSTATUS FlushCoalescingRecords(
    __in OverlayStream::CoalesceRecords& CR,
    __in BOOLEAN IsClosing = FALSE
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    OverlayStream::CoalesceRecords::SPtr coalesceRecords = &CR;

    OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr flush;

    status = coalesceRecords->CreateAsyncAppendOrFlushContext(flush);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    flush->StartFlush(IsClosing, sync);
    status = sync.WaitForCompletion();
    return(status);
}

static const ULONG numberWrites = 5;

VOID WriteABunchOfRandomRecords(
    __in OverlayStream::SPtr overlayStream,
    __in ULONG maxIndividualRecordSize,
    __in ULONG coreLoggerOffset,
    __in ULONG reservedMetadataSize,
    __in PUCHAR dataBufferPtr,
    __in ULONG delayBetweenWritesInMs,
    __inout ULONGLONG& asn,
    __inout ULONGLONG& version,
    __inout ULONG& dataWritePosition
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    ULONG dataSizeWritten;
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    RvdLogStreamId logStreamId;
    overlayStream->QueryLogStreamId(logStreamId);

    OverlayStream::AsyncWriteContextOverlay::SPtr writeContexts[numberWrites];
    for (ULONG i = 0; i < numberWrites; i++)
    {
        CreateAsyncWriteContextOverlay(*overlayStream, writeContexts[i]);
    }

    for (ULONG i = 0; i < numberWrites; i++)
    {
        version++;

        dataSizeWritten = (maxIndividualRecordSize / numberWrites) -1 ; // rand() % (maxIndividualRecordSize / numberWrites);
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr + dataWritePosition, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContexts[i]->Reuse();
        writeContexts[i]->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        dataWritePosition += dataSizeWritten;

        asn += dataSizeWritten;

        KNt::Sleep(delayBetweenWritesInMs);
    }
}

VOID CoalescedWriteTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize - 16 * 0x1000;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
        activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayLog::SPtr overlayLog;
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // For this test, disable behavior where first write is
    // automatically sent to both shared and dedicated as we are
    // testing the coalescing
    //
    overlayStream->ResetIsNextRecordFirstWrite();

    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Create a set of useful contexts
    //
    RvdLogStream::AsyncReadContext::SPtr coreSharedReadContext;
    RvdLogStream::AsyncReadContext::SPtr coreDedicatedReadContext;

    status = sharedStream->CreateAsyncReadContext(coreSharedReadContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = dedicatedStream->CreateAsyncReadContext(coreDedicatedReadContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Allocate buffers and stuff for writing data
    //
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    ULONG dataSizeWritten;

    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    ULONG fullRecordSize;
    fullRecordSize = 3 * (maxStreamRecordSize / 4);
    fullRecordSize -= (2 * KLogicalLogInformation::FixedMetadataSize);
    fullRecordSize += KLogicalLogInformation::FixedMetadataSize - (coreLoggerOffset + reservedMetadataSize +
        sizeof(KLogicalLogInformation::MetadataBlockHeader) +
        sizeof(KLogicalLogInformation::StreamBlockHeader));


    //
    // Max record size also includes the metadata size so subtract it
    // here. Also the core logger wants to reserve 1/4 of the record space for
    // checkpoint records so remove that as well.
    //
    ULONG maxIndividualRecordSize;
    maxIndividualRecordSize = 2 * (maxContainerRecordSize / 4);
    maxIndividualRecordSize -= (2 * KLogicalLogInformation::FixedMetadataSize);
    maxIndividualRecordSize += KLogicalLogInformation::FixedMetadataSize - (coreLoggerOffset + reservedMetadataSize +
                                                                            sizeof(KLogicalLogInformation::MetadataBlockHeader) +
                                                                            sizeof(KLogicalLogInformation::StreamBlockHeader));

    HRESULT hr;
    ULONG result;
    hr = ULongMult(numberWrites, maxStreamRecordSize, &result);
    KInvariant(SUCCEEDED(hr));

    status = KBuffer::Create(result, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();

    for (ULONG i = 0; i < numberWrites*maxStreamRecordSize; i++)
    {
        dataBufferPtr[i] = i % 255;
    }

    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);

    OverlayStream::AsyncIoctlContextOverlay::SPtr ioctlContext;
    CreateAsyncIoctlContextOverlay(*overlayStream, ioctlContext);
    KBuffer::SPtr outBuffer;

    srand((ULONG)GetTickCount());

    // Test 1: Write a partial record and flush. Verify that the
    //         partial record is written correctly.
    {
        //
        // Write metadata only record through coalescing engine
        //
        version++;

        RvdLogAsn expectedAsn = asn;
        ULONGLONG expectedVersion = version;

        dataSizeWritten = 40;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn, dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        //
        // Read from dedicated log and expect record to not exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_NOT_FOUND, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);
        //
        // Flush record out to dedicated log
        //
        status = FlushCoalescingRecords(*overlayStream->GetCoalesceRecords());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read from dedicated log and expect record to exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);

        //
        // Write metadata and data record through coalescing engine
        //
        version++;

        expectedAsn = asn;
        expectedVersion = version;

        dataSizeWritten = 0x2800;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn, dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        //
        // Read from dedicated log and expect record to not exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_NOT_FOUND, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);
        //
        // Flush record out to dedicated log
        //
        status = FlushCoalescingRecords(*overlayStream->GetCoalesceRecords());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read from dedicated log and expect record to exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);
    }

    // Test 2: Write a partial record with force flush. Verify that the
    //         partial record is written correctly.
    {
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::DisableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Write metadata only record through coalescing engine
        //
        version++;

        RvdLogAsn expectedAsn = asn;
        ULONGLONG expectedVersion = version;

        dataSizeWritten = 40;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn, dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(250);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);

        //
        // Write metadata and data record through coalescing engine
        //
        version++;

        expectedAsn = asn;
        expectedVersion = version;

        dataSizeWritten = 0x2800;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn, dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(250);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);

        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::EnableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Test 3: Write a few partial records that are less than a full
    //         record and flush. Verify that a single record is written.
    {
        {
            OverlayStream::AsyncWriteContextOverlay::SPtr writeContexts[numberWrites];
            for (ULONG i = 0; i < numberWrites; i++)
            {
                CreateAsyncWriteContextOverlay(*overlayStream, writeContexts[i]);
            }

            ULONG dataWritePosition = 0;
            RvdLogAsn expectedAsn = asn;
            ULONGLONG expectedVersion = version;

            for (ULONG i = 0; i < numberWrites; i++)
            {
                version++;

                dataSizeWritten = 0x200 + (rand() % 64);
                status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                    logStreamId, version, asn,
                    dataBufferPtr + dataWritePosition, dataSizeWritten,
                    coreLoggerOffset, reservedMetadataSize, TRUE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                writeContexts[i]->Reuse();
                writeContexts[i]->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                dataWritePosition += dataSizeWritten;

                asn += dataSizeWritten;
            }

            //
            // Read from dedicated log and expect record to not exist
            //
            expectedVersion = version;

            ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
                STATUS_NOT_FOUND, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);
            //
            // Flush record out to dedicated log
            //
            status = FlushCoalescingRecords(*overlayStream->GetCoalesceRecords());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Read from dedicated log and expect record to exist
            //
            KNt::Sleep(250);
            ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
                STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);
        }

        {
            ULONG dataWritePosition = 0;
            RvdLogAsn expectedAsn = asn;
            ULONGLONG expectedVersion;

            ULONGLONG asnU = asn;
            WriteABunchOfRandomRecords(overlayStream,
                                       maxIndividualRecordSize,
                                       coreLoggerOffset,
                                       reservedMetadataSize,
                                       dataBufferPtr,
                                       0,
                                       asnU,
                                       version,
                                       dataWritePosition);
            asn = asnU;


            //
            // Read from dedicated log and expect record to not exist
            //
            expectedVersion = version;

            ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
                STATUS_NOT_FOUND, expectedAsn, expectedVersion, dataSizeWritten, dataBufferPtr, logStreamId);
            //
            // Flush record out to dedicated log
            //
            status = FlushCoalescingRecords(*overlayStream->GetCoalesceRecords());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Read from dedicated log and expect record to exist
            //
            KNt::Sleep(250);
            ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
                STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);
        }

    }

    // Test 4: Write a few partial records that are less than a full
    //         record with force flush on last record. Verify that a single record is written.
    {
        OverlayStream::AsyncWriteContextOverlay::SPtr writeContexts[numberWrites];
        for (ULONG i = 0; i < numberWrites; i++)
        {
            CreateAsyncWriteContextOverlay(*overlayStream, writeContexts[i]);
        }

        {
            ULONG dataWritePosition = 0;
            RvdLogAsn expectedAsn = asn;
            ULONGLONG expectedVersion = version;

            for (ULONG i = 0; i < numberWrites; i++)
            {
                version++;

                dataSizeWritten = 0x200;
                status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                    logStreamId, version, asn,
                    dataBufferPtr + dataWritePosition, dataSizeWritten,
                    coreLoggerOffset, reservedMetadataSize, TRUE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                writeContexts[i]->Reuse();
                writeContexts[i]->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                dataWritePosition += dataSizeWritten;

                asn += dataSizeWritten;
            }

            ioctlContext->Reuse();
            ioctlContext->StartIoctl(KLogicalLogInformation::DisableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            version++;

            dataSizeWritten = 0x200;
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                logStreamId, version, asn,
                dataBufferPtr + dataWritePosition, dataSizeWritten,
                coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            dataWritePosition += dataSizeWritten;

            asn += dataSizeWritten;

            ioctlContext->Reuse();
            ioctlContext->StartIoctl(KLogicalLogInformation::EnableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Read from dedicated log and expect record to not exist
            //
            expectedVersion = version;

            //
            // Read from dedicated log and expect record to exist
            //
            KNt::Sleep(250);
            ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
                STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);
        }

        {
            ULONG dataWritePosition = 0;
            RvdLogAsn expectedAsn = asn;
            ULONGLONG expectedVersion = version;

            ULONGLONG asnU = asn;
            WriteABunchOfRandomRecords(overlayStream,
                                       maxIndividualRecordSize,
                                       coreLoggerOffset,
                                       reservedMetadataSize,
                                       dataBufferPtr,
                                       0,
                                       asnU,
                                       version,
                                       dataWritePosition);
            asn = asnU;

            ioctlContext->Reuse();
            ioctlContext->StartIoctl(KLogicalLogInformation::DisableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            version++;

            dataSizeWritten = 0x200;
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                logStreamId, version, asn,
                dataBufferPtr + dataWritePosition, dataSizeWritten,
                coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            dataWritePosition += dataSizeWritten;

            asn += dataSizeWritten;

            ioctlContext->Reuse();
            ioctlContext->StartIoctl(KLogicalLogInformation::EnableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Read from dedicated log and expect record to not exist
            //
            expectedVersion = version;

            //
            // Read from dedicated log and expect record to exist
            //
            KNt::Sleep(2000);
            ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
                STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);
        }

    }

    // Test 5: Write and verify a whole bunch of random length records

    {
        OverlayStream::AsyncWriteContextOverlay::SPtr writeContexts[numberWrites];
        for (ULONG i = 0; i < numberWrites; i++)
        {
            CreateAsyncWriteContextOverlay(*overlayStream, writeContexts[i]);
        }

        for (ULONG loops = 0; loops < 5; loops++)
        {
            ULONG dataWritePosition = 0;
            RvdLogAsn expectedAsn = asn;
            ULONGLONG expectedVersion = version;

            ULONGLONG asnU = asn;
            WriteABunchOfRandomRecords(overlayStream,
                                       maxIndividualRecordSize,
                                       coreLoggerOffset,
                                       reservedMetadataSize,
                                       dataBufferPtr,
                                       0,
                                       asnU,
                                       version,
                                       dataWritePosition);
            asn = asnU;

            ioctlContext->Reuse();
            ioctlContext->StartIoctl(KLogicalLogInformation::DisableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            version++;
            dataSizeWritten = 1;
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                logStreamId, version, asn,
                dataBufferPtr + dataWritePosition, dataSizeWritten,
                coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            dataWritePosition += dataSizeWritten;

            asn += dataSizeWritten;

            ioctlContext->Reuse();
            ioctlContext->StartIoctl(KLogicalLogInformation::EnableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Read from dedicated log and expect record to not exist
            //
            expectedVersion = version;

            //
            // Read from dedicated log and expect record to exist
            //
            KNt::Sleep(2000);
            ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
                STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);
        }
    }


    // Test 7: Write a few partial records and then a truncate tail
    //         record. Verify that the partial record is flushed and
    //         truncate tail record written.
    {
        static const ULONGLONG truncateTailAmount = 100;
        OverlayStream::AsyncWriteContextOverlay::SPtr writeContexts[numberWrites];
        for (ULONG i = 0; i < numberWrites; i++)
        {
            CreateAsyncWriteContextOverlay(*overlayStream, writeContexts[i]);
        }

        ULONG dataWritePosition = 0;
        RvdLogAsn expectedAsn = asn;

        ULONGLONG asnU = asn;
        WriteABunchOfRandomRecords(overlayStream,
                                   maxIndividualRecordSize,
                                   coreLoggerOffset,
                                   reservedMetadataSize,
                                   dataBufferPtr,
                                   0,
                                   asnU,
                                   version,
                                   dataWritePosition);
        asn = asnU;

        ULONGLONG expectedVersion = version;

        asn -= truncateTailAmount;
        RvdLogAsn expectedTTAsn = asn;

        printf("Write TT record 0x%llx version 0x%llx amount %llx\n", asn, version+1, truncateTailAmount);
        
        version++;
        dataSizeWritten = 1;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        ULONGLONG expectedTTVersion = version;

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(2000);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(250);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedTTAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedTTAsn, expectedTTVersion, dataSizeWritten, dataBufferPtr, logStreamId);

        //
        // Advance Asn beyond the highest possible truncation point for
        // the shared log. See TFS 11215692 where the test below fails
        // since the first write is not coalesced.
        //

        version++;
        dataSizeWritten = (ULONG)truncateTailAmount + 10;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        printf("After wrote Asn 0x%llx Version 0x%llx length 0x%x\n", asn, version, dataSizeWritten);
        
        asn += dataSizeWritten;

        //
        // Flush record out to dedicated log
        //
        status = FlushCoalescingRecords(*overlayStream->GetCoalesceRecords());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    // Test 8: Write a few partial records and then a record larger than
    //         the container record size. Verify partial records flushed
    //         and larger record written.
    {
        ULONG dataWritePosition = 0;
        RvdLogAsn expectedAsn = asn;

        printf("Write Random Asn 0x%llx version %llx\n", asn, version+1);
        
        ULONGLONG asnU = asn;
        WriteABunchOfRandomRecords(overlayStream,
                                   maxIndividualRecordSize,
                                   coreLoggerOffset,
                                   reservedMetadataSize,
                                   dataBufferPtr,
                                   0,
                                   asnU,
                                   version,
                                   dataWritePosition);
        asn = asnU;

        ULONGLONG expectedVersion = version;

        // Large record
        version++;
        dataSizeWritten = maxContainerRecordSize + 64;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr + dataWritePosition, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Large record will completely fill the coalesced record so account for that here
        //
        ULONG leftoverInBuffer = fullRecordSize - dataWritePosition;
        dataWritePosition = fullRecordSize;

        asn += dataSizeWritten;

        RvdLogAsn expectedTTAsn = expectedAsn.Get() + fullRecordSize;
        ULONGLONG expectedTTVersion = version;

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(2000);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);

        //
        // Read from dedicated log and expect record to exist
        //
        dataSizeWritten = dataSizeWritten - leftoverInBuffer;
        KNt::Sleep(250);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedTTAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedTTAsn, expectedTTVersion, dataSizeWritten, dataBufferPtr+fullRecordSize, logStreamId);
    }

    //
    // Test 9: Write a record that almost fills the full record and
    //         then a second record that would cause a flush. Verify
    //         that the first record is flushed correctly.
    {
        ULONG dataWritePosition = 0;
        RvdLogAsn expectedAsn = asn;

        version++;

        dataSizeWritten = fullRecordSize - ((32 * 0x1000) - 1);
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr + dataWritePosition, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        dataWritePosition += dataSizeWritten;

        asn += dataSizeWritten;
        ULONGLONG expectedVersion = version;

        ULONG leftoverInBuffer = fullRecordSize - dataWritePosition;

        //
        // Record to force flush
        //
        version++;
        dataSizeWritten = (32 * 0x1000) + 257;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr + dataWritePosition, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        RvdLogAsn expectedTTAsn = expectedAsn.Get() + fullRecordSize;
        ULONGLONG expectedTTVersion = version;

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(2000);
        dataWritePosition = fullRecordSize;
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(250);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedTTAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_NOT_FOUND, expectedTTAsn, expectedTTVersion, dataSizeWritten - leftoverInBuffer, dataBufferPtr + fullRecordSize, logStreamId);

        status = FlushCoalescingRecords(*overlayStream->GetCoalesceRecords());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedTTAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedTTAsn, expectedTTVersion, dataSizeWritten - leftoverInBuffer, dataBufferPtr + fullRecordSize, logStreamId);

    }

    //
    // Test 9: Write a record that almost fills the full record and
    //         then a second record that would cause a flush. Verify
    //         that the first record is flushed correctly.
    {
        ULONG dataWritePosition = 0;
        RvdLogAsn expectedAsn = asn;

        version++;

        dataSizeWritten = fullRecordSize - ((32 * 0x1000) - 1);
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr + dataWritePosition, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        dataWritePosition += dataSizeWritten;

        asn += dataSizeWritten;
        ULONGLONG expectedVersion = version;

        ULONG leftoverInBuffer = fullRecordSize - dataWritePosition;

        //
        // Record to force flush
        //
        version++;
        dataSizeWritten = (32 * 0x1000) + 257;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version, asn,
            dataBufferPtr + dataWritePosition, dataSizeWritten,
            coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        RvdLogAsn expectedTTAsn = expectedAsn.Get() + fullRecordSize;
        ULONGLONG expectedTTVersion = version;

        //
        // Read from dedicated log and expect record to exist
        //
        KNt::Sleep(2000);
        dataWritePosition = fullRecordSize;
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);

        //
        // Read from dedicated log and expect record to not exist
        //
        KNt::Sleep(250);
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedTTAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_NOT_FOUND, expectedTTAsn, expectedTTVersion, dataSizeWritten - leftoverInBuffer, dataBufferPtr + fullRecordSize, logStreamId);

        status = FlushCoalescingRecords(*overlayStream->GetCoalesceRecords());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedTTAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedTTAsn, expectedTTVersion, dataSizeWritten - leftoverInBuffer, dataBufferPtr + fullRecordSize, logStreamId);

    }


    //
    // Test 10: Write a few records and pause for less than the periodic timer between records.
    //          After the last record is written wait long enough for the flush timer to fire and
    //          then verify the records are written correctly
    //
    {
        ULONG dataWritePosition = 0;
        RvdLogAsn expectedAsn = asn;

        ULONG periodicFlushTimerInSec = 1 * 60;

        ULONGLONG asnU = asn;
        WriteABunchOfRandomRecords(overlayStream,
            maxIndividualRecordSize,
            coreLoggerOffset,
            reservedMetadataSize,
            dataBufferPtr,
            (periodicFlushTimerInSec/2) * 1000,
            asnU,
            version,
            dataWritePosition);
        asn = asnU;

        ULONGLONG expectedVersion = version;

        printf("Record Written.  Waiting....\n");
        //
        // Do not expect record to be written yet
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_NOT_FOUND, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);

        //
        // Sleep to allow periodic flush to happen
        KNt::Sleep(2 * periodicFlushTimerInSec * 1000);

        printf("Record Written.  Waited....\n");
        
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::DisableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::EnableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Read from dedicated log and expect record to exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn, expectedVersion, dataWritePosition, dataBufferPtr, logStreamId);
    }

    //
    // Test 11: Verify record marker offset is correct on direct write, both
    //         inline and after close
    //

    //
    // Test 12: Verify record marker offset is correct on write in the
    //          middle of coalesced block, both inline and after close

    //
    // Final Cleanup
    //
    {
        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
        
        coreDedicatedReadContext = nullptr;
        coreSharedReadContext = nullptr;
        dedicatedStream = nullptr;
        sharedStream = nullptr;
        writeContext = nullptr;
        ioctlContext = nullptr;
        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;

        coreOpenLog = nullptr;
        coreOpenStream = nullptr;
        
    }

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
                     logContainerGuid);
    DeleteRvdlogFile(diskId,
                     logStreamGuid);

}


VOID CoalescedWrite2Test(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
        activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // For this test, disable behavior where first write is
    // automatically sent to both shared and dedicated as we are
    // testing the coalescing
    //
    overlayStream->ResetIsNextRecordFirstWrite();

    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Create a set of useful contexts
    //
    RvdLogStream::AsyncReadContext::SPtr coreSharedReadContext;
    RvdLogStream::AsyncReadContext::SPtr coreDedicatedReadContext;

    status = sharedStream->CreateAsyncReadContext(coreSharedReadContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = dedicatedStream->CreateAsyncReadContext(coreDedicatedReadContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Allocate buffers and stuff for writing data
    //
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    ULONG dataSizeWritten;

    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    ULONG fullRecordSize;
    fullRecordSize = 3 * (maxStreamRecordSize / 4);
    fullRecordSize -= (2 * KLogicalLogInformation::FixedMetadataSize);
    fullRecordSize += KLogicalLogInformation::FixedMetadataSize - (coreLoggerOffset + reservedMetadataSize +
        sizeof(KLogicalLogInformation::MetadataBlockHeader) +
        sizeof(KLogicalLogInformation::StreamBlockHeader));


    //
    // Max record size also includes the metadata size so subtract it
    // here. Also the core logger wants to reserve 1/4 of the record space for
    // checkpoint records so remove that as well.
    //
    ULONG maxIndividualRecordSize;
    maxIndividualRecordSize = 3 * (maxContainerRecordSize / 4);
    maxIndividualRecordSize -= (2 * KLogicalLogInformation::FixedMetadataSize);
    maxIndividualRecordSize += KLogicalLogInformation::FixedMetadataSize - (coreLoggerOffset + reservedMetadataSize +
        sizeof(KLogicalLogInformation::MetadataBlockHeader) +
        sizeof(KLogicalLogInformation::StreamBlockHeader));

    HRESULT hr;
    ULONG result;
    hr = ULongMult(numberWrites, maxStreamRecordSize, &result);
    KInvariant(SUCCEEDED(hr));
    status = KBuffer::Create(result, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();

    for (ULONG i = 0; i < numberWrites*maxStreamRecordSize; i++)
    {
        dataBufferPtr[i] = i % 255;
    }

    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);

    OverlayStream::AsyncIoctlContextOverlay::SPtr ioctlContext;
    CreateAsyncIoctlContextOverlay(*overlayStream, ioctlContext);
    KBuffer::SPtr outBuffer;

    srand((ULONG)GetTickCount());


    // Test 10: Write 3 records that are exactly a full record and
    //          verify that they get written correctly
    {
        dataSizeWritten = fullRecordSize;

        //
        // Write record 1
        //
        version++;
        RvdLogAsn expectedAsn1 = asn;
        ULONGLONG expectedVersion1 = version;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn, dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += dataSizeWritten;

        //
        // Read from dedicated log and expect record to exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn1, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn1, expectedVersion1, dataSizeWritten, dataBufferPtr, logStreamId);

        //
        // Write record 2
        //
        version++;
        RvdLogAsn expectedAsn2 = asn;
        ULONGLONG expectedVersion2 = version;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn, dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += dataSizeWritten;

        //
        // Read from dedicated log and expect record to exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn2, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn2, expectedVersion2, dataSizeWritten, dataBufferPtr, logStreamId);

        //
        // Write record 3
        //
        version++;
        RvdLogAsn expectedAsn3 = asn;
        ULONGLONG expectedVersion3 = version;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn, dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += dataSizeWritten;

        //
        // Read from dedicated log and expect record to exist
        //
        ReadAndVerifyRecord(coreDedicatedReadContext, reservedMetadataSize, coreLoggerOffset, expectedAsn3, RvdLogStream::AsyncReadContext::ReadExactRecord,
            STATUS_SUCCESS, expectedAsn3, expectedVersion3, dataSizeWritten, dataBufferPtr, logStreamId);
    }

    //
    // Final Cleanup
    //
    {
        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    
        coreDedicatedReadContext = nullptr;
        coreSharedReadContext = nullptr;
        dedicatedStream = nullptr;
        sharedStream = nullptr;
        writeContext = nullptr;
        ioctlContext = nullptr;
        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
        serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;

        coreOpenLog = nullptr;
        coreOpenStream = nullptr;
    }

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;   
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}



VOID StreamQuotaTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    LONGLONG streamSize = 256*1024*1024;

    logContainerGuid.CreateNew();
    logStreamGuid.CreateNew();

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                           &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Test 1: Open log container that holds a single stream and verify
    // the quota assigned to it is correct
    //
    {
        CreateInitialKtlLogStream(diskId,
                                  logContainerGuid,
                                  1,
                                  &logStreamGuid,
                                  KtlLogInformation::KtlLogDefaultStreamType());

        OverlayLog::SPtr overlayLog;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        rvdLogId = logContainerGuid;
        overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                                   diskId,
                                                                   NULL,        // Path
                                                                   rvdLogId,
                                                                   *throttledAllocator);
        VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
        status = overlayLog->Status();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Force recompute of the quotas and check for our stream
        //
        ULONGLONG quota = 0;

        overlayLog->RecomputeStreamQuotas(TRUE, DEFAULT_STREAM_SIZE);    // Include closed streams

        KtlLogStreamId logStreamId = logStreamGuid;
        status = overlayLog->GetStreamQuota(logStreamId,
                                            quota);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(quota == DEFAULT_STREAM_SIZE);

        status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayLog = nullptr;
    }

    //
    // Test 2: Open log container that holds a set of streams that are
    // less that the total capacity of the shared log and verify
    // the quota assigned to it is correct
    //
    {
        KtlLogStreamId logStreamId;
        GUID logStreamGuid1 = logStreamGuid;

        //
        // initial stream is 256MB, plus 1GB is below 2GB for
        // container
        //
        for (ULONG i = 0; i < 4; i++)
        {
            logStreamGuid1.Data1++;
            KGuid guid = logStreamGuid1;
            AddNewKtlLogStream(diskId,
                               logContainerGuid,
                               guid,
                               streamSize);
        }

        OverlayLog::SPtr overlayLog;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        rvdLogId = logContainerGuid;
        overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                                   diskId,
                                                                   NULL,        // Path
                                                                   rvdLogId,
                                                                   *throttledAllocator);
        VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
        status = overlayLog->Status();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Force recompute of the quotas and check for our stream
        //
        ULONGLONG quota = 0;

        overlayLog->RecomputeStreamQuotas(TRUE, DEFAULT_STREAM_SIZE + 4*streamSize);

        logStreamId = logStreamGuid;
        status = overlayLog->GetStreamQuota(logStreamId,
                                            quota);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(quota == DEFAULT_STREAM_SIZE);

        logStreamGuid1 = logStreamGuid;
        for (ULONG i = 0; i < 4; i++)
        {
            logStreamGuid1.Data1++;
            KGuid guid = logStreamGuid1;
            logStreamId = guid;
            status = overlayLog->GetStreamQuota(logStreamId,
                                                quota);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(quota == (ULONGLONG)streamSize);
        }

        status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayLog = nullptr;
    }

    //
    // Test 3: Open log container that holds a set of streams that are
    // equal to the total capacity of the shared log and verify
    // the quota assigned to it is correct
    //
    {
        KtlLogStreamId logStreamId;
        GUID logStreamGuid1 = logStreamGuid;

        //
        // Add 3 more to hit container size of 2GB
        //
        for (ULONG i = 0; i < 3; i++)
        {
            logStreamGuid1.Data2++;
            KGuid guid = logStreamGuid1;
            AddNewKtlLogStream(diskId,
                               logContainerGuid,
                               guid,
                               streamSize);
        }

        OverlayLog::SPtr overlayLog;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        rvdLogId = logContainerGuid;
        overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                                   diskId,
                                                                   NULL,        // Path
                                                                   rvdLogId,
                                                                   *throttledAllocator);
        VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
        status = overlayLog->Status();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Force recompute of the quotas and check for our stream
        //
        ULONGLONG quota = 0;

        overlayLog->RecomputeStreamQuotas(TRUE, DEFAULT_STREAM_SIZE + 7*streamSize);

        logStreamId = logStreamGuid;
        status = overlayLog->GetStreamQuota(logStreamId,
                                            quota);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(quota == DEFAULT_STREAM_SIZE);

        logStreamGuid1 = logStreamGuid;
        for (ULONG i = 0; i < 4; i++)
        {
            logStreamGuid1.Data1++;
            KGuid guid = logStreamGuid1;
            logStreamId = guid;
            status = overlayLog->GetStreamQuota(logStreamId,
                                                quota);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(quota == (ULONGLONG)streamSize);
        }

        logStreamGuid1 = logStreamGuid;
        for (ULONG i = 0; i < 3; i++)
        {
            logStreamGuid1.Data2++;
            KGuid guid = logStreamGuid1;
            logStreamId = guid;
            status = overlayLog->GetStreamQuota(logStreamId,
                                                quota);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(quota == (ULONGLONG)streamSize);
        }

        status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayLog = nullptr;
    }

    //
    // Test 4: Open log container that holds a set of streams that are
    // more than the total capacity of the shared log and verify
    // the quota assigned to it is correct
    //
    {
        KtlLogStreamId logStreamId;
        GUID logStreamGuid1 = logStreamGuid;

        //
        // Streams are 1GB so add new streams to make up to 19GB
        //
        LONGLONG streamSize512 = 512*1024*1024;
        for (ULONG i = 0; i < 12; i++)
        {
            logStreamGuid1.Data3++;
            KGuid guid = logStreamGuid1;
            AddNewKtlLogStream(diskId,
                               logContainerGuid,
                               guid,
                               streamSize512);
        }

        OverlayLog::SPtr overlayLog;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        rvdLogId = logContainerGuid;
        overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                                   diskId,
                                                                   NULL,        // Path
                                                                   rvdLogId,
                                                                   *throttledAllocator);
        VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
        status = overlayLog->Status();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Force recompute of the quotas and check for our stream
        //
        ULONGLONG quota = 0;

        overlayLog->RecomputeStreamQuotas(TRUE, DEFAULT_STREAM_SIZE + 7*streamSize + 12*streamSize512);

        logStreamId = logStreamGuid;
        status = overlayLog->GetStreamQuota(logStreamId,
                                            quota);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(quota == (DEFAULT_STREAM_SIZE / 4));

        logStreamGuid1 = logStreamGuid;
        for (ULONG i = 0; i < 3; i++)
        {
            logStreamGuid1.Data2++;
            KGuid guid = logStreamGuid1;
            logStreamId = guid;
            status = overlayLog->GetStreamQuota(logStreamId,
                                                quota);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(quota == (ULONGLONG)(streamSize / 4));
        }

        logStreamGuid1 = logStreamGuid;
        for (ULONG i = 0; i < 4; i++)
        {
            logStreamGuid1.Data1++;
            KGuid guid = logStreamGuid1;
            logStreamId = guid;
            status = overlayLog->GetStreamQuota(logStreamId,
                                                quota);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(quota == (ULONGLONG)(streamSize / 4));
        }

        logStreamGuid1 = logStreamGuid;
        for (ULONG i = 0; i < 12; i++)
        {
            logStreamGuid1.Data3++;
            KGuid guid = logStreamGuid1;
            logStreamId = guid;
            status = overlayLog->GetStreamQuota(logStreamId,
                                                quota);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(quota == (ULONGLONG)(streamSize512 / 4));
        }

        status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayLog = nullptr;
    }

    //
    // Test 5 & 6: Open one of the streams and set quota to be smaller
    // and larger than the record size and verify that the write is sent to D log only
    //
    {
        RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
        RvdLog::SPtr coreLog;

        status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogContainerId logId = logContainerGuid;
        KtlLogStreamId logStreamId = logStreamGuid;
        coreOpenLog->StartOpenLog(diskId,
                                  logId,
                                  coreLog,
                                  NULL,
                                  sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

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
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        rvdLogId = logContainerGuid;
        overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                                   diskId,
                                                                   NULL,        // Path
                                                                   rvdLogId,
                                                                   *throttledAllocator);
        VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
        status = overlayLog->Status();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
                                                                         DEFAULT_MAX_RECORD_SIZE,
                                                                         DEFAULT_STREAM_SIZE,
                                                                         0x10000,
                                                                         *throttledAllocator);

        VERIFY_IS_TRUE(overlayStream ? TRUE : FALSE);
        status = overlayStream->Status();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = overlayStream->StartServiceOpen(NULL,      // ParentAsync
                                                 serviceSync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        OverlayStream::AsyncWriteContext::SPtr writeContext1;
        OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;

        status = overlayStream->CreateAsyncWriteContext(writeContext1);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeContext.Attach(static_cast<OverlayStream::AsyncWriteContextOverlay*>(writeContext1.Detach()));
        KIoBuffer::SPtr ioBufferSmall;
        KIoBuffer::SPtr ioBufferBig;
        KBuffer::SPtr metadataBuffer;
        VOID* p;
        BOOLEAN sendToShared;

        status = KIoBuffer::CreateSimple(0x10000 / 2,
                                         ioBufferSmall,
                                         p,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(0x10000 + 0x1000,
                                         ioBufferBig,
                                         p,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        status = KBuffer::Create(0x1024,
                                 metadataBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Test 5: Set quota larger than record size
        //
        overlayStream->SetSharedQuota(0x10000);
        ULONGLONG asn = 1001;

        writeContext->Reuse();
        writeContext->StartReservedWrite(0,
                                         asn,
                                         1,       // Version
                                         metadataBuffer,
                                         ioBufferSmall,
                                         FALSE,
                                         &sendToShared,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(sendToShared ? TRUE : FALSE);

        writeContext = nullptr; 
        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;

        coreLog = nullptr;
        coreOpenLog = nullptr;
        status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayLog = nullptr;
    }

    //
    // Cleanup
    //
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    // TODO: There is a bug here where this will crash if the log container is deleted as a container.
    // Right now we workaround by deleting by file
    //DeleteInitialKtlLogStream(diskId,
    //                        logContainerGuid);

    KGuid logStreamGuid1 = logStreamGuid;
    for (ULONG i = 0; i < 4; i++)
    {
        logStreamGuid1.Data1++;
        KGuid guid = logStreamGuid1;
        DeleteRvdlogFile(diskId, guid);
    }

    logStreamGuid1 = logStreamGuid;
    for (ULONG i = 0; i < 3; i++)
    {
        logStreamGuid1.Data2++;
        KGuid guid = logStreamGuid1;
        DeleteRvdlogFile(diskId, guid);
    }

    logStreamGuid1 = logStreamGuid;
    for (ULONG i = 0; i < 12; i++)
    {
        logStreamGuid1.Data3++;
        KGuid guid = logStreamGuid1;
        DeleteRvdlogFile(diskId, guid);
    }

    DeleteRvdlogFile(diskId, logContainerGuid);
    DeleteRvdlogFile(diskId, logStreamGuid);
}

BOOLEAN IsInRecordsList(
    KArray<RvdLogStream::RecordMetadata>& Records,
    ULONGLONG Asn,
    ULONGLONG Version
    )
{
    BOOLEAN isInList = FALSE;

    for (ULONG i = 0; i < Records.Count(); i++)
    {
        if ((Records[i].Asn.Get() == Asn) &&
            (Records[i].Version == Version))
        {
            isInList = TRUE;
            break;
        }
    }

    return(isInList);
}


class ReadRaceRvdLogStream : public RvdLogStreamShim
{
    K_FORCE_SHARED(ReadRaceRvdLogStream);

public:

    enum TestCaseIds
    {
        NoAction = 0,
        TrimReadWriteRace = 1,
        LogTruncatedRace = 2
    };

    struct TrimReadWriteRaceStruct
    {
        ULONGLONG DedicatedAsnToPending;
        BOOLEAN OneShot;
    };

    struct LogTruncatedRaceStruct
    {
        ULONGLONG AsnToTruncate;
        BOOLEAN OneShot;
    };

    static NTSTATUS Create(ReadRaceRvdLogStream::SPtr& Context,
                           KAllocator& Allocator,
                           ULONG AllocationTag)
    {
        ReadRaceRvdLogStream::SPtr context;

        context = _new(AllocationTag, Allocator) ReadRaceRvdLogStream();
        VERIFY_IS_TRUE( (context) ? true : false );
        VERIFY_IS_TRUE(NT_SUCCESS(context->Status()));

        Context = Ktl::Move(context);
        return(STATUS_SUCCESS);
    }

public:
    //
    // RvdLogStream Interface
    //
    LONGLONG
    QueryLogStreamUsage()
    {
        return(RvdLogStreamShim::QueryLogStreamUsage());
    }

    VOID
    QueryLogStreamType(__out RvdLogStreamType& LogStreamType)
    {
        RvdLogStreamShim::QueryLogStreamType(LogStreamType);
    }

    VOID
    QueryLogStreamId(__out RvdLogStreamId& LogStreamId)
    {
        RvdLogStreamShim::QueryLogStreamId(LogStreamId);
    }

    NTSTATUS
    QueryRecordRange(
        __out_opt RvdLogAsn* const LowestAsn,
        __out_opt RvdLogAsn* const HighestAsn,
        __out_opt RvdLogAsn* const LogTruncationAsn)
    {
        return(RvdLogStreamShim::QueryRecordRange(LowestAsn, HighestAsn, LogTruncationAsn));
    }

    ULONGLONG QueryCurrentReservation()
    {
        return(RvdLogStreamShim::QueryCurrentReservation());
    }

    NTSTATUS
    SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal)
    {
        return(RvdLogStreamShim::SetTruncationCompletionEvent(EventToSignal));
    }   

    NTSTATUS
    CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context)
    {
        return(RvdLogStreamShim::CreateAsyncWriteContext(Context));
    }

    class AsyncReadContextShim : public AsyncReadContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadContextShim);

        friend ReadRaceRvdLogStream;

    public:

        AsyncReadContextShim(ReadRaceRvdLogStream& LogStreamShim);

        VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
        {
            _RecordAsn = RecordAsn;
            _Type = (RvdLogStream::AsyncReadContext::ReadType)-1;
            _Version = Version;
            _MetaDataBuffer = &MetaDataBuffer;
            _IoBuffer = &IoBuffer;
            Start(ParentAsyncContext, CallbackPtr);
        }

        VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __in RvdLogStream::AsyncReadContext::ReadType Type,
            __out_opt RvdLogAsn* const ActualRecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
        {
            _RecordAsn = RecordAsn;
            _Type = Type;
            _ActualRecordAsn = ActualRecordAsn;
            _Version = Version;
            _MetaDataBuffer = &MetaDataBuffer;
            _IoBuffer = &IoBuffer;
            Start(ParentAsyncContext, CallbackPtr);
        }

    private:
        VOID OperationCompletion(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase& Async
            )
        {
            NTSTATUS status;

            status = Async.Status();

            ULONG action = _LogStreamShim->GetTestCaseId();

            if (action == LogTruncatedRace)
            {
                LogTruncatedRaceStruct* s = (LogTruncatedRaceStruct*)_LogStreamShim->GetTestCaseData();
                if ((_RecordAsn.Get() == s->AsnToTruncate) && (! s->OneShot))
                {
                    *_MetaDataBuffer = nullptr;
                    *_IoBuffer = nullptr;
                    status = STATUS_NOT_FOUND;
                }
            }

            Complete(status);
        }

    protected:
        VOID
        OnStart(
            )
        {
            KAsyncContextBase::CompletionCallback completion(this, &ReadRaceRvdLogStream::AsyncReadContextShim::OperationCompletion);

            if (_Type == (RvdLogStream::AsyncReadContext::ReadType)-1)
            {
                _CoreReadContext->StartRead(_RecordAsn,
                                            _Version,
                                            *_MetaDataBuffer,
                                            *_IoBuffer,
                                            this,
                                            completion);
            } else {
                _CoreReadContext->StartRead(_RecordAsn,
                                            _Type,
                                            _ActualRecordAsn,
                                            _Version,
                                            *_MetaDataBuffer,
                                            *_IoBuffer,
                                            this,
                                            completion);
            }
        }

        VOID
        OnReuse(
            )
        {
            _CoreReadContext->Reuse();
        }

        VOID
        OnCompleted(
            )
        {
        }

        VOID
        OnCancel(
            )
        {
            _CoreReadContext->Cancel();
        }

    private:
        RvdLogStreamShim::SPtr _LogStreamShim;
        RvdLogStream::AsyncReadContext::SPtr _CoreReadContext;

        RvdLogAsn _RecordAsn;
        RvdLogStream::AsyncReadContext::ReadType _Type;
        RvdLogAsn* _ActualRecordAsn;
        ULONGLONG* _Version;
        KBuffer::SPtr* _MetaDataBuffer;
        KIoBuffer::SPtr* _IoBuffer;
    };

    NTSTATUS
    CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context)
    {
        NTSTATUS status;

        ReadRaceRvdLogStream::AsyncReadContextShim::SPtr context;

        context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadContextShim(*this);
        VERIFY_IS_TRUE(context ? true : false);
        VERIFY_IS_TRUE(NT_SUCCESS(context->Status()));

        status = RvdLogStreamShim::CreateAsyncReadContext(context->_CoreReadContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        Context = context.RawPtr();

        return(status);
    }

    NTSTATUS
    QueryRecord(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize,
        __out_opt ULONGLONG* const DebugInfo1)
    {
        NTSTATUS status;

        status = RvdLogStreamShim::QueryRecord(RecordAsn, Version, Disposition, IoBufferSize, DebugInfo1);

        return(status);
    }


    NTSTATUS
    QueryRecord(
        __inout RvdLogAsn& RecordAsn,
        __in RvdLogStream::AsyncReadContext::ReadType Type,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize,
        __out_opt ULONGLONG* const DebugInfo1)
    {
        NTSTATUS status;

        ULONG action = GetTestCaseId();
        TrimReadWriteRaceStruct* s = (TrimReadWriteRaceStruct*)GetTestCaseData();

        status = RvdLogStreamShim::QueryRecord(RecordAsn, Type, Version, Disposition, IoBufferSize, DebugInfo1);

        if ((action == TrimReadWriteRace) && (! s->OneShot))
        {
            s->OneShot = TRUE;
            *Disposition = RvdLogStream::RecordDisposition::eDispositionPending;
        }

        return(status);
    }


    NTSTATUS
    QueryRecords(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RvdLogStream::RecordMetadata>& ResultsArray)
    {
        return(RvdLogStreamShim::QueryRecords(LowestAsn, HighestAsn, ResultsArray));
    }

    NTSTATUS
    DeleteRecord(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version)
    {
        return(RvdLogStreamShim::DeleteRecord(RecordAsn, Version));
    }

    VOID
    Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint)
    {
        RvdLogStreamShim::Truncate(TruncationPoint, PreferredTruncationPoint);
    }

    VOID
    TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version)
    {
        RvdLogStreamShim::TruncateBelowVersion(TruncationPoint, Version);
    }

    NTSTATUS
    CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context)
    {
        return(RvdLogStreamShim::CreateUpdateReservationContext(Context));
    }
};

ReadRaceRvdLogStream::ReadRaceRvdLogStream()
{
}

ReadRaceRvdLogStream::~ReadRaceRvdLogStream()
{
}

ReadRaceRvdLogStream::AsyncReadContextShim::AsyncReadContextShim(
    ReadRaceRvdLogStream& LogStreamShim
    ) :
   _LogStreamShim(&LogStreamShim)
{
}

ReadRaceRvdLogStream::AsyncReadContextShim::~AsyncReadContextShim()
{
}

NTSTATUS FormatLLRecordFromCircularBuffer(
    LLRecordObject& llRecord,
    KtlLogStreamId StreamId,
    ULONG CoreLoggerOverhead,
    ULONGLONG StreamOffset,
    ULONGLONG Version,
    ULONG DataSize,
    PUCHAR CircularBuffer,
    ULONG CircularBufferSize,
    ULONG& CircularBufferOffset,
    KBuffer::SPtr& MetadataBuffer,
    KIoBuffer::SPtr& IoBuffer
    )
{
    NTSTATUS status;
    ULONG metadataSizeAvailable;
    ULONG headerSkipOffset = CoreLoggerOverhead;
    CoreLoggerOverhead = 0;
    PVOID p;
    ULONG ioBufferSize;

    ULONG modCircularBufferOffset = (StreamOffset - 1) % CircularBufferSize;

    printf("StreamOffset: %I64x, CircOffset: %x, ModCirc: %x, DataSize %x\n", StreamOffset, CircularBufferOffset, modCircularBufferOffset, DataSize);

    metadataSizeAvailable = KLogicalLogInformation::FixedMetadataSize - (headerSkipOffset +
                                                                         ((sizeof(KtlLogVerify::KtlMetadataHeader) + 7) & ~7) +
                                                                         (sizeof(KLogicalLogInformation::MetadataBlockHeader)) +
                                                                         (sizeof(KLogicalLogInformation::StreamBlockHeader)));

    status = KBuffer::Create(KLogicalLogInformation::FixedMetadataSize - headerSkipOffset,
                             MetadataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    if (DataSize <= metadataSizeAvailable)
    {
        ioBufferSize = 0;
        status = KIoBuffer::CreateEmpty(IoBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    } else {
        ioBufferSize = DataSize - metadataSizeAvailable;

        status = KIoBuffer::CreateSimple(KLoggerUtilities::RoundUpTo4K(ioBufferSize), IoBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    llRecord.Initialize(headerSkipOffset,
                        CoreLoggerOverhead,
                        *MetadataBuffer,
                        IoBuffer.RawPtr());

    llRecord.InitializeHeadersForNewRecord(StreamId,
                                           StreamOffset,
                                           Version);
    ULONG dataCopySize1, dataCopyOffset1;
    ULONG dataCopySize2, dataCopyOffset2;

    if ((CircularBufferOffset + DataSize) > CircularBufferSize)
    {
        dataCopyOffset1 = CircularBufferOffset;
        dataCopySize1 = CircularBufferSize - CircularBufferOffset;
        dataCopyOffset2 = 0;
        dataCopySize2 = DataSize - dataCopySize1;
        CircularBufferOffset = dataCopyOffset2 + dataCopySize2;
    } else {
        dataCopyOffset1 = CircularBufferOffset;
        dataCopySize1 = DataSize;
        dataCopySize2 = 0;
        CircularBufferOffset = dataCopyOffset1 + dataCopySize1;
    }

    llRecord.CopyFrom(CircularBuffer, dataCopyOffset1, dataCopySize1);
    if (dataCopySize2 != 0)
    {
        llRecord.CopyFrom(CircularBuffer, dataCopyOffset2, dataCopySize2);
    }

    llRecord.UpdateHeaderInformation(Version, 0, KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord);

    return(STATUS_SUCCESS);
}

VOID CompareBytes(
    PUCHAR a,
    PUCHAR b,
    ULONG len
    )
{
    for (ULONG i = 0; i < len; i++)
    {
        VERIFY_IS_TRUE(a[i] == b[i]);
    }
}

VOID VerifyCircularData(
    PUCHAR circularData,
    ULONG circularBufferSize,
    ULONGLONG StreamOffset,
    ULONG DataSize,
    ULONG dataOffset,
    ULONG coreLoggerOffset,
    KBuffer& metadataBuffer,
    KIoBuffer& ioBuffer
)
{
    ULONG dataSizeInMetadata, dataSizeInIoBuffer;
    ULONG maxDataInMetadata = KLogicalLogInformation::FixedMetadataSize - dataOffset;

    if (DataSize < maxDataInMetadata)
    {
        dataSizeInMetadata = DataSize;
        dataSizeInIoBuffer = 0;
    } else {
        dataSizeInMetadata = maxDataInMetadata;
        dataSizeInIoBuffer = DataSize - maxDataInMetadata;
    }

    ULONG circularOffset = (StreamOffset - 1) % circularBufferSize;
    ULONG circularBytes = circularBufferSize - circularOffset;

    PUCHAR dataPtr;

    dataPtr = (PUCHAR)(metadataBuffer.GetBuffer()) + dataOffset - coreLoggerOffset;
    if (dataSizeInMetadata > circularBytes)
    {
        CompareBytes(dataPtr, circularData + circularOffset, circularBytes);
        dataSizeInMetadata -= circularBytes;
        CompareBytes(dataPtr+circularBytes, circularData, dataSizeInMetadata);
        circularOffset += dataSizeInMetadata;
    } else {
        CompareBytes(dataPtr, circularData + circularOffset, dataSizeInMetadata);
        circularOffset += dataSizeInMetadata;
    }

    circularBytes = circularBufferSize - circularOffset;
    dataPtr = (PUCHAR)(ioBuffer.First()->GetBuffer());
    if (dataSizeInIoBuffer > circularBytes)
    {
        CompareBytes(dataPtr, circularData + circularOffset, circularBytes);
        dataSizeInIoBuffer -= circularBytes;
        CompareBytes(dataPtr+circularBytes, circularData, dataSizeInIoBuffer);
        circularOffset += dataSizeInMetadata;
    } else {
        CompareBytes(dataPtr, circularData + circularOffset, dataSizeInIoBuffer);
        circularOffset += dataSizeInMetadata;
    }
}


VOID ReadRaceConditionsTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
        activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Replace streams with shims
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    ReadRaceRvdLogStream::SPtr dedicatedShim;
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    ReadRaceRvdLogStream::SPtr sharedShim;

    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    sharedCoreStream = overlayStream->GetSharedLogStream();

    status = ReadRaceRvdLogStream::Create(dedicatedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);

    status = ReadRaceRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);

    //
    // Create data buffer we use for writing and verifying results
    //
    const ULONG circularBufferSize = 1024 * 1024 * 1024;
    KBuffer::SPtr circularDataBuffer;
    PUCHAR circularData;
    status = KBuffer::Create(circularBufferSize, circularDataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    circularData = (PUCHAR)circularDataBuffer->GetBuffer();

    for (ULONG i = 0; i < circularBufferSize; i++)
    {
        circularData[i] = i % 255;
    }

    //
    // Test 1: Simulate the race condition where a dedicated write
    // completes in the middle of a read. The read will find the record
    // in the shared log and then read the record. However after the
    // record is read a write to the dedicated will overwrite part of
    // the shared log record and thus cause the shared log record to be
    // trimmed to before the location of the read.
    //

    static const ULONG sharedPatternCount = 21;
    ULONG sharedPattern[sharedPatternCount] = { 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x1696c, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0,
                                                0x17203, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x17f15, 0x3eef0, 0x3eef0, 0x927,
                                                0x66ce, 0x55d7, 0x55d7 };

    // must be a Dedicated record 2603969 (27BBC1)
    static const ULONG dedicatedPatternCount = 12;
    ULONG dedicatedPattern[dedicatedPatternCount] = { 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0,
                                                       0x6660, 0x3eef0, 0x3eef0, 0x3eef0 };
    ULONG dedicatedVersion[dedicatedPatternCount] = {       1,       2,       3,       4,       6,       7,       8,       9,
                                                          0xb,     0xd,     0xe,     0xf };
    static const ULONGLONG readAt = 0x29030c;

    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;

    status = KBuffer::Create(0x3eef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();

    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    //
    // Configure shared log
    //
    ULONGLONG asn = 1;
    ULONGLONG version = 0;
    LLRecordObject llRecord;
    ULONG circularOffset;

    circularOffset = 0;
    for (ULONG i = 0; i < sharedPatternCount; i++)
    {
        version++;

        llRecord.Clear();
        status = FormatLLRecordFromCircularBuffer(llRecord,
                                                  logStreamId,
                                                  coreLoggerOffset,
                                                  asn,
                                                  version,
                                                  sharedPattern[i],
                                                  circularData,
                                                  circularBufferSize,
                                                  circularOffset,
                                                  metadataKBuffer,
                                                  dataIoBuffer);
        status = WriteARawRecord(sharedStream,
                                 version, asn, metadataKBuffer, 0, dataIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += sharedPattern[i];
    }
    ULONGLONG tail = asn;

    //
    // Configure dedicated log
    //
    asn = 1;
    version = 0;
    circularOffset = 0;

    for (ULONG i = 0; i < dedicatedPatternCount; i++)
    {
        llRecord.Clear();
        status = FormatLLRecordFromCircularBuffer(llRecord,
                                                  logStreamId,
                                                  coreLoggerOffset,
                                                  asn,
                                                  version,
                                                  dedicatedPattern[i],
                                                  circularData,
                                                  circularBufferSize,
                                                  circularOffset,
                                                  metadataKBuffer,
                                                  dataIoBuffer);

        status = WriteARawRecord(dedicatedStream,
                                 dedicatedVersion[i], asn, metadataKBuffer, 0, dataIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += dedicatedPattern[i];
    }

    overlayStream->SetLogicalLogTailOffset(tail);

    {
        //
        // Setup test to have QueryRecords on dedicated show record is
        // pending
        //
        ReadRaceRvdLogStream::TrimReadWriteRaceStruct testCaseData;

        testCaseData.DedicatedAsnToPending = 0x27BBC1;
        testCaseData.OneShot = FALSE;
        dedicatedShim->SetTestCaseData(ReadRaceRvdLogStream::TestCaseIds::TrimReadWriteRace,
                                       &testCaseData);

        sharedShim->SetTestCaseData(ReadRaceRvdLogStream::TestCaseIds::NoAction, NULL);

        //
        // Perform the read and verify we get all of our data back
        //
        OverlayStream::AsyncReadContext::SPtr context;
        KIoBuffer::SPtr ioBuffer;
        KBuffer::SPtr metadataBuffer;

        status = overlayStream->CreateAsyncReadContext(context);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogAsn RecordAsn = readAt;
        KtlLogAsn ActualRecordAsn;
        ULONGLONG ActualVersion;

        context->StartRead(RecordAsn,
                             RvdLogStream::AsyncReadContext::ReadContainingRecord,
                             &ActualRecordAsn,
                             &ActualVersion,
                             metadataBuffer,
                             ioBuffer,
                             NULL,
                             sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Verify record read contains the data we requested
        //
        KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
        ULONG dataSize;
        ULONG dataOffset;
        status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset((PUCHAR)metadataBuffer->GetBuffer(),
                                                               coreLoggerOffset,
                                                               *ioBuffer,
                                                               reservedMetadataSize,
                                                               metadataHeader,
                                                               streamBlockHeader,
                                                               dataSize,
                                                               dataOffset);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONGLONG streamOffset = streamBlockHeader->StreamOffset;
        VERIFY_IS_TRUE((streamOffset + streamBlockHeader->DataSize) > readAt);

        VerifyCircularData(circularData,
                           circularBufferSize,
                           streamBlockHeader->StreamOffset,
                           streamBlockHeader->DataSize,
                           dataOffset,
                           coreLoggerOffset,
                           *metadataBuffer,
                           *ioBuffer);
    }

    //
    // Test 2: Record found in shared but read from shared fails. This
    //         simulates scenario where shared is truncated while read in
    //         progress
    //
    {
        //
        // Fail read from shared log to simulate truncation
        //
        ReadRaceRvdLogStream::LogTruncatedRaceStruct testCaseData1;
        testCaseData1.AsnToTruncate = 0x2641e0;
        testCaseData1.OneShot = FALSE;
        sharedShim->SetTestCaseData(ReadRaceRvdLogStream::TestCaseIds::LogTruncatedRace,
                                       &testCaseData1);

        //
        // Show record as pending to force first read from shared
        //
        ReadRaceRvdLogStream::TrimReadWriteRaceStruct testCaseData2;
        testCaseData2.DedicatedAsnToPending = 0x27BBC1;
        testCaseData2.OneShot = FALSE;
        dedicatedShim->SetTestCaseData(ReadRaceRvdLogStream::TestCaseIds::TrimReadWriteRace,
                                       &testCaseData2);

        //
        // Perform the read and verify we get all of our data back
        //
        OverlayStream::AsyncReadContext::SPtr context;
        KIoBuffer::SPtr ioBuffer;
        KBuffer::SPtr metadataBuffer;

        status = overlayStream->CreateAsyncReadContext(context);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogAsn RecordAsn = readAt;
        KtlLogAsn ActualRecordAsn;
        ULONGLONG ActualVersion;

        context->StartRead(RecordAsn,
                             RvdLogStream::AsyncReadContext::ReadContainingRecord,
                             &ActualRecordAsn,
                             &ActualVersion,
                             metadataBuffer,
                             ioBuffer,
                             NULL,
                             sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Verify record read contains the data we requested
        //
        KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
        ULONG dataSize;
        ULONG dataOffset;
        status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset((PUCHAR)metadataBuffer->GetBuffer(),
                                                               coreLoggerOffset,
                                                               *ioBuffer,
                                                               reservedMetadataSize,
                                                               metadataHeader,
                                                               streamBlockHeader,
                                                               dataSize,
                                                               dataOffset);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VerifyCircularData(circularData,
                           circularBufferSize,
                           streamBlockHeader->StreamOffset,
                           streamBlockHeader->DataSize,
                           dataOffset,
                           coreLoggerOffset,
                           *metadataBuffer,
                           *ioBuffer);
    }

    //
    // Test 3: Dedicated truncated after DetermineRecordLocation
    //
    {
        //
        // Fail read from dedicated log to simulate truncation
        //
        ReadRaceRvdLogStream::LogTruncatedRaceStruct testCaseData1;
        testCaseData1.AsnToTruncate = 0x27BBC1;
        testCaseData1.OneShot = FALSE;
        dedicatedShim->SetTestCaseData(ReadRaceRvdLogStream::TestCaseIds::LogTruncatedRace,
                                       &testCaseData1);

        //
        // Show record as pending to force first read from shared
        //
        sharedShim->SetTestCaseData(ReadRaceRvdLogStream::TestCaseIds::NoAction,
                                    NULL);

        //
        // Perform the read and verify we get all of our data back
        //
        OverlayStream::AsyncReadContext::SPtr context;
        KIoBuffer::SPtr ioBuffer;
        KBuffer::SPtr metadataBuffer;

        status = overlayStream->CreateAsyncReadContext(context);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogAsn RecordAsn = readAt;
        KtlLogAsn ActualRecordAsn;
        ULONGLONG ActualVersion;

        context->StartRead(RecordAsn,
                             RvdLogStream::AsyncReadContext::ReadContainingRecord,
                             &ActualRecordAsn,
                             &ActualVersion,
                             metadataBuffer,
                             ioBuffer,
                             NULL,
                             sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
    }

    //
    // Test 4: Dedicated QueryRecord shows Persisted but write not
    //         completed
    //

    //
    // Test 5: Shared QueryRecord shows persisted but write not
    //         completed
    //

    //
    // Final Cleanup
    //
    {
        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
        
        dedicatedShim = nullptr;
        dedicatedCoreStream = nullptr;

        sharedShim = nullptr;
        sharedCoreStream = nullptr;

        dedicatedStream = nullptr;
        sharedStream = nullptr;
        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
            serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;

        coreOpenLog = nullptr;
        coreOpenStream = nullptr;
    }

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}

//**
class WriteStuckRvdLogStream : public RvdLogStreamShim
{
    K_FORCE_SHARED(WriteStuckRvdLogStream);

public:

    enum TestCaseIds
    {
        NoAction = 0,
        FailWithNTStatus = 1,
        DelayOnStart = 2,
        WaitForEvent = 3,
        WaitForEventAndFail = 4
    };

    struct FailWithNTStatusStruct
    {
        NTSTATUS FailStatus;
        ULONG PassFrequency;   // Use 0 to fail always, 1 pass every other time, 2 pass every third time
        ULONG FailCounter;
        ULONG NumberFails;
        ULONG NumberSuccess;
    };

    struct DelayOnStartStruct
    {
        ULONG DelayInMs;
    };

    struct WaitForEventStruct
    {
        KAsyncEvent* WaitEvent;
    };

    struct WaitForEventAndFailStruct
    {
        KAsyncEvent* WaitEvent;
        NTSTATUS FailStatus;
        volatile LONG FailedCount;
    };

    static NTSTATUS Create(WriteStuckRvdLogStream::SPtr& Context,
                           KAllocator& Allocator,
                           ULONG AllocationTag)
    {
        WriteStuckRvdLogStream::SPtr context;

        context = _new(AllocationTag, Allocator) WriteStuckRvdLogStream();
        VERIFY_IS_TRUE( (context) ? true : false );
        VERIFY_IS_TRUE(NT_SUCCESS(context->Status()));

        Context = Ktl::Move(context);
        return(STATUS_SUCCESS);
    }

public:
    //
    // RvdLogStream Interface
    //
    LONGLONG
    QueryLogStreamUsage()
    {
        return(RvdLogStreamShim::QueryLogStreamUsage());
    }

    VOID
    QueryLogStreamType(__out RvdLogStreamType& LogStreamType)
    {
        RvdLogStreamShim::QueryLogStreamType(LogStreamType);
    }

    VOID
    QueryLogStreamId(__out RvdLogStreamId& LogStreamId)
    {
        RvdLogStreamShim::QueryLogStreamId(LogStreamId);
    }

    NTSTATUS
    QueryRecordRange(
        __out_opt RvdLogAsn* const LowestAsn,
        __out_opt RvdLogAsn* const HighestAsn,
        __out_opt RvdLogAsn* const LogTruncationAsn)
    {
        return(RvdLogStreamShim::QueryRecordRange(LowestAsn, HighestAsn, LogTruncationAsn));
    }

    ULONGLONG QueryCurrentReservation()
    {
        return(RvdLogStreamShim::QueryCurrentReservation());
    }

    NTSTATUS
    SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal)
    {
        return(RvdLogStreamShim::SetTruncationCompletionEvent(EventToSignal));
    }   
    
    NTSTATUS
    CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context)
    {
        return(RvdLogStreamShim::CreateAsyncReadContext(Context));
    }

    class AsyncWriteContextShim : public AsyncWriteContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContextShim);

        friend WriteStuckRvdLogStream;

    public:

        AsyncWriteContextShim(WriteStuckRvdLogStream& LogStreamShim);

        enum ApiCalled
        {
            StartWriteApi,
            StartWriteWithLowPriorityIOApi,
            StartReservedWriteApi,
            StartReservedWriteWithLowPriorityIOApi,
            StartReservedWriteWithLogSizeAndSpaceRemaining
        } _ApiCalled;

        VOID
        StartWrite(
            __in BOOLEAN LowPriorityIO,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            _ApiCalled = StartWriteWithLowPriorityIOApi;

            _LowPriorityIO = LowPriorityIO;
            _RecordAsn = RecordAsn;
            _Version = Version;
            _MetaDataBuffer = MetaDataBuffer;
            _IoBuffer = IoBuffer;
            _LogSize = nullptr;
            _LogSpaceRemaining = nullptr;

            Start(ParentAsyncContext, CallbackPtr);
        }

        VOID
        StartWrite(
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            _ApiCalled = StartWriteApi;

            _RecordAsn = RecordAsn;
            _Version = Version;
            _MetaDataBuffer = MetaDataBuffer;
            _IoBuffer = IoBuffer;
            _LogSize = nullptr;
            _LogSpaceRemaining = nullptr;

            Start(ParentAsyncContext, CallbackPtr);
        }

        VOID
        StartReservedWrite(
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            _ApiCalled = StartReservedWriteApi;

            _ReserveToUse = ReserveToUse;
            _RecordAsn = RecordAsn;
            _Version = Version;
            _MetaDataBuffer = MetaDataBuffer;
            _IoBuffer = IoBuffer;
            _LogSize = nullptr;
            _LogSpaceRemaining = nullptr;

            Start(ParentAsyncContext, CallbackPtr);
        }
        
        VOID
        StartReservedWrite(
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __out ULONGLONG& LogSize,
            __out ULONGLONG& LogSpaceRemaining,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            _ApiCalled = StartReservedWriteWithLogSizeAndSpaceRemaining;

            _ReserveToUse = ReserveToUse;
            _RecordAsn = RecordAsn;
            _Version = Version;
            _MetaDataBuffer = MetaDataBuffer;
            _IoBuffer = IoBuffer;
            _LogSize = &LogSize;
            _LogSpaceRemaining = &LogSpaceRemaining;

            Start(ParentAsyncContext, CallbackPtr);
        }

        VOID
        StartReservedWrite(
            __in BOOLEAN LowPriorityIO,
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            _ApiCalled = StartReservedWriteWithLowPriorityIOApi;

            _LowPriorityIO = LowPriorityIO;
            _ReserveToUse = ReserveToUse;
            _RecordAsn = RecordAsn;
            _Version = Version;
            _MetaDataBuffer = MetaDataBuffer;
            _IoBuffer = IoBuffer;
            _LogSize = nullptr;
            _LogSpaceRemaining = nullptr;

            Start(ParentAsyncContext, CallbackPtr);
        }

    private:
        VOID OperationCompletion(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase& Async
            )
        {
            NTSTATUS status;

            status = Async.Status();

            Complete(status);
        }

    protected:

        VOID ForwardStartWrite()
        {
            //
            // Forward write to core logger
            //
            KAsyncContextBase::CompletionCallback completion(this, &WriteStuckRvdLogStream::AsyncWriteContextShim::OperationCompletion);

            switch (_ApiCalled)
            {
                case StartWriteApi:
                {
                    _CoreWriteContext->StartWrite(_RecordAsn,
                                                  _Version,
                                                  _MetaDataBuffer,
                                                  _IoBuffer,
                                                  this,
                                                  completion);
                    break;
                }

                case StartWriteWithLowPriorityIOApi:
                {
                    _CoreWriteContext->StartWrite(_LowPriorityIO,
                                                  _RecordAsn,
                                                  _Version,
                                                  _MetaDataBuffer,
                                                  _IoBuffer,
                                                  this,
                                                  completion);
                    break;
                }

                case StartReservedWriteApi:
                {
                    _CoreWriteContext->StartReservedWrite(_ReserveToUse,
                                                          _RecordAsn,
                                                          _Version,
                                                          _MetaDataBuffer,
                                                          _IoBuffer,
                                                          this,
                                                          completion);
                    break;
                }

                case StartReservedWriteWithLowPriorityIOApi:
                {
                    _CoreWriteContext->StartReservedWrite(_LowPriorityIO,
                                                          _ReserveToUse,
                                                          _RecordAsn,
                                                          _Version,
                                                          _MetaDataBuffer,
                                                          _IoBuffer,
                                                          this,
                                                          completion);
                    break;
                }

                case StartReservedWriteWithLogSizeAndSpaceRemaining:
                {
                    KInvariant(_LogSize != nullptr);
                    KInvariant(_LogSpaceRemaining != nullptr);

                    _CoreWriteContext->StartReservedWrite(_ReserveToUse,
                                                          _RecordAsn,
                                                          _Version,
                                                          _MetaDataBuffer,
                                                          _IoBuffer,
                                                          *_LogSize,
                                                          *_LogSpaceRemaining,
                                                          this,
                                                          completion);
                    break;
                }

                default:
                {
                    VERIFY_IS_TRUE(FALSE);
                }
            }
        }

        VOID TimerCompletion(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase&
            )
        {
            //
            // When timer is completed then forward the OnStart() call to
            // the underlying async
            //
            _Timer->Reuse();
            ForwardStartWrite();
        }

        VOID EventCompletion(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase&
            )
        {
            //
            // When event fires then forward the OnStart() call to
            // the underlying async
            //
            ForwardStartWrite();
        }

        VOID EventFailCompletion(
            __in_opt KAsyncContextBase* const,
            __in KAsyncContextBase&
            )
        {
            //
            // When event fires then forward the OnStart() call to
            // the underlying async
            //
            WaitForEventAndFailStruct* s = (WaitForEventAndFailStruct*)_LogStreamShim->GetTestCaseData();
            Complete(s->FailStatus);
            InterlockedIncrement(&s->FailedCount);
        }
                
        VOID
        OnStart(
            )
        {
            NTSTATUS status;

            ULONG action = _LogStreamShim->GetTestCaseId();

            if (action == NoAction)
            {
                ForwardStartWrite();
            } else if (action == FailWithNTStatus) {
                //
                // Complete with specified status code
                //
                FailWithNTStatusStruct* s = (FailWithNTStatusStruct*)_LogStreamShim->GetTestCaseData();
                if (s->PassFrequency == 0)
                {
                    s->NumberFails++;
                    status = s->FailStatus;                 
                } else if (s->FailCounter != s->PassFrequency) {
                    s->NumberFails++;
                    s->FailCounter++;
                    status = s->FailStatus;
                } else {
                    s->FailCounter = 0;
                    s->NumberSuccess++;
                    status = STATUS_SUCCESS;
                }
                Complete(status);
            } else if (action == DelayOnStart) {
                KAsyncContextBase::CompletionCallback completion(this,
                                                                 &WriteStuckRvdLogStream::AsyncWriteContextShim::TimerCompletion);

                DelayOnStartStruct* s = (DelayOnStartStruct*)_LogStreamShim->GetTestCaseData();
                _Timer->StartTimer(s->DelayInMs, nullptr, completion);
            } else if (action == WaitForEvent) {
                KAsyncContextBase::CompletionCallback completion(this,
                                                                 &WriteStuckRvdLogStream::AsyncWriteContextShim::EventCompletion);

                WaitForEventStruct* s = (WaitForEventStruct*)_LogStreamShim->GetTestCaseData();
                status = s->WaitEvent->CreateWaitContext(KTL_TAG_TEST, *g_Allocator, _WaitContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                _WaitContext->StartWaitUntilSet(nullptr, completion);
            } else if (action == WaitForEventAndFail) {                
                KAsyncContextBase::CompletionCallback completion(this,
                                                                 &WriteStuckRvdLogStream::AsyncWriteContextShim::EventFailCompletion);

                WaitForEventAndFailStruct* s = (WaitForEventAndFailStruct*)_LogStreamShim->GetTestCaseData();
                status = s->WaitEvent->CreateWaitContext(KTL_TAG_TEST, *g_Allocator, _WaitContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                _WaitContext->StartWaitUntilSet(nullptr, completion);
            } else {
                VERIFY_IS_TRUE(FALSE);
            }
        }

        VOID
        OnReuse(
            )
        {
            _CoreWriteContext->Reuse();
        }

        VOID
        OnCompleted(
            )
        {
        }

        VOID
        OnCancel(
            )
        {
            _CoreWriteContext->Cancel();
        }

    private:
        RvdLogStreamShim::SPtr _LogStreamShim;
        RvdLogStream::AsyncWriteContext::SPtr _CoreWriteContext;
        KTimer::SPtr _Timer;
        KAsyncEvent::WaitContext::SPtr _WaitContext;

        BOOLEAN _LowPriorityIO;
        ULONGLONG _ReserveToUse;
        RvdLogAsn _RecordAsn;
        ULONGLONG _Version;
        KBuffer::SPtr _MetaDataBuffer;
        KIoBuffer::SPtr _IoBuffer;
        ULONGLONG* _LogSize;
        ULONGLONG* _LogSpaceRemaining;
    };

    NTSTATUS
    CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context)
    {
        NTSTATUS status;

        WriteStuckRvdLogStream::AsyncWriteContextShim::SPtr context;

        context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteContextShim(*this);
        VERIFY_IS_TRUE(context ? true : false);
        VERIFY_IS_TRUE(NT_SUCCESS(context->Status()));

        status = RvdLogStreamShim::CreateAsyncWriteContext(context->_CoreWriteContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KTimer::Create(context->_Timer, GetThisAllocator(), GetThisAllocationTag());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        Context = context.RawPtr();

        return(status);
    }

    NTSTATUS
    QueryRecord(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize,
        __out_opt ULONGLONG* const DebugInfo1)
    {
        NTSTATUS status;

        status = RvdLogStreamShim::QueryRecord(RecordAsn, Version, Disposition, IoBufferSize, DebugInfo1);

        return(status);
    }


    NTSTATUS
    QueryRecord(
        __inout RvdLogAsn& RecordAsn,
        __in RvdLogStream::AsyncReadContext::ReadType Type,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize,
        __out_opt ULONGLONG* const DebugInfo1)
    {
        NTSTATUS status;

        status = RvdLogStreamShim::QueryRecord(RecordAsn, Type, Version, Disposition, IoBufferSize, DebugInfo1);

        return(status);
    }


    NTSTATUS
    QueryRecords(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RvdLogStream::RecordMetadata>& ResultsArray)
    {
        return(RvdLogStreamShim::QueryRecords(LowestAsn, HighestAsn, ResultsArray));
    }

    NTSTATUS
    DeleteRecord(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version)
    {
        return(RvdLogStreamShim::DeleteRecord(RecordAsn, Version));
    }

    VOID
    Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint)
    {
        RvdLogStreamShim::Truncate(TruncationPoint, PreferredTruncationPoint);
    }

    VOID
    TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version)
    {
        RvdLogStreamShim::TruncateBelowVersion(TruncationPoint, Version);
    }

    NTSTATUS
    CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context)
    {
        return(RvdLogStreamShim::CreateUpdateReservationContext(Context));
    }
};

WriteStuckRvdLogStream::WriteStuckRvdLogStream()
{
}

WriteStuckRvdLogStream::~WriteStuckRvdLogStream()
{
}

WriteStuckRvdLogStream::AsyncWriteContextShim::AsyncWriteContextShim(
    WriteStuckRvdLogStream& LogStreamShim
    ) :
   _LogStreamShim(&LogStreamShim)
{
}

WriteStuckRvdLogStream::AsyncWriteContextShim::~AsyncWriteContextShim()
{
}


WaitEventInterceptor<OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext>::SPtr WriteStuckInterceptor;


VOID WriteStuckConditionsTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // The test scenario is as follows:
    //
    //   * Coalsce buffer fill with data and the periodic flush starts
    //   * While periodic flush is outstanding, a write occurs but the
    //     shared log write fails while the dedicated log portion fills
    //     the next coalesce buffer
    //   * The periodic flush takes longer than the periodic timer that
    //     was reset by the write associated with the failed shared log
    //     write
    //   * The periodic timer fires and this should cause another
    //     periodic flush but since the first periodic flush is not
    //     completed, it is ignored.
    //
    //   Bug is that when the first periodic flush completes, it does
    //   not determine if there is another periodic flush that should
    //   occur.
    //



    //
    // Replace the shared log stream with shim
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();

    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);

    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;

    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();

    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);

    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);

    //
    // Hook the interceptor on the periodic flush async
    //
    KAsyncEvent holdPeriodicFlushEvent(TRUE, FALSE);      // ManualReset, not signalled
    KAsyncEvent startedPeriodicFlushEvent(FALSE, FALSE);  // AutoReset, not signalled
    status = WaitEventInterceptor<OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext>::Create(KTL_TAG_TEST,
                                                                     *g_Allocator,
                                                                     NULL,
                                                                     NULL,
                                                                     &startedPeriodicFlushEvent,
                                                                     NULL,
                                                                     WriteStuckInterceptor);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    OverlayStream::CoalesceRecords::AsyncAppendOrFlushContext::SPtr periodicFlushContext = overlayStream->GetCoalesceRecords()->GetPeriodicFlushContext();
    WriteStuckInterceptor->EnableIntercept(*periodicFlushContext);


    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;

    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();

    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;

    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);

    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;


    //
    // Configure the dedicated write ot delay 2 min
    //
    WriteStuckRvdLogStream::DelayOnStartStruct delayOnStartStruct;
    delayOnStartStruct.DelayInMs = 2 * 60 * 1000;
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::DelayOnStart,
                                &delayOnStartStruct);

    //
    // Second write should get placed in coalesce buffer
    //
    version++;
    dataSizeWritten = 40;
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;

    //
    // Next wait for the periodic flush to start
    //
    KAsyncEvent::WaitContext::SPtr waitStartedPeriodicFlushEvent;

    status = startedPeriodicFlushEvent.CreateWaitContext(KTL_TAG_TEST,
                                                          *g_Allocator,
                                                          waitStartedPeriodicFlushEvent);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    waitStartedPeriodicFlushEvent->StartWaitUntilSet(NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Give periodic flush a chance to empty the coalesce buffer
    //
    KNt::Sleep(50);

    //
    // reset dedicated stream back to no delay for second periodic
    // flush
    //
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);

    //
    // Next set the shared log stream shim to fail the next write
    //
    WriteStuckRvdLogStream::FailWithNTStatusStruct failWithNTStatusStruct;
    failWithNTStatusStruct.FailStatus = STATUS_INSUFFICIENT_RESOURCES;
    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::FailWithNTStatus,
                                &failWithNTStatusStruct);

    //
    // Write a little something so the shared log can fail it and the
    // coalesce buffers get something
    //
    version++;
    dataSizeWritten = 90;
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);

    asn += dataSizeWritten;

    //
    // Wait 2 minutes for the second periodic timer to fire. It should
    // fire while the first is still outstanding
    //
    printf("\nWait for 2min\n\n");

    KNt::Sleep(2 * 60 * 1000);

    printf("\nWait for 2min Completed\n\n");

    //
    // Finally wait up to 2 minutes for the second write to complete. We expect that the
    // first periodic flush will kick off a second
    //
    status = sync.WaitForCompletion(2 * 60 * 1000);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Cleanup interceptor
    //
    periodicFlushContext = nullptr;
    WriteStuckInterceptor->DisableIntercept();
    WriteStuckInterceptor = nullptr;


    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    
    dedicatedStream = nullptr;
    sharedStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;
    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    writeContext = nullptr;

    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}

VOID VerifySharedLogUsageThrottlingTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logStreamGuid1;
    KGuid logContainerGuid;
    KSynchronizer sync, sync1;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x40 * 0x1000; // 256k
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    
    logStreamGuid.CreateNew();
    logStreamGuid1.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize,
                              nullptr,                   // Alias
                              512 * 1024 * 1024,         // Stream Size
                              32 * 1024 * 1024);         // Shared Log Size

    AddNewKtlLogStream(diskId,
                       logContainerGuid,
                       logStreamGuid1,
                       streamSize);

    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStreamId logStreamId1 = logStreamGuid1;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    OverlayStream::SPtr overlayStream1;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream1 = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId1,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
                                                                     streamSize,
                                                                     metadataLength,
                                                                     *throttledAllocator);

    VERIFY_IS_TRUE(overlayStream ? TRUE : FALSE);
    status = overlayStream->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayStream1->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    //
    // The test scenario is as follows:
    //
    //    * Write a little to the stream
    //    * Block writes to the dedicated log so a backlog builds in
    //      the shared log. Eventually the shared log should start
    //      throttling writes.
    //    * Verify that new writes are getting throttled
    //    * Perform write to a separate and quiecent log
    //    * Allow dedicated log writes to complete and verify that
    //      writes for both logs get unthrottled.
    //

    //
    // Replace the shared log stream with shim
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();

    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);

    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;

    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();

    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);

    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);

    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONGLONG asn1 = KtlLogAsn::Min().Get();
    ULONGLONG version1 = 0;

    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();

    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;

    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext1;
    CreateAsyncWriteContextOverlay(*overlayStream1, writeContext1);

    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;


    KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
    struct WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
    waitForEventStruct.WaitEvent = &waitEvent;
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                               &waitForEventStruct);  
    
    for (ULONG i = 0; i < 4; i++)
    {   
        //
        // Configure dedicated writes to block
        //
        waitEvent.ResetEvent();

        //
        // Write stuff up until we expect to start throttling
        //
        BOOLEAN writing = TRUE;
        ULONG iterations = 0;
        while(writing)
        {
            iterations++;
            version++;
            dataSizeWritten = 40;
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);

            status = sync.WaitForCompletion(1000);
            if (status == STATUS_IO_TIMEOUT)
            {
                status = sync.WaitForCompletion(30 * 1000);
                if (status == STATUS_IO_TIMEOUT)
                {
                    printf("Write throttled ASN 0x%llx Version %lld Iteration %d\n", asn, version, iterations);
                    break;
                } else {
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }
            } else {
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            asn += dataSizeWritten;
        }

        //
        // Write to other OverlayStream and verify that it is also
        // throttled
        //
        version1++;
        dataSizeWritten = 40;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId1, version1, asn1,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KDbgPrintf("Write1 asn %llx, version %d\n", asn1, version1);
        writeContext1->Reuse();
        writeContext1->StartWrite(asn1, version1, metadataKBuffer, dataIoBuffer, NULL, sync1);

        status = sync1.WaitForCompletion(1000);
        if (status == STATUS_IO_TIMEOUT)
        {
            status = sync1.WaitForCompletion(30 * 1000);
            if (status == STATUS_IO_TIMEOUT)
            {
                printf("Other Write throttled\n");
            } else {
                VERIFY_IS_TRUE(FALSE);
            }
        } else {
            VERIFY_IS_TRUE(FALSE);
        }
        asn1 += dataSizeWritten;

        //
        // Allow dedicated log writes to continue
        //
        printf("Allow dedicated Writes\n");
        waitEvent.SetEvent();

        //
        // Verify that writes gets unthrottled
        //
        status = sync.WaitForCompletion(30 * 1000);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        printf("Original write released\n");
        
        status = sync1.WaitForCompletion(30 * 1000);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        printf("Initial write released\n");

        //
        // Make sure we can still write
        //
        version++;
        dataSizeWritten = 40;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;
    }

    //
    // reset dedicated stream back to no delay 
    //
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);
    

    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    
    dedicatedStream = nullptr;
    sharedStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;
    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    writeContext = nullptr;

    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    writeContext1 = nullptr;

    status = overlayStream1->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream1 = nullptr;
    
    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}
//***


VOID PeriodicTimerCloseRaceTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // The test scenario is as follows:
    //
    //    When closing an OverlayStream the PeriodicTimer may still be
    //    active after the close completed. In this case the
    //    OverlayStream and CoalesceRecords structures are freed and
    //    since the PeriodicTimerCompletion accesses these then there
    //    is a crash. The window for this race is between the time that
    //    periodic time checks if the OverlayStream is closing and it
    //    finishing processing. If the close sneaks in between this
    //    then there is a crash.
    //
    //    * Configure the periodic flush time to be 15 seconds and the
    //      PeriodicTimerCompletion to wait 60 seconds so the close can
    //      race ahead of it.
    //    * Write some data to cause the periodic timer to activate
    //    * Wait for the periodic flush to occur
    //    * Close the OverlayStream
    //    * Close will complete while the PeriodicTimerCompletion is
    //      waiting. Once the PeriodicTimerCompletion continues, the
    //      close has cleaned up the objects that it needs.
    //
    overlayStream->SetPeriodicFlushTimeInSec(15);
    overlayStream->SetPeriodicTimerTestDelayInSec(60);

    //
    // Write a whole bunch of stuff to cause the PeriodicTimer to be started
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;

    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();

    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;

    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);

    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;

    //
    // Second write should get placed in coalesce buffer
    //
    version++;
    dataSizeWritten = 40;
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;


    //
    // Wait for flush to occur
    //
    KNt::Sleep(25 * 1000);

    //
    // Close the overlay stream
    //
    writeContext = nullptr;

    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    status = sync.WaitForCompletion();

    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    printf("CloseCompleted\n");
    KNt::Sleep(75 * 1000);

    //
    // Final Cleanup
    //
    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);
}


VOID VerifyOverlayRecordReadByPointer(
    __in RvdLogAsn ReadAsn,
    __in RvdLogAsn ExpectedAsn,
    __in ULONGLONG ReadVersion,
    __in ULONGLONG ExpectedVersion,
    __in RvdLogStreamId& ExpectedStreamId,
    __in PUCHAR ReadMetadataPtr,
    __in KIoBuffer& ReadIoData,
    __in ULONG reservedMetadataSize,
    __in ULONG coreLoggerOffset,
    __in ULONG expectedLength = 0,
    __in BYTE entropy = 1
    )
{
    NTSTATUS status;
    
    
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
    ULONG dataSize;
    ULONG dataOffset;
    KIoBuffer::SPtr streamIoBuffer;

    status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(ReadMetadataPtr,
                                                                               coreLoggerOffset,
                                                                               ReadIoData,
                                                                               reservedMetadataSize,
                                                                               metadataBlockHeader,
                                                                               streamBlockHeader,
                                                                               dataSize,
                                                                               dataOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    VERIFY_IS_TRUE((ExpectedAsn.Get() >= ReadAsn.Get()) && (ExpectedAsn.Get() < (ReadAsn.Get() + dataSize)) ? true : false);
    VERIFY_IS_TRUE((ExpectedVersion == 0) || (ReadVersion == ExpectedVersion));

    VERIFY_IS_TRUE((streamBlockHeader->Signature == KLogicalLogInformation::StreamBlockHeader::Sig) ? true : false);

    VERIFY_IS_TRUE((ExpectedAsn.Get() >= streamBlockHeader->StreamOffset) && (ExpectedAsn.Get() < (streamBlockHeader->StreamOffset + dataSize)) ? true : false);
#if DBG
    ULONGLONG crc64;

    //  crc64 = KChecksum::Crc64(dataRead, dataSizeRead, 0);
    //    VERIFY_IS_TRUE(crc64 == streamBlockHeader->DataCRC64);

    ULONGLONG headerCrc64 = streamBlockHeader->HeaderCRC64;
    streamBlockHeader->HeaderCRC64 = 0;
    crc64 = KChecksum::Crc64(streamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader), 0);
    streamBlockHeader->HeaderCRC64 = headerCrc64;
    VERIFY_IS_TRUE((crc64 == headerCrc64) ? true : false);
#endif
    VERIFY_IS_TRUE(((ExpectedVersion == 0) || (ExpectedVersion == streamBlockHeader->HighestOperationId)) ? true : false);

    //
    // Build KIoBufferStream that spans the metadata and the data
    //
    KIoBuffer::SPtr metadataIoBuffer;
    PVOID metadataIoBufferPtr;

    status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize, metadataIoBuffer, metadataIoBufferPtr, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KMemCpySafe((PVOID)((PUCHAR)metadataIoBufferPtr + coreLoggerOffset),
                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset, 
                ReadMetadataPtr, 
                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);

    status = KIoBuffer::CreateEmpty(streamIoBuffer,
                                    *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = streamIoBuffer->AddIoBufferReference(*metadataIoBuffer,
                                                  0,
                                                  metadataIoBuffer->QuerySize());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = streamIoBuffer->AddIoBufferReference(ReadIoData,
                                                  0,
                                                  ReadIoData.QuerySize());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    VERIFY_IS_TRUE( (expectedLength == 0) || (expectedLength == dataSize) );
    
    KIoBufferStream stream(*streamIoBuffer,
                            dataOffset);

    for (ULONG i = 0; i < dataSize; i++)
    {
        UCHAR expectedByte = (entropy != 1) ? StreamOffsetToExpectedData(entropy) :
                                               StreamOffsetToExpectedData(ReadAsn.Get() + i);
        UCHAR byte;
        BOOLEAN b = stream.Pull(byte);
        if (!b) VERIFY_IS_TRUE(b ? true : false);
        if (byte != expectedByte) VERIFY_IS_TRUE((byte == expectedByte) ? true : false);
    }

    VERIFY_IS_TRUE((ExpectedStreamId.Get() == streamBlockHeader->StreamId.Get()) ? true : false);   
}

VOID VerifyOverlayRecordRead(
    __in RvdLogAsn ReadAsn,
    __in RvdLogAsn ExpectedAsn,
    __in ULONGLONG ReadVersion,
    __in ULONGLONG ExpectedVersion,
    __in RvdLogStreamId& ExpectedStreamId,
    __in KBuffer& ReadMetadata,
    __in KIoBuffer& ReadIoData,
    __in ULONG reservedMetadataSize,
    __in ULONG coreLoggerOffset,
    __in ULONG expectedLength = 0,
    __in BYTE entropy = 1
    )
{
    VerifyOverlayRecordReadByPointer(ReadAsn,
                                     ExpectedAsn,
                                     ReadVersion,
                                     ExpectedVersion,
                                     ExpectedStreamId,
                                     (PUCHAR)ReadMetadata.GetBuffer(),
                                     ReadIoData,
                                     reservedMetadataSize,
                                     coreLoggerOffset,
                                     expectedLength,
                                     entropy);
}

VOID ReadFromCoalesceBuffersTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    srand((ULONG)GetTickCount());
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Replace the shared log stream with shim that is a pass through
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);  
    
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim that holds writes waiting on
    // an event
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;

    KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
    struct WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
    waitForEventStruct.WaitEvent = &waitEvent;
        
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);


    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    OverlayStream::AsyncReadContext::SPtr readContext1;
    OverlayStream::AsyncReadContextOverlay::SPtr readContext;
    status = overlayStream->CreateAsyncReadContext(readContext1);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
    readContext1 = nullptr;
    
    RvdLogAsn readAsn;
    RvdLogAsn actualReadAsn;
    ULONGLONG actualReadVersion;
    KBuffer::SPtr readMetadata;
    KIoBuffer::SPtr readIoBuffer;
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader;
    ULONGLONG baseAsn;
    ULONGLONG baseVersion;
    BOOLEAN readFromDedicated;
    
    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    for (ULONG i = 0; i < dataSizeWritten; i++)
    {
        dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
    }
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            asn,
                                            version,
                                            0x1000,
                                            NULL,
                                            dataIoBuffer);                                  
    
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;

    //
    // Now configure dedicated log to hold all writes until the event
    // is set
    //
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                                   &waitForEventStruct);  
    
    
    //
    // Test 1: Write a bunch of stuff that sits in the current
    //         FlushRecordsContext and attempt to read from it and
    //         verify the contents.
    //
    {
        version++;
        dataSizeWritten = 400;
        for (ULONG i = 0; i < dataSizeWritten; i++)
        {
            dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
        }
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        //
        // now read from the dedicated log
        //
        readContext->Reuse();
        readAsn = 10;
        readContext->StartRead(readAsn,
                               RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                               &actualReadAsn,
                               &actualReadVersion,
                               readMetadata,
                               readIoBuffer,
                               &readFromDedicated,
                               NULL,
                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(readFromDedicated ? true : false);
        VerifyOverlayRecordRead(actualReadAsn, 1, actualReadVersion, 1, logStreamId, *readMetadata, *readIoBuffer,
                                reservedMetadataSize,
                                coreLoggerOffset);


        //
        // Try an out of range read and verify that the correct error code
        readContext->Reuse();
        readAsn = 500;
        readContext->StartRead(readAsn,
                               RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                               &actualReadAsn,
                               &actualReadVersion,
                               readMetadata,
                               readIoBuffer,
                               &readFromDedicated,
                               NULL,
                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);


        //
        // now read from the currently coalescing buffer
        //
        readContext->Reuse();
        readAsn = 300;
        readContext->StartRead(readAsn,
                               RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                               &actualReadAsn,
                               &actualReadVersion,
                               readMetadata,
                               readIoBuffer,
                               &readFromDedicated,
                               NULL,
                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(! readFromDedicated);
        VerifyOverlayRecordRead(actualReadAsn, 41, actualReadVersion, 2, logStreamId, *readMetadata, *readIoBuffer,
                                reservedMetadataSize,
                                coreLoggerOffset);
    }



    //
    // Test 2: After writing to cause a number of (16MB)
    //         FlushingRecordsContexts to be pending, read from them
    //         plus the current FlushingRecordsContext and verify the contents.
    //
    {
        baseAsn = asn;
        baseVersion = version + 1;

        for (ULONG i = 0; i < 256; i++)
        {
            version++;
            dataSizeWritten = 0x40000;
            for (ULONG i1 = 0; i1 < dataSizeWritten; i1++)
            {
                dataBufferPtr[i1] = StreamOffsetToExpectedData(asn + i1);
            }
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }

        for (ULONG i = 0; i < 256; i++)
        {
            readContext->Reuse();
            readAsn = baseAsn + (i * dataSizeWritten) + (rand() % (dataSizeWritten - 1));
            readContext->StartRead(readAsn,
                RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                &actualReadAsn,
                &actualReadVersion,
                readMetadata,
                readIoBuffer,
                &readFromDedicated,
                NULL,
                sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(!readFromDedicated);
            VerifyOverlayRecordRead(actualReadAsn, readAsn,
                actualReadVersion, 0,
                logStreamId,
                *readMetadata, *readIoBuffer,
                reservedMetadataSize,
                coreLoggerOffset);

        }
    }

    //
    // Test 3: With a lot of data in the coalesced buffers, perform a
    //         MultiRecordRead to verify that the MultiRecordRead can
    //         handle data that comes form the coalesce buffers. In the
    //         past the MultiRecordRead only supported read buffers
    //         where the KIoBuffer had a single element. There are 2
    //         cases: one where the MultiRecordRead takes the fast path
    //         (single record read) and one where it takes the slow
    //         path (copies from records to output)
    //
    {
        KIoBuffer::SPtr multiReadIoMetadata;
        KIoBuffer::SPtr multiReadIoData;
        PVOID p;

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize, multiReadIoMetadata, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        OverlayStream::AsyncMultiRecordReadContextOverlay::SPtr multiRead;
        status = overlayStream->CreateAsyncMultiRecordReadContextOverlay(multiRead);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // First read with a smaller data buffer to force the code path where multirecord
        // read rebuilds the record.
        //
        status = KIoBuffer::CreateSimple(384 * 1024, multiReadIoData, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        readAsn = baseAsn;
        multiRead->StartRead(readAsn, *multiReadIoMetadata, *multiReadIoData, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VerifyOverlayRecordReadByPointer(readAsn, readAsn, 0, 0, logStreamId, 
                                         (PUCHAR)multiReadIoMetadata->First()->GetBuffer(), *multiReadIoData,
                                         reservedMetadataSize + coreLoggerOffset,
                                         0);

        //
        // Next read with a larger data buffer to force the code path where multirecord
        // read does not rebuild the record.
        //
        multiReadIoData = nullptr;
        status = KIoBuffer::CreateSimple(1024 * 1024, multiReadIoData, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        multiRead->Reuse();
        multiRead->StartRead(readAsn, *multiReadIoMetadata, *multiReadIoData, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        readAsn = 41;
        VerifyOverlayRecordReadByPointer(readAsn, readAsn, 0, 0, logStreamId, 
                                         (PUCHAR)multiReadIoMetadata->First()->GetBuffer(), *multiReadIoData,
                                         reservedMetadataSize + coreLoggerOffset,
                                         0);
    }


    
    //
    // Test 4: Let the FlushingRecordsContexts complete writing to the
    //         dedicated log. Read from them plus the current
    //         FlushingRecordsContext and verify the contents.
    {
        //
        // Let all of the dedicated writes complete
        //
        waitEvent.SetEvent();
        KNt::Sleep(60 * 1000);
        
        //
        // Reread some stuff from the dedicated log
        //
        for (ULONG i = 0; i < 256; i++)
        {
            readContext->Reuse();
            readAsn = baseAsn + (i * dataSizeWritten) + (rand() % (dataSizeWritten-1));
            readContext->StartRead(readAsn,
                                   RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                                   &actualReadAsn,
                                   &actualReadVersion,
                                   readMetadata,
                                   readIoBuffer,
                                   &readFromDedicated,
                                   NULL,
                                   sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(readFromDedicated ? true : false);
            VerifyOverlayRecordRead(actualReadAsn, readAsn,
                                    actualReadVersion, 0,
                                    logStreamId,
                                    *readMetadata, *readIoBuffer,
                                    reservedMetadataSize,
                                    coreLoggerOffset);
        }       
        

        readContext->Reuse();
        readAsn = 10;
        readContext->StartRead(readAsn,
                               RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                               &actualReadAsn,
                               &actualReadVersion,
                               readMetadata,
                               readIoBuffer,
                               &readFromDedicated,
                               NULL,
                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(readFromDedicated ? true : false);
        VerifyOverlayRecordRead(actualReadAsn, 1, actualReadVersion, 1, logStreamId, *readMetadata, *readIoBuffer,
                                reservedMetadataSize,
                                coreLoggerOffset);                              


        //
        // Try an out of range read and verify that the correct error code
        readContext->Reuse();
        readAsn = 500;
        readContext->StartRead(readAsn,
                               RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                               &actualReadAsn,
                               &actualReadVersion,
                               readMetadata,
                               readIoBuffer,
                               &readFromDedicated,
                               NULL,
                               sync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(readFromDedicated ? true : false);
        VerifyOverlayRecordRead(actualReadAsn, 41, actualReadVersion, 0, logStreamId, *readMetadata, *readIoBuffer,
                                reservedMetadataSize,
                                coreLoggerOffset);
                                
        readContext->Reuse();
        readAsn = 300;
        readContext->StartRead(readAsn,
                               RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                               &actualReadAsn,
                               &actualReadVersion,
                               readMetadata,
                               readIoBuffer,
                               &readFromDedicated,
                               NULL,
                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(readFromDedicated ? true : false);
        VerifyOverlayRecordRead(actualReadAsn, 41, actualReadVersion, 0, logStreamId, *readMetadata, *readIoBuffer,
                                reservedMetadataSize,
                                coreLoggerOffset);
    }
        
    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    
    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    dedicatedStream = nullptr;
    sharedStream = nullptr;
    
    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    writeContext = nullptr;
    readContext = nullptr;
    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}

VOID WriteThrottleUnitTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    srand((ULONG)GetTickCount());
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize,
                              nullptr,
                              (LONGLONG)1024 * 1024 * 1024);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Replace the shared log stream with shim that is a pass through
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);  
    
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim that holds writes waiting on
    // an event
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;

    KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
    struct WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
    waitForEventStruct.WaitEvent = &waitEvent;
        
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);


    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    for (ULONG i = 0; i < dataSizeWritten; i++)
    {
        dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
    }
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            asn,
                                            version,
                                            0x1000,
                                            NULL,
                                            dataIoBuffer);                                  
    
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;


    OverlayStream::AsyncIoctlContextOverlay::SPtr ioctlContext;
    CreateAsyncIoctlContextOverlay(*overlayStream, ioctlContext);
    ULONG result;
    KLogicalLogInformation::WriteThrottleThreshold* writeThrottleThreshold;
    KBuffer::SPtr outBuffer;
    KBuffer::SPtr inBuffer;

    status = KBuffer::Create(sizeof(KLogicalLogInformation::WriteThrottleThreshold), inBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Now configure dedicated log to hold all writes until the event
    // is set
    //
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                                   &waitForEventStruct);

    
    //
    // Test 1: Set write throttle threshold to have no limit and write
    //         32MB of data and verify that writes do not get held up
    //
    {
        //
        // Configure stream to have no limit on write threshold
        //
        VERIFY_IS_TRUE(overlayStream->GetDedicatedWriteBytesOutstandingThreshold() == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
        
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeThrottleThreshold->MaximumOutstandingBytes = KLogicalLogInformation::WriteThrottleThresholdNoLimit;
        
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(outBuffer->QuerySize() == sizeof(KLogicalLogInformation::WriteThrottleThreshold));
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)outBuffer->GetBuffer();
        VERIFY_IS_TRUE(writeThrottleThreshold->MaximumOutstandingBytes == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
        outBuffer = nullptr;

        VERIFY_IS_TRUE(overlayStream->GetDedicatedWriteBytesOutstandingThreshold() == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
        
        //
        // Write a 32 MB and verify that none get held
        //
        for (ULONG i = 0; i < 64; i++)
        {
            version++;
            dataSizeWritten = 512*1024;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }       
    }


    //
    // Test 2: Set write throttle threshold to be 34MB and write up to
    //         that point. Verify that writes are not held up. Write
    //         beyond threshold limit and verify that writes are held
    //         up.
    //
    {
        //
        // Configure stream to have no limit on write threshold
        //
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeThrottleThreshold->MaximumOutstandingBytes = 34 * (1024 * 1024);
        
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(outBuffer->QuerySize() == sizeof(KLogicalLogInformation::WriteThrottleThreshold));
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)outBuffer->GetBuffer();
        VERIFY_IS_TRUE(writeThrottleThreshold->MaximumOutstandingBytes == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
        outBuffer = nullptr;
        
        VERIFY_IS_TRUE(overlayStream->GetDedicatedWriteBytesOutstandingThreshold() == 34 * (1024 * 1024));
        
        //
        // Write a 2 MB and verify that none get held
        //
        for (ULONG i = 0; i < 4; i++)
        {
            version++;
            dataSizeWritten = 512*1024;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }

        //
        // Now write beyond the threshold limit and verify write is
        // held.
        // NOTE: this is limited to just a single IO since the logical log assumes
        //       there will only be one IO outstanding at one time.
        //
        const ULONG NumberWaitingIO = 1;
        OverlayStream::AsyncWriteContextOverlay::SPtr writeContexts[NumberWaitingIO];
        KSynchronizer writeSyncs[NumberWaitingIO];

        
        for (ULONG i = 0; i < NumberWaitingIO; i++)
        {
            CreateAsyncWriteContextOverlay(*overlayStream, writeContexts[i]);
    
            version++;
            dataSizeWritten = 512*1024;

            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContexts[i]->Reuse();
            
            writeContexts[i]->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, writeSyncs[i]);
            status = writeSyncs[i].WaitForCompletion(15 * 1000);
            VERIFY_IS_TRUE((status == STATUS_IO_TIMEOUT) ? true : false);

            asn += dataSizeWritten;
        }
        
        //
        // Allow dedicated IO to progress and verify that throttled IO
        // gets written and we can keep writing without delay
        //
        waitEvent.SetEvent();

        for (ULONG i = 0; i < NumberWaitingIO; i++)
        {
            status = writeSyncs[i].WaitForCompletion(300 * 1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < 4; i++)
        {
            version++;
            dataSizeWritten = 512*1024;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }
        
        for (ULONG i = 0; i < 300; i++)
        {
            KNt::Sleep(1000);
            LONGLONG outstanding = overlayStream->GetDedicatedWriteBytesOutstanding();
            if (outstanding == 0)
            {
                break;
            }
        }
    }
    


    //
    // Test 3: Stress case where we keep writing and expect some
    //         throttling/unthrottling but that all get completed.
    //         Build up a backlog of 512MB and then let it go while at
    //         the same time doing a lot of writing.
    //
    {
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeThrottleThreshold->MaximumOutstandingBytes = 512 * (1024 * 1024);
        
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(outBuffer->QuerySize() == sizeof(KLogicalLogInformation::WriteThrottleThreshold));
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)outBuffer->GetBuffer();
        VERIFY_IS_TRUE(writeThrottleThreshold->MaximumOutstandingBytes == 34 * (1024 * 1024));
        outBuffer = nullptr;

        VERIFY_IS_TRUE(overlayStream->GetDedicatedWriteBytesOutstandingThreshold() == 512 * (1024 * 1024));

        //
        // Stop dedicated log from writing to build backlog
        //
        waitEvent.ResetEvent();

        
        for (ULONG i = 0; i < 1024; i++)
        {
            version++;
            dataSizeWritten = 512*1024;

            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }

        //
        // Let the backlog drain while at the same time writing
        //
        waitEvent.SetEvent();

        
        for (ULONG i = 0; i < 4; i++)
        {
            for (ULONG j = 0; j < 256; j++)
            {
                version++;
                dataSizeWritten = 512*1024;
            
                status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                            dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                writeContext->Reuse();
                writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;
            }

            //
            // Truncate to make space
            //
            ULONGLONG truncateAsn = asn - (8 * (512 * 1024));
            overlayStream->Truncate(truncateAsn,
                                    truncateAsn);
        }
    }

    //
    // Test 4: Set throttle limit failure conditions tests
    //
    {
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeThrottleThreshold->MaximumOutstandingBytes = (1024 * 1024);
        
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE((status == STATUS_INVALID_PARAMETER) ? true : false);
        
        VERIFY_IS_TRUE(overlayStream->GetDedicatedWriteBytesOutstandingThreshold() == 512 * (1024 * 1024));
    }
        
    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    status = sync.WaitForCompletion();

     
    for (ULONG i = 0; i < 300; i++)
    {
        KNt::Sleep(1000);
        LONGLONG outstanding = overlayStream->GetDedicatedWriteBytesOutstanding();
        if (outstanding == 0)
        {
            break;
        }
    }
    
    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    dedicatedStream = nullptr;
    sharedStream = nullptr;
    
    writeContext = nullptr;
    ioctlContext = nullptr;
    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}

VOID VerifySharedTruncateOnDedicatedFailureTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    srand((ULONG)GetTickCount());
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize,
                              nullptr,
                              (LONGLONG)1024 * 1024 * 1024);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                          &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Replace the shared log stream with shim that is a pass through
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);  
    
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim that fails all writes
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;
        
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);


    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    ULONGLONG expectedHighAsn, expectedLowAsn, expectedTruncationAsn;

    expectedTruncationAsn = 1;
    
    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    for (ULONG i = 0; i < dataSizeWritten; i++)
    {
        dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
    }
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            asn,
                                            version,
                                            0x1000,
                                            NULL,
                                            dataIoBuffer);                                  
    
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;


    //
    // Test scenario where earlier dedicated writes will fail but
    // subsequent writes will succeed. In this case we found that the
    // shared log was being truncated by the lower writes that fail
    // even though their data did not make it to the dedicated log.
    // This test verifies that the shared log will not get truncated if
    // the dedicated log data is not written.
    //

    //
    // Now configure first set of dedicated log to fail all writes
    // after subsequent writes succeed.
    //
    KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
    WriteStuckRvdLogStream::WaitForEventAndFailStruct waitForEventAndFail;
    waitForEventAndFail.WaitEvent = &waitEvent;
    waitForEventAndFail.FailStatus = STATUS_UNSUCCESSFUL;
    waitForEventAndFail.FailedCount = 0;
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEventAndFail,
                                   &waitForEventAndFail);

    ULONGLONG asnHeldData = asn;
    {
        //
        // Write a 32 MB 
        //
        for (ULONG i = 0; i < 64; i++)
        {
            version++;
            dataSizeWritten = 512*1024;

            expectedHighAsn = asn;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }       
    }


    //
    // Second set of writes go to dedicated log successfully
    //
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   &waitForEventAndFail);

    {
        //
        // Write a 32 MB
        //
        for (ULONG i = 0; i < 64; i++)
        {
            version++;
            dataSizeWritten = 512*1024;

            expectedHighAsn = asn;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            writeContext->Reuse();
            
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }       
    }

    //
    // Let the first set of dedicated writes complete with failure
    //
    waitEvent.SetEvent();

    //
    // Wait for the first set of dedicated write failures to complete
    // before doing any new writes
    //
    ULONG counter = 0;
    while ((counter < 15) && (waitForEventAndFail.FailedCount == 0))
    {
        KNt::Sleep(1000);
        counter++;
    }
    VERIFY_IS_TRUE(waitForEventAndFail.FailedCount > 0);

    //
    // Future writes should fail
    //
    version++;
    dataSizeWritten = 512*1024;

    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            asn,
                                            version,
                                            0x1000,
                                            NULL,
                                            dataIoBuffer);                                  
    writeContext->Reuse();

    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_UNSUCCESSFUL);
    version--;

    
    //
    // Allow time for flush
    //
    KNt::Sleep(15 * 1000);

    
    expectedLowAsn = 41;
    expectedTruncationAsn = 1;
    RvdLogAsn lowAsn, highAsn, truncationAsn;
    status = sharedStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    VERIFY_IS_TRUE(lowAsn.Get() <= expectedLowAsn);
    VERIFY_IS_TRUE(highAsn.Get() == expectedHighAsn);
    VERIFY_IS_TRUE(truncationAsn.Get() <= expectedTruncationAsn);

    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    status = sync.WaitForCompletion();

    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    dedicatedStream = nullptr;
    sharedStream = nullptr;

    writeContext = nullptr;

    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    //
    // Reopen and read the records that had failed write above
    //
    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));     

    OverlayStream::AsyncReadContext::SPtr readContext1;
    OverlayStream::AsyncReadContextOverlay::SPtr readContext;
    RvdLogAsn readAsn;
    RvdLogAsn actualReadAsn;
    ULONGLONG actualReadVersion;
    KBuffer::SPtr readMetadata;
    KIoBuffer::SPtr readIoBuffer;
    BOOLEAN readFromDedicated;
    
    status = overlayStream->CreateAsyncReadContext(readContext1);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
    readContext1 = nullptr;

    //
    // now read from the dedicated log
    //
    readContext->Reuse();
    readAsn = asnHeldData;
    readContext->StartRead(readAsn,
                           RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                           &actualReadAsn,
                           &actualReadVersion,
                           readMetadata,
                           readIoBuffer,
                           &readFromDedicated,
                           NULL,
                           sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
        
    readContext = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;    
        
    //
    // Final Cleanup
    //
    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
        
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}

//
VOID VerifyCopyFromSharedToBackupTest(
    UCHAR driveLetter,
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    RvdLogStreamId logStreamId;
    RvdLogId logContainerId;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    srand((ULONG)GetTickCount());

    KString::SPtr containerFullPathName;
    KString::SPtr containerFullPathNameBackup;
    KString::SPtr streamFullPathName;
    ContainerCloseSynchronizer closeContainerSync;
    StreamCloseSynchronizer closeStreamSync;        

    CreateStreamAndContainerPathnames(driveLetter,
                                      containerFullPathName,
                                      logContainerId,
                                      streamFullPathName,
                                      logStreamId);

    logContainerGuid = logContainerId.Get();
    logStreamGuid = logStreamId.Get();

    containerFullPathNameBackup = streamFullPathName->Clone();
    VERIFY_IS_TRUE(containerFullPathNameBackup ? TRUE : FALSE);
    

    BOOLEAN b;
    KStringView tempString(L".Backup");
    b = containerFullPathNameBackup->Concat(tempString);
    VERIFY_IS_TRUE(b ? TRUE : FALSE);
    
    //
    // Setup the test by create a log container and a stream in it
    //
    KtlLogManager::SPtr logManager;
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

    //
    // Create a log container
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = (LONGLONG)((LONGLONG)2 * (LONGLONG)1024 * (LONGLONG)0x100000);   // 2GB
    KtlLogContainer::SPtr logContainer;

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(*containerFullPathName,
                                         logContainerId,
                                         logSize,
                                         0,            // Max Number Streams
                                         maxContainerRecordSize,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogStream::SPtr logStream;
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KString::CSPtr streamFullPathNameConst = streamFullPathName.RawPtr();   
    KBuffer::SPtr securityDescriptor = nullptr;
    createStreamAsync->StartCreateLogStream(logStreamId,
                                            KLogicalLogInformation::GetLogicalLogStreamType(),
                                            nullptr,           // Alias
                                            streamFullPathNameConst,
                                            securityDescriptor,
                                            metadataLength,
                                            (LONGLONG)1024 * 1024 * 1024,
                                            maxStreamRecordSize,
                                            logStream,
                                            NULL,    // ParentAsync
                                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream->StartClose(NULL,
                     closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS);
    logStream = nullptr;

    createStreamAsync = nullptr;
    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS);
    logContainer = nullptr;
    createContainerAsync = nullptr;

    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager = nullptr;

    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));     
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                          &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(KStringView(*containerFullPathName),
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               containerFullPathName.RawPtr(),        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     streamFullPathName.RawPtr(),
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Replace the shared log stream with shim that is a pass through
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);  
    
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim that fails all writes
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;
        
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);


    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    ULONGLONG expectedHighAsn, expectedLowAsn, expectedTruncationAsn;

    expectedTruncationAsn = 1;
    
    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    for (ULONG i = 0; i < dataSizeWritten; i++)
    {
        dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
    }
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            asn,
                                            version,
                                            0x1000,
                                            NULL,
                                            dataIoBuffer);                                  
    
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;

    //
    // Now configure first set of dedicated log to fail all writes
    // after subsequent writes succeed.
    //
    KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
    WriteStuckRvdLogStream::WaitForEventAndFailStruct waitForEventAndFail;
    waitForEventAndFail.WaitEvent = &waitEvent;
    waitForEventAndFail.FailStatus = STATUS_UNSUCCESSFUL;
    waitForEventAndFail.FailedCount = 0;
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEventAndFail,
                                   &waitForEventAndFail);
    
    {
        //
        // Write a 32 MB 
        //
        for (ULONG i = 0; i < 64; i++)
        {
            version++;
            dataSizeWritten = 512*1024;

            expectedHighAsn = asn;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }       
    }

    //
    // Second set of writes go to shared log successfully
    //
    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   &waitForEventAndFail);

    {
        //
        // Write a 32 MB
        //
        for (ULONG i = 0; i < 64; i++)
        {
            version++;
            dataSizeWritten = 512*1024;

            expectedHighAsn = asn;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            writeContext->Reuse();
            
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }       
    }

    //
    // Let the first set of dedicated writes complete with failure
    //
    waitEvent.SetEvent();

    //
    // Allow time for destaging write to complete (fail)
    //
    KNt::Sleep(5 * 1000);
	
    //
    // Future writes should fail
    //
    version++;
    dataSizeWritten = 512*1024;

    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            asn,
                                            version,
                                            0x1000,
                                            NULL,
                                            dataIoBuffer);                                  
    writeContext->Reuse();

    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_UNSUCCESSFUL);
    version--;

    
    //
    // Allow time for flush
    //
    KNt::Sleep(15 * 1000);

    
    expectedLowAsn = 41;
    expectedTruncationAsn = 1;
    RvdLogAsn lowAsn, highAsn, truncationAsn;
    status = sharedStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    VERIFY_IS_TRUE(lowAsn.Get() <= expectedLowAsn);
    VERIFY_IS_TRUE(highAsn.Get() == expectedHighAsn);
    VERIFY_IS_TRUE(truncationAsn.Get() <= expectedTruncationAsn);

    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    status = sync.WaitForCompletion();

    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    dedicatedStream = nullptr;
    sharedStream = nullptr;

    writeContext = nullptr;

    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
        
    
    //
    // Corrupt the log
    //
    {
        KBlockFile::SPtr blockfile;
        KWString pathName(*g_Allocator);
        pathName = *streamFullPathName;
        VERIFY_IS_TRUE(NT_SUCCESS(pathName.Status()));

        status = KBlockFile::Create(pathName,
                                    TRUE,
                                    KBlockFile::eOpenExisting,
                                    blockfile,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KIoBuffer::SPtr ioBuffer;
        PVOID p;
        PUCHAR pc;
        status = KIoBuffer::CreateSimple(0x1000, ioBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        pc = (PUCHAR)p;
        for (ULONG i = 0; i < 0x1000; i++)
        {
            pc[i] = 0xFA;
        }

        status = blockfile->Transfer(KBlockFile::eForeground,
                                     KBlockFile::eWrite,
                                     0,
                                     *ioBuffer,
                                     sync);

        VERIFY_IS_TRUE(NT_SUCCESS(status));     
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        blockfile->Close();
    }
    
    //
    // Reopen corrupted log and verify failure
    //
    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(KStringView(*containerFullPathName),
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               containerFullPathName.RawPtr(),        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);     
    overlayStream = nullptr;    

    //
    // Read from backup log and verify data
    //
    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(KStringView(*containerFullPathNameBackup),
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreBackupLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreBackupLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogAsn lowAsnBackup, highAsnBackup, truncationAsnBackup;
    
    status = coreBackupLogStream->QueryRecordRange(&lowAsnBackup, &highAsnBackup, &truncationAsnBackup);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    VERIFY_IS_TRUE(lowAsnBackup.Get() <= expectedLowAsn);
    VERIFY_IS_TRUE(highAsnBackup.Get() == expectedHighAsn);
    VERIFY_IS_TRUE(truncationAsnBackup.Get() <= expectedTruncationAsn); 
    
    coreBackupLogStream = nullptr;
    coreLog = nullptr;

    
    //
    // Final Cleanup
    //
    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
        
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}
//


WaitEventInterceptor<OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext>::SPtr WaitFlushingRecordsInterceptor;
VOID FlushAllRecordsForCloseWaitTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    srand((ULONG)GetTickCount());
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize,
                              nullptr,
                              (LONGLONG)1024 * 1024 * 1024);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                           &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Replace the shared log stream with shim that succeeds but does
    // not write to the log
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    WriteStuckRvdLogStream::FailWithNTStatusStruct failWithNTStatusStruct;
    failWithNTStatusStruct.FailStatus = STATUS_SUCCESS;
    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::FailWithNTStatus,
                                &failWithNTStatusStruct);   

    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim that holds writes waiting on
    // an event
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;

    KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
    struct WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
    waitForEventStruct.WaitEvent = &waitEvent;
        
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);


    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    for (ULONG i = 0; i < dataSizeWritten; i++)
    {
        dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
    }
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                            asn,
                                            version,
                                            0x1000,
                                            NULL,
                                            dataIoBuffer);                                  
    
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;


    OverlayStream::AsyncIoctlContextOverlay::SPtr ioctlContext;
    CreateAsyncIoctlContextOverlay(*overlayStream, ioctlContext);
    ULONG result;
    KLogicalLogInformation::WriteThrottleThreshold* writeThrottleThreshold;
    KBuffer::SPtr outBuffer;
    KBuffer::SPtr inBuffer;

    status = KBuffer::Create(sizeof(KLogicalLogInformation::WriteThrottleThreshold), inBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Do a small write to get stuff in the coalesce buffers and a
    // AsyncFlushingRecordsContext alloced
    ULONGLONG asnHeldData = asn;    
    {
        version++;
        dataSizeWritten = 512*1024;

        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
        KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                asn,
                                                version,
                                                0x1000,
                                                NULL,
                                                dataIoBuffer);
            
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;
    }

    //
    // Hook interceptor
    //
    KAsyncEvent holdPeriodicFlushEvent(TRUE, FALSE);      // ManualReset, not signalled
    status = WaitEventInterceptor<OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext>::Create(KTL_TAG_TEST,
                                                                     *g_Allocator,
                                                                     &holdPeriodicFlushEvent,
                                                                     NULL,
                                                                     NULL,
                                                                     NULL,
                                                                     WaitFlushingRecordsInterceptor);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    OverlayStream::CoalesceRecords::AsyncFlushingRecordsContext::SPtr flushingRecordsContext = overlayStream->GetCoalesceRecords()->GetFlushingRecordsContext();
    WaitFlushingRecordsInterceptor->EnableIntercept(*flushingRecordsContext);
    

    //
    // Remember asn for data that is going to be held 
    //
    {
        //
        // Configure stream to have no limit on write threshold
        //
        VERIFY_IS_TRUE(overlayStream->GetDedicatedWriteBytesOutstandingThreshold() == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
        
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeThrottleThreshold->MaximumOutstandingBytes = KLogicalLogInformation::WriteThrottleThresholdNoLimit;
        
        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(outBuffer->QuerySize() == sizeof(KLogicalLogInformation::WriteThrottleThreshold));
        writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)outBuffer->GetBuffer();
        VERIFY_IS_TRUE(writeThrottleThreshold->MaximumOutstandingBytes == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
        outBuffer = nullptr;

        VERIFY_IS_TRUE(overlayStream->GetDedicatedWriteBytesOutstandingThreshold() == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
        
        //
        // Write a 32 MB and verify that none get held
        //
        for (ULONG i = 0; i < 64; i++)
        {
            version++;
            dataSizeWritten = 512*1024;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }       
    }


    {
        //
        // Write a 32 MB and verify that none get held
        //
        for (ULONG i = 0; i < 64; i++)
        {
            version++;
            dataSizeWritten = 512*1024;
            
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }       
    }

    //
    // Wait for flush to occur
    //
    KNt::Sleep(5 * 1000);
    
    //
    // Close the overlay stream and expect that all records will be
    // flushed
    //
    writeContext = nullptr;

    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);

    //
    // Release interceptor
    //
    KNt::Sleep(30 * 1000);
    printf("Release the interceptor\n");
    holdPeriodicFlushEvent.SetEvent();
    KNt::Sleep(5 * 1000);
    
    //
    // Hold up completion of the final flush
    //
    status = sync.WaitForCompletion();

    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    dedicatedStream = nullptr;
    sharedStream = nullptr;
    
    writeContext = nullptr;
    ioctlContext = nullptr;
    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Cleanup interceptor
    //
    flushingRecordsContext = nullptr;
    WaitFlushingRecordsInterceptor->DisableIntercept();
    WaitFlushingRecordsInterceptor = nullptr;

    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;
    

    //
    // Reopen the stream and verify that held data is in the log
    //
    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));     

    OverlayStream::AsyncReadContext::SPtr readContext1;
    OverlayStream::AsyncReadContextOverlay::SPtr readContext;
    RvdLogAsn readAsn;
    RvdLogAsn actualReadAsn;
    ULONGLONG actualReadVersion;
    KBuffer::SPtr readMetadata;
    KIoBuffer::SPtr readIoBuffer;
    BOOLEAN readFromDedicated;
    
    status = overlayStream->CreateAsyncReadContext(readContext1);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
    readContext1 = nullptr;

    //
    // now read from the dedicated log
    //
    readContext->Reuse();
    readAsn = asnHeldData;
    readContext->StartRead(readAsn,
                           RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                           &actualReadAsn,
                           &actualReadVersion,
                           readMetadata,
                           readIoBuffer,
                           &readFromDedicated,
                           NULL,
                           sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
        
    readContext = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}
//


class Corner1WriteAsync : public WorkerAsync
{
    K_FORCE_SHARED(Corner1WriteAsync);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out Corner1WriteAsync::SPtr& Context
        )
    {
            NTSTATUS status;
            Corner1WriteAsync::SPtr context;

            context = _new(AllocationTag, Allocator) Corner1WriteAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (!NT_SUCCESS(status))
            {
                return(status);
            }

            context->_OverlayStream = nullptr;
            context->_WriteContext = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);
        }

    struct StartParameters
    {
        OverlayStream::SPtr OverlayStream;
        RvdLogAsn StartAsn;
        ULONGLONG StartVersion;
        ULONG Length;
        RvdLogStreamId LogStreamId;
        ULONG CoreLoggerOffset;
        ULONG ReservedMetadataSize;
    };

    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;
        _OverlayStream = startParameters->OverlayStream;
        _StartAsn = startParameters->StartAsn;
        _StartVersion = startParameters->StartVersion;
        _Length = startParameters->Length;
        _LogStreamId = startParameters->LogStreamId;
        _CoreLoggerOffset = startParameters->CoreLoggerOffset;
        _ReservedMetadataSize = startParameters->ReservedMetadataSize;
        
        Start(ParentAsyncContext, CallbackPtr);
    }

protected:
    VOID FSMContinue(
        __in NTSTATUS Status
        ) override
    {
        if (!NT_SUCCESS(Status))
        {
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            Complete(Status);
            return;
        }

        switch (_State)
        {
            case Initial:
            {
                Status = _OverlayStream->CreateAsyncWriteContext(_WriteContext);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                ULONG dataSizeWritten;
                KBuffer::SPtr dataBuffer;
                PUCHAR dataBufferPtr;
                Status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
                KBuffer::SPtr metadataKBuffer;
                KIoBuffer::SPtr dataIoBuffer;

                dataSizeWritten = _Length;

                for (ULONG i = 0; i < dataSizeWritten; i++)
                {
                    dataBufferPtr[i] = StreamOffsetToExpectedData(_StartAsn.Get() + i);
                }

                Status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, _LogStreamId, _StartVersion, _StartAsn.Get(),
                                            dataBufferPtr, dataSizeWritten, _CoreLoggerOffset, _ReservedMetadataSize, TRUE);

                struct KtlLogVerify::KtlMetadataHeader* metaDataHeader;
                metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());
                KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                        _StartAsn,
                                                        _StartVersion,
                                                        0x1000,
                                                        NULL,
                                                        dataIoBuffer);

                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                _WriteContext->Reuse();
                _WriteContext->StartWrite(_StartAsn, _StartVersion, metadataKBuffer, dataIoBuffer, NULL, _Completion);
                
                _State = Completed;
                break;
            }

            case Completed:
            {
                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                Complete(STATUS_SUCCESS);
                return;
            }

            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               VERIFY_IS_TRUE(NT_SUCCESS(Status));
               return;
            }
        }

        return;
    }

    VOID OnReuse() override
    {
    }

    VOID OnCompleted() override
    {
        _WriteContext = nullptr;
        _OverlayStream = nullptr;
    }

private:
    enum  FSMState { Initial = 0, Completed = 1 };
    FSMState _State;

    //
    // Parameters
    //
    OverlayStream::SPtr _OverlayStream;
    RvdLogAsn _StartAsn;
    ULONGLONG _StartVersion;
    ULONG _Length;
    RvdLogStreamId _LogStreamId;
    ULONG _CoreLoggerOffset;
    ULONG _ReservedMetadataSize;

    //
    // Internal
    //
    OverlayStream::AsyncWriteContext::SPtr _WriteContext;
};

Corner1WriteAsync::Corner1WriteAsync()
{
}

Corner1WriteAsync::~Corner1WriteAsync()
{
}

class Corner1ReadAsync : public WorkerAsync
{
    K_FORCE_SHARED(Corner1ReadAsync);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out Corner1ReadAsync::SPtr& Context
        )
    {
            NTSTATUS status;
            Corner1ReadAsync::SPtr context;

            context = _new(AllocationTag, Allocator) Corner1ReadAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (!NT_SUCCESS(status))
            {
                return(status);
            }

            context->_OverlayStream = nullptr;
            context->_ReadContext = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);
        }

    struct StartParameters
    {
        OverlayStream::SPtr OverlayStream;
        RvdLogAsn StartAsn;
        RvdLogStreamId LogStreamId;
        ULONG CoreLoggerOffset;
        ULONG ReservedMetadataSize;
    };

    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;
        _OverlayStream = startParameters->OverlayStream;
        _StartAsn = startParameters->StartAsn;
        _LogStreamId = startParameters->LogStreamId;
        _CoreLoggerOffset = startParameters->CoreLoggerOffset;
        _ReservedMetadataSize = startParameters->ReservedMetadataSize;

        Start(ParentAsyncContext, CallbackPtr);
    }

protected:
    VOID FSMContinue(
        __in NTSTATUS Status
        ) override
    {
        if (!NT_SUCCESS(Status))
        {
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            Complete(Status);
            return;
        }

        switch (_State)
        {
            case Initial:
            {
                OverlayStream::AsyncReadContext::SPtr readContext1;
                Status = _OverlayStream->CreateAsyncReadContext(readContext1);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                _ReadContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
                readContext1 = nullptr;

                _ReadContext->StartRead(_StartAsn,
                                        RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                                        &_ActualReadAsn,
                                        &_ActualReadVersion,
                                        _ReadMetadata,
                                        _ReadIoBuffer,
                                        &_ReadFromDedicated,
                                        NULL,
                                        _Completion);
                _State = Completed;
                break;
            }

            case Completed:
            {
                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                VERIFY_IS_TRUE(! _ReadFromDedicated);
                VerifyOverlayRecordRead(_ActualReadAsn, _StartAsn, _ActualReadVersion, 0, _LogStreamId, *_ReadMetadata, *_ReadIoBuffer,
                                        _ReservedMetadataSize,
                                        _CoreLoggerOffset);

                Complete(STATUS_SUCCESS);
                return;
            }

            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               VERIFY_IS_TRUE(NT_SUCCESS(Status));
               return;
            }
        }

        return;
    }

    VOID OnReuse() override
    {
    }

    VOID OnCompleted() override
    {
        _ReadContext = nullptr;
        _OverlayStream = nullptr;
        _ReadMetadata = nullptr;
        _ReadIoBuffer = nullptr;
    }

private:
    enum  FSMState { Initial = 0, Completed = 3 };
    FSMState _State;

    //
    // Parameters
    //
    OverlayStream::SPtr _OverlayStream;
    RvdLogAsn _StartAsn;
    RvdLogStreamId _LogStreamId;
    ULONG _CoreLoggerOffset;
    ULONG _ReservedMetadataSize;

    //
    // Internal
    //
    OverlayStream::AsyncReadContextOverlay::SPtr _ReadContext;
    KIoBuffer::SPtr _ReadIoBuffer;
    KBuffer::SPtr _ReadMetadata;
    BOOLEAN _ReadFromDedicated;
    RvdLogAsn _ActualReadAsn;
    ULONGLONG _ActualReadVersion;
};

Corner1ReadAsync::Corner1ReadAsync()
{
}

Corner1ReadAsync::~Corner1ReadAsync()
{
}




VOID ReadFromCoalesceBuffersCornerCase1Test(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    srand((ULONG)GetTickCount());
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Test: There is a race condition where one thread is reading from
    //       the currently coalescing buffer while another thread is
    //       writing into it. This test verifies that the data read
    //       will be consistent.
    //
    

    //
    // Configure the periodic timer to not kick off flushes
    //
    overlayStream->GetCoalesceRecords()->DisablePeriodicFlush();


    //
    // First step is to write a set of records to fill the coalesce
    // buffer in a specific way
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    Corner1ReadAsync::SPtr readCorner;
    status = Corner1ReadAsync::Create(*g_Allocator, KTL_TAG_TEST, readCorner);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    Corner1ReadAsync::StartParameters readParameters;
    readParameters.OverlayStream = overlayStream;
    readParameters.LogStreamId = logStreamId;
    readParameters.CoreLoggerOffset = coreLoggerOffset;
    readParameters.ReservedMetadataSize = reservedMetadataSize;


    Corner1WriteAsync::SPtr writeCorner;
    status = Corner1WriteAsync::Create(*g_Allocator, KTL_TAG_TEST, writeCorner);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    Corner1WriteAsync::StartParameters writeParameters;
    writeParameters.OverlayStream = overlayStream;
    writeParameters.Length = 0x40;
    writeParameters.LogStreamId = logStreamId;
    writeParameters.CoreLoggerOffset = coreLoggerOffset;
    writeParameters.ReservedMetadataSize = reservedMetadataSize;

    version = 0;

    KSynchronizer syncRead, syncWrite;
    RvdLogAsn readAsn;

    asn = KtlLogAsn::Min().Get();       
    
    for (ULONG loops = 0; loops < 200; loops++)
    {
        //
        // First write goes to both shared and dedicated
        //
        version++;
        dataSizeWritten = 0x3eef0;
        for (ULONG i = 0; i < dataSizeWritten; i++)
        {
            dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
        }
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        KtlLogVerify::KtlMetadataHeader* metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
        KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                asn,
                                                version,
                                                0x1000,
                                                NULL,
                                                dataIoBuffer);                                  

        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;
        readAsn = asn;

        //
        // Now write additional records to fill the coalesce buffer in an
        // exact way
        //
        const ULONG writeCount = 3;
        ULONG writeSize[writeCount] = { 0x3eef0, 0x3eef0, 0x3c0eb };
        for (ULONG i1 = 0; i1 < writeCount; i1++)
        {
            version++;
            dataSizeWritten = writeSize[i1];
            for (ULONG i = 0; i < dataSizeWritten; i++)
            {
                dataBufferPtr[i] = StreamOffsetToExpectedData(asn + i);
            }
            status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                        dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
            metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                    asn,
                                                    version,
                                                    0x1000,
                                                    NULL,
                                                    dataIoBuffer);                                  

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeContext->Reuse();
            writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }

        //
        // Kick off simultaneous read and write to hit the race
        //      
        readParameters.StartAsn = readAsn;
        readCorner->Reuse();

        version++;
        writeParameters.StartVersion = version;
        writeParameters.StartAsn = asn;
        writeCorner->Reuse();

        if ((loops % 2) == 1)
        {
            writeCorner->StartIt(&writeParameters,
                                NULL,
                                syncWrite);
            
            readCorner->StartIt(&readParameters,
                                NULL,
                                syncRead);
        } else {
            readCorner->StartIt(&readParameters,
                                NULL,
                                syncRead);

            writeCorner->StartIt(&writeParameters,
                                NULL,
                                syncWrite);
        }

        status = syncRead.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = syncWrite.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        if (((loops+1) % 50) == 0)
        {
            RvdLogAsn truncationAsn = asn - (5 * 0x3eef0);
            overlayStream->Truncate(truncationAsn, truncationAsn);
            KNt::Sleep(250);
        }

        
        //
        // Move ASN back so there is a truncate tail on the next
        // write.
        //
        asn -= (0x3eef0 - 1);
    }
    
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    status = sync.WaitForCompletion();

    //
    // Final Cleanup
    //
    readParameters.OverlayStream = nullptr;
    writeParameters.OverlayStream = nullptr;
    readCorner = nullptr;
    writeCorner = nullptr;
    writeContext = nullptr;
    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}
//

VOID WriteSpecificPattern(
    __in OverlayStream& Os,
    __in ULONG Count,
    __inout ULONGLONG& Version,
    __inout ULONGLONG& BaseAsn,                       
    __in LONGLONG* AsnOffset,
    __in ULONG* Lengths,
    __in ULONG coreLoggerOffset,
    __in BYTE entropy,
    __in BOOLEAN IndependentWrite
   )
{
    NTSTATUS status;
    KSynchronizer sync;
    OverlayStream::SPtr overlayStream = &Os;
    RvdLogStreamId logStreamId;
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    for (ULONG i = 0; i < Count; i++)
    {
        Version++;

        ULONGLONG asn;
        
        if (AsnOffset[i] <= 0)
        {       
            asn = BaseAsn + AsnOffset[i];
        } else {
            asn = AsnOffset[i];
        }
        
        dataSizeWritten = Lengths[i];
        for (ULONG i1 = 0; i1 < dataSizeWritten; i1++)
        {
            dataBufferPtr[i1] = StreamOffsetToExpectedData(entropy);
        }
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, Version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        KtlLogVerify::KtlMetadataHeader* metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());        
        KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                asn,
                                                Version,
                                                0x1000,
                                                NULL,
                                                dataIoBuffer);                                  

        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartReservedWrite(0, asn, Version, metadataKBuffer, dataIoBuffer, IndependentWrite, NULL, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        BaseAsn = asn + (ULONGLONG)dataSizeWritten;
    }   
}

VOID ReadAndVerifyTTRecord(
    __in OverlayStream& os,
    __in ULONGLONG AsnToRead,
    __in ULONGLONG ExpectedAsn,
    __in ULONGLONG ExpectedVersion,
    __in ULONG ExpectedLength,
    __in BOOLEAN ExpectedFromDedicated,                           
    __in ULONG coreLoggerOffset,
    __in BYTE entropy
)
{
    NTSTATUS status;
    KSynchronizer sync;
    OverlayStream::SPtr overlayStream = &os;
    RvdLogStreamId logStreamId;

    overlayStream->QueryLogStreamId(logStreamId);
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);    

    OverlayStream::AsyncReadContext::SPtr readContext1;
    OverlayStream::AsyncReadContextOverlay::SPtr readContext;
    status = overlayStream->CreateAsyncReadContext(readContext1);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    readContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
    readContext1 = nullptr;

    RvdLogAsn actualReadAsn;
    ULONGLONG actualReadVersion;
    KBuffer::SPtr readMetadata;
    KIoBuffer::SPtr readIoBuffer;
    BOOLEAN actualReadFromDedicated;
    
    readContext->StartRead(AsnToRead,
                            RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                            &actualReadAsn,
                            &actualReadVersion,
                            readMetadata,
                            readIoBuffer,
                            &actualReadFromDedicated,
                            NULL,
                            sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    VERIFY_IS_TRUE(actualReadFromDedicated == ExpectedFromDedicated ? true : false);

    VerifyOverlayRecordRead(actualReadAsn, ExpectedAsn, actualReadVersion, ExpectedVersion, logStreamId, *readMetadata, *readIoBuffer,
                            reservedMetadataSize,
                            coreLoggerOffset, ExpectedLength, entropy);
}

VOID ReadFromCoalesceBuffersTruncateTailTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    srand((ULONG)GetTickCount());
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();

    
    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));



    //
    // Replace the shared log stream with shim
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);  
    
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;
    
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);  
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);




    
    //
    // These tests verify that all conditions where truncate tail
    // happens are covered.
    //
    ULONGLONG version = 0;
    ULONGLONG baseAsn = RvdLogAsn::Min().Get();

    //
    // Configure the periodic timer to not kick off flushes
    //
    overlayStream->GetCoalesceRecords()->DisablePeriodicFlush();
    

    //
    // Test 1: Write pattern where overwritten records are in the
    //         dedicated log
    //         1 (1) 10 (2) 20 (3) 30 (4) 40 (5) 15 (6) 25 (7) 30 (8) 35 (9)
    //
    {
        //
        // Write the specific pattern
        //
        ULONGLONG startVersion = version;
        ULONGLONG startAsn = baseAsn;

        const ULONG count1 = 5;     
        LONGLONG asnOffsets1[count1] = { 0, 0, 0, 0, 0 }; 
        ULONG lengths1[count1] = { 10, 10, 10, 10, 10 };
        const ULONG count2 = 4;
        LONGLONG asnOffsets2[count2] = { (LONGLONG)baseAsn + 15, 0, 0, 0 };
        ULONG lengths2[count2] = { 10, 5, 5, 10 };
        
        WriteSpecificPattern(*overlayStream, count1, version, baseAsn, asnOffsets1, lengths1, coreLoggerOffset, 2, FALSE);
        WriteSpecificPattern(*overlayStream, count2, version, baseAsn, asnOffsets2, lengths2, coreLoggerOffset, 3, FALSE);

        //
        // Read asn 10 and verify
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 11, startAsn + 10, startVersion + 5, 5, TRUE, coreLoggerOffset, 2);

        //
        // Read asn 11 and verify
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 12, startAsn + 10, startVersion + 5, 5, TRUE, coreLoggerOffset, 2);

        //
        // Read asn 30 and verify
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 31, startAsn + 30, startVersion + 8, 5, TRUE, coreLoggerOffset, 3);

        //
        // Read asn 33 and verify
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 33, startAsn + 30, startVersion + 8, 5, TRUE, coreLoggerOffset, 3);

        //
        // Read asn 40 and verify
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 40, startAsn + 35, startVersion + 9, 10, TRUE, coreLoggerOffset, 3);

        //
        // Read asn 43 and veriy
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 43, startAsn + 35, startVersion + 9, 10, TRUE, coreLoggerOffset, 3);
       
        //
        // Force flush of coalescing buffer
        //
        overlayStream->GetCoalesceRecords()->ForcePeriodicFlush(sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));             
    }

    //
    // Test 2: Write pattern where overwritten records are in flushed
    //         records list and currently coalescing buffer
    //
    {
        //
        // Write specific pattern  beef0
        //
        ULONGLONG startVersion = version;
        ULONGLONG startAsn = baseAsn;

        //
        // Hold up writes to dedicated log to force records into
        // flushing records list
        //
        KAsyncEvent holdWriteEvent(TRUE, FALSE);
        WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
        waitForEventStruct.WaitEvent = &holdWriteEvent;
        dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                                       &waitForEventStruct);

        
        const ULONG count = 12;
        LONGLONG asnOffsets[count] = {       0,       0,       0,       0,       0,        0,        0,       0,        0,
                                             0,       0,       0 };
        ULONG lengths[count] = {       0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0,  0x3eef0,  0x3eef0, 0x3eef0,  0x3eef0,
                                       0x3eef0, 0x3eef0, 0x3eef0 };
        WriteSpecificPattern(*overlayStream, count, version, baseAsn, asnOffsets, lengths, coreLoggerOffset, 4, FALSE);


        //
        // Read from disk
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn-1, startAsn - 10, startVersion, 10, TRUE, coreLoggerOffset, 3);

        //
        // Read from flushing records
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 6000, startAsn, startVersion + 3, 0xbeef0, FALSE, coreLoggerOffset, 4);

        //
        // Read from coalescing buffers  2346238 747120
        //
        ReadAndVerifyTTRecord(*overlayStream, baseAsn - 6000, 2346238, startVersion + 12, 747120, FALSE, coreLoggerOffset, 4);

        //
        // Release the writes to the dedicated log
        //
        holdWriteEvent.SetEvent();

        //
        // Force flush of coalescing buffer
        //
        overlayStream->GetCoalesceRecords()->ForcePeriodicFlush(sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));             
        
    }

    //
    // Test 3: Write pattern where overwritten records are on
    //         independent write list
    //
    {
        //
        // Write specific pattern  beef0
        //
        ULONGLONG startVersion = version;
        ULONGLONG startAsn = baseAsn;

        //
        // Hold up writes to dedicated log to force records into
        // flushing records list
        //
        KAsyncEvent holdWriteEvent(TRUE, FALSE);
        WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
        waitForEventStruct.WaitEvent = &holdWriteEvent;
        dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                                       &waitForEventStruct);

        
        const ULONG count = 12;
        LONGLONG asnOffsets[count] = {       0,       0,       0,       0,       0,        0,        0,       0,        0,
                                             0,       0,       0 };
        ULONG lengths[count] = {       0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0, 0x3eef0,  0x3eef0,  0x3eef0, 0x3eef0,  0x3eef0,
                                       0x3eef0, 0x3eef0, 0x3eef0 };
        WriteSpecificPattern(*overlayStream, count, version, baseAsn, asnOffsets, lengths, coreLoggerOffset, 5, TRUE);

        //
        // Read from independent records
        //
        ReadAndVerifyTTRecord(*overlayStream, startAsn + 6000, startAsn, startVersion + 1, 0x3eef0, FALSE, coreLoggerOffset, 5);

        //
        // Release the writes to the dedicated log
        //
        holdWriteEvent.SetEvent();

        //
        // Force flush of coalescing buffer
        //
        overlayStream->GetCoalesceRecords()->ForcePeriodicFlush(sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));             
        
    }
    
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    status = sync.WaitForCompletion();

    //
    // Final Cleanup
    //
    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    dedicatedStream = nullptr;
    sharedStream = nullptr;

    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}


//
class RWRaceReadAsync : public WorkerAsync
{
    K_FORCE_SHARED(RWRaceReadAsync);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out RWRaceReadAsync::SPtr& Context
        )
    {
            NTSTATUS status;
            RWRaceReadAsync::SPtr context;

            context = _new(AllocationTag, Allocator) RWRaceReadAsync();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (!NT_SUCCESS(status))
            {
                return(status);
            }

            context->_OverlayStream = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);
        }

    enum OperationType { ReadAsync, WriteAsync };
    
    struct StartParameters
    {
        OverlayStream::SPtr OverlayStream;
        RvdLogStreamId LogStreamId;
        ULONG CoreLoggerOffset;
        ULONG ReservedMetadataSize;
        OperationType AsyncOperationType;
        ULONG Iterations;
        ULONGLONG* LastWrittenAsn;
    };

    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;
        _OverlayStream = startParameters->OverlayStream;
        _LogStreamId = startParameters->LogStreamId;
        _CoreLoggerOffset = startParameters->CoreLoggerOffset;
        _ReservedMetadataSize = startParameters->ReservedMetadataSize;
        _AsyncOperationType = startParameters->AsyncOperationType;
        _Iterations = startParameters->Iterations;
        _LastWrittenAsn = startParameters->LastWrittenAsn;
        
        Start(ParentAsyncContext, CallbackPtr);
    }

protected:
    VOID FSMContinue(
        __in NTSTATUS Status
        ) override
    {
        switch (_State)
        {
            case Initial:
            {
                if (_AsyncOperationType == WriteAsync)
                {
                    goto InitialWrite;
                }
                // fall through
            }
            
            case InitialRead:
            {
                srand((ULONG)GetTickCount());
                
                OverlayStream::AsyncReadContext::SPtr readContext1;
                Status = _OverlayStream->CreateAsyncReadContext(readContext1);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                _ReadContext = down_cast<OverlayStream::AsyncReadContextOverlay, OverlayStream::AsyncReadContext>(readContext1);
                readContext1 = nullptr;

                _State = ReadRecord;
                // Fall through
            }

            case ReadRecord:
            {
ReadRecord:
                _State = VerifyRecord;

                ULONG skip = rand();
                ULONG multiplier = rand() % 256;
                ULONG moveBack = skip * multiplier;
                if (moveBack >= *(_LastWrittenAsn))
                {
                    moveBack = 0;
                }               
                _ReadAsn = *_LastWrittenAsn - (ULONGLONG)(moveBack);

//                printf("Read %I64x %x\n", _ReadAsn, moveBack);
                
                _ReadContext->Reuse();
                _ReadContext->StartRead(_ReadAsn,
                                        RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord,
                                        &_ActualReadAsn,
                                        &_ActualReadVersion,
                                        _ReadMetadata,
                                        _ReadIoBuffer,
                                        &_ReadFromDedicated,
                                        NULL,
                                        _Completion);
                break;
            }

            case VerifyRecord:
            {
                if (Status == STATUS_NOT_FOUND)
                {
                    VERIFY_IS_TRUE(_ReadAsn == 1);
                    goto ReadRecord;
                }

                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }

                VerifyOverlayRecordRead(_ActualReadAsn, _ReadAsn, _ActualReadVersion, 0, _LogStreamId, *_ReadMetadata, *_ReadIoBuffer,
                                        _ReservedMetadataSize,
                                        _CoreLoggerOffset);

                _Iterations--;
                if (_Iterations == 0)
                {
                    Complete(Status);
                    return;
                }
                
                goto ReadRecord;
                break;
            }

            case InitialWrite:
            {
InitialWrite:
                srand((ULONG)GetTickCount());

                Status = _OverlayStream->CreateAsyncWriteContext(_WriteContext);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                _Version = 0;
                _WriteAsn = RvdLogAsn::Min().Get();

                _State = WriteRecord;
                
                // fall through
            }               

            case WriteRecord:
            {
WriteRecord:                
                ULONG dataSizeWritten;
                KBuffer::SPtr dataBuffer;
                PUCHAR dataBufferPtr;
                Status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
                KBuffer::SPtr metadataKBuffer;
                KIoBuffer::SPtr dataIoBuffer;

                ULONG skip = rand() % 32767;
                ULONG multiplier = rand() % 20;
                dataSizeWritten = (skip * multiplier) + 1;
                VERIFY_IS_TRUE(dataSizeWritten < 0xbeef0);

                for (ULONG i = 0; i < dataSizeWritten; i++)
                {
                    dataBufferPtr[i] = StreamOffsetToExpectedData(_WriteAsn + i);
                }

                _Version++;

                Status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, _LogStreamId, _Version, _WriteAsn,
                                            dataBufferPtr, dataSizeWritten, _CoreLoggerOffset, _ReservedMetadataSize, TRUE);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                
                struct KtlLogVerify::KtlMetadataHeader* metaDataHeader;
                metaDataHeader = (KtlLogVerify::KtlMetadataHeader*)(metadataKBuffer->GetBuffer());
                KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                        _WriteAsn,
                                                        _Version,
                                                        0x1000,
                                                        NULL,
                                                        dataIoBuffer);

                _DataSizeWritten = dataSizeWritten;
                
//                printf("Write %d: %I64x %x\n", _Iterations, _WriteAsn, _DataSizeWritten);
                
                _State = Written;
                _WriteContext->Reuse();
                _WriteContext->StartWrite(_WriteAsn, _Version, metadataKBuffer, dataIoBuffer, NULL, _Completion);
                break;
            }


            case Written:
            {
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }

//              printf("Completed Write %d: %I64x %x\n", _Iterations, _WriteAsn, _DataSizeWritten);

                *_LastWrittenAsn = _WriteAsn;
                _WriteAsn += _DataSizeWritten;
                 
                --_Iterations;
                if (_Iterations == 0)
                {
                    Complete(STATUS_SUCCESS);
                    return;
                }
                
                goto WriteRecord;
            }
            
            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               VERIFY_IS_TRUE(NT_SUCCESS(Status));
               return;
            }
        }

        return;
    }

    VOID OnReuse() override
    {
    }

    VOID OnCompleted() override
    {
        _ReadContext = nullptr;
        _WriteContext = nullptr;
        _OverlayStream = nullptr;
        _ReadMetadata = nullptr;
        _ReadIoBuffer = nullptr;
    }
    
private:
    enum  FSMState { Initial, InitialRead, ReadRecord, VerifyRecord, InitialWrite, WriteRecord, Written };
    FSMState _State;

    //
    // Parameters
    //
    OverlayStream::SPtr _OverlayStream;
    RvdLogAsn _StartAsn;
    RvdLogStreamId _LogStreamId;
    ULONG _CoreLoggerOffset;
    ULONG _ReservedMetadataSize;
    ULONG _MinimumReadSize;
    ULONG _MaximumReadSize;
    ULONG _MinimumWriteSize;
    ULONG _MaximumWriteSize;
    OperationType _AsyncOperationType;
    ULONG _Iterations;
    ULONGLONG* _LastWrittenAsn;

    //
    // Internal
    //

    // Write
    OverlayStream::AsyncWriteContext::SPtr _WriteContext;
    ULONGLONG _Version;
    ULONGLONG _WriteAsn;
    ULONG _DataSizeWritten;
    

    // Read
    OverlayStream::AsyncReadContextOverlay::SPtr _ReadContext;
    KIoBuffer::SPtr _ReadIoBuffer;
    KBuffer::SPtr _ReadMetadata;
    BOOLEAN _ReadFromDedicated;
    RvdLogAsn _ActualReadAsn;
    ULONGLONG _ActualReadVersion;
    ULONGLONG _ReadAsn;
};

RWRaceReadAsync::RWRaceReadAsync()
{
}

RWRaceReadAsync::~RWRaceReadAsync()
{
}

static const ULONG readCount = 4;
static const ULONG writeIterations = 1500;
static const ULONG readIterations = 1500;
static const ULONG threadCount = 4;

DWORD __stdcall ReadWriteRaceTestThread(
    LPVOID Context
    )
{
    KGuid* diskIdPtr = (KGuid*)Context;
    KGuid diskId = *diskIdPtr;


    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);

    srand((ULONG)GetTickCount());
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();

    
    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));



    //
    // Replace the shared log stream with shim
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);  
    
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;
    
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);  
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);

    //
    // Configure the periodic timer to not kick off flushes
    //
    overlayStream->GetCoalesceRecords()->DisablePeriodicFlush();

    {
        //
        // Setup asyncs to read and write simultaneously
        //
        ULONGLONG lastWrittenAsn = RvdLogAsn::Min().Get();
        RWRaceReadAsync::StartParameters startParameters;
        
        startParameters.OverlayStream = overlayStream;
        startParameters.LogStreamId = logStreamId;
        startParameters.CoreLoggerOffset = coreLoggerOffset;
        startParameters.ReservedMetadataSize = reservedMetadataSize;
        startParameters.Iterations = writeIterations;
        startParameters.LastWrittenAsn = &lastWrittenAsn;


        //
        // Give write a head start so we have something to read
        //
        RWRaceReadAsync::SPtr writeAsync;
        KSynchronizer writeSync;
        status = RWRaceReadAsync::Create(*g_Allocator, KTL_TAG_TEST, writeAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        startParameters.AsyncOperationType = RWRaceReadAsync::OperationType::WriteAsync;
        writeAsync->StartIt(&startParameters, NULL, writeSync);

        //
        // Start readers
        //
        KNt::Sleep(100);
        RWRaceReadAsync::SPtr readAsyncs[readCount];
        KSynchronizer readSyncs[readCount];

        startParameters.Iterations = readIterations;
        startParameters.AsyncOperationType = RWRaceReadAsync::OperationType::ReadAsync;
        for (ULONG i = 0; i < readCount; i++)
        {
            status = RWRaceReadAsync::Create(*g_Allocator, KTL_TAG_TEST, readAsyncs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            readAsyncs[i]->StartIt(&startParameters, NULL, readSyncs[i]);
        }

        // CONSIDER: Use more than one stream

        // CONSIDER: Play games with the dedicated and shared core log
        //           streams to simulate slow file IO


        //
        // Wait for completions
        //
        status = writeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < readCount; i++)
        {
            status = readSyncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }   
    }

    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    status = sync.WaitForCompletion();

    //
    // Final Cleanup
    //
    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    sharedShim = nullptr;
    sharedCoreStream = nullptr;

    dedicatedStream = nullptr;
    sharedStream = nullptr;

    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

    return(0);
}

VOID ReadWriteRaceTest(
    KGuid& diskId
    )
{
    DWORD error;
    HANDLE threads[threadCount];

    for (ULONG i = 0; i < threadCount; i++)
    {
        DWORD threadId;

        threads[i] = CreateThread(NULL, 0, ReadWriteRaceTestThread, &diskId, 0, &threadId);
        if (threads[i] == NULL)
        {
            error = GetLastError();
            VERIFY_IS_TRUE(error == ERROR_SUCCESS ? true : false);
        }
    }

    WaitForMultipleObjects(threadCount, threads, TRUE, INFINITE);
}


typedef struct
{
    RvdLogAsn StartAsn;
    RvdLogAsn EndAsn;
    ULONGLONG AsnIncrement;
    ULONGLONG StartVersion;
    BOOLEAN ExpectInResult;
} TTTESTRCORDINFO, *PTTTESTRCORDINFO;

VOID DeleteTruncatedTailRecordsWorker(
    KGuid& diskId,
    PTTTESTRCORDINFO recordInfo,
    ULONG recordInfoCount
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;
    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();
    
    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType());

    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                           &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logStreamGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Write initial records into the dedicated log stream
    //
    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;
    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreDedicatedLogStream;
    
    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreDedicatedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream = nullptr;

    for (ULONG i = 0; i < recordInfoCount; i++)
    {
        ULONGLONG start = recordInfo[i].StartAsn.Get();
        ULONGLONG end = recordInfo[i].EndAsn.Get();
        ULONGLONG increment = recordInfo[i].AsnIncrement;
        ULONGLONG version = recordInfo[i].StartVersion;

        for (ULONGLONG asn = start; asn <= end; )
        {
            status = WriteALogicalLogRecord(coreDedicatedLogStream,
                                            version,
                                            asn,
                                            coreLog->QueryUserRecordSystemMetadataOverhead(),
                                            'X');
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn = asn + increment;
            version++;
        }
    }
    
    coreDedicatedLogStream = nullptr;
    coreLog = nullptr;
    KNt::Sleep(2000);

    //
    // Reopen the streams via the overlay log. This will cause records truncated at the tail
    // to be deleted
    //

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenLog = nullptr;

    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 
    
    RvdLogStream::SPtr dedicatedStream;
    RvdLogStream::SPtr sharedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();
    sharedStream = overlayStream->GetSharedLogStream();

    //
    // Verify that only those records that should be in the list are actually in the list
    //
    KArray<RvdLogStream::RecordMetadata> records(*g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(records.Status()));

    status = dedicatedStream->QueryRecords(RvdLogAsn::Min(), RvdLogAsn::Max(), records);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG i = 0; i < records.Count(); i++)
    {
        printf("        Record %I64u 0x%I64x, Version %I64u 0x%I64x, Size %d 0x%x, Disposition %2d, Debug_LSN 0x%I64x\n",
            records[i].Asn.Get(),
            records[i].Asn.Get(),
            records[i].Version,
            records[i].Version,
            records[i].Size,
            records[i].Size,
            records[i].Disposition,
            records[i].Debug_LsnValue);
    }

    for (ULONG i = 0; i < recordInfoCount; i++)
    {
        ULONGLONG start = recordInfo[i].StartAsn.Get();
        ULONGLONG end = recordInfo[i].EndAsn.Get();
        ULONGLONG increment = recordInfo[i].AsnIncrement;
        ULONGLONG version = recordInfo[i].StartVersion;
        BOOLEAN shouldBeInList = recordInfo[i].ExpectInResult;

        for (ULONGLONG asn = start; asn <= end;)
        {
            BOOLEAN isInList = IsInRecordsList(records, asn, version);

            if ((shouldBeInList) && (! isInList))
            {
                printf("ASN %I64d Version %I64d should be in list but is not\n", asn, version);
                VERIFY_IS_TRUE(isInList ? true : false);
            }

            if ((! shouldBeInList) && (isInList))
            {
                printf("ASN %I64d Version %I64d should not be in list but is\n", asn, version);
                VERIFY_IS_TRUE(! isInList ? true : false);
            }

            asn = asn + increment;
            version++;
        }
    }

    
    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);
    
    dedicatedStream = nullptr;
    sharedStream = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;
    coreLog = nullptr;
    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();    

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);
}

VOID DeleteTruncatedTailRecords1Test(
    KGuid& diskId
    )
{
    //
    // Open dedicated stream and write a set of records where some of
    // them are truncated tail and should be deleted. QueryRecords and
    // verify that only those records expected are in the list
    //
    // Test will the following ASN/Versions
    //
    TTTESTRCORDINFO recordInfo[] = {
        {    2,     5,    1,    6, TRUE},
        {   10,    10,   10,   10, TRUE},
        {   20,    50,   10,    2, FALSE},
        {  110,   340,   10,   11, TRUE},
        {  350,   500,   10,   35, FALSE},
        {  345,   345,   10,   51, TRUE},
        {  600,   650,   5,    52, TRUE}
    };
    ULONG recordInfoCount = sizeof(recordInfo) / sizeof(TTTESTRCORDINFO);

    DeleteTruncatedTailRecordsWorker(diskId, recordInfo, recordInfoCount);
}

VOID RetryOnSharedLogFullTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxStreamRecordSize = 0x100 * 0x1000; // 1MB
    ULONG maxContainerRecordSize = maxStreamRecordSize;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;

    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType(),
                              maxContainerRecordSize,
                              maxStreamRecordSize);
    
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    coreOpenLog->Reuse();
    coreOpenLog->StartOpenLog(diskId,
        rvdLogId,
        coreLog,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId = logStreamGuid;
    RvdLogStream::SPtr coreSharedLogStream;

    coreOpenStream->StartOpenLogStream(logStreamId,
        coreSharedLogStream,
        NULL,
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    coreSharedLogStream = nullptr;
    //
    // Reopen the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    OverlayLog::SPtr overlayLog;
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                     *coreLogManager,
                                                                     *coreLog,
                                                                     logStreamId,
                                                                     diskId,
                                                                     nullptr,
                                                                     maxStreamRecordSize,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // The test scenario is as follows:
    //
    // Shared log write fails with STATUS_LOG_FULL and we should verify
    // that the shared log write gets retried.
    //
    // Verify with multiple retries
    //
    // Verify case where dedicated races ahead while retry pending
    

    //
    // Replace the shared log stream with shim
    //
    RvdLogStream::SPtr sharedCoreStream;
    RvdLogStream::SPtr sharedStream;
    WriteStuckRvdLogStream::SPtr sharedShim;

    sharedCoreStream = overlayStream->GetSharedLogStream();
    
    status = WriteStuckRvdLogStream::Create(sharedShim,
                                          *g_Allocator,
                                          KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                NULL);  
    
    sharedShim->SetRvdLogStream(*sharedCoreStream);
    sharedStream = up_cast<RvdLogStream>(sharedShim);
    overlayStream->SetSharedLogStream(*sharedStream);


    //
    // Replace the dedicated log with shim
    //
    RvdLogStream::SPtr dedicatedCoreStream;
    RvdLogStream::SPtr dedicatedStream;
    WriteStuckRvdLogStream::SPtr dedicatedShim;
    
    dedicatedCoreStream = overlayStream->GetDedicatedLogStream();
    
    status = WriteStuckRvdLogStream::Create(dedicatedShim,
                                            *g_Allocator,
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::NoAction,
                                   NULL);  
    
    dedicatedShim->SetRvdLogStream(*dedicatedCoreStream);
    dedicatedStream = up_cast<RvdLogStream>(dedicatedShim);
    overlayStream->SetDedicatedLogStream(*dedicatedStream);

    //
    // First step is to write a record to fill the coalesce buffer
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    
    ULONG dataSizeWritten;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;
    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();
    
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    
    overlayStream->QueryLogStreamId(logStreamId);
    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    
    OverlayStream::AsyncWriteContextOverlay::SPtr writeContext;
    CreateAsyncWriteContextOverlay(*overlayStream, writeContext);
    
    //
    // First write goes to both shared and dedicated
    //
    version++;
    dataSizeWritten = 40;
    status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    writeContext->Reuse();
    writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    asn += dataSizeWritten;


    //
    // Test 1: Fail first shared write with log full but allow second
    //         to pass
    //
    {
        //
        // Hold writes to dedicated log until I say so
        //
        KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
        struct WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
        waitForEventStruct.WaitEvent = &waitEvent;
        dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                                       &waitForEventStruct);   

        //
        // Next set the shared log stream shim to fail with STATUS_LOG_FULL
        // and then allow the retry to succeed
        //
        WriteStuckRvdLogStream::FailWithNTStatusStruct failWithNTStatusStruct;
        memset(&failWithNTStatusStruct, 0, sizeof(failWithNTStatusStruct));
        failWithNTStatusStruct.FailStatus = STATUS_LOG_FULL;
        failWithNTStatusStruct.PassFrequency = 1;
        failWithNTStatusStruct.FailCounter = 0;

        sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::FailWithNTStatus,
                                    &failWithNTStatusStruct);   

        //
        // Write a little something so the shared log can fail it and the
        // coalesce buffers get something
        //
        version++;
        dataSizeWritten = 90;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);

        asn += dataSizeWritten;

        //
        // Expect the shared log write to be failed once and succeeded once
        //
        status = sync.WaitForCompletion(1 * 60 * 1000);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(failWithNTStatusStruct.NumberFails == 1);
        VERIFY_IS_TRUE(failWithNTStatusStruct.NumberSuccess == 1);

        //
        // Allow dedicated to go
        //
        waitEvent.SetEvent();
    }

    //
    // Test 2: Fail first shared write with log full and 15 retries and
    //         then succeed
    //
    {
        //
        // Hold writes to dedicated log until I say so
        //
        KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
        struct WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
        waitForEventStruct.WaitEvent = &waitEvent;
        dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                                       &waitForEventStruct);   

        //
        // Next set the shared log stream shim to fail with STATUS_LOG_FULL
        // and then allow the retry to succeed
        //
        WriteStuckRvdLogStream::FailWithNTStatusStruct failWithNTStatusStruct;
        memset(&failWithNTStatusStruct, 0, sizeof(failWithNTStatusStruct));
        failWithNTStatusStruct.FailStatus = STATUS_LOG_FULL;
        failWithNTStatusStruct.PassFrequency = 15;
        failWithNTStatusStruct.FailCounter = 0;

        sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::FailWithNTStatus,
                                    &failWithNTStatusStruct);   

        //
        // Write a little something so the shared log can fail it and the
        // coalesce buffers get something
        //
        version++;
        dataSizeWritten = 90;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);

        asn += dataSizeWritten;

        //
        // Expect the shared log write to be failed once and succeeded once
        //
        status = sync.WaitForCompletion(1 * 60 * 1000);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(failWithNTStatusStruct.NumberFails == 15);
        VERIFY_IS_TRUE(failWithNTStatusStruct.NumberSuccess == 1);

        //
        // Allow dedicated to go
        //
        waitEvent.SetEvent();
    }

    //
    // Test 3: Fail all shared write with log full and keep retrying
    //
    {
        //
        // Hold writes to dedicated log until I say so
        //
        KAsyncEvent waitEvent(TRUE, FALSE);   // ManualReset, Initial FALSE
        struct WriteStuckRvdLogStream::WaitForEventStruct waitForEventStruct;
        waitForEventStruct.WaitEvent = &waitEvent;
        dedicatedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::WaitForEvent,
                                       &waitForEventStruct);   

        //
        // Next set the shared log stream shim to fail with STATUS_LOG_FULL
        // and then allow the retry to succeed
        //
        WriteStuckRvdLogStream::FailWithNTStatusStruct failWithNTStatusStruct;
        memset(&failWithNTStatusStruct, 0, sizeof(failWithNTStatusStruct));
        failWithNTStatusStruct.FailStatus = STATUS_LOG_FULL;
        failWithNTStatusStruct.PassFrequency = 0;

        sharedShim->SetTestCaseData(WriteStuckRvdLogStream::TestCaseIds::FailWithNTStatus,
                                    &failWithNTStatusStruct);   

        //
        // Write a little something so the shared log can fail it and the
        // coalesce buffers get something
        //
        version++;
        dataSizeWritten = 90;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer, logStreamId, version, asn,
                                    dataBufferPtr, dataSizeWritten, coreLoggerOffset, reservedMetadataSize, TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        writeContext->Reuse();
        writeContext->StartWrite(asn, version, metadataKBuffer, dataIoBuffer, NULL, sync);

        asn += dataSizeWritten;

        //
        // Expect the shared log write to be failed once and succeeded once
        //
        status = sync.WaitForCompletion(1 * 60 * 1000);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);
        VERIFY_IS_TRUE(failWithNTStatusStruct.NumberFails > 5);
        VERIFY_IS_TRUE(failWithNTStatusStruct.NumberSuccess == 0);

        //
        // Allow dedicated to go
        //
        waitEvent.SetEvent();
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    
    //
    // Final Cleanup
    //
    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);

    dedicatedStream = nullptr;
    sharedStream = nullptr;
    
    sharedShim = nullptr;
    sharedCoreStream = nullptr;
    dedicatedShim = nullptr;
    dedicatedCoreStream = nullptr;

    writeContext = nullptr;
    
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;

    coreOpenLog = nullptr;
    coreOpenStream = nullptr;

    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteRvdlogFile(diskId,
        logContainerGuid);
    DeleteRvdlogFile(diskId,
        logStreamGuid);

}


VOID DiscontiguousRecordsRecoveryTest(
    KGuid& diskId
    )
{
    NTSTATUS status;
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    KSynchronizer sync;
    KSynchronizer activateSync;
    KServiceSynchronizer        serviceSync;
    ULONG metadataLength = 0x10000;
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE;
    LONGLONG streamSize = DEFAULT_STREAM_SIZE;


    //
    // This test is to verify that the stream open detects a situation
    // where there is a gap between the end of the dedicated log and
    // the beginning of the shared log.
    //


    
    logStreamGuid.CreateNew();
    logContainerGuid.CreateNew();

    CreateInitialKtlLogStream(diskId,
                              logContainerGuid,
                              1,
                              &logStreamGuid,
                              KLogicalLogInformation::GetLogicalLogStreamType());
    //
    // Open up base log manager and container and stream via the core logger
    //
    RvdLogManager::SPtr coreLogManager;
    RvdLogManager::AsyncOpenLog::SPtr coreOpenLog;
    RvdLog::SPtr coreLog;
    RvdLogId rvdLogId;
    RvdLogStreamId logStreamId;

    status = RvdLogManager::Create(ALLOCATION_TAG, *g_Allocator, coreLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                          &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                           &VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = coreLogManager->Activate(NULL,       // parentasync
                                      activateSync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = coreLogManager->CreateAsyncOpenLogContext(coreOpenLog);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rvdLogId = logContainerGuid;
    logStreamId = logStreamGuid;
    coreOpenLog->StartOpenLog(diskId,
                              rvdLogId,
                              coreLog,
                              NULL,
                              sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenLog = nullptr;
        
    RvdLog::AsyncOpenLogStreamContext::SPtr coreOpenStream;

    status = coreLog->CreateAsyncOpenLogStreamContext(coreOpenStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStream::SPtr coreSharedLogStream;
    coreOpenStream->StartOpenLogStream(logStreamId,
                                       coreSharedLogStream,
                                       NULL,
                                       sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    coreOpenStream = nullptr;

    
    //
    // Open the streams via the overlay log. This will move records
    // from shared to dedicated
    //
    OverlayStream::SPtr overlayStream;
    OverlayLog::SPtr overlayLog;
    ThrottledKIoBufferAllocator::SPtr throttledAllocator;

    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
        memoryThrottleLimits,
        *g_Allocator,
        KTL_TAG_TEST,
        throttledAllocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    overlayLog = _new(ALLOCATION_TAG, *g_Allocator) OverlayLog(*coreLogManager,
                                                               diskId,
                                                               NULL,        // Path
                                                               rvdLogId,
                                                               *throttledAllocator);
    VERIFY_IS_TRUE(overlayLog ? TRUE : FALSE);
    status = overlayLog->Status();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = overlayLog->StartServiceOpen(NULL,      // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    {
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    RvdLogStream::SPtr dedicatedStream;

    dedicatedStream = overlayStream->GetDedicatedLogStream();


    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    KBuffer::SPtr dataBuffer;
    PUCHAR dataBufferPtr;

    status = KBuffer::Create(0xbeef0, dataBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    dataBufferPtr = (PUCHAR)dataBuffer->GetBuffer();


    ULONG coreLoggerOffset = coreLog->QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    

    //
    // Write a set of records into the dedicated log
    //
    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;

    for (ULONG i = 0; i < 25; i++)
    {
        version++;
        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
                                    logStreamId, version, asn,
                                    dataBufferPtr, 25000,
                                    coreLoggerOffset, reservedMetadataSize, TRUE, 25000,
                                    KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARawRecord(dedicatedStream,
                                 version, asn, metadataKBuffer, 0, dataIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += 25000;
    }


    overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);

    dedicatedStream = nullptr;
    status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayStream = nullptr;
    

    
    //
    // Test 1: Shared log is empty, stream opens ok
    //
    {
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));



        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);

        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;
        
    }

    
    //
    // Test 2: Shared log has gap detected by checking the IoBufferSize
    //         in the corelogger metadata. stream opens with error
    //
    {
        ULONGLONG endOfDedicatedRecord = asn + 0x8000;

        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version + 1, endOfDedicatedRecord,
            dataBufferPtr, 25000,
            coreLoggerOffset, reservedMetadataSize, TRUE, 25000,
            KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARawRecord(coreSharedLogStream,
            version + 1, endOfDedicatedRecord, metadataKBuffer, 0, dataIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(status == K_STATUS_LOG_STRUCTURE_FAULT);

        overlayStream = nullptr;        
    }
    
    //
    // Test 3: Shared log has gap detected by checking the record
    //         length in the logical log stream block header. stream opens with error
    //
    // TODO:

    //
    // Test 4: Shared log record has older version, stream opens ok
    //
    {
        ULONGLONG endOfDedicatedRecord = asn + 500;

        status = SetupToWriteRecord(metadataKBuffer, dataIoBuffer,
            logStreamId, version -2 , endOfDedicatedRecord,
            dataBufferPtr, 25000,
            coreLoggerOffset, reservedMetadataSize, TRUE, 25000,
            KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteARawRecord(coreSharedLogStream,
            version -2, endOfDedicatedRecord, metadataKBuffer, 0, dataIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        overlayStream = _new(ALLOCATION_TAG, *g_Allocator) OverlayStream(*overlayLog,
                                                                         *coreLogManager,
                                                                         *coreLog,
                                                                         logStreamId,
                                                                         diskId,
                                                                         nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        overlayStream->GetCoalesceRecords()->FlushAllRecordsForClose(sync);

        status = overlayStream->StartServiceClose(NULL,      // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = serviceSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        overlayStream = nullptr;        
    }


    
    //
    // Final Cleanup
    //

    coreSharedLogStream = nullptr;
    coreLog = nullptr;

    status = overlayLog->StartServiceClose(NULL,      // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    overlayLog = nullptr;
    
    coreLogManager->Deactivate();
    coreLogManager = nullptr;
    status = activateSync.WaitForCompletion();

    DeleteInitialKtlLogStream(diskId,
                              logContainerGuid);

}

VOID SetupOverlayLogTests(
    KGuid& DiskId,
    UCHAR& driveLetter,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    NTSTATUS status;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    System->SetStrictAllocationChecks(TRUE);

    StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

#if !defined(PLATFORM_UNIX)
    driveLetter = 0;
    status = FindDiskIdForDriveLetter(driveLetter, DiskId);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#else
    // {204F01E4-6DEC-48fe-8483-B2C6B79ABECB}
    GUID randomGuid = 
        { 0x204f01e4, 0x6dec, 0x48fe, { 0x84, 0x83, 0xb2, 0xc6, 0xb7, 0x9a, 0xbe, 0xcb } };
    
    DiskId = randomGuid;    
#endif

#if defined(UDRIVER)
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif

    DeleteAllContainersOnDisk(DiskId);

}

VOID CleanupOverlayLogTests(
    KGuid&,
    UCHAR&,
    ULONGLONG&,
    KtlSystem*&
    )
{
#if defined(UDRIVER)
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    NTSTATUS status;

    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    KInvariant(NT_SUCCESS(status));
#endif

    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

