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

#include <windows.h>

#include <KtlPhysicalLog.h>

#include <ktllogger.h>
#include <KLogicalLog.h>

#if defined(PLATFORM_UNIX)
#include <stdlib.h>
#endif

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

#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

static const ULONG fourKB = 4 * 1024;
 
KAllocator* g_Allocator;


NTSTATUS
VerifyRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const,
    __in ULONG,
    __in const KIoBuffer::SPtr&
)
{
    return(STATUS_SUCCESS);
}

VOID CreateLogFile(
    __in RvdLogManager& LogManager,                
    __in KStringView const& LogFileName,
    __in KGuid LogGuid,
    __out RvdLog::SPtr& Log,
    __in ULONG MaxRecordSize = 16 * 1024,
    __in LONGLONG LogSize = 256 * 1024,
    __in ULONG NumberStreams = 2
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    
    KWString logType(KtlSystem::GlobalNonPagedAllocator(), L"TestLogType");     
    KInvariant(NT_SUCCESS(logType.Status()));

    RvdLogManager::AsyncCreateLog::SPtr createLogOp;
    RvdLog::SPtr log;
    status = LogManager.CreateAsyncCreateLogContext(createLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogId logId;
    logId = static_cast<RvdLogId>(LogGuid);

    createLogOp->StartCreateLog(LogFileName,
                                logId,
                                logType,
                                LogSize,
                                NumberStreams,
                                MaxRecordSize,
                                log,
                                nullptr,
                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    createLogOp = nullptr;
    Log = Ktl::Move(log);
}

VOID CreateLogStream(
    __in RvdLog& Log,
    __out KGuid& StreamGuid,
    __out RvdLogStream::SPtr& LogStream
    )
{
    NTSTATUS status;
    KSynchronizer sync;

    RvdLog::AsyncCreateLogStreamContext::SPtr createStreamOp;
    RvdLogStream::SPtr logStream;
    status = Log.CreateAsyncCreateLogStreamContext(createStreamOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    RvdLogStreamId logStreamId;
    StreamGuid = WellKnownGuid;
    logStreamId = static_cast<RvdLogStreamId>(StreamGuid);

    createStreamOp->StartCreateLogStream(logStreamId,
                                         KLogicalLogInformation::GetLogicalLogStreamType(),
                                         logStream,
                                         nullptr,
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    createStreamOp = nullptr;
    LogStream = Ktl::Move(logStream);
}


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

NTSTATUS WriteARawRecord(
    RvdLogStream& logStream,
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

        status = logStream.CreateAsyncWriteContext(writeContext);
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

NTSTATUS WriteLogicalLogRecord(
    __in RvdLog& Log,
    __in RvdLogStream& LogStream,
    __in ULONGLONG Version,
    __in ULONGLONG Asn,
    __in KBuffer& Data,
    __in ULONG DataSize
    )
{
    NTSTATUS status;
    KBuffer::SPtr metadataKBuffer;
    KIoBuffer::SPtr dataIoBuffer;
    ULONG coreLoggerOffset = Log.QueryUserRecordSystemMetadataOverhead();
    ULONG reservedMetadataSize = sizeof(KtlLogVerify::KtlMetadataHeader);
    RvdLogStreamId logStreamId;
    LogStream.QueryLogStreamId(logStreamId);

    //
    // Write dedicated log records
    //
    if (DataSize == 0)
    {
        DataSize = Data.QuerySize();
    }
    
    status = SetupToWriteRecord(metadataKBuffer,
                                dataIoBuffer,
                                logStreamId,
                                Version, Asn,
                                static_cast<PUCHAR>(Data.GetBuffer()),
                                DataSize,                               
                                coreLoggerOffset, reservedMetadataSize,
                                TRUE, DataSize,
                                KLogicalLogInformation::FixedMetadataSize - coreLoggerOffset);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteARawRecord(LogStream,
                             Version, Asn,
                             metadataKBuffer,
                             0,
                             dataIoBuffer);

    return(status);
}
                     

void MakeSeedFiles()
{
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

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


    //
    // Test file 1: Create empty log file with a single stream
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\Container.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;

        logGuid = WellKnownGuid2;
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);
        
        log = nullptr;
        logStream = nullptr;
    }

    //
    // Seed file 1: Create empty log file with a single stream
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\EmptyLog.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;

        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);
        
        log = nullptr;
        logStream = nullptr;
    }

    //
    // Seed file 2: Log file with logical log record in metadata only
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\MetadataOnly.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        ULONG dataSize = 500;
        KBuffer::SPtr dataBuffer;
        PUCHAR p;

        status = KBuffer::Create(dataSize, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBuffer->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSize; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;

        version++;
        status = WriteLogicalLogRecord(*log,
                              *logStream,
                              version,
                              asn,
                              *dataBuffer,
                              0);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += dataBuffer->QuerySize();
        
        log = nullptr;
        logStream = nullptr;
    }


    //
    // Seed file 3: Log file with logical log record in metadata and
    //              data only
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\MetadataAndData.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        const ULONG dataSize = 5000;
        KBuffer::SPtr dataBuffer;
        PUCHAR p;

        status = KBuffer::Create(dataSize, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBuffer->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSize; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;

        version++;
        status = WriteLogicalLogRecord(*log,
                              *logStream,
                              version,
                              asn,
                              *dataBuffer,
                              0);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        asn += dataBuffer->QuerySize();
        
        log = nullptr;
        logStream = nullptr;
    }


    //
    // Seed file 4: Log file with logical log records that are metadata
    //              only and metadata and data
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\ManyRecords.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        const ULONG dataSizeMD = 5000;
        const ULONG dataSizeMO = 500;
        KBuffer::SPtr dataBufferMD;
        KBuffer::SPtr dataBufferMO;
        PUCHAR p;

        status = KBuffer::Create(dataSizeMD, dataBufferMD, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBufferMD->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSizeMD; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        status = KBuffer::Create(dataSizeMO, dataBufferMO, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBufferMO->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSizeMO; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;

        for (ULONG i = 0; i < 7; i++)
        {
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBufferMO,
                                  0);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataBufferMO->QuerySize();

            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBufferMD,
                                  0);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataBufferMD->QuerySize();           
        }
        
        log = nullptr;
        logStream = nullptr;
    }

    //
    // Seed file 5: Create a log file that has missing records in the
    //              region of chaos
    {
        KStringView fileName(L"\\??\\c:\\seeds\\ChaosRegion.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        const ULONG dataSizeMD = 5000;
        const ULONG dataSizeMO = 500;
        KBuffer::SPtr dataBufferMD;
        KBuffer::SPtr dataBufferMO;
        PUCHAR p;

        status = KBuffer::Create(dataSizeMD, dataBufferMD, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBufferMD->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSizeMD; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        status = KBuffer::Create(dataSizeMO, dataBufferMO, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBufferMO->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSizeMO; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;

        for (ULONG i = 0; i < 7; i++)
        {
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBufferMO,
                                  0);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataBufferMO->QuerySize();

            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBufferMD,
                                  0);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataBufferMD->QuerySize();           
        }
        
        log = nullptr;
        logStream = nullptr;


        //
        // Manually overwrite a few records
        //
        KBlockFile::SPtr blockFile;
        KWString fileNameString(*g_Allocator);
        fileNameString = (PWCHAR)fileName;
        KIoBuffer::SPtr zeroBuffer;
        PVOID pv;

        status = KIoBuffer::CreateSimple(fourKB, zeroBuffer, pv, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        p = static_cast<PUCHAR>(pv);
        for (ULONG i = 0; i < fourKB; i++)
        {
            p[i] = 0;
        }
        
        status = KBlockFile::Create(fileNameString,
                                    FALSE,
                                    KBlockFile::eOpenExisting,
                                    blockFile,
                                    sync,
                                    nullptr,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Log should look like this and so we will remove record as
        // LSN 15000 to force data in region of chaos
        //
        // --LC           0 length         1000 expectnext:         1000 HIGHLSN: FFFFFFFFFFFFFFFF CHKPTLSN: FFFFFFFFFFFFFFFF PREVLSN: FFFFFFFFFFFFFFFF Asn:            1 Version:           -1
        // --LR        1000 length         1000 expectnext:         2000 highlsn:            0 chkptlsn:            0 PREVLSN: FFFFFFFFFFFFFFFF Asn:            1 Version:            1
        // ---S        2000 length         1000 expectnext:         3000 highlsn:            0 chkptlsn:            0 prevlsn:         1000 Asn:            1 Version:           -1
        // ---R        3000 length         2000 expectnext:         5000 highlsn:         2000 chkptlsn:            0 prevlsn:         2000 Asn:          501 Version:            2
        // ---R        5000 length         1000 expectnext:         6000 highlsn:         3000 chkptlsn:            0 prevlsn:         3000 Asn:         5501 Version:            3
        // ---R        6000 length         2000 expectnext:         8000 highlsn:         5000 chkptlsn:            0 prevlsn:         5000 Asn:         6001 Version:            4
        // ---R        8000 length         1000 expectnext:         9000 highlsn:         6000 chkptlsn:            0 prevlsn:         6000 Asn:        11001 Version:            5
        // ---R        9000 length         2000 expectnext:         b000 highlsn:         8000 chkptlsn:            0 prevlsn:         8000 Asn:        11501 Version:            6
        // ---R        b000 length         1000 expectnext:         c000 highlsn:         9000 chkptlsn:            0 prevlsn:         9000 Asn:        16501 Version:            7
        // ---R        c000 length         2000 expectnext:         e000 highlsn:         b000 chkptlsn:            0 prevlsn:         b000 Asn:        17001 Version:            8
        // ---R        e000 length         1000 expectnext:         f000 highlsn:         c000 chkptlsn:            0 prevlsn:         c000 Asn:        22001 Version:            9
        // ---R        f000 length         2000 expectnext:        11000 highlsn:         e000 chkptlsn:            0 prevlsn:         e000 Asn:        22501 Version:           10
        // ---C       11000 length         1000 expectnext:        12000 highlsn:         e000 chkptlsn:            0 prevlsn:            0 Asn:            1 Version:           -1
        // ---R       12000 length         1000 expectnext:        13000 highlsn:        11000 chkptlsn:        11000 prevlsn:         f000 Asn:        27501 Version:           11
        // ---R       13000 length         2000 expectnext:        15000 highlsn:        12000 chkptlsn:        11000 prevlsn:        12000 Asn:        28001 Version:           12
        // ---R       15000 length         1000 expectnext:        16000 highlsn:        13000 chkptlsn:        11000 prevlsn:        13000 Asn:        33001 Version:           13
        // ---R       16000 length         2000 expectnext:        18000 highlsn:        15000 chkptlsn:        11000 prevlsn:        15000 Asn:        33501 Version:           14     
        //
        status = blockFile->Transfer(KBlockFile::IoPriority::eForeground,
                                     KBlockFile::TransferType::eWrite,
                                     0x16000,
                                     *zeroBuffer,
                                     sync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        blockFile->Close();
        
    }

    //
    // Seed file 6: Log file with logical log records that are metadata
    //              only and metadata and data and wrapped around
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\Wrapped.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        const ULONG dataSizeMD = 5000;
        const ULONG dataSizeMO = 500;
        KBuffer::SPtr dataBufferMD;
        KBuffer::SPtr dataBufferMO;
        PUCHAR p;

        status = KBuffer::Create(dataSizeMD, dataBufferMD, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBufferMD->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSizeMD; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        status = KBuffer::Create(dataSizeMO, dataBufferMO, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBufferMO->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < dataSizeMO; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
    
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;

        for (ULONG j = 0; j < 64; j++)
        {
            logStream->Truncate(asn, asn);
            for (ULONG i = 0; i < 7; i++)
            {
                version++;
                status = WriteLogicalLogRecord(*log,
                                      *logStream,
                                      version,
                                      asn,
                                      *dataBufferMO,
                                      0);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataBufferMO->QuerySize();

                version++;
                status = WriteLogicalLogRecord(*log,
                                      *logStream,
                                      version,
                                      asn,
                                      *dataBufferMD,
                                      0);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataBufferMD->QuerySize();           
            }
            
        }
        
        log = nullptr;
        logStream = nullptr;
    }


    //
    // Seed file 7: Log file with randomly sized logical log records that are metadata
    //              only and metadata and data and at a log full
    //              condition
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\Logfull.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        const ULONG maxDataSize = 12 * 1024;
        KBuffer::SPtr dataBuffer;
        PUCHAR p;

        status = KBuffer::Create(maxDataSize, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBuffer->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < maxDataSize; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
        
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;
        ULONG dataWritten;

        status = STATUS_SUCCESS;
        while (NT_SUCCESS(status))
        {
            dataWritten = (rand() % 5000);
            
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;            
        }
        
        log = nullptr;
        logStream = nullptr;
    }


    //
    // Seed file 8: Log file with that has tail truncation
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\TailTruncate.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        const ULONG maxDataSize = 12 * 1024;
        KBuffer::SPtr dataBuffer;
        PUCHAR p;

        status = KBuffer::Create(maxDataSize, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBuffer->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < maxDataSize; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
        
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;
        ULONG dataWritten;

        {
            dataWritten = 500;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }

        asn -= 200;
        {
            dataWritten = 500;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }

        {
            dataWritten = 7325;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }

        asn -= 5419;
        {
            dataWritten = 5000;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }

        {
            dataWritten = 500;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }

        {
            dataWritten = 500;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }

        {
            dataWritten = 500;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }
        
        {
            dataWritten = 500;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }
        
        asn -= 1234;
        {
            dataWritten = 500;
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  dataWritten);
            VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || (NT_SUCCESS(status)));
            asn += dataWritten;
        }
        
        log = nullptr;
        logStream = nullptr;
    }


    //
    // Seed file 9: Data at the very end of the log
    //
    {
        KStringView fileName(L"\\??\\c:\\seeds\\AtEnd.log");
        RvdLog::SPtr log;
        KGuid logGuid;
        RvdLogStream::SPtr logStream;
        KGuid logStreamGuid;
        const ULONG maxDataSize = 5 * 1024;
        KBuffer::SPtr dataBuffer;
        PUCHAR p;

        status = KBuffer::Create(maxDataSize, dataBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        p = static_cast<PUCHAR>(dataBuffer->GetBuffer());

        srand((ULONG)GetTickCount());
        for (ULONG i = 0; i < maxDataSize; i++)
        {
            p[i] = static_cast<UCHAR>(rand() % 255);
        }
        
        logGuid = WellKnownGuid;        
        CreateLogFile(*logManager,
                      fileName,
                      logGuid,
                      log,
                      16 * 1024,        // MaxRecordSize
                      256 * 1024,       // LogSize
                      2);               // NumberStreams

        CreateLogStream(*log,
                        logStreamGuid,
                        logStream);

        ULONGLONG asn;
        ULONGLONG version;
        asn = RvdLogAsn::Min().Get();;
        version = 0;

        status = STATUS_SUCCESS;
        while (NT_SUCCESS(status))
        {
            version++;
            status = WriteLogicalLogRecord(*log,
                                  *logStream,
                                  version,
                                  asn,
                                  *dataBuffer,
                                  0);
            if (NT_SUCCESS(status))
            {
                asn += dataBuffer->QuerySize();
            }
        }
        
        VERIFY_IS_TRUE(status == STATUS_LOG_FULL);

        RvdLogAsn truncationAsn = 12000;
        logStream->Truncate(truncationAsn, truncationAsn);
        

        version++;
        status = WriteLogicalLogRecord(*log,
                              *logStream,
                              version,
                              asn,
                              *dataBuffer,
                              500);
        asn += dataBuffer->QuerySize();
        
        log = nullptr;
        logStream = nullptr;
    }
    
    
    
    logManager->Deactivate();
    activateSync.WaitForCompletion();   
    logManager.Reset(); 
}


NTSTATUS
TheMain(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    EventRegisterMicrosoft_Windows_KTL();

    KtlSystem* system = nullptr;
    NTSTATUS Result = KtlSystem::Initialize(FALSE, &system);
    KInvariant(NT_SUCCESS(Result));

    PreviousKInvariantCallout = SetKInvariantCallout(MyKInvariantCallout);
    
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();
    
    MakeSeedFiles();
    
    EventUnregisterMicrosoft_Windows_KTL();
    
    KtlSystem::Shutdown();

    return Result;
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
