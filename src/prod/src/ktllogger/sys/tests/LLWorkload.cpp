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

#include "KtlLoggerTests.h"

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"
#endif

#include "CloseSync.h"

#if !defined(PLATFORM_UNIX)
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;
#endif

#ifdef BUILD_IN_CONTROLLER_HOST
//
// This is being built within a controller host so define this so
// results can be returned
//
#define RETURN_VERIFY_RESULTS_TO_CONTROLLER 1
#else
#include "VerifyQuiet.h"
#endif

#include "ControllerHost.h"
#include "LLWorkload.h"

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

 extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

#define ALLOCATION_TAG 'LLKT'
 
extern KAllocator* g_Allocator;

#if KTL_USER_MODE

extern volatile LONGLONG gs_AllocsRemaining;

#endif

#include "TestUtil.h"

NTSTATUS StartLLWorkload(    
    __in KtlLogManager::SPtr logManager,
    __in KGuid DiskId,
    __in KtlLogContainerId LogContainerId,
    __in ULONG TestDurationInSeconds,
    __in ULONG MaxRecordsWrittenPerCycle,
    __in ULONGLONG StreamSize,
    __in ULONG MaxWriteLatencyInMs,
    __in ULONG MaxAverageWriteLatencyInMs,
    __in ULONG DelayBetweenWritesInMs,
    __out LLWORKLOADSHAREDINFO* llWorkloadInfo
    )
{
    NTSTATUS status;
    KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    TestIPC::TestIPCResult* ipcResult = &llWorkloadInfo->Results;

    UNREFERENCED_PARAMETER(ipcResult);
    
    srand((ULONG)GetTickCount());

    //
    // Establish reasonable defaults
    //
    if (MaxRecordsWrittenPerCycle == 0)
    {
        MaxRecordsWrittenPerCycle = 0x8000;
    }
    
    if (StreamSize == 0)
    {
        StreamSize = DEFAULT_STREAM_SIZE/2;
    }

    if (MaxWriteLatencyInMs == 0)
    {
        //
        // Indicates no checking of write latency
        //
        MaxWriteLatencyInMs = 0xffffffff;
    }

    if (MaxAverageWriteLatencyInMs == 0)
    {
        //
        // Indicates no checking of average write latency
        //
        MaxAverageWriteLatencyInMs = 0xffffffff;
    }
    
    ULONG maxRecordSize = DEFAULT_MAX_RECORD_SIZE / 4;

    llWorkloadInfo->AverageWriteLatencyInMs = 0;
    llWorkloadInfo->HighestWriteLatencyInMs = 0;
        
    //
    // Open up our log container
    //
    status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    openAsync->StartOpenLogContainer(DiskId,
                                         LogContainerId,
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
    
    //
    // Create our logical log
    //
    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;
        KBuffer::SPtr securityDescriptor = nullptr;
        KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamAsync->StartCreateLogStream(logStreamId,
                                                logStreamType,
                                                nullptr,           // Alias
                                                nullptr,
                                                securityDescriptor,
                                                metadataLength,
                                                StreamSize,
                                                DEFAULT_MAX_RECORD_SIZE/2,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Verify that log stream id is set correctly
        //
        KtlLogStreamId queriedLogStreamId;
        logStream->QueryLogStreamId(queriedLogStreamId);

        VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    }

    //
    // Setup for long running tests
    //
    KIoBuffer::SPtr emptyIoBuffer;

    PUCHAR dataWritten;
    ULONG dataSizeWritten;

    KIoBuffer::SPtr iodata;
    PVOID iodataBuffer;
    KIoBuffer::SPtr metadata;
    PVOID metadataBuffer;
    KBuffer::SPtr workKBuffer;
    PUCHAR workBuffer;

    ULONG offsetToStreamBlockHeaderM;
    ULONG offsetToStreamBlockHeaderI;

    status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KIoBuffer::CreateSimple(maxRecordSize,
                                     iodata,
                                     iodataBuffer,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                     metadata,
                                     metadataBuffer,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KBuffer::Create(maxRecordSize,
                             workKBuffer,
                             *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    workBuffer = (PUCHAR)workKBuffer->GetBuffer();
    dataWritten = workBuffer;
    dataSizeWritten = maxRecordSize;

    for (ULONG i = 0; i < dataSizeWritten; i++)
    {
        dataWritten[i] = (i * 10) % 255;
    }           

    
    //
    // Setup and perform the long running tests
    //
    KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
    status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    KtlLogAsn headAsn;
    
    KLogicalLogInformation::StreamBlockHeader* lastStreamBlockHeader;
    KLogicalLogInformation::StreamBlockHeader emptyStreamBlockHeader;
    RtlZeroMemory(&emptyStreamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));
    lastStreamBlockHeader = &emptyStreamBlockHeader;

    //
    // Timebound this test
    //
    ULONGLONG endTime;
    if (TestDurationInSeconds == 0)
    {
        endTime = 0xffffffffffffffff;
    } else {        
        endTime = GetTickCount64() + (TestDurationInSeconds * 1000);
    }
    
    while ((asn < 0xffffffffff000000) && (GetTickCount64() < endTime))
    {
        //
        // First step is to open the log stream
        //
        KtlLogStream::SPtr logStream;
        ULONG openedMetadataLength;

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &openedMetadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);
        offsetToStreamBlockHeaderI = KLogicalLogInformation::FixedMetadataSize;
        
        //
        // Recover the head, tail and highest opertion id along with
        // verifying the last written context and the log stream id
        //
        VerifyTailAndVersion(logStream,
                             version,
                             asn,
                             NULL    // TODO: remember last header
                             );     
        
        KtlLogStreamId queriedLogStreamId;
        logStream->QueryLogStreamId(queriedLogStreamId);
        VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

        status = GetStreamTruncationPoint(logStream,
                                          headAsn);     
        VERIFY_IS_TRUE(NT_SUCCESS(status));
                                  
        
        //
        // Next read some part of the tail of the log
        //
        if (asn > headAsn.Get())
        {
            ULONGLONG written = (asn - headAsn.Get());
            ULONGLONG amountToReadU = (written / 50);
            ULONGLONG readFrom = asn - amountToReadU;
            ULONGLONG truncateTail = asn - (amountToReadU / 2);
            ULONG amountToRead = (ULONG)amountToReadU;

            while (amountToRead > 0)
            {
                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;
                ULONG amountRead;
                
                status = ReadContainingRecord(logStream,
                                              readFrom,
                                              versionRead,
                                              metadataSizeRead,
                                              dataIoBufferRead,
                                              metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                {
                    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
                    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
                    ULONG dataRead;
                    ULONG dataSizeRead;
                    
                    KLogicalLogInformation::FindLogicalLogHeaders((PUCHAR)metadataIoBufferRead->First()->GetBuffer(),
                                                                 *dataIoBufferRead,
                                                                 logStream->QueryReservedMetadataSize(),
                                                                 metadataBlockHeader,
                                                                 streamBlockHeader,
                                                                 dataSizeRead,
                                                                 dataRead);
                
                    amountRead = dataSizeRead;
                }

                if (amountRead > amountToRead)
                {
                    amountRead = amountToRead;
                }
                
                readFrom += amountRead;
                amountToRead -= amountRead;
            }
            
            //
            // Truncate the tail of the log
            //
            version++;
            asn = truncateTail;
            dataSizeWritten = 0;

            //
            // CONSIDER: Do we need to worry about latency for this
            // write ?
            //
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            llWorkloadInfo->TotalBytesWritten += (KLogicalLogInformation::FixedMetadataSize);
        }

        //
        // Now setup for the long running write/truncate head phase
        //

        //
        // Establish a truncation notification callback
        //
        KSynchronizer completionSignal;
        KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext;
        status = logStream->CreateAsyncStreamNotificationContext(notificationContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        
        notificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                      25,                  // 25% utilization
                                                      NULL,
                                                      completionSignal);
        
        
        ULONG numberOfRecords = rand() % MaxRecordsWrittenPerCycle;
        ULONGLONG latencyCounter = 0;
        ULONGLONG latency;
        
        for (ULONG i = 0; i < numberOfRecords; i++)
        {
            //
            // First check if any head truncation is needed
            //
            BOOLEAN isCompleted;
            status = completionSignal.WaitForCompletion(0, &isCompleted);

            VERIFY_IS_TRUE((NT_SUCCESS(status) || (status == STATUS_PENDING) || (status == STATUS_IO_TIMEOUT)) ? true : false);

            if (status == STATUS_SUCCESS)
            {
                //
                // truncation notification fired so take care of
                // truncating and then repriming the notification
                //
                status = GetStreamTruncationPoint(logStream,
                                                  headAsn);     
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                ULONGLONG truncateHeadAsn;
                ULONGLONG amountToTruncate;

                //
                // truncate about half of the log
                //
                amountToTruncate = 3 * ((asn - headAsn.Get()) / 4);
                truncateHeadAsn = headAsn.Get() + amountToTruncate;

				printf("Truncate at %llx\n", truncateHeadAsn);
				
                status = TruncateStream(logStream,
                                        truncateHeadAsn);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

				KNt::Sleep(2000);
				
                notificationContext->Reuse();
                notificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                              25,                  // 25% utilization
                                                              NULL,
                                                              completionSignal);                
            }

            //
            // Next go ahead and write our record
            //
            KNt::Sleep(DelayBetweenWritesInMs);

            ULONG dataSizeWritten2 = rand() % maxRecordSize;
            version++;

            status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             iodata,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten2,
                                             offsetToStreamBlockHeaderM,
                                             TRUE,
                                             &latency);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            llWorkloadInfo->TotalBytesWritten += (KLogicalLogInformation::FixedMetadataSize + iodata->QuerySize());

            if (latency > llWorkloadInfo->HighestWriteLatencyInMs)
            {
                llWorkloadInfo->HighestWriteLatencyInMs = latency;
            }
            
            latencyCounter += latency;
            llWorkloadInfo->AverageWriteLatencyInMs = (latencyCounter) / (i+1);

            VERIFY_IS_TRUE(latency <= MaxWriteLatencyInMs);
            VERIFY_IS_TRUE(llWorkloadInfo->AverageWriteLatencyInMs <= MaxAverageWriteLatencyInMs);
            
            asn += dataSizeWritten2;
            
        }

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Ensure the truncation notification gets completed before
        // proceeding
        //
        completionSignal.WaitForCompletion(INFINITE);        
    };

    //
    // All done, close up the log container
    //
    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS);

    return(STATUS_SUCCESS);
}

