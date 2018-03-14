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

#include "workerasync.h"

#if !defined(PLATFORM_UNIX)
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;
#endif

#include "VerifyQuiet.h"

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
 
extern KAllocator* g_Allocator;


VOID
ContainerLimitsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;
    ULONG metadataLength = 0x10000;
    static const ULONG MaxStreamsInContainer = 64;
    KArray<KtlLogStreamId> streamIds(*g_Allocator);

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    //
    // Create container and a stream within it
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             MaxStreamsInContainer,            // Max Number Streams
                                             0,            // Max Record Size
                                             KtlLogManager::FlagSparseFile,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Create streams until an error occurs
        //
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        KBuffer::SPtr securityDescriptor = nullptr;
        NTSTATUS statusDontCare;
        ULONG expectedStreamCount = MaxStreamsInContainer - 1;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONG streamCount = 0;
        while (NT_SUCCESS(status))
        {
            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

            createStreamAsync->Reuse();
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                        KtlLogInformation::KtlLogDefaultStreamType(),
                                                        nullptr,           // Alias
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        KtlLogManager::FlagSparseFile,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();

            if (NT_SUCCESS(status))
            {
                streamCount++;
                streamIds.Append(logStreamId);
                logStream->StartClose(NULL,
                                 closeStreamSync.CloseCompletionCallback());

                statusDontCare = closeStreamSync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(statusDontCare));
				logStream = nullptr;
            }
        }
        
        VERIFY_IS_TRUE(status == STATUS_INSUFFICIENT_RESOURCES);
        VERIFY_IS_TRUE(streamCount == expectedStreamCount);

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));             
    }


    {
        //
        // Reopen the container
        //
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        
        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(diskId,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Reopen the streams
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG openedMetadataLength;
            
            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            for (ULONG i = 0; i < streamIds.Count(); i++)
            {

                openStreamAsync->Reuse();
                openStreamAsync->StartOpenLogStream(streamIds[i],
                                                        &openedMetadataLength,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                        sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE(openedMetadataLength == metadataLength);

                logStream->StartClose(NULL,
                                 closeStreamSync.CloseCompletionCallback());

                status = closeStreamSync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
				logStream = nullptr;
            }
        }

        //
        // Close the container
        //
        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    {
        //
        // Reopen the container
        //
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        
        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(diskId,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Delete the stream
        //
        KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;

        status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        for (ULONG i = 0; i < streamIds.Count(); i++)
        {

            deleteStreamAsync->Reuse();
            deleteStreamAsync->StartDeleteLogStream(streamIds[i],
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }
}

VOID
WrapStreamFileTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    //
    // Create container and a stream within it
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = 64 * 1024 * 1024;
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
                                             KtlLogManager::FlagSparseFile,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;
            
            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                        KtlLogInformation::KtlLogDefaultStreamType(),
                                                        nullptr,           // Alias
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        logSize,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        KtlLogManager::FlagSparseFile,
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
			logStream = nullptr;
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;
            
            KtlLogAsn recordAsn = 0;

            KIoBuffer::SPtr dataIoBufferWritten;
            
            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Write a record
            //
			ULONG loops = 64;
			ULONG recordCount = 128;
			ULONG recordSize = 128 * 1024;

			ULONGLONG asn = KtlLogAsn::Min().Get();
			ULONGLONG version = 0;         
			KIoBuffer::SPtr iodata, metadata;
			PVOID iodataBuffer, metadataBuffer;
			KBuffer::SPtr workKBuffer;
			PUCHAR dataWritten;
			ULONG dataSizeWritten;
			ULONG offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);
            
            status = KIoBuffer::CreateSimple(recordSize,
                                             iodata,
                                             iodataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                             metadata,
                                             metadataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KBuffer::Create(recordSize,
                                     workKBuffer,
                                     *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
			
            dataWritten = (PUCHAR)workKBuffer->GetBuffer();
            dataSizeWritten = recordSize;

            for (ULONG i = 0; i < dataSizeWritten; i++)
            {
                dataWritten[i] = (i * 10) % 255;
            }           


			for (ULONG iLoops = 0; iLoops < loops; iLoops++)
			{
				ULONGLONG oldAsn = asn;
				for (ULONG iRecords = 0; iRecords < recordCount; iRecords++)
				{
					version++;
					status = SetupAndWriteRecord(logStream,
												 metadata,
												 iodata,
												 version,
												 asn,
												 dataWritten,
												 dataSizeWritten,
												 offsetToStreamBlockHeaderM);

					VERIFY_IS_TRUE(NT_SUCCESS(status));
					asn = asn + dataSizeWritten;					
				}

				TruncateStream(logStream, oldAsn);
				KNt::Sleep(1000);
			}
            
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
			logStream = nullptr;
        }

        //
        // Delete the stream
        //
        {
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;
            
            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));         
        
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }
}
