/*++

Copyright (c) Microsoft Corporation

Module Name:

    KtlLoggerTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KtlLoggerTests.h.
    2. Add an entry to the gs_KuTestCases table in KtlLoggerTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KtlLoggerTests.h"
#include <CommandLineParser.h>

#include <ktllogger.h>

#include "ServiceSync.h"

#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

#define ALLOCATION_TAG 'LLKT'
 
KAllocator* g_Allocator = nullptr;

#if KTL_USER_MODE

extern volatile LONGLONG gs_AllocsRemaining;

#endif

const LONGLONG DefaultTestLogFileSize = (LONGLONG)1024*1024*1024;

NTSTATUS FindDiskIdForDriveLetter(
    UCHAR DriveLetter,
    GUID& DiskIdGuid
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    

    KVolumeNamespace::VolumeInformationArray volInfo(*g_Allocator);
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, *g_Allocator, sync);
    if (!K_ASYNC_SUCCESS(status))
    {
        KTestPrintf("BasicLogCreateTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        return status;
    }
    status = sync.WaitForCompletion();
    
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicLogCreateTest: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        return(status);
    }

    // Find Drive volume GUID
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].DriveLetter == DriveLetter)
        {
            DiskIdGuid = volInfo[i].VolumeId;
            return(STATUS_SUCCESS);
        }
    }

    KTestPrintf("BasicLogCreateTest: KVolumeNamespace::QueryVolumeListEx did not return volume guid for drive %c: %i\n",
               DriveLetter,
               __LINE__);
    return(STATUS_UNSUCCESSFUL);
}

VOID
ManagerTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);
    
    //
    // Create a log container
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
        LONGLONG logSize = DefaultTestLogFileSize;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;
        
        status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
        KFatal(NT_SUCCESS(status));
        createAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));

        // Note that container is closed since last reference to
        // logContainer is lost in its destructor
    }

    //
    // Re-open the just created log container
    //
    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        KFatal(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(diskId,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));
        
        // Note that container is closed since last reference to
        // logContainer is lost in its destructor        
    }

    //
    // Delete the log container
    //
    {
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
        KSynchronizer sync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        KFatal(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));
        
        // Note that container is closed since last reference to
        // logContainer is lost in its destructor        
    }
}

VOID
ContainerTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;

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
        KFatal(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;
            
            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            KFatal(NT_SUCCESS(status));
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                    nullptr,
                                                    metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;
            
            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            KFatal(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            // TODO: Verify size of metadataLength
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }       

        //
        // Delete the stream
        //
        {
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;
            
            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            KFatal(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Setting logContainer to null will remove the reference and
        // close the container.
        //
        logContainer = nullptr;
        
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        KFatal(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));     
    }
}

VOID
StreamTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;

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
        KFatal(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;
            
            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            KFatal(NT_SUCCESS(status));
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                    nullptr,
                                                    metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;
            
            KtlLogAsn recordAsn = 42;
            ULONG myMetadataSize = 0x100;
            
            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            KFatal(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            // TODO: Verify size of metadataLength
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor

            //
            // Write a record
            //
            {
                KSynchronizer sync;
                
                //
                // Build a data buffer (8 different 64K elements)
                //
                ULONG dataBufferSize = 16 * 0x1000;
                KIoBuffer::SPtr dataIoBuffer;
                
                status = KIoBuffer::CreateEmpty(dataIoBuffer,
                                                *g_Allocator);
                KFatal(NT_SUCCESS(status));

                for (int i = 0; i < 8; i++)
                {
                    KIoBufferElement::SPtr dataElement;
                    VOID* dataBuffer;

                    status = KIoBufferElement::CreateNew(dataBufferSize,        // Size
                                                              dataElement,
                                                              dataBuffer,
                                                              *g_Allocator);
                    KFatal(NT_SUCCESS(status));
                    
                    for (int j = 0; j < 0x1000; j++)
                    {
                        ((PUCHAR)dataBuffer)[j] = (UCHAR)(i*j);
                    }
                    
                    dataIoBuffer->AddIoBufferElement(*dataElement);
                }
            
                
                //
                // Build metadata
                //
                ULONG metadataBufferSize = ((logStream->QueryReservedMetadataSize() + myMetadataSize) + 0xFFF) & ~(0xFFF);
                KIoBuffer::SPtr metadataIoBuffer;
                PVOID metadataBuffer;
                PUCHAR myMetadataBuffer;

                status = KIoBuffer::CreateSimple(metadataBufferSize,
                                                 metadataIoBuffer,
                                                 metadataBuffer,
                                                 *g_Allocator);
                KFatal(NT_SUCCESS(status));
                
                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    myMetadataBuffer[i] = (UCHAR)i;
                }
                                                 
                //
                // Now write the record
                //
                KtlLogStream::AsyncWriteContext::SPtr writeContext;
                
                status = logStream->CreateAsyncWriteContext(writeContext);
                KFatal(NT_SUCCESS(status));

                ULONGLONG version = 1;
                
                writeContext->StartWrite(recordAsn,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // Reservation
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));             
            }

            //
            // Read a record
            //
            {
                KtlLogStream::AsyncReadContext::SPtr readContext;
                
                status = logStream->CreateAsyncReadContext(readContext);
                KFatal(NT_SUCCESS(status));

                ULONGLONG version;
                ULONG metadataSize;
                KIoBuffer::SPtr dataIoBuffer;
                KIoBuffer::SPtr metadataIoBuffer;
                PUCHAR myMetadataBuffer;
                
                readContext->StartRead(recordAsn,
                                         &version,
                                         metadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                KFatal(version == 1);

                const VOID* metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    KFatal(myMetadataBuffer[i] == (UCHAR)i);
                }

                // TODO: Validate data buffer
                
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                KFatal(NT_SUCCESS(status));

                truncateContext->StartTruncate(recordAsn,
                                               recordAsn,
                                               NULL,       // ParentAsync
                                               sync);
                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));
            }           
        }

        //
        // Delete the stream
        //
        {
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;
            
            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            KFatal(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Setting logContainer to null will remove the reference and
        // close the container.
        //
        logContainer = nullptr;
        
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        KFatal(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));     
    }
}


VOID
ReadAsnTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;

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
        KFatal(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;
            
            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            KFatal(NT_SUCCESS(status));
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                    nullptr,
                                                    metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;
            
            KtlLogAsn recordAsn42 = 42;
            KtlLogAsn recordAsn96 = 96;
            KtlLogAsn recordAsn156 = 156;
            KtlLogAsn recordAsn112 = 112;
            KtlLogAsn recordAsnRead;
            ULONG myMetadataSize = 0x100;
            
            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            KFatal(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            // TODO: Verify size of metadataLength
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor

            //
            // Write records
            //
            {
                KSynchronizer sync;
                
                //
                // Build a data buffer (8 different 64K elements)
                //
                ULONG dataBufferSize = 16 * 0x1000;
                KIoBuffer::SPtr dataIoBuffer;
                
                status = KIoBuffer::CreateEmpty(dataIoBuffer,
                                                *g_Allocator);
                KFatal(NT_SUCCESS(status));

                for (int i = 0; i < 8; i++)
                {
                    KIoBufferElement::SPtr dataElement;
                    VOID* dataBuffer;

                    status = KIoBufferElement::CreateNew(dataBufferSize,        // Size
                                                              dataElement,
                                                              dataBuffer,
                                                              *g_Allocator);
                    KFatal(NT_SUCCESS(status));
                    
                    for (int j = 0; j < 0x1000; j++)
                    {
                        ((PUCHAR)dataBuffer)[j] = (UCHAR)(i*j);
                    }
                    
                    dataIoBuffer->AddIoBufferElement(*dataElement);
                }
            
                
                //
                // Build metadata
                //
                ULONG metadataBufferSize = ((logStream->QueryReservedMetadataSize() + myMetadataSize) + 0xFFF) & ~(0xFFF);
                KIoBuffer::SPtr metadataIoBuffer;
                PVOID metadataBuffer;
                PUCHAR myMetadataBuffer;

                status = KIoBuffer::CreateSimple(metadataBufferSize,
                                                 metadataIoBuffer,
                                                 metadataBuffer,
                                                 *g_Allocator);
                KFatal(NT_SUCCESS(status));
                
                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    myMetadataBuffer[i] = (UCHAR)i;
                }
                                                 
                //
                // Now write the records
                //
                KtlLogStream::AsyncWriteContext::SPtr writeContext;
                
                status = logStream->CreateAsyncWriteContext(writeContext);
                KFatal(NT_SUCCESS(status));

                ULONGLONG version = 1;
                
                writeContext->StartWrite(recordAsn42,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // Reservation
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));

                writeContext->Reuse();
                version = 2;
                writeContext->StartWrite(recordAsn96,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         NULL,    // Reservation
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));

                writeContext->Reuse();
                version = 3;
                writeContext->StartWrite(recordAsn156,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // Reservation
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));                             
            }

            //
            // Read a record
            //
            {
                KtlLogStream::AsyncReadContext::SPtr readContext;
                
                status = logStream->CreateAsyncReadContext(readContext);
                KFatal(NT_SUCCESS(status));

                ULONGLONG version;
                ULONG metadataSize;
                KIoBuffer::SPtr dataIoBuffer;
                KIoBuffer::SPtr metadataIoBuffer;
                PUCHAR myMetadataBuffer;
                
                readContext->StartRead(recordAsn42,
                                         &version,
                                         metadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                KFatal(version == 1);

                const VOID* metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    KFatal(myMetadataBuffer[i] == (UCHAR)i);
                }

                // TODO: Validate data buffer


                //
                // Now try ReadNext
                //
                readContext->Reuse();
                recordAsnRead = 0;
                readContext->StartReadNext(recordAsn42,
                                           &recordAsnRead,
                                         &version,
                                         metadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                KFatal(recordAsnRead == 96);
                KFatal(version == 2);

                metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    KFatal(myMetadataBuffer[i] == (UCHAR)i);
                }

                // TODO: Validate data buffer


                //
                // ReadNext at end of log
                //
                readContext->Reuse();
                recordAsnRead = 0;
                readContext->StartReadNext(recordAsn156,
                                           &recordAsnRead,
                                         &version,
                                         metadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(status == STATUS_NOT_FOUND);



                //
                // Now try ReadPrevious
                //
                readContext->Reuse();
                recordAsnRead = 0;
                readContext->StartReadPrevious(recordAsn156,
                                           &recordAsnRead,
                                         &version,
                                         metadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                KFatal(version == 2);
                KFatal(recordAsnRead == 96);

                metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    KFatal(myMetadataBuffer[i] == (UCHAR)i);
                }

                // TODO: Validate data buffer


                //
                // ReadPrevious at begin of log
                //
                readContext->Reuse();
                recordAsnRead = 0;
                readContext->StartReadPrevious(recordAsn42,
                                               &recordAsnRead,
                                         &version,
                                         metadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(status == STATUS_NOT_FOUND);
                
                
                //
                // Now try ReadContaining
                //
                readContext->Reuse();
                recordAsnRead = 0;
                readContext->StartReadContaining(recordAsn112,
                                               &recordAsnRead,
                                         &version,
                                         metadataSize,
                                         metadataIoBuffer,
                                         dataIoBuffer,
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                KFatal(version == 2);
                KFatal(recordAsnRead == 96);

                metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    KFatal(myMetadataBuffer[i] == (UCHAR)i);
                }

                // TODO: Validate data buffer
                
                
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                KFatal(NT_SUCCESS(status));

                truncateContext->StartTruncate(recordAsn96,
                                               recordAsn96,
                                               NULL,       // ParentAsync
                                               sync);
                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));
            }           
        }

        //
        // Delete the stream
        //
        {
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;
            
            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            KFatal(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Setting logContainer to null will remove the reference and
        // close the container.
        //
        logContainer = nullptr;
        
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        KFatal(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));     
    }
}

NTSTATUS CreateAndFillIoBuffer(
    UCHAR Entropy,
    ULONG BufferSize,
    KIoBuffer::SPtr& IoBuffer
)
{
    NTSTATUS status;    
    PVOID buffer;
    PUCHAR bufferU;
    
    status = KIoBuffer::CreateSimple(BufferSize,
                                     IoBuffer,
                                     buffer,
                                     *g_Allocator);
    KFatal(NT_SUCCESS(status));

    bufferU = static_cast<PUCHAR>(buffer);
    for (ULONG i = 0; i < BufferSize; i++)
    {
        bufferU[i] = (UCHAR)i + Entropy;
    }
    
    return(status);
}

NTSTATUS CompareKIoBuffers(
    KIoBuffer& IoBufferA,
    KIoBuffer& IoBufferB
)
{
    ULONG sizeA, sizeB;
    ULONG indexA, indexB;
    KIoBufferElement::SPtr elementA;
    KIoBufferElement::SPtr elementB;
    PUCHAR bufferA, bufferB;
    ULONG bytesToCompare;

    if (IoBufferA.QuerySize() != IoBufferB.QuerySize())
    {
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    elementA = IoBufferA.First();
    bufferA = (PUCHAR)(elementA->GetBuffer());
    sizeA = elementA->QuerySize();
    indexA = 0;
    
    elementB = IoBufferB.First();
    bufferB = (PUCHAR)(elementB->GetBuffer());
    sizeB = elementB->QuerySize();
    indexB = 0;

    bytesToCompare = IoBufferA.QuerySize();
    
    while(bytesToCompare)
    {
        ULONG len = __min(sizeA, sizeB);

        if (RtlCompareMemory(&bufferA[indexA], &bufferB[indexB], len) != len)
        {
            return(STATUS_UNSUCCESSFUL);
        }

        bytesToCompare -= len;
        if (bytesToCompare == 0)
        {
            break;
        }
        
        sizeA -= len;
        sizeB -= len;

        if (sizeA == 0)
        {
            elementA = IoBufferA.Next(*elementA);
            bufferA = (PUCHAR)(elementA->GetBuffer());
            indexA = 0;
            sizeA = elementA->QuerySize();
        } else {
            indexA += len;
        }

        if (sizeB == 0)
        {
            elementB = IoBufferB.Next(*elementB);
            bufferB = (PUCHAR)(elementB->GetBuffer());
            indexB = 0;
            sizeB = elementB->QuerySize();
        } else {
            indexB += len;
        }       
    }

    return(STATUS_SUCCESS);
}
                           


VOID
StreamMetadataTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;

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
        ULONG metadataLength = 0x10000;
        
        KIoBuffer::SPtr metadataWrittenIoBuffer;
        status = CreateAndFillIoBuffer(0xAB,
                                       metadataLength,
                                       metadataWrittenIoBuffer);
        KFatal(NT_SUCCESS(status));


        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
        
        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        KFatal(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            
            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            KFatal(NT_SUCCESS(status));
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                    nullptr,
                                                    metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));


            //
            // Write then read back the metadata
            //
            {                
                KtlLogStream::AsyncWriteMetadataContext::SPtr writeMetadataAsync;
                KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
                KIoBuffer::SPtr metadataReadIoBuffer;
                
                status = logStream->CreateAsyncWriteMetadataContext(writeMetadataAsync);
                KFatal(NT_SUCCESS(status));

                status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
                KFatal(NT_SUCCESS(status));
        
                
                writeMetadataAsync->StartWriteMetadata(metadataWrittenIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));

                
                readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));

                status = CompareKIoBuffers(*metadataWrittenIoBuffer, *metadataReadIoBuffer);
                KFatal(NT_SUCCESS(status));
            }

            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;
            
            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            KFatal(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            // TODO: Verify size of metadataLength
            //
            // BUGBUG: OpenLogStream does not return metadata length
            metadataLength = 0x10000;

            //
            // Read and verify the metadta
            //
            {
                KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
                KIoBuffer::SPtr metadataReadIoBuffer;
                
                status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
                KFatal(NT_SUCCESS(status));
                        
                readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));

                status = CompareKIoBuffers(*metadataWrittenIoBuffer, *metadataReadIoBuffer);
                KFatal(NT_SUCCESS(status));
            }
            //
            // Write then read back the metadata
            //
            {
                KtlLogStream::AsyncWriteMetadataContext::SPtr writeMetadataAsync;
                KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
                KIoBuffer::SPtr metadataReadIoBuffer;
                
                metadataWrittenIoBuffer = nullptr;
                status = CreateAndFillIoBuffer(0x2F,
                                               metadataLength,
                                               metadataWrittenIoBuffer);
                KFatal(NT_SUCCESS(status));
                
                status = logStream->CreateAsyncWriteMetadataContext(writeMetadataAsync);
                KFatal(NT_SUCCESS(status));

                status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
                KFatal(NT_SUCCESS(status));
        
                
                writeMetadataAsync->StartWriteMetadata(metadataWrittenIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));

                
                readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                KFatal(NT_SUCCESS(status));

                status = CompareKIoBuffers(*metadataWrittenIoBuffer, *metadataReadIoBuffer);
                KFatal(NT_SUCCESS(status));
            }
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }       

        
        //
        // Delete the stream
        //
        {
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;
            
            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            KFatal(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor        
        }

        //
        // Setting logContainer to null will remove the reference and
        // close the container.
        //
        logContainer = nullptr;
        
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        KFatal(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));     
    }
}

VOID
AliasTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    KString::SPtr persistedAlias = KString::Create(L"PersistedAliasIsInMyHead",
                                   *g_Allocator);
    KGuid persistedLogStreamGuid;
    KtlLogStreamId persistedLogStreamId;

    persistedLogStreamGuid.CreateNew();
    persistedLogStreamId = static_cast<KtlLogStreamId>(persistedLogStreamGuid);
    
    //
    // Create a log container
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
        LONGLONG logSize = DefaultTestLogFileSize;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;
        
        status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
        KFatal(NT_SUCCESS(status));
        createAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));


        //
        // Do simple alias work: Add, Resolve, Remove
        //

        //
        // Create a stream with an alias and then resolve for it
        //      
        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;
            
            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            KFatal(NT_SUCCESS(status));

            KGuid logStreamGuid;
            KtlLogStreamId logStreamId;

            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
                        
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                    persistedAlias,
                                                    metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            //
            // Now let's resolve the alias and verify
            //
            KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
            KtlLogStreamId resolvedLogStreamId;

            status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
            KFatal(NT_SUCCESS(status));

            resolveAsync->StartResolveAlias(persistedAlias,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            KFatal(resolvedLogStreamId == logStreamId);


            //
            // try with a name too long
            //
            KtlLogStreamId logStreamId2;

            KGuid logStreamGuid2;
            logStreamGuid2.CreateNew();
            logStreamId2 = static_cast<KtlLogStreamId>(logStreamGuid2);
            KString::SPtr aliasTooLong = KString::Create(L"0123456789012345678901234567890123456789012345678901234567890123456789",
                                                   *g_Allocator);
            KAssert(aliasTooLong != nullptr);
            
            createStreamAsync->Reuse();
            createStreamAsync->StartCreateLogStream(logStreamId2,
                                                    aliasTooLong,
                                                    metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            KFatal(status == STATUS_NAME_TOO_LONG);                                 

            
            // Note that stream is closed since last reference to
            // logStream is lost in its destructor                  
        }

        //
        // Some straight assign, resolve, remove work
        //
        {
            KtlLogContainer::AsyncAssignAliasContext::SPtr assignAsync;
            KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
            KtlLogContainer::AsyncRemoveAliasContext::SPtr removeAsync;
            KGuid logStreamGuid;
            KtlLogStreamId logStreamId;
            KtlLogStreamId resolvedLogStreamId;

            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

            KString::SPtr string = KString::Create(L"AliasIsInMyHead",
                                                   *g_Allocator);
            KAssert(string != nullptr);
                                            
            status = logContainer->CreateAsyncAssignAliasContext(assignAsync);
            KFatal(NT_SUCCESS(status));
            
            status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
            KFatal(NT_SUCCESS(status));
            
            status = logContainer->CreateAsyncRemoveAliasContext(removeAsync);
            KFatal(NT_SUCCESS(status));

            assignAsync->StartAssignAlias(string,
                                          logStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));
            
            resolveAsync->StartResolveAlias(string,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            KFatal(resolvedLogStreamId == logStreamId);
            
            removeAsync->StartRemoveAlias(string,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));


            //
            // Use max name length string
            //
            KString::SPtr aliasMaxLength = KString::Create(L"0123456789012345678901234567890123456789012345678901234",
                                                   *g_Allocator);
            KAssert(aliasMaxLength != nullptr);
            
            assignAsync->Reuse();
            assignAsync->StartAssignAlias(aliasMaxLength,
                                          logStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            resolveAsync->Reuse();
            resolveAsync->StartResolveAlias(aliasMaxLength,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            KFatal(resolvedLogStreamId == logStreamId);

                                
            //
            // try with a name too long
            //
            KString::SPtr aliasTooLong = KString::Create(L"0123456789012345678901234567890123456789012345678901234567890123456789",
                                                   *g_Allocator);
            KAssert(aliasTooLong != nullptr);
            
            assignAsync->Reuse();
            assignAsync->StartAssignAlias(aliasTooLong,
                                          logStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            KFatal(status == STATUS_NAME_TOO_LONG);                                 


            //
            // Remove a non existant alias
            //
            KString::SPtr aliasDontExist = KString::Create(L"ThisAliasDoesntExist",
                                                   *g_Allocator);
            KAssert(aliasTooLong != nullptr);
            removeAsync->Reuse();
            removeAsync->StartRemoveAlias(aliasDontExist,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            KFatal(status == STATUS_NOT_FOUND);
            
            //
            // Note that the assignment and closing of the log
            // container serves 3 purposes:
            // 1. Test that memory gets freed from the alias table when
            // the log container is closed
            // 2. When log container reopened then test that the alias
            // is recovered
            // 3. The previous assignment for this alias should be
            // overwritten
            //
            assignAsync->Reuse();
            assignAsync->StartAssignAlias(persistedAlias,
                                          persistedLogStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            KFatal(NT_SUCCESS(status));                     
        }
        
        // Note that container is closed since last reference to
        // logContainer is lost in its destructor
    }

    //
    // Re-open the just created log container
    //
    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        KFatal(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(diskId,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));

        //
        // Now let's resolve the alias and verify
        //
        KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
        KtlLogStreamId resolvedLogStreamId;

        status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
        KFatal(NT_SUCCESS(status));

        resolveAsync->StartResolveAlias(persistedAlias,
                                        &resolvedLogStreamId,
                                        NULL,          // ParentAsync
                                        sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));

        //
        // LogStreamId should resolve back to itself
        //
        KFatal(resolvedLogStreamId == persistedLogStreamId);
        
        // Note that container is closed since last reference to
        // logContainer is lost in its destructor        
    }

    //
    // Delete the log container
    //
    {
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
        KSynchronizer sync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        KFatal(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KFatal(NT_SUCCESS(status));
        
        // Note that container is closed since last reference to
        // logContainer is lost in its destructor        
    }
}

class AsyncAliasStressContext : public KAsyncContextBase
{
    K_FORCE_SHARED(AsyncAliasStressContext);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG _AllocationTag,
            __out AsyncAliasStressContext::SPtr& Context
        );

        VOID StartStress(
            __in KtlLogContainer::SPtr& LogContainer,
            __in ULONG RepeatCount,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );
        
    private:
        VOID AliasStressCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        );

        //
        // Initial -> CreateStream or AssignAlias
        // CreateStream -> StartVerifyAlias (repeat count not changed)
        // AssignAlias -> StartVerifyAlias  (repeat count not changed)
        // StartVerifyAlias -> VerifyAlias  (repeat count not changed)
        // VerifyAlias -> RemoveAlias or CreateStream or AssignAlias
        // RemoveAlias -> Initial
        // When RepeatCount == 0 then move to Completed
        //      
        enum FSMState { Initial, CreateStream, AssignAlias, StartVerifyAlias, VerifyAlias, RemoveAlias, Completed };
        
        FSMState _State;

        VOID NextState();
        
        VOID AliasStressFSM(
            __in NTSTATUS Status
        );
        
        VOID PickStreamAliasAndId(
            );
        
    protected:
        VOID OnStart();
        
        VOID OnReuse();

    private:
        //
        // Parameters
        //
        KtlLogContainer::SPtr _LogContainer;
        ULONG _RepeatCount;

        //
        // Internal
        //
        ULONG _AllocationTag;
        KtlLogStreamId _LogStreamId;
        KtlLogStreamId _ResolvedLogStreamId;
        KString::SPtr _Alias;
        KtlLogStream::SPtr _LogStream;
};

        
VOID AsyncAliasStressContext::AliasStressCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
)
{
    AliasStressFSM(Async.Status());
}

VOID AsyncAliasStressContext::NextState(
    )
{
    if ((_State == CreateStream) || (_State == AssignAlias))
    {
        _State = StartVerifyAlias;
        return;
    }

    if (_State == StartVerifyAlias)
    {
        _State = VerifyAlias;
        return;
    }

    if (_RepeatCount-- == 0)
    {
        _State = Completed;
        return;
    }

    if ((_State == Initial) || (_State == RemoveAlias))
    {
        if (_State == Initial)
        {
            srand((ULONG)this);
        }

        ULONG r = rand() % 4;
        if (r == 0)
        {
            _State = CreateStream;
        } else {
            _State = AssignAlias;
        }
        return;
    }

    if (_State == VerifyAlias)
    {
#if 0
        ULONG r = rand() % 2;

        switch(r)
        {
            case 0:             
            {
                _State = RemoveAlias;
                break;
            }

            case 1:
            {
                _State = AssignAlias;
            }
        }
#else
        _State = RemoveAlias;
#endif
    }
}

VOID AsyncAliasStressContext::PickStreamAliasAndId(
    )
{
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;
    BOOLEAN b;

    _Alias = nullptr;
    
    logStreamGuid.CreateNew();
    _LogStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    _Alias = KString::Create(*g_Allocator,
                             KtlLogContainer::MaxAliasLength);
    KFatal(_Alias);

    b = _Alias->FromULONGLONG( ((ULONGLONG)(this) + _RepeatCount),
                              TRUE);   // In hex
    KFatal(b);
}

VOID AsyncAliasStressContext::AliasStressFSM(
    __in NTSTATUS Status
)
{
    KAsyncContextBase::CompletionCallback completion(this, &AsyncAliasStressContext::AliasStressCompletion);
    
    if (! NT_SUCCESS(Status))
    {
        _LogContainer = nullptr;
        _Alias = nullptr;
        Complete(Status);
        return;
    }

    do
    {
        switch (_State)
        {
            case Initial:
            {
                NextState();
                break;
            }

            case CreateStream:
            {
                KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
                ULONG metadataLength = 0x10000;

                PickStreamAliasAndId();
                
                Status = _LogContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
                KFatal(NT_SUCCESS(Status));

                _LogStream = nullptr;
                createStreamAsync->StartCreateLogStream(_LogStreamId,
                                                        _Alias,
                                                        metadataLength,
                                                        _LogStream,
                                                        this,
                                                        completion);

                NextState();
                return;
            }

            case AssignAlias:
            {
                KtlLogContainer::AsyncAssignAliasContext::SPtr assignAsync;
                
                PickStreamAliasAndId();
                
                Status = _LogContainer->CreateAsyncAssignAliasContext(assignAsync);
                KFatal(NT_SUCCESS(Status));

                assignAsync->StartAssignAlias(_Alias,
                                              _LogStreamId,
                                              this,
                                              completion);              
                NextState();                
                return;
            }

            case StartVerifyAlias:
            {
                KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
                
                Status = _LogContainer->CreateAsyncResolveAliasContext(resolveAsync);
                KFatal(NT_SUCCESS(Status));
                
                resolveAsync->StartResolveAlias(_Alias,
                                                &_ResolvedLogStreamId,
                                                this,
                                                completion);
            
                NextState();                
                return;
            }

            case VerifyAlias:
            {
                KFatal(_ResolvedLogStreamId == _LogStreamId);
            
                NextState();                
                break;
            }

            case RemoveAlias:
            {
                KtlLogContainer::AsyncRemoveAliasContext::SPtr removeAsync;
                
                Status = _LogContainer->CreateAsyncRemoveAliasContext(removeAsync);
                KFatal(NT_SUCCESS(Status));

                removeAsync->StartRemoveAlias(_Alias,
                                              this,
                                              completion);
            
                NextState();                
                return;
            }

            case Completed:
            {
                _LogContainer = nullptr;
                _Alias = nullptr;
                Complete(STATUS_SUCCESS);
                return;
            }

            default:
            {
                KAssert(FALSE);

                Status = STATUS_UNSUCCESSFUL;
                _LogContainer = nullptr;
                _Alias = nullptr;
                Complete(Status);
                return;
            }
        }
    } while (TRUE);
}

VOID AsyncAliasStressContext::OnStart()
{   
    AliasStressFSM(STATUS_SUCCESS); 
}


VOID AsyncAliasStressContext::StartStress(
    __in KtlLogContainer::SPtr& LogContainer,
    __in ULONG RepeatCount,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;

    _LogContainer = LogContainer;
    _RepeatCount = RepeatCount;
    Start(ParentAsyncContext, CallbackPtr);
}
        

AsyncAliasStressContext::AsyncAliasStressContext()
{
}

AsyncAliasStressContext::~AsyncAliasStressContext()
{
}

VOID AsyncAliasStressContext::OnReuse()
{
}

NTSTATUS AsyncAliasStressContext::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out AsyncAliasStressContext::SPtr& Context
    )
{
    NTSTATUS status;
    AsyncAliasStressContext::SPtr context;
    
    context = _new(AllocationTag, Allocator) AsyncAliasStressContext();
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
    
    context->_AllocationTag = AllocationTag;
    context->_Alias = nullptr;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);     
}

#pragma prefix(push)
#pragma prefix(disable: 6262, "Function uses 6336 bytes of stack space")
VOID
AliasStress(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    KFatal(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    KFatal(NT_SUCCESS(status));


    {
        const ULONG numberAsyncs = 64;
        const ULONG numberRepeats = 8;
        KSynchronizer syncArr[numberAsyncs];
        AsyncAliasStressContext::SPtr asyncAliasStress[numberAsyncs];

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = AsyncAliasStressContext::Create(*g_Allocator,
                                                     ALLOCATION_TAG,
                                                     asyncAliasStress[i]);
            KFatal(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            asyncAliasStress[i]->StartStress(logContainer,
                                          numberRepeats, // Repeat count
                                          NULL,          // ParentAsync
                                          syncArr[i]);
        }
        
        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = syncArr[i].WaitForCompletion();
            KFatal(NT_SUCCESS(status));
        }
    }   
}
#pragma prefix(enable: 6262, "Function uses 6336 bytes of stack space")
#pragma prefix(pop)

NTSTATUS
TestSequence()
{
    NTSTATUS status = STATUS_SUCCESS;
    KtlLogManager::SPtr logManager;
    KGuid diskId;

    status = FindDiskIdForDriveLetter('C', diskId);
    KFatal(NT_SUCCESS(status));

    //
    // Access the log manager
    //
    status = KtlLogManager::Create(ALLOCATION_TAG, *g_Allocator, logManager);
    KFatal(NT_SUCCESS(status));

    KServiceSynchronizer        sync;
    status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                             sync.OpenCompletionCallback());
    KFatal(NT_SUCCESS(status));     
    status = sync.WaitForCompletion();
    KFatal(NT_SUCCESS(status));


    ManagerTest(diskId, logManager);
    ContainerTest(diskId, logManager);
    StreamTest(diskId,logManager);

    ReadAsnTest(diskId,logManager);

    StreamMetadataTest(diskId, logManager);

    AliasTest(diskId, logManager);

    AliasStress(diskId, logManager);
    
    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              sync.CloseCompletionCallback());
    KFatal(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    KFatal(NT_SUCCESS(status));
   
    
    return(status);
}

NTSTATUS
KtlLoggerTest(
    __in int argc, __in WCHAR* args[]
    )
{
    NTSTATUS Result;
    KtlSystem* system;

    KTestPrintf("KtlLoggerTest: STARTED\n");

    Result = KtlSystem::Initialize(&system);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    system->SetStrictAllocationChecks(TRUE);
    
    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

    if (NT_SUCCESS(Result))
    {
       Result = TestSequence();
    }

    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %d\n", Leaks);
//        KtlSystemBase::DebugOutputKtlSystemBases();
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KtlLoggerTest: COMPLETED\n");
    return Result;
}



#if CONSOLE_TEST
int
wmain(__in int argc, __in WCHAR* args[])
{
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KtlLoggerTest(argc, args);
}
#endif


