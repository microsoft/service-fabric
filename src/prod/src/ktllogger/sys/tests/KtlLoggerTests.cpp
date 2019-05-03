// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(PLATFORM_UNIX)
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

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

#include "RWTStress.h"

#if !defined(PLATFORM_UNIX)
#include "WexTestClass.h"
#endif

#include "CloseSync.h"

#include "ControllerHost.h"
#include "LLWorkload.h"

#include "AsyncInterceptor.h"
#include "workerasync.h"

#include <InternalKtlLogger.h>

#if !defined(PLATFORM_UNIX)
using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;
#endif

#include "VerifyQuiet.h"

#include "TestUtil.h"

#include "lwtasync.h"

extern "C" ULONG LoadDriver();
extern "C" ULONG UnloadDriver();

#if KTL_USER_MODE
 #define _printf(...)   printf(__VA_ARGS__)
// #define _printf(...)   KDbgPrintf(__VA_ARGS__)

extern volatile LONGLONG gs_AllocsRemaining;
#else
 #define _printf(...)   KDbgPrintf(__VA_ARGS__)
 #define wprintf(...)    KDbgPrintf(__VA_ARGS__)
#endif

#define ALLOCATION_TAG 'LLKT'

KAllocator* g_Allocator = nullptr;

class CompletionCounter : public KObject<CompletionCounter>
{
    K_DENY_COPY(CompletionCounter);

public:

    FAILABLE
    CompletionCounter()
    {
        _Callback.Bind(this, &CompletionCounter::AsyncCompletion);
        SuccessCounter = 0;
        FailCounter = 0;
    }

    ~CompletionCounter()
    {
    }

    KAsyncContextBase::CompletionCallback CompletionCallback()
    {
        return(_Callback);
    }

    operator KAsyncContextBase::CompletionCallback()
    {
        return(_Callback);
    }

    ULONG GetCount()
    {
        return(SuccessCounter + FailCounter);
    }


    VOID
    Reset()
    {
        SuccessCounter = 0;
        FailCounter = 0;
    }


protected:
    VOID
    AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
    )
    {
        UNREFERENCED_PARAMETER(Parent);

        if (NT_SUCCESS(CompletingOperation.Status()))
        {
            InterlockedIncrement(&SuccessCounter);
        } else {
            InterlockedIncrement(&FailCounter);
        }
    }

private:
    volatile LONG SuccessCounter;
    volatile LONG FailCounter;
    KAsyncContextBase::CompletionCallback _Callback;
};


#if !defined(PLATFORM_UNIX)
NTSTATUS FindDiskIdForDriveLetter(
    UCHAR& DriveLetter,
    GUID& DiskIdGuid
    )
{
    NTSTATUS status;
    
    KSynchronizer sync;

    if (DriveLetter == 0)
    {
        CHAR systemDrive[32];
        ULONG result = ExpandEnvironmentStringsA("%SYSTEMDRIVE%", (LPSTR)systemDrive, 32);
        VERIFY_ARE_NOT_EQUAL((ULONG)0, result, L"Failed to get systemdrive");
        VERIFY_IS_TRUE(result <= 32, L"Failed to get systemdrive");

        DriveLetter = systemDrive[0];
    }

    KVolumeNamespace::VolumeInformationArray volInfo(*g_Allocator);
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, *g_Allocator, sync);
    if (!K_ASYNC_SUCCESS(status))
    {
        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"KVolumeNamespace::QueryVolumeListEx Failed");
        return status;
    }
    status = sync.WaitForCompletion();

    if (!NT_SUCCESS(status))
    {
        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"KVolumeNamespace::QueryVolumeListEx Failed");
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

    return(STATUS_UNSUCCESSFUL);
}
#endif

NTSTATUS CompareKIoBuffersA(
    __in KIoBuffer& IoBufferA,
    __in KIoBuffer& IoBufferB
)
{
    if (IoBufferA.QuerySize() != IoBufferB.QuerySize())
    {
        return(STATUS_UNSUCCESSFUL);
    }

    KIoBufferElement* ioBufferElementA;
    ioBufferElementA = IoBufferA.First();
    SIZE_T offsetA = 0;

    KIoBufferElement* ioBufferElementB;
    ioBufferElementB = IoBufferB.First();
    SIZE_T offsetB = 0;

    SIZE_T bytesToCompare = IoBufferA.QuerySize();

    while (bytesToCompare > 0)
    {
        SIZE_T l;
        SIZE_T segmentToCompare = min( (ioBufferElementA->QuerySize() - offsetA),
                                      (ioBufferElementB->QuerySize() - offsetB) );

        l = RtlCompareMemory((PVOID)((PUCHAR)ioBufferElementA->GetBuffer() + offsetA),
                             (PVOID)((PUCHAR)ioBufferElementB->GetBuffer() + offsetB),
                             segmentToCompare);
        if (l != segmentToCompare)
        {
            return(STATUS_UNSUCCESSFUL);
        }

        offsetA += segmentToCompare;
        offsetB += segmentToCompare;

        if (offsetA == ioBufferElementA->QuerySize())
        {
            ioBufferElementA = IoBufferA.Next(*ioBufferElementA);
            offsetA = 0;
        }

        if (offsetB == ioBufferElementB->QuerySize())
        {
            ioBufferElementB = IoBufferB.Next(*ioBufferElementB);
            offsetB = 0;
        }

        bytesToCompare -= segmentToCompare;
    }

    KAssert(ioBufferElementA == NULL);
    KAssert(ioBufferElementB == NULL);

    return(STATUS_SUCCESS);
}


NTSTATUS PrepareRecordForWrite(
    __in KtlLogStream::SPtr& logStream,
    __in ULONGLONG version,
    __in KtlLogAsn recordAsn,
    __in ULONG myMetadataSize, // = 0x100;
    __in ULONG dataBufferBlocks, // 8
    __in ULONG dataBufferBlockSize, // 16* 0x1000
    __out KIoBuffer::SPtr& dataIoBuffer,
    __out KIoBuffer::SPtr& metadataIoBuffer
    )
{
    UNREFERENCED_PARAMETER(version);
    UNREFERENCED_PARAMETER(recordAsn);

    NTSTATUS status;

    //
    // Write records
    //
    {
        //
        // Build a data buffer (In different elements)
        //
        ULONG dataBufferSize = dataBufferBlockSize;

        status = KIoBuffer::CreateEmpty(dataIoBuffer,
                                        *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < dataBufferBlocks; i++)
        {
            KIoBufferElement::SPtr dataElement;
            VOID* dataBuffer;

            status = KIoBufferElement::CreateNew(dataBufferSize,        // Size
                                                      dataElement,
                                                      dataBuffer,
                                                      *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (int j = 0; j < 0x1000; j++)
            {
                ((PUCHAR)dataBuffer)[j] = (UCHAR)(i*j);
            }

            dataIoBuffer->AddIoBufferElement(*dataElement);
        }


        if (metadataIoBuffer == nullptr)
        {
            //
            // Build metadata
            //
            ULONG metadataBufferSize = ((logStream->QueryReservedMetadataSize() + myMetadataSize) + 0xFFF) & ~(0xFFF);
            PVOID metadataBuffer;
            PUCHAR myMetadataBuffer;

            status = KIoBuffer::CreateSimple(metadataBufferSize,
                                             metadataIoBuffer,
                                             metadataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
            for (ULONG i = 0; i < myMetadataSize; i++)
            {
                myMetadataBuffer[i] = (UCHAR)i;
            }
        }
    }

    return(status);
}


NTSTATUS WriteRecord(
    KtlLogStream::SPtr& logStream,
    ULONGLONG version,
    KtlLogAsn recordAsn,
    KIoBuffer::SPtr IoBuffer,
    ULONGLONG ReservationToUse,
    KIoBuffer::SPtr MetadataBuffer,
    ULONG metadataIoSize,
    ULONGLONG* WriteLatencyInMs
    )
{
    NTSTATUS status;
    ULONGLONG startTime, endTime;

    //
    // Write records
    //
    {
        KSynchronizer sync;

        //
        // Now write the records
        //
        KtlLogStream::AsyncWriteContext::SPtr writeContext;

        status = logStream->CreateAsyncWriteContext(writeContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        startTime = GetTickCount64();
        writeContext->StartWrite(recordAsn,
                                 version,
                                 metadataIoSize,
                                 MetadataBuffer,
                                 IoBuffer,
                                 ReservationToUse,    // Reservation
                                 NULL,    // ParentAsync
                                 sync);

        status = sync.WaitForCompletion();

        endTime = GetTickCount64();
        if (WriteLatencyInMs != NULL)
        {
            *WriteLatencyInMs = (endTime - startTime);
        }
    }

    return(status);
}


NTSTATUS WriteRecord(
    KtlLogStream::SPtr& logStream,
    ULONGLONG version,
    KtlLogAsn recordAsn,
    ULONG DataBufferBlocks,
    ULONG DataBufferBlockSize,
    ULONGLONG ReservationToUse,
    KIoBuffer::SPtr metadataIoBuffer,
    ULONG metadataIoSize,
    ULONGLONG* WriteLatencyInMs
    )
{
    NTSTATUS status;
    ULONGLONG startTime, endTime;

    //
    // Write records
    //
    {
        KSynchronizer sync;
        KIoBuffer::SPtr dataIoBuffer;
        ULONG myMetadataSize = 0x100;

        if (metadataIoBuffer == nullptr)
        {
            metadataIoSize = logStream->QueryReservedMetadataSize() + myMetadataSize;
        }

        status = PrepareRecordForWrite(logStream,
                                       version,
                                       recordAsn,
                                       myMetadataSize,// myMetadataSize
                                       DataBufferBlocks,
                                       DataBufferBlockSize,
                                       dataIoBuffer,
                                       metadataIoBuffer);

        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Now write the records
        //
        KtlLogStream::AsyncWriteContext::SPtr writeContext;

        status = logStream->CreateAsyncWriteContext(writeContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        startTime = GetTickCount64();
        writeContext->StartWrite(recordAsn,
                                 version,
                                 metadataIoSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 ReservationToUse,    // Reservation
                                 NULL,    // ParentAsync
                                 sync);

        status = sync.WaitForCompletion();

        endTime = GetTickCount64();
        if (WriteLatencyInMs != NULL)
        {
            *WriteLatencyInMs = (endTime - startTime);
        }
    }

    return(status);
}

NTSTATUS TruncateStream(
    KtlLogStream::SPtr& logStream,
    KtlLogAsn recordAsn
)
{
    NTSTATUS status;
    KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncTruncateContext(truncateContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    truncateContext->Truncate(recordAsn,
                                   recordAsn);
    return(STATUS_SUCCESS);
}

NTSTATUS ExtendReservation(
    KtlLogStream::SPtr& logStream,
    ULONG ReservationSize
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncReservationContext::SPtr reservationContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncReservationContext(reservationContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"CreateAsyncReservationContext failed");

    reservationContext->StartExtendReservation(ReservationSize,
                                               nullptr,
                                               sync);

    status = sync.WaitForCompletion();
    return(status);
}

NTSTATUS ContractReservation(
    KtlLogStream::SPtr& logStream,
    ULONG ReservationSize
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncReservationContext::SPtr reservationContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncReservationContext(reservationContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"CreateAsyncReservationContext failed");

    reservationContext->StartContractReservation(ReservationSize,
                                               nullptr,
                                               sync);

    status = sync.WaitForCompletion();
    return(status);
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
    ContainerCloseSynchronizer closeSync;

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
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
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


    //
    // Re-open the just created log container
    //
    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(diskId,
                                             logContainerId,
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

    //
    // Delete the log container
    //
    {
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
        KSynchronizer sync;

        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Now test delete with log container open
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;

        LONGLONG logSize = DefaultTestLogFileSize;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;

        //
        // Create log container
        status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_SUCCESS);


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
        // Delete the log container
        //
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }
}

VOID
CorruptedLCMBInfoTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    )
{
#if 0
    //
    // Save this code for testing reading of a specific corrupted log
    // container
    //
    UNREFERENCED_PARAMETER(driveLetter);
    
    NTSTATUS status;
    ContainerCloseSynchronizer closeSync;
    KString::SPtr containerFullPathName;
    KStringView containerName(L"\\??\\C:\\savelogs\\{438319b8-4f8f-45bd-a59b-1228faddb956}\\{84ac856c-320e-482b-b998-73c2b7432c9b}.LogContainer");
    GUID nullGUID = { 0 };
    BOOLEAN b;
    KtlLogContainerId logContainerId = nullGUID;
    KSynchronizer sync;
    
    containerFullPathName = KString::Create(*g_Allocator,
                                         KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerFullPathName != nullptr) ? true : false);
    
    b = containerFullPathName->CopyFrom(containerName);
    VERIFY_IS_TRUE(b ? true : false);
    
    KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
    KtlLogContainer::SPtr logContainer;

    status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    openAsync->StartOpenLogContainer(*containerFullPathName,
                                         logContainerId,
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logContainer->StartClose(NULL,
                     closeSync.CloseCompletionCallback());

    status = closeSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS);

#else
    //
    // This code creates a corrupted log container and ensures the
    // right error code.
    //
    NTSTATUS status;
    BOOLEAN b;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeSync;
    KString::SPtr containerPath;
    KString::SPtr containerFile;
    KString::SPtr containerFullPathName;
    WCHAR drive[7];
    KWString fileName(*g_Allocator);
    KSynchronizer sync;

    containerPath = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerPath != nullptr) ? true : false);

    containerFile = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerFile != nullptr) ? true : false);

    containerFullPathName = KString::Create(*g_Allocator,
                                         KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerFullPathName != nullptr) ? true : false);

#if ! defined(PLATFORM_UNIX)    
    drive[0] = L'\\';
    drive[1] = L'?';
    drive[2] = L'?';
    drive[3] = L'\\';
    drive[4] = (WCHAR)driveLetter;
    drive[5] = L':';
    drive[6] = 0;
#else
    drive[0] = 0;
#endif

    b = containerPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);


    KString::SPtr directoryString;
    directoryString = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((directoryString != nullptr) ? true : false);
    KGuid directoryGuid;
    directoryGuid. CreateNew();
    b = directoryString->FromGUID(directoryGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(*directoryString, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    KWString dirc(*g_Allocator, (WCHAR*)*containerPath);
    VERIFY_IS_TRUE(NT_SUCCESS(dirc.Status()));
    status = KVolumeNamespace::DeleteFileOrDirectory(dirc,
                                               *g_Allocator,
                                               sync,
                                               NULL);            // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

    
    logContainerGuid.CreateNew();
    logContainerId = logContainerGuid;

    b = containerFile->FromGUID(logContainerGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFile->Concat(KStringView(L".LogContainer"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->CopyFrom(*containerPath);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(*containerFile);
    VERIFY_IS_TRUE(b ? true : false);

    
    //
    // Create a log container
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;
        KtlLogContainer::SPtr logContainer;

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
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

    //
    // Re-open the just created log container
    //
    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
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


    //
    // Corrupt the shared log metadata associated with the container
    // and open it
    //
    {
        KBlockFile::SPtr blockfile;

        fileName = (PWCHAR)(*containerFullPathName);
        fileName += L":MBInfo";
        VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));
        
        status = KBlockFile::Create(fileName,
                                    TRUE,
                                    KBlockFile::eOpenExisting,
                                    blockfile,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONGLONG fileSize = blockfile->QueryFileSize();
        ULONG count = (ULONG)(fileSize / 0x1000);

        KIoBuffer::SPtr ioBuffer;
        PVOID p;
        status = KIoBuffer::CreateSimple(0x1000, ioBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        

        for (ULONG i = 0; i < count; i++)
        {
            status = blockfile->Transfer(KBlockFile::eForeground,
                                         KBlockFile::eWrite,
                                         i*0x1000,
                                         *ioBuffer,
                                         sync);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        blockfile->Close();
    }

    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == K_STATUS_LOG_STRUCTURE_FAULT);
    }
    
    //
    // Delete the log container
    //
    {
        fileName = (PWCHAR)(*containerFullPathName);
        VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));

        status = KVolumeNamespace::DeleteFileOrDirectory(fileName, *g_Allocator, sync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }
#endif
}


VOID
CreateStreamAndContainerPathnames(
    __in UCHAR DriveLetter,
    __out KString::SPtr& containerFullPathName,
    __out KtlLogContainerId& LogContainerId,
    __out KString::SPtr& streamFullPathName,
    __out KtlLogStreamId& LogStreamId
)
{
    NTSTATUS status;
    KString::SPtr containerPath;
    KString::SPtr containerFile;
    KString::SPtr streamPath;
    KString::SPtr streamFile;
    WCHAR drive[7];
    KGuid logStreamGuid;
    KGuid logContainerGuid;
    BOOLEAN b;
    KSynchronizer sync;

    containerPath = KString::Create(*g_Allocator,
        MAX_PATH);
    VERIFY_IS_TRUE((containerPath != nullptr) ? true : false);

    containerFile = KString::Create(*g_Allocator,
        MAX_PATH);
    VERIFY_IS_TRUE((containerFile != nullptr) ? true : false);

    containerFullPathName = KString::Create(*g_Allocator,
        KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerFullPathName != nullptr) ? true : false);

    streamPath = KString::Create(*g_Allocator,
        MAX_PATH);
    VERIFY_IS_TRUE((streamPath != nullptr) ? true : false);

    streamFile = KString::Create(*g_Allocator,
        MAX_PATH);
    VERIFY_IS_TRUE((streamFile != nullptr) ? true : false);

    streamFullPathName = KString::Create(*g_Allocator,
        KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((streamFullPathName != nullptr) ? true : false);

#if ! defined(PLATFORM_UNIX)    
    //
    // Build up container and stream paths
    //
    drive[0] = L'\\';
    drive[1] = L'?';
    drive[2] = L'?';
    drive[3] = L'\\';
    drive[4] = (WCHAR)DriveLetter;
    drive[5] = L':';
    drive[6] = 0;
#else
    drive[0] = 0;
#endif

    b = containerPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = containerPath->Concat(KStringView(KVolumeNamespace::PathSeparator), TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);
    b = containerPath->Concat(KStringView(L"AlternateContainerPath"), TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);

    KWString dirc(*g_Allocator, (WCHAR*)*containerPath);
    VERIFY_IS_TRUE(NT_SUCCESS(dirc.Status()));
    status = KVolumeNamespace::CreateDirectory(dirc,
        *g_Allocator,
        sync,
        NULL);            // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_COLLISION));

    b = streamPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = streamPath->Concat(KStringView(KVolumeNamespace::PathSeparator), TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);
    b = streamPath->Concat(KStringView(L"AlternateStreamPath"), TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);

    KWString dirs(*g_Allocator, (WCHAR*)*streamPath);
    VERIFY_IS_TRUE(NT_SUCCESS(dirs.Status()));
    status = KVolumeNamespace::CreateDirectory(dirs,
        *g_Allocator,
        sync,
        NULL);            // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_COLLISION));

    logContainerGuid.CreateNew();
    LogContainerId = logContainerGuid;

    b = containerFile->FromGUID(logContainerGuid);
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = containerFile->Concat(KStringView(L".LogContainer"));
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = containerFullPathName->CopyFrom(*containerPath);
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = containerFullPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = containerFullPathName->Concat(*containerFile);
    VERIFY_IS_TRUE(b ? true : FALSE);

    //
    // Create the good stream file name
    //
    logStreamGuid.CreateNew();
    LogStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    b = streamFile->FromGUID(logStreamGuid);
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = streamFile->Concat(KStringView(L".LogStream"));
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = streamFullPathName->CopyFrom(*streamPath);
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = streamFullPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = streamFullPathName->Concat(*streamFile);
    VERIFY_IS_TRUE(b ? true : FALSE);

}


VOID
ConfiguredMappedLogTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KtlLogManager::SharedLogContainerSettings* settings;
    KtlLogManager::AsyncConfigureContext::SPtr configureContext;
    KBuffer::SPtr inBuffer;
    KBuffer::SPtr outBuffer;
    ULONG result;
    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
    KtlLogContainer::SPtr logContainer;
    ContainerCloseSynchronizer closeContainerSync;
    KGuid guidNull(0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
    KGuid defaultGuid(0x3CA2CCDA,0xDD0F,0x49c8,0xA7,0x41,0x62,0xAA,0xC0,0xD4,0xEB,0x62);

    status = logManager->CreateAsyncConfigureContext(configureContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KBuffer::Create(sizeof(KtlLogManager::SharedLogContainerSettings),
                             inBuffer,
                             *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    HRESULT hr;
    KtlLogContainerId logContainerIdDefault = defaultGuid;
    KString::SPtr containerNameMap;
    KString::SPtr streamNameMap;
    KtlLogContainerId logContainerIdMap;
    KtlLogStreamId logStreamIdMap;

    KString::SPtr containerNameOther;
    KString::SPtr streamNameOther;
    KtlLogContainerId logContainerIdOther;
    KtlLogStreamId logStreamIdOther;

    KString::SPtr containerNameOther2;
    KString::SPtr streamNameOther2;
    KtlLogContainerId logContainerIdOther2;
    KtlLogStreamId logStreamIdOther2;

    KString::SPtr containerNameX;
    KString::SPtr streamNameX;
    KtlLogContainerId logContainerIdX;
    KtlLogStreamId logStreamIdX;
    
    CreateStreamAndContainerPathnames(driveLetter,
                              containerNameMap,
                              logContainerIdMap,
                              streamNameMap,
                              logStreamIdMap
                              );

    CreateStreamAndContainerPathnames(driveLetter,
                              containerNameOther,
                              logContainerIdOther,
                              streamNameOther,
                              logStreamIdOther
                              );

    CreateStreamAndContainerPathnames(driveLetter,
                              containerNameOther2,
                              logContainerIdOther2,
                              streamNameOther2,
                              logStreamIdOther2
                              );

    CreateStreamAndContainerPathnames(driveLetter,
                              containerNameX,
                              logContainerIdX,
                              streamNameX,
                              logStreamIdX
                              );

    KWString pathMap(*g_Allocator, (PWCHAR)*containerNameMap);
    KWString pathOther(*g_Allocator, (PWCHAR)*containerNameOther);
    KWString pathOther2(*g_Allocator, (PWCHAR)*containerNameOther2);
    KBlockFile::SPtr file;
    KArray<KBlockFile::AllocatedRange> allocations(*g_Allocator);
    ULONGLONG fileLength;

    //
    // Test 0:  Do not map shared log but instead just change the size
    //          and verify log created is correct size
    //
    settings = (KtlLogManager::SharedLogContainerSettings*)inBuffer->GetBuffer();
    settings->DiskId = guidNull;
    *settings->Path = 0;
    settings->LogContainerId = guidNull;
    settings->LogSize = 768 * 1024 * 1024;
    settings->MaximumNumberStreams = 3 * 2048;
    settings->MaximumRecordSize = 0;
    settings->Flags = KtlLogManager::FlagSparseFile;

    configureContext->Reuse();
    configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                     inBuffer.RawPtr(),
                                     result,
                                     outBuffer,
                                     NULL,
                                     sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    LONGLONG logSize = 8589934592;
    createAsync->Reuse();
    createAsync->StartCreateLogContainer(*containerNameOther2,
        logContainerIdDefault,
        logSize,
        3*8192,            // Max Number Streams
        16 * 1024 * 1024,            // Max Record Size
        0,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;

    //
    // Validate mapped log created correctly
    //
    status = KBlockFile::Create(pathOther2, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

#if defined(PLATFORM_UNIX)
    status = file->SetSparseFile(TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
    fileLength = file->QueryFileSize();
    VERIFY_IS_TRUE(fileLength == ((768 * 1024 * 1024) + 0x2000) );
    
    status = file->QueryAllocations(0, fileLength, allocations, sync, NULL, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(allocations.Count() == 2);
    allocations.Clear();
    file = nullptr;

    //
    // Test 1: Map default shared log to a different location and
    //         create default log. Verify that the log created is in the mapped
    //         location with mapped size
    //
    settings = (KtlLogManager::SharedLogContainerSettings*)inBuffer->GetBuffer();
    settings->DiskId = guidNull;
    hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), (LPCWSTR)(*containerNameMap));
    VERIFY_IS_TRUE(SUCCEEDED(hr));
    settings->LogContainerId = logContainerIdMap;
    settings->LogSize = 1024 * 1024 * 1024;
    settings->MaximumNumberStreams = 3 * 2048;
    settings->MaximumRecordSize = 0;
    settings->Flags = KtlLogManager::FlagSparseFile;

    configureContext->Reuse();
    configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                     inBuffer.RawPtr(),
                                     result,
                                     outBuffer,
                                     NULL,
                                     sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    createAsync->Reuse();
    createAsync->StartCreateLogContainer(*containerNameOther,
        logContainerIdDefault,
        256 * 1024 * 1024,
        3*8192,            // Max Number Streams
        16 * 1024 * 1024,            // Max Record Size
        0,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;

    //
    // Validate mapped log created correctly
    //
    status = KBlockFile::Create(pathMap, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

#if defined(PLATFORM_UNIX)
    status = file->SetSparseFile(TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
    fileLength = file->QueryFileSize();
    VERIFY_IS_TRUE(fileLength == ((1024 * 1024 * 1024) + 0x2000) );

    status = file->QueryAllocations(0, fileLength, allocations, sync, NULL, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(allocations.Count() == 2);
    file = nullptr;

    status = KBlockFile::Create(pathOther, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((status == STATUS_OBJECT_NAME_NOT_FOUND) ? true : false);
    file = nullptr;

    //
    // Test 2: Map default shared log to a different location and
    //         create a non default log. Verify the log is created in
    //         the non default location with non default size
    //
    createAsync->Reuse();
    createAsync->StartCreateLogContainer(*containerNameOther,
        logContainerIdOther,
        512 * 1024 * 1024,
        3*8192,            // Max Number Streams
        16 * 1024 * 1024,            // Max Record Size
        0,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;


    //
    // Validate other log created correctly
    //
    status = KBlockFile::Create(pathOther, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    fileLength = file->QueryFileSize();
    VERIFY_IS_TRUE(fileLength == ((512 * 1024 * 1024) + 0x2000));

    status = file->QueryAllocations(0, fileLength, allocations, sync, NULL, NULL);
    VERIFY_IS_TRUE(status == STATUS_NOT_SUPPORTED);
    file = nullptr;

#if !defined(PLATFORM_UNIX)

    // TODO: This test is failing since Linux does not enforce any kind
    // of file sharing violations, that is, there are no equivalents to
    // FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE, etc

    
    //
    // Test 3: Open log container and verify correct one was opened
    //
    openAsync->StartOpenLogContainer(*containerNameX,
                                     logContainerIdDefault,
                                     logContainer,
                                                NULL,
                                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KBlockFile::Create(pathMap, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SHARING_VIOLATION ? true : false);

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;

    openAsync->Reuse();
    openAsync->StartOpenLogContainer(*containerNameOther,
                                     logContainerIdOther,
                                     logContainer,
                                                NULL,
                                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KBlockFile::Create(pathOther, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(((status == STATUS_SHARING_VIOLATION) || (status == STATUS_ACCESS_DENIED)) ? true : false);
    file = nullptr;

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;
#endif

    //
    // Test 4: Delete the log containers verifying that the right was
    //         is being deleted
    //
    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteContainerAsync->StartDeleteLogContainer(*containerNameOther,
                                                  logContainerIdDefault,
                                                  NULL,         // ParentAsync
                                                  sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Verify mapped container deleted
    //
    status = KBlockFile::Create(pathMap, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((status == STATUS_OBJECT_NAME_NOT_FOUND) ? true : false);
    file = nullptr;

    status = KBlockFile::Create(pathOther, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    file = nullptr;

    deleteContainerAsync->Reuse();
    deleteContainerAsync->StartDeleteLogContainer(*containerNameOther,
                                                  logContainerIdOther,
                                                  NULL,         // ParentAsync
                                                  sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Verify other container deleted
    //
    status = KBlockFile::Create(pathMap, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((status == STATUS_OBJECT_NAME_NOT_FOUND) ? true : false);
    file = nullptr;

    status = KBlockFile::Create(pathOther, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((status == STATUS_OBJECT_NAME_NOT_FOUND) ? true : false);
    file = nullptr;
}

VOID
SetConfigurationTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KSynchronizer sync;

    KtlLogManager::MemoryThrottleLimits* memoryThrottleLimits;
    KtlLogManager::AsyncConfigureContext::SPtr configureContext;
    KBuffer::SPtr inBuffer;
    KBuffer::SPtr outBuffer;
    ULONG result;

#if !defined(PLATFORM_UNIX)                     
        static const PWCHAR randomLogName = L"\\??\\c:\\abcde\\xyz.log";
        static const PWCHAR badLogName = L"c:\\abcde\\xyz.log";
#else
        static const PWCHAR randomLogName = L"/abcde/xyz.log";
        static const PWCHAR badLogName = L"\\fred";
#endif
    
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
    memoryThrottleLimits->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMax;
    memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
    memoryThrottleLimits->WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
    memoryThrottleLimits->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_DefaultPinnedMemoryLimit;
    memoryThrottleLimits->PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;
    memoryThrottleLimits->PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
    memoryThrottleLimits->AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;
    memoryThrottleLimits->MaximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_DefaultMaximumDestagingWriteOutstanding;

    //
    // Test 1: Set memory throttle limits, positive test cases
    //
    {
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::MemoryThrottleUsage* memoryThrottleUsage;
        KtlLogManager::MemoryThrottleLimits* outMemoryThrottleLimits;
        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        outBuffer = nullptr;
    }


    //
    // Test 2: PeriodicFlushTimeInSec in range
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->PeriodicFlushTimeInSec = 10;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        configureContext->Reuse();
        memoryThrottleLimits->PeriodicFlushTimeInSec = 20 * 60;
        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
        
        //
        // reset to defaults
        //
        memoryThrottleLimits->PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;     
    }

    //
    // Test 3: PeriodicTimerIntervalInSec in range
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->PeriodicTimerIntervalInSec = 0;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        configureContext->Reuse();
        memoryThrottleLimits->PeriodicTimerIntervalInSec = 20 * 60;
        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
    }

    //
    // Test 4: AllocationTimeoutInMs in range
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->AllocationTimeoutInMs = 2 * 60 * 60 * 1000;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;
    }

    //
    // Test 5: PinnedMemoryLimit is in range
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_PinnedMemoryLimitMin - 1024;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_DefaultPinnedMemoryLimit;
    }

    //
    // Test 6: WriteBufferMemoryPoolPerStream is in range
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->WriteBufferMemoryPoolPerStream = 512;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
    }

    //
    // Test 7: WriteBufferMemoryPoolMin in range
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->WriteBufferMemoryPoolMin = 1024;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
    }

    //
    // Test 8: WriteBufferMemoryPoolMin above WriteBufferMemoryPoolMax
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->WriteBufferMemoryPoolMin = 0x2000000;
        memoryThrottleLimits->WriteBufferMemoryPoolMax = 0x1000000;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
        memoryThrottleLimits->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMax;
    }


    //
    // Test 8.5a: SharedLogThrottleLimit above 100
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->SharedLogThrottleLimit = 101;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits3,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;
    }

    //
    // Test 8.5b: SharedLogThrottleLimit zero
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->SharedLogThrottleLimit = 0;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits3,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;
    }

    {
        configureContext->Reuse();
        memoryThrottleLimits->SharedLogThrottleLimit = (ULONG)-32;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits3,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;
    }
    
    
    //
    // Test 8.5c: SharedLogThrottleLimit 90%
    //
    {
        configureContext->Reuse();
        memoryThrottleLimits->SharedLogThrottleLimit = 90;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits3,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        memoryThrottleLimits->SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;
    }
    
    //
    // Test 9: WriteBufferMemoryPoolMin is no limit but WriteBufferMemoryPoolMax is
    //         limited
    {
        configureContext->Reuse();
        memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
        memoryThrottleLimits->WriteBufferMemoryPoolMax = 0x4000000;

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
        memoryThrottleLimits->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMax;
    }

    //
    // Test 9a: Set MaximumDestagingWriteOutstanding
    {
        configureContext->Reuse();

        memoryThrottleLimits->MaximumDestagingWriteOutstanding = 64 * (1024 * 1024);
        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::MemoryThrottleUsage* memoryThrottleUsage;
        KtlLogManager::MemoryThrottleLimits* outMemoryThrottleLimits;
        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        configureContext->Reuse();
        memoryThrottleLimits->MaximumDestagingWriteOutstanding = 4 * (1024 * 1024);
        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        //
        // reset to defaults
        //
        memoryThrottleLimits->MaximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_DefaultMaximumDestagingWriteOutstanding;
    }

    //
    // Test 9b: Backward compatibility - older fabric.exe and newer driver
    //
    {
        KBuffer::SPtr inBufferV1;
        KtlLogManager::MemoryThrottleLimits* memoryThrottleLimitsV1;

        status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleLimits),
                                 inBufferV1,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Configure defaults
        //
        memoryThrottleLimitsV1 = (KtlLogManager::MemoryThrottleLimits*)inBufferV1->GetBuffer();
        memoryThrottleLimitsV1->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMax;
        memoryThrottleLimitsV1->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
        memoryThrottleLimitsV1->WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
        memoryThrottleLimitsV1->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_DefaultPinnedMemoryLimit;
        memoryThrottleLimitsV1->PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;
        memoryThrottleLimitsV1->PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
        memoryThrottleLimitsV1->AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;

        configureContext->Reuse();

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits,
                                         inBufferV1.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    //
    // Test 9c: Verify that additional per stream allocation gets added
    //          and removed from total allocation limit correctly
    //
    {
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::MemoryThrottleUsage* memoryThrottleUsage;
        KtlLogManager::MemoryThrottleLimits* outMemoryThrottleLimits;
        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        outBuffer = nullptr;

        //
        // Add some log streams and validate TotalAllocationLimit
        // increases correctly
        //
        KGuid logContainerGuid;
        KtlLogContainerId logContainerId;
        ContainerCloseSynchronizer closeContainerSync;
        KtlLogContainer::SPtr logContainer;

        
        static const ULONG NumberStreams = 4;
        const LONGLONG StreamSize = 268435456; // 256MB
        KtlLogStreamId logStreamId[NumberStreams];
        KtlLogStream::SPtr logStream[NumberStreams];
        KGuid logStreamGuid;
        ULONG metadataLength = 0x10000;
        StreamCloseSynchronizer closeStreamSync;
        
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = 0x200000000;  // 8GB

        logContainerGuid.CreateNew();
        logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
            logContainerId,
            logSize,
            0,            // Max Number Streams
            0,            // Max Record Size
            0,
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
            logStreamGuid.CreateNew();
            logStreamId[i] = static_cast<KtlLogStreamId>(logStreamGuid);

            createStreamAsync->Reuse();
            createStreamAsync->StartCreateLogStream(logStreamId[i],
                KLogicalLogInformation::GetLogicalLogStreamType(),
                nullptr,           // Alias
                nullptr,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        LONGLONG expected = memoryThrottleLimits->WriteBufferMemoryPoolMin +
                            (NumberStreams * memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == expected);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        outBuffer = nullptr;
        
        //
        // Close log streams and validate TotalAllocationLimit returns
        // to original value
        //
        for (ULONG i = 0; i < NumberStreams; i++)
        {
            logStream[i]->StartClose(NULL,
                                     closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream[i] = nullptr;
        }

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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        expected = memoryThrottleLimits->WriteBufferMemoryPoolMin;
        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == expected);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        outBuffer = nullptr;


    //
    // Test 9d: Verify that additional per stream allocation gets added
    //          and removed from total allocation limit correctly when
    //          the WriteBufferMemoryPoolMax is less than the number of
    //          stream
    //
        LONGLONG expectedMax = memoryThrottleLimits->WriteBufferMemoryPoolMin +
                               ((NumberStreams-2) * memoryThrottleLimits->WriteBufferMemoryPoolPerStream);  
        memoryThrottleLimits->WriteBufferMemoryPoolMax =  expectedMax;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        outBuffer = nullptr;

        //
        // Add some log streams and validate TotalAllocationLimit
        // increases correctly
        //
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        for (ULONG i = 0; i < NumberStreams; i++)
        {
            ULONG openedMetadataLength;
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId[i],
                                                    &openedMetadataLength,
                                                    logStream[i],
                                                    NULL,    // ParentAsync
                                                    sync);
            
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == expectedMax);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        outBuffer = nullptr;
        
        //
        // Close log streams and validate TotalAllocationLimit returns
        // to original value
        //
        for (ULONG i = 0; i < NumberStreams; i++)
        {
            logStream[i]->StartClose(NULL,
                                     closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream[i] = nullptr;
        }

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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        outMemoryThrottleLimits = &memoryThrottleUsage->ConfiguredLimits;

        expected = memoryThrottleLimits->WriteBufferMemoryPoolMin;
        VERIFY_IS_TRUE(memoryThrottleUsage->TotalAllocationLimit == expected);
        VERIFY_IS_TRUE(memoryThrottleUsage->CurrentAllocations == 0);
        VERIFY_IS_TRUE(memoryThrottleUsage->IsUnderMemoryPressure ? false : true);
        
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMax == memoryThrottleLimits->WriteBufferMemoryPoolMax);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolMin == memoryThrottleLimits->WriteBufferMemoryPoolMin);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->WriteBufferMemoryPoolPerStream == memoryThrottleLimits->WriteBufferMemoryPoolPerStream);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PinnedMemoryLimit == memoryThrottleLimits->PinnedMemoryLimit);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicFlushTimeInSec == memoryThrottleLimits->PeriodicFlushTimeInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->PeriodicTimerIntervalInSec == memoryThrottleLimits->PeriodicTimerIntervalInSec);
        VERIFY_IS_TRUE(outMemoryThrottleLimits->AllocationTimeoutInMs == memoryThrottleLimits->AllocationTimeoutInMs);
        
        outBuffer = nullptr;

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

        //
        // reset to defaults
        //
        memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;     
    }    

    //
    // SharedLogContainerSettings
    //
    HRESULT hr;
    KGuid guidNull(0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
    inBuffer = nullptr;
    status = KBuffer::Create(sizeof(KtlLogManager::SharedLogContainerSettings),
                             inBuffer,
                             *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    static const LONGLONG InitialLogSize = 0x200000000;
    static const ULONG InitialNumberStreams = 24576;
    static const ULONG InitialRecordSize = 0;
    static const DWORD InitialFlags = 0;

    KtlLogManager::SharedLogContainerSettings* settings = (KtlLogManager::SharedLogContainerSettings*)inBuffer->GetBuffer();
    *settings->Path = 0;
    settings->DiskId  = guidNull;
    settings->LogContainerId = guidNull;
    settings->LogSize = InitialLogSize;
    settings->MaximumNumberStreams = InitialNumberStreams;
    settings->MaximumRecordSize = InitialRecordSize;
    settings->Flags = InitialFlags;


    //
    // Test 10: Positive - configure settings
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
    }

    //
    // Test 11: Positive - reset settings
    //
    {
        configureContext->Reuse();

        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Test 12: Negative - path doesn't start with something invalid
    // like \??\ on windows or \ on linux
    //
    {
        configureContext->Reuse();

        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), badLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(! NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
    }

    //
    // Test 13: LogSize < KtlLogManager::SharedLogContainerSettings::_DefaultLogSizeMin
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        // Negative
        settings->LogSize = KtlLogManager::SharedLogContainerSettings::_DefaultLogSizeMin - (1024 * 1024);
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        // Positive
        settings->LogSize = KtlLogManager::SharedLogContainerSettings::_DefaultLogSizeMin;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Positive
        settings->LogSize = 0x800000000;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
        settings->LogSize = InitialLogSize;
    }

    //
    // Test 14: MaximumNumberStreams < KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMin
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        // Negative
        settings->MaximumNumberStreams = KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMin - 128;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        // Positive
        settings->MaximumNumberStreams = KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMin;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Positive
        settings->MaximumNumberStreams = KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMin + 128;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
        settings->MaximumNumberStreams = InitialNumberStreams;
    }

    //
    // Test 15: MaximumNumberStreams > KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMa
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        // Negative
        settings->MaximumNumberStreams = KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMax + 128;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        // Positive
        settings->MaximumNumberStreams = KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMax;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Positive
        settings->MaximumNumberStreams = KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMax - 128;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
        settings->MaximumNumberStreams = InitialNumberStreams;
    }

    //
    // Test 16: Negative - MaximumRecordSize != 0
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        // Negative
        settings->MaximumRecordSize = 7;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        // Positive
        settings->MaximumRecordSize = 0;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
        settings->MaximumRecordSize = InitialRecordSize;
    }

    //
    // Test 17:  (Flags != 0 ) && (Flags != KtlLogManager::FlagSparseFile)
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        // Negative
        settings->Flags = 7;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        // Positive
        settings->Flags = 0;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Positive
        settings->Flags = KtlLogManager::FlagSparseFile;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
        settings->Flags = InitialFlags;
    }

    //
    // Test 18:  (settings->DiskId != guidNull) && (*settings->Path != 0)
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->DiskId = randomGuid;
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        // Negative
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        // Positive
        settings->DiskId = guidNull;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Positive
        settings->DiskId = randomGuid;
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->DiskId = guidNull;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
    }

    //
    // Test 19:  (settings->LogContainerId.Get() == guidNull) && (*settings->Path != 0)
    //
    {
        hr = StringCchCopy(settings->Path, sizeof(settings->Path) / sizeof(WCHAR), randomLogName);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        KGuid randomGuid;
        randomGuid.CreateNew();
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);

        // Positive
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Negative
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        // Negative
        settings->LogContainerId = static_cast<KtlLogContainerId>(randomGuid);
        *settings->Path = 0;
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::ConfigureSharedLogContainerSettings,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // reset to defaults
        //
        *settings->Path = 0;
        settings->LogContainerId = static_cast<KtlLogContainerId>(guidNull);
    }

    configureContext = nullptr;
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
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;
    ULONG metadataLength = 0x10000;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;

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

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
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
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Create a stream with an invalid log stream type and
            // expect an error
            //
            createStreamAsync->Reuse();

            KGuid logStreamGuidBad;
            KtlLogStreamId logStreamIdBad;
            KtlLogStreamType logStreamTypeBad;

            logStreamGuidBad.CreateNew();
            logStreamIdBad = static_cast<KtlLogStreamId>(logStreamGuidBad);

            logStreamGuidBad.CreateNew();
            logStreamTypeBad = static_cast<KtlLogStreamType>(logStreamGuidBad);

            createStreamAsync->StartCreateLogStream(logStreamIdBad,
                logStreamTypeBad,
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
            VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

            //
            // Create a stream with a null streamid and expect an error
            //
            createStreamAsync->Reuse();

            GUID guidNull = { 0 };
            KtlLogStreamId logStreamIdNull(guidNull);

            createStreamAsync->StartCreateLogStream(logStreamIdNull,
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
            VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
        }

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
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG openedMetadataLength;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &openedMetadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(openedMetadataLength == metadataLength);

#ifdef UPASSTHROUGH
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
        }

#if UPASSTHROUGH
        //
        // Close the container
        //
        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
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
        {
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;

            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

        }

        // TODO: Add test that deltes stream while it is open

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

        logContainer = nullptr;
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

//
VOID
MissingStreamFileTest(
    UCHAR driveLetter,
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
    KGuid logStreamGuid2;
    KGuid logStreamGuidLong;
    KtlLogStreamId logStreamId;
    KtlLogStreamId logStreamId2;
    KtlLogStreamId logStreamIdLong;
    ULONG metadataLength = 0x10000;
    WCHAR drive[7];
    KString::SPtr containerPath;
    KString::SPtr containerFile;
    KString::SPtr containerFullPathName;
    KString::SPtr streamPath;
    KString::SPtr streamFile;
    KString::SPtr streamFullPathName;
    WCHAR longString[KtlLogManager::MaxPathnameLengthInChar+3];
    BOOLEAN b;

    //
    // Prealloc the strings
    //
    containerPath = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerPath != nullptr) ? true : false);

    containerFile = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerFile != nullptr) ? true : false);

    containerFullPathName = KString::Create(*g_Allocator,
                                         KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerFullPathName != nullptr) ? true : false);

    streamPath = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((streamPath != nullptr) ? true : false);

    streamFile = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((streamFile != nullptr) ? true : false);

    streamFullPathName = KString::Create(*g_Allocator,
                                         KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((streamFullPathName != nullptr) ? true : false);

    for (ULONG i = 0; i < KtlLogManager::MaxPathnameLengthInChar+3; i++)
    {
        longString[i] = L'x';
    }
    longString[KtlLogManager::MaxPathnameLengthInChar+2] = 0;

#if ! defined(PLATFORM_UNIX)    
    //
    // Build up container and stream paths
    //
    drive[0] = L'\\';
    drive[1] = L'?';
    drive[2] = L'?';
    drive[3] = L'\\';
    drive[4] = (WCHAR)driveLetter;
    drive[5] = L':';
    drive[6] = 0;
#else
    drive[0] = L'/';
    drive[1] = 0;
#endif
    
    b = containerPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);


    KString::SPtr directoryString;
    directoryString = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((directoryString != nullptr) ? true : false);
    KGuid directoryGuid;
    directoryGuid. CreateNew();
    b = directoryString->FromGUID(directoryGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(KStringView(L"\\"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(*directoryString, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    KWString dirc(*g_Allocator, (WCHAR*)*containerPath);
    VERIFY_IS_TRUE(NT_SUCCESS(dirc.Status()));
    status = KVolumeNamespace::DeleteFileOrDirectory(dirc,
                                               *g_Allocator,
                                               sync,
                                               NULL);            // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

    b = streamPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    b = streamPath->Concat(KStringView(L"\\"));
    VERIFY_IS_TRUE(b ? true : false);

    b = streamPath->Concat(*directoryString, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    //
    // Create the good container file name
    //
    logContainerGuid.CreateNew();
    logContainerId = logContainerGuid;

    b = containerFile->FromGUID(logContainerGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFile->Concat(KStringView(L".LogContainer"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->CopyFrom(*containerPath);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(KStringView(L"\\"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(*containerFile);
    VERIFY_IS_TRUE(b ? true : false);


    //
    // Create the good stream file name
    //
    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    b = streamFile->FromGUID(logStreamGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFile->Concat(KStringView(L".LogStream"));
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFullPathName->CopyFrom(*streamPath);
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFullPathName->Concat(KStringView(L"\\"));
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFullPathName->Concat(*streamFile);
    VERIFY_IS_TRUE(b ? true : false);

    KString::CSPtr streamFullPathNameConst = streamFullPathName.RawPtr();
    
    //
    // Create container and a stream within it
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                        nullptr,           // Alias
                                                        streamFullPathNameConst,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            //
            // Create a few more log streams
            //
            for (ULONG i = 0; i < 3; i++)
            {
                KGuid logStreamXGuid;
                KtlLogStreamId logStreamXId;
                KString::SPtr streamFullPathNameX;
                KString::SPtr streamNameX;

                streamFullPathNameX = KString::Create(*g_Allocator,
                                                MAX_PATH);
                VERIFY_IS_TRUE((streamFullPathNameX != nullptr) ? true : false);

                streamNameX = KString::Create(*g_Allocator,
                                                MAX_PATH);
                VERIFY_IS_TRUE((streamNameX != nullptr) ? true : false);

                logStreamXGuid.CreateNew();
                logStreamXId = static_cast<KtlLogStreamId>(logStreamXGuid);


                b = streamFullPathNameX->CopyFrom(*streamPath);
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamFullPathNameX->Concat(KStringView(L"\\"));
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamNameX->FromGUID(logStreamXGuid);
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamFullPathNameX->Concat(*streamNameX);
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamFullPathNameX->Concat(KStringView(L".XtraLogStream"));
                VERIFY_IS_TRUE(b ? true : FALSE);

                KString::CSPtr streamFullPathNameXConst = streamFullPathNameX.RawPtr();
                
                createStreamAsync->Reuse();
                createStreamAsync->StartCreateLogStream(logStreamXId,
                                                            nullptr,           // Alias
                                                            streamFullPathNameXConst,
                                                            securityDescriptor,
                                                            metadataLength,
                                                            DEFAULT_STREAM_SIZE,
                                                            DEFAULT_MAX_RECORD_SIZE,
                                                            logStream,
                                                            NULL,    // ParentAsync
                                                        sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                logStream->StartClose(NULL,
                                 closeStreamSync.CloseCompletionCallback());

                status = closeStreamSync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Delete the file backing one of the streams
    //
    KWString s(*g_Allocator, (WCHAR*)*streamFullPathName);
    status = KVolumeNamespace::DeleteFileOrDirectory(s,
                                               *g_Allocator,
                                               sync,
                                               NULL);            // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // TODO: Reopen log container and verify that stream no longer exists
    //
    {
        //
        // Reopen the container
        //
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Enumerate and verify that the stream has been removed
        //
        KtlLogContainer::AsyncEnumerateStreamsContext::SPtr enumerateStreamsAsync;
        KArray<KtlLogStreamId> enumeratedStreams(*g_Allocator);

        status = logContainer->CreateAsyncEnumerateStreamsContext(enumerateStreamsAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        enumerateStreamsAsync->StartEnumerateStreams(enumeratedStreams,
                                                     NULL,
                                                     sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < enumeratedStreams.Count(); i++)
        {
            KtlLogStreamId id = enumeratedStreams[i];
            VERIFY_IS_TRUE(id != logStreamId ? true : false);
        }

        enumerateStreamsAsync = nullptr;

        //
        // Close the container
        //
        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Delete log container
    //
    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteContainerAsync->StartDeleteLogContainer(*containerFullPathName,
                                                  logContainerId,
                                                  NULL,         // ParentAsync
                                                  sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

}
//


VOID
ContainerConfigurationTest(
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

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Test1 : Create container using an invalid record size
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = 8589934592;

        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
            logContainerId,
            logSize,
            0,            // Max Number Streams
            0x4000,            // Max Record Size
            KtlLogManager::FlagSparseFile,
            logContainer,
            NULL,         // ParentAsync
            sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
    }
}

NTSTATUS
CreateLogContainerByFilename(
    __out KtlLogContainer::SPtr& logContainer,
    __in KtlLogManager::SPtr logManager,
    __in KtlLogContainerId logContainerId,
    __in KString::SPtr containerFullPathName
)
{
    NTSTATUS createStatus, status;
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KSynchronizer sync;

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(*containerFullPathName,
                                            logContainerId,
                                            logSize,
                                            0,            // Max Number Streams
                                            0,            // Max Record Size
                                            logContainer,
                                            NULL,         // ParentAsync
                                            sync);
    createStatus = sync.WaitForCompletion();

    if (NT_SUCCESS(createStatus))
    {
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        deleteContainerAsync->StartDeleteLogContainer(*containerFullPathName, logContainerId, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    return(createStatus);
}

VOID
PartialContainerCreateTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;

    WCHAR drive[7];
    KString::SPtr containerPath;
    KString::SPtr containerFile;
    KString::SPtr containerFullPathName;
    WCHAR longString[KtlLogManager::MaxPathnameLengthInChar+3];
    BOOLEAN b;

    //
    // Prealloc the strings
    //
    containerPath = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerPath != nullptr) ? true : false);

    containerFile = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerFile != nullptr) ? true : false);

    containerFullPathName = KString::Create(*g_Allocator,
                                         KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerFullPathName != nullptr) ? true : false);


    for (ULONG i = 0; i < KtlLogManager::MaxPathnameLengthInChar+3; i++)
    {
        longString[i] = L'x';
    }
    longString[KtlLogManager::MaxPathnameLengthInChar+2] = 0;

    //
    // Build up container and stream paths
    //
#if ! defined(PLATFORM_UNIX)    
    drive[0] = L'\\';
    drive[1] = L'?';
    drive[2] = L'?';
    drive[3] = L'\\';
    drive[4] = (WCHAR)driveLetter;
    drive[5] = L':';
    drive[6] = 0;
#else
    drive[0] = 0;
#endif

    b = containerPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);


    KString::SPtr directoryString;
    directoryString = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((directoryString != nullptr) ? true : false);
    KGuid directoryGuid;
    directoryGuid. CreateNew();
    b = directoryString->FromGUID(directoryGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(*directoryString, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    KWString dirc(*g_Allocator, (WCHAR*)*containerPath);
    VERIFY_IS_TRUE(NT_SUCCESS(dirc.Status()));
    status = KVolumeNamespace::DeleteFileOrDirectory(dirc,
                                               *g_Allocator,
                                               sync,
                                               NULL);            // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

    //
    // Create the container file name
    //
    logContainerGuid.CreateNew();
    logContainerId = logContainerGuid;

    b = containerFile->FromGUID(logContainerGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFile->Concat(KStringView(L".LogContainer"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->CopyFrom(*containerPath);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(*containerFile);
    VERIFY_IS_TRUE(b ? true : false);
    
    //
    // Build some handy filenames
    //
    KWString fileName(*g_Allocator, (WCHAR*)*containerFullPathName);
    KWString mbinfoName(*g_Allocator, (WCHAR*)*containerFullPathName);
    mbinfoName += L":MBInfo";
    VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));
    VERIFY_IS_TRUE(NT_SUCCESS(mbinfoName.Status()));

    KWString fileNameTemp(*g_Allocator, (WCHAR*)*containerFullPathName);
    fileNameTemp += L".temp";
    KWString mbinfoNameTemp(*g_Allocator, (WCHAR*)fileNameTemp);
    mbinfoNameTemp += L":MBInfo";
    VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));
    VERIFY_IS_TRUE(NT_SUCCESS(mbinfoName.Status()));
    
    //
    // Cleanup any old files
    //
    status = KVolumeNamespace::DeleteFileOrDirectory(fileName, *g_Allocator, sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    
    status = KVolumeNamespace::DeleteFileOrDirectory(fileNameTemp, *g_Allocator, sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    
#if defined(PLATFORM_UNIX)
    status = KVolumeNamespace::DeleteFileOrDirectory(mbinfoName, *g_Allocator, sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    
    status = KVolumeNamespace::DeleteFileOrDirectory(mbinfoNameTemp, *g_Allocator, sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
#endif

    status = KVolumeNamespace::CreateDirectory(dirc,
                                               *g_Allocator,
                                               sync,
                                               NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
                                                                  
    //
    // Test 1: Fail with STATUS_OBJECT_NAME_NOT_FOUND if no log file is
    //         there
    //
    {               
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);     
    }

#if defined(PLATFORM_UNIX)
    //
    // Test 2: Fail with STATUS_OBJECT_NAME_NOT_FOUND if only MBInfo
    //         file is there
    //
    {
        KBlockFile::SPtr file;      
        status = KBlockFile::Create(mbinfoName,
                                    FALSE,
                                    KBlockFile::eCreateAlways,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status)); 
                
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = KVolumeNamespace::DeleteFileOrDirectory(mbinfoName, *g_Allocator, sync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();      
    }
#endif

    //
    // Test 3: Create succeeds if temp files are present and temp files
    //         are removed
    //
    {
        KBlockFile::SPtr file;
        
        status = KBlockFile::Create(fileNameTemp,
                                    FALSE,
                                    KBlockFile::eCreateAlways,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        file = nullptr;

        status = KBlockFile::Create(mbinfoNameTemp,
                                    FALSE,
                                    KBlockFile::eCreateAlways,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        file = nullptr;


        //
        // Try to open and expect failure
        //
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        ContainerCloseSynchronizer closeContainerSync;
        KtlLogContainer::SPtr logContainer;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);

        //
        // Create the log container
        //
        LONGLONG logSize = DefaultTestLogFileSize;
        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logContainer = nullptr;

        
        //
        // Verify temp files are gone
        //
        status = KBlockFile::Create(fileNameTemp,
                                    FALSE,
                                    KBlockFile::eOpenExisting,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);

#if defined(PLATFORM_UNIX)
        status = KBlockFile::Create(mbinfoNameTemp,
                                    FALSE,
                                    KBlockFile::eOpenExisting,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);
#endif
        
        //
        // Verify that log container will open ok
        //
        openAsync->Reuse();
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Delete the container
        //
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(*containerFullPathName,
                                                      logContainerId,
                                                      NULL,         // ParentAsync
                                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

#if defined(PLATFORM_UNIX)
    //
    // Test 4: Create succeeds if MBInfo and temp container exist
    //
    {
        KBlockFile::SPtr file;
        
        status = KBlockFile::Create(fileNameTemp,
                                    FALSE,
                                    KBlockFile::eCreateAlways,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        file = nullptr;

        status = KBlockFile::Create(mbinfoName,
                                    FALSE,
                                    KBlockFile::eCreateAlways,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        file = nullptr;


        //
        // Try to open and expect failure
        //
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        ContainerCloseSynchronizer closeContainerSync;
        KtlLogContainer::SPtr logContainer;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);

        //
        // Create the log container
        //
        LONGLONG logSize = DefaultTestLogFileSize;
        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logContainer = nullptr;

        
        //
        // Verify temp files are gone
        //
        status = KBlockFile::Create(fileNameTemp,
                                    FALSE,
                                    KBlockFile::eOpenExisting,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);

        status = KBlockFile::Create(mbinfoNameTemp,
                                    FALSE,
                                    KBlockFile::eOpenExisting,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);
        
        //
        // Verify that log container will open ok
        //
        openAsync->Reuse();
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Delete the container
        //
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(*containerFullPathName,
                                                      logContainerId,
                                                      NULL,         // ParentAsync
                                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }   
#endif
}


VOID
ContainerWithPathTest(
    UCHAR driveLetter,
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
    KGuid logStreamGuid2;
    KGuid logStreamGuidLong;
    KtlLogStreamId logStreamId;
    KtlLogStreamId logStreamId2;
    KtlLogStreamId logStreamIdLong;
    ULONG metadataLength = 0x10000;
    WCHAR drive[7];
    KString::SPtr containerPath;
    KString::SPtr containerFile;
    KString::SPtr containerFullPathName;
    KString::SPtr streamPath;
    KString::SPtr streamFile;
    KString::SPtr streamFullPathName;
    WCHAR longString[KtlLogManager::MaxPathnameLengthInChar+3];
    BOOLEAN b;
    ULONG len, lenLeft;
    GUID nullGUID = { 0 };
    KtlLogContainerId nullLogContainerId = static_cast<KtlLogContainerId>(nullGUID);        
    
    //
    // Prealloc the strings
    //
    containerPath = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerPath != nullptr) ? true : false);

    containerFile = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerFile != nullptr) ? true : false);

    containerFullPathName = KString::Create(*g_Allocator,
                                         KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((containerFullPathName != nullptr) ? true : false);

    streamPath = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((streamPath != nullptr) ? true : false);

    streamFile = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((streamFile != nullptr) ? true : false);

    streamFullPathName = KString::Create(*g_Allocator,
                                         KtlLogManager::MaxPathnameLengthInChar + 10);
    VERIFY_IS_TRUE((streamFullPathName != nullptr) ? true : false);

    for (ULONG i = 0; i < KtlLogManager::MaxPathnameLengthInChar+3; i++)
    {
        longString[i] = L'x';
    }
    longString[KtlLogManager::MaxPathnameLengthInChar+2] = 0;

    //
    // Build up container and stream paths
    //
#if ! defined(PLATFORM_UNIX)    
    drive[0] = L'\\';
    drive[1] = L'?';
    drive[2] = L'?';
    drive[3] = L'\\';
    drive[4] = (WCHAR)driveLetter;
    drive[5] = L':';
    drive[6] = 0;
#else
    drive[0] = 0;
#endif

    b = containerPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);


    KString::SPtr directoryString;
    directoryString = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((directoryString != nullptr) ? true : false);
    KGuid directoryGuid;
    directoryGuid. CreateNew();
    b = directoryString->FromGUID(directoryGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerPath->Concat(*directoryString, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    KWString dirc(*g_Allocator, (WCHAR*)*containerPath);
    VERIFY_IS_TRUE(NT_SUCCESS(dirc.Status()));
    status = KVolumeNamespace::DeleteFileOrDirectory(dirc,
                                               *g_Allocator,
                                               sync,
                                               NULL);            // Parent
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

    b = streamPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    b = streamPath->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = streamPath->Concat(*directoryString, TRUE);
    VERIFY_IS_TRUE(b ? true : false);

    //
    // Create container file name that is one char too long
    //
    logContainerGuid.CreateNew();
    logContainerId = logContainerGuid;

    b = containerFile->FromGUID(logContainerGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFile->Concat(KStringView(L".LogContainer"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->CopyFrom(*containerPath);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(*containerFile);
    VERIFY_IS_TRUE(b ? true : false);

    len = containerFullPathName->Length();
    lenLeft = (KtlLogManager::MaxPathnameLengthInChar - len) + 1;

    longString[lenLeft] = 0;
    b = containerFullPathName->Concat(KStringView(longString));
    VERIFY_IS_TRUE(b ? true : false);
    longString[lenLeft] = L'x';

    status = CreateLogContainerByFilename(logContainer, logManager, logContainerId, containerFullPathName);
    VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

    //
    // Create the good container file name
    //
    logContainerGuid.CreateNew();
    logContainerId = logContainerGuid;

    b = containerFile->FromGUID(logContainerGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFile->Concat(KStringView(L".LogContainer"));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->CopyFrom(*containerPath);
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = containerFullPathName->Concat(*containerFile);
    VERIFY_IS_TRUE(b ? true : false);


    //
    // Create the good stream file name
    //
    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    b = streamFile->FromGUID(logStreamGuid);
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFile->Concat(KStringView(L".LogStream"));
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFullPathName->CopyFrom(*streamPath);
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFullPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
    VERIFY_IS_TRUE(b ? true : false);

    b = streamFullPathName->Concat(*streamFile);
    VERIFY_IS_TRUE(b ? true : false);

    KString::CSPtr streamFullPathNameConst = streamFullPathName.RawPtr();
    
    //
    // Create container and a stream within it
    //
    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                        nullptr,           // Alias
                                                        streamFullPathNameConst,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Now create a log stream with a filename that is too long
            //
            KString::SPtr streamLongPathName;

            streamLongPathName = KString::Create(*g_Allocator, KtlLogManager::MaxPathnameLengthInChar+1);
            VERIFY_IS_TRUE(streamLongPathName ? true : false);

            b = streamLongPathName->CopyFrom(*streamPath);
            VERIFY_IS_TRUE(b ? true : false);

            b = streamLongPathName->Concat(KStringView(KVolumeNamespace::PathSeparator));
            VERIFY_IS_TRUE(b ? true : false);

            len = streamLongPathName->Length();
            lenLeft = KtlLogManager::MaxPathnameLengthInChar - len;
            longString[lenLeft] = 0;

            b = streamLongPathName->Concat(KStringView(longString));
            VERIFY_IS_TRUE(b ? true : FALSE);
            longString[lenLeft] = L'x';
            
            KString::CSPtr streamLongPathNameConst = streamLongPathName.RawPtr();

            logStreamGuidLong.CreateNew();
            logStreamIdLong = static_cast<KtlLogStreamId>(logStreamGuid);

            createStreamAsync->Reuse();
            createStreamAsync->StartCreateLogStream(logStreamIdLong,
                                                        nullptr,           // Alias
                                                        KString::CSPtr(streamLongPathName.RawPtr()),
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);



#if 0  // TODO: Debug later
            //
            // Create an unnamed stream within a named container
            //
            logStreamGuid2.CreateNew();
            logStreamId2 = static_cast<KtlLogStreamId>(logStreamGuid2);
            createStreamAsync->Reuse();

            createStreamAsync->StartCreateLogStream(logStreamId2,
                                                        nullptr,           // Alias
                                                        nullptr,           // Path
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif


            for (ULONG i = 0; i < 3; i++)
            {
                KGuid logStreamXGuid;
                KtlLogStreamId logStreamXId;
                KString::SPtr streamFullPathNameX;
                KString::SPtr streamNameX;

                streamFullPathNameX = KString::Create(*g_Allocator,
                                                MAX_PATH);
                VERIFY_IS_TRUE((streamFullPathNameX != nullptr) ? true : false);

                streamNameX = KString::Create(*g_Allocator,
                                                MAX_PATH);
                VERIFY_IS_TRUE((streamNameX != nullptr) ? true : false);

                logStreamXGuid.CreateNew();
                logStreamXId = static_cast<KtlLogStreamId>(logStreamXGuid);


                b = streamFullPathNameX->CopyFrom(*streamPath);
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamFullPathNameX->Concat(KStringView(KVolumeNamespace::PathSeparator));
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamNameX->FromGUID(logStreamXGuid);
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamFullPathNameX->Concat(*streamNameX);
                VERIFY_IS_TRUE(b ? true : FALSE);

                b = streamFullPathNameX->Concat(KStringView(L".XtraLogStream"));
                VERIFY_IS_TRUE(b ? true : FALSE);
                
                KString::CSPtr streamFullPathNameXConst = streamFullPathNameX.RawPtr();

                createStreamAsync->Reuse();
                createStreamAsync->StartCreateLogStream(logStreamXId,
                                                            nullptr,           // Alias
                                                            streamFullPathNameXConst,
                                                            securityDescriptor,
                                                            metadataLength,
                                                            DEFAULT_STREAM_SIZE,
                                                            DEFAULT_MAX_RECORD_SIZE,
                                                            logStream,
                                                            NULL,    // ParentAsync
                                                        sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                logStream->StartClose(NULL,
                                 closeStreamSync.CloseCompletionCallback());

                status = closeStreamSync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    {
        //
        // Reopen the container with log container id specified
        //
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG openedMetadataLength;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            openStreamAsync->StartOpenLogStream(logStreamId,
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

#if 0  // TODO: Debug later
            //
            // Also reopen the unnamed log container
            //
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId2,
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
#endif

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
        // Reopen the container without log container id specified
        //
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             nullLogContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG openedMetadataLength;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            openStreamAsync->StartOpenLogStream(logStreamId,
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

#if 0  // TODO: Debug later
            //
            // Also reopen the unnamed log container
            //
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId2,
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
#endif

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
        openAsync->StartOpenLogContainer(*containerFullPathName,
                                             nullLogContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

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

        }

        //
        // Delete the container while open with null guid
        //
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(*containerFullPathName,
                                                      nullLogContainerId,
                                                      NULL,         // ParentAsync
                                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }


    //
    // Now create using the volume guid format
    //
#if !defined(PLATFORM_UNIX)
    {
        KString::SPtr containerPath2;
        KString::SPtr streamPath2;
        KString::SPtr guidString;
        KStringView prefix(L"\\??\\Volume");
        KStringView prefixGlobal(L"\\Global??\\Volume");
        KStringView volume(L"Volume");
        GUID volumeGuid;

        //
        // Get a volume guid
        //
        KVolumeNamespace::VolumeInformationArray volInfo(*g_Allocator);
        status = KVolumeNamespace::QueryVolumeListEx(volInfo, *g_Allocator, sync);
        if (!K_ASYNC_SUCCESS(status))
        {
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"KVolumeNamespace::QueryVolumeListEx Failed");
            return;
        }
        status = sync.WaitForCompletion();

        if (!NT_SUCCESS(status))
        {
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"KVolumeNamespace::QueryVolumeListEx Failed");
            return;
        }

        ULONG i;
        for (i = 0; i < volInfo.Count(); i++)
        {
            if (driveLetter == volInfo[i].DriveLetter)
            {
                volumeGuid = volInfo[i].VolumeId;
                break;
            }
        }

        VERIFY_IS_TRUE(i < volInfo.Count());

        //
        // Allocate some work strings
        //
        containerPath2 = KString::Create(*g_Allocator,
                                        MAX_PATH);
        VERIFY_IS_TRUE((containerPath2 != nullptr) ? true : false);

        streamPath2 = KString::Create(*g_Allocator,
                                        MAX_PATH);
        VERIFY_IS_TRUE((streamPath2 != nullptr) ? true : false);

        guidString = KString::Create(*g_Allocator,
                                        MAX_PATH);
        VERIFY_IS_TRUE((guidString != nullptr) ? true : false);



        //
        // First step is to create the named container
        //
        logContainerGuid.CreateNew();
        logContainerId = logContainerGuid;

        b = containerPath2->CopyFrom(prefix);
        VERIFY_IS_TRUE(b ? true : false);

        b = guidString->FromGUID(volumeGuid);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(*guidString);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L"\\"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L"AlternateContainerPath"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L"\\"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = guidString->FromGUID(logContainerGuid);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(*guidString);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L".LogContainer"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        //
        // Go ahead and create the log container
        //
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;


        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(*containerPath2,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Now create the log stream
        //
        logStreamGuid.CreateNew();
        logStreamId = logStreamGuid;

        b = streamPath2->CopyFrom(prefixGlobal);
        VERIFY_IS_TRUE(b ? true : false);

        b = guidString->FromGUID(volumeGuid);
        VERIFY_IS_TRUE(b ? true : false);

        b = streamPath2->Concat(*guidString);
        VERIFY_IS_TRUE(b ? true : false);

        b = streamPath2->Concat(KStringView(L"\\"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = streamPath2->Concat(KStringView(L"AlternateStreamPath"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = streamPath2->Concat(KStringView(L"\\"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = guidString->FromGUID(logStreamGuid);
        VERIFY_IS_TRUE(b ? true : false);

        b = streamPath2->Concat(*guidString);
        VERIFY_IS_TRUE(b ? true : false);

        b = streamPath2->Concat(KStringView(L".LogStream"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);
        
        KString::CSPtr streamPathConst = streamPath2.RawPtr();

        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
                                                    nullptr,           // Alias
                                                    streamPathConst,
                                                    securityDescriptor,
                                                    metadataLength,
                                                    DEFAULT_STREAM_SIZE,
                                                    DEFAULT_MAX_RECORD_SIZE,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                sync);

        //
        // Close log stream and container
        //
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream = nullptr;
        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Now open them using the opposite prefix
        //
        b = containerPath2->CopyFrom(prefixGlobal);
        VERIFY_IS_TRUE(b ? true : false);

        b = guidString->FromGUID(volumeGuid);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(*guidString);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L"\\"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L"AlternateContainerPath"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L"\\"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = guidString->FromGUID(logContainerGuid);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(*guidString);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath2->Concat(KStringView(L".LogContainer"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);


        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*containerPath2,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Now open up the stream
        //
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG openedMetadataLength;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->StartOpenLogStream(logStreamId,
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
        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(*containerPath2,
                                                      logContainerId,
                                                      NULL,         // ParentAsync
                                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }
#endif

    //
    // Now try to create a log container in the reserved directory and expect and error
    //
    {
        KString::SPtr guidString;

        guidString = KString::Create(*g_Allocator,
                                MAX_PATH);
        VERIFY_IS_TRUE((guidString != nullptr) ? true : false);

        logContainerGuid.CreateNew();
        logContainerId = logContainerGuid;

        b = containerPath->CopyFrom(drive, TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        wchar_t const* dirName = &RvdDiskLogConstants::DirectoryName();
        b = containerPath->Concat(KStringView(dirName), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        b = guidString->FromGUID(logContainerGuid);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath->Concat(*guidString);
        VERIFY_IS_TRUE(b ? true : false);

        b = containerPath->Concat(KStringView(L".LogContainer"), TRUE);
        VERIFY_IS_TRUE(b ? true : false);

        //
        // Go ahead and create the log container
        //
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        KtlLogManager::AsyncOpenLogContainer::SPtr openContainerAsync;
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;


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
        VERIFY_IS_TRUE(status == STATUS_ACCESS_DENIED);


        status = logManager->CreateAsyncOpenLogContainerContext(openContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openContainerAsync->StartOpenLogContainer(*containerPath,
                                                    logContainerId,
                                                    logContainer,
                                                    NULL,
                                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_ACCESS_DENIED);

        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteContainerAsync->StartDeleteLogContainer(*containerPath,
                                                    logContainerId,
                                                    NULL,
                                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_ACCESS_DENIED);
    }
}


VOID
DeleteOpenStreamTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    ULONG metadataLength = 0x10000;

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

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
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

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG openedMetadataLength;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &openedMetadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(openedMetadataLength == metadataLength);

            //
            // Delete the stream
            //
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;

            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE((status == STATUS_NOT_FOUND) || (status == STATUS_OBJECT_NO_LONGER_EXISTS));
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer = nullptr;
        
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
OpenCloseStreamTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    ULONG metadataLength = 0xC0000;

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

        //
        // Loop Open the stream
        //
#ifdef FEATURE_TEST
        const ULONG numberLoops = 8;
#else
        const ULONG numberLoops = 3;
#endif
        for (ULONG i = 0; i < numberLoops; i++)
        {
            KGuid logStreamGuid;
            KtlLogStreamId logStreamId;

            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

            {
                KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
                KtlLogStream::SPtr logStream;

                status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KBuffer::SPtr securityDescriptor = nullptr;
                createStreamAsync->StartCreateLogStream(logStreamId,
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

            {
                KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
                KtlLogStream::SPtr logStream;
                ULONG openedMetadataLength;

                status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                openStreamAsync->StartOpenLogStream(logStreamId,
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
            }

        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

        logContainer = nullptr;
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
StreamTest(
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
                                                        DEFAULT_STREAM_SIZE,
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
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            // TODO: Verify size of metadataLength

            // Note that stream is closed since last reference to
            // logStream is lost in its destructor

            //
            // Write a record
            //
            {
                //
                // Build a data buffer (8 different 64K elements)
                //
                ULONG dataBufferSize = 16 * 0x1000;

                status = KIoBuffer::CreateEmpty(dataIoBufferWritten,
                                                *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (int i = 0; i < 8; i++)
                {
                    KIoBufferElement::SPtr dataElement;
                    VOID* dataBuffer;

                    status = KIoBufferElement::CreateNew(dataBufferSize,        // Size
                                                              dataElement,
                                                              dataBuffer,
                                                              *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    for (int j = 0; j < 0x1000; j++)
                    {
                        ((PUCHAR)dataBuffer)[j] = (UCHAR)(i*j);
                    }

                    dataIoBufferWritten->AddIoBufferElement(*dataElement);
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
                VERIFY_IS_TRUE(NT_SUCCESS(status));

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
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                ULONGLONG version = 1;

                writeContext->StartWrite(recordAsn,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBufferWritten,
                                         NULL,    // Reservation
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Read a record
            //
            {
                KtlLogStream::AsyncReadContext::SPtr readContext;

                status = logStream->CreateAsyncReadContext(readContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

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
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                VERIFY_IS_TRUE(version == 1);
                VERIFY_IS_TRUE(metadataSize == logStream->QueryReservedMetadataSize() + myMetadataSize);

                const VOID* metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    VERIFY_IS_TRUE(myMetadataBuffer[i] == (UCHAR)i);
                }

                //
                // Validate data buffer
                //
                status = CompareKIoBuffers(*dataIoBuffer, *dataIoBufferWritten);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                truncateContext->Truncate(recordAsn,
                                          recordAsn);
            }

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
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

VOID
EnunerateStreamsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    const ULONG StreamCount = 32;

    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Test 1: Create container and a bunch of streams within it.
    //         Enumerate streams and verify list is correct
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;


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

    KArray<KtlLogStreamId> createdStreams(*g_Allocator);
    KArray<KtlLogStreamId> enumeratedStreams(*g_Allocator);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        ULONG metadataLength = 0x10000;
        StreamCloseSynchronizer closeStreamSync;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;

        for (ULONG i = 0; i < StreamCount; i++)
        {
            KGuid logStreamGuid;
            KtlLogStreamId logStreamId;
            KtlLogStream::SPtr logStream;

            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

            status = createdStreams.Append(logStreamId);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

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
    }

    //
    // Enumerate and verify the results
    //
    KtlLogContainer::AsyncEnumerateStreamsContext::SPtr enumerateStreamsAsync;

    status = logContainer->CreateAsyncEnumerateStreamsContext(enumerateStreamsAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    enumerateStreamsAsync->StartEnumerateStreams(enumeratedStreams,
                                                 NULL,
                                                 sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    VERIFY_IS_TRUE( enumeratedStreams.Count() == createdStreams.Count() );

    ULONG foundCount = 0;
    for (ULONG i = 0; i < enumeratedStreams.Count(); i++)
    {
        BOOLEAN found = FALSE;
        KtlLogStreamId id = enumeratedStreams[i];

        for (ULONG j = 0; j < createdStreams.Count(); j++)
        {
            if (id == createdStreams[j])
            {
                foundCount++;
                found = TRUE;
                break;
            }
        }

        VERIFY_IS_TRUE(found ? true : false);
    }

    VERIFY_IS_TRUE(foundCount == createdStreams.Count());

    enumerateStreamsAsync = nullptr;

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

VOID
ReadAsnTest(
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

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
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

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_SUCCESS);
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;
            KIoBuffer::SPtr dataIoBufferWritten;

            KtlLogAsn recordAsn42 = 42;
            KtlLogAsn recordAsn96 = 96;
            KtlLogAsn recordAsn156 = 156;
            KtlLogAsn recordAsn112 = 112;
            KtlLogAsn recordAsnRead;
            ULONG myMetadataSize = 0x100;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // TODO: Verify size of metadataLength

            // Note that stream is closed since last reference to
            // logStream is lost in its destructor

            //
            // Write records
            //
            {
                //
                // Build a data buffer (8 different 64K elements)
                //
                ULONG dataBufferSize = 16 * 0x1000;

                status = KIoBuffer::CreateEmpty(dataIoBufferWritten,
                                                *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (int i = 0; i < 8; i++)
                {
                    KIoBufferElement::SPtr dataElement;
                    VOID* dataBuffer;

                    status = KIoBufferElement::CreateNew(dataBufferSize,        // Size
                                                              dataElement,
                                                              dataBuffer,
                                                              *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    for (int j = 0; j < 0x1000; j++)
                    {
                        ((PUCHAR)dataBuffer)[j] = (UCHAR)(i*j);
                    }

                    dataIoBufferWritten->AddIoBufferElement(*dataElement);
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
                VERIFY_IS_TRUE(NT_SUCCESS(status));

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
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                ULONGLONG version = 1;

                writeContext->StartWrite(recordAsn42,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBufferWritten,
                                         NULL,    // Reservation
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                writeContext->Reuse();
                version = 2;
                writeContext->StartWrite(recordAsn96,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBufferWritten,
                                         NULL,    // ParentAsync
                                         NULL,    // Reservation
                                         sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                writeContext->Reuse();
                version = 3;
                writeContext->StartWrite(recordAsn156,
                                         version,
                                         logStream->QueryReservedMetadataSize() + myMetadataSize,
                                         metadataIoBuffer,
                                         dataIoBufferWritten,
                                         NULL,    // Reservation
                                         NULL,    // ParentAsync
                                         sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Read a record
            //
            {
                KtlLogStream::AsyncReadContext::SPtr readContext;

                status = logStream->CreateAsyncReadContext(readContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

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
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                VERIFY_IS_TRUE(version == 1);

                const VOID* metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    VERIFY_IS_TRUE(myMetadataBuffer[i] == (UCHAR)i);
                }

                //
                // Validate data buffer
                //
                status = CompareKIoBuffers(*dataIoBuffer, *dataIoBufferWritten);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

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
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                VERIFY_IS_TRUE((recordAsnRead == 96) ? true : false);
                VERIFY_IS_TRUE(version == 2);

                metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    if (myMetadataBuffer[i] != (UCHAR)i) VERIFY_IS_TRUE(myMetadataBuffer[i] == (UCHAR)i);
                }

                //
                // Validate data buffer
                //
                status = CompareKIoBuffers(*dataIoBuffer, *dataIoBufferWritten);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

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
                VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);



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
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                VERIFY_IS_TRUE((version == 2) ? true : false);
                VERIFY_IS_TRUE((recordAsnRead == 96) ? true : false);

                metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    VERIFY_IS_TRUE(myMetadataBuffer[i] == (UCHAR)i);
                }

                //
                // Validate data buffer
                //
                status = CompareKIoBuffers(*dataIoBuffer, *dataIoBufferWritten);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

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
                VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);


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
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                //
                // Validate some stuff
                //
                VERIFY_IS_TRUE((version == 2) ? true : false);
                VERIFY_IS_TRUE((recordAsnRead == 96) ? true : false);

                metadataBuffer = metadataIoBuffer->First()->GetBuffer();

                myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
                for (ULONG i = 0; i < myMetadataSize; i++)
                {
                    VERIFY_IS_TRUE(myMetadataBuffer[i] == (UCHAR)i);
                }

                //
                // Validate data buffer
                //
                status = CompareKIoBuffers(*dataIoBuffer, *dataIoBufferWritten);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                truncateContext->Truncate(recordAsn96,
                                               recordAsn96);
            }

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_SUCCESS);
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
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_SUCCESS);

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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    bufferU = static_cast<PUCHAR>(buffer);
    for (ULONG i = 0; i < BufferSize; i++)
    {
        bufferU[i] = (UCHAR)i + Entropy;
    }

    return(status);
}


NTSTATUS CompareKIoBuffers(
    KIoBuffer& IoBufferA,
    KIoBuffer& IoBufferB,
    ULONG HeaderBytesToSkip
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

    //
    // Skip any header bytes
    //
    bufferA += HeaderBytesToSkip;
    sizeA -= HeaderBytesToSkip;
    bufferB += HeaderBytesToSkip;
    sizeB -= HeaderBytesToSkip;
    bytesToCompare -= HeaderBytesToSkip;

    while(bytesToCompare)
    {
        ULONG len = min(sizeA, sizeB);

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
QueryRecordsTest(
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

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
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
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            // TODO: Verify size of metadataLength


            //
            // Test 1: Write a set of records with different ASN and
            // versions and sizes and query the metadata of each one to
            // verify these values.
            //
            ULONG numberRecords = 25;
            for (ULONG i = 0; i < numberRecords; i++)
            {
                KtlLogAsn asn = (i * 25)+1;

                status = WriteRecord(logStream,
                                     i+1,         // version
                                     asn,
                                     8,
                                     16 * 0x1000);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            {
                KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;

                status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (ULONG i = 0; i < numberRecords; i++)
                {
                    KtlLogAsn asn = (i * 25)+1;
                    queryRecordContext->Reuse();

                    ULONGLONG version;
                    RvdLogStream::RecordDisposition disposition;
                    ULONG ioBufferSize;
                    ULONGLONG debugInfo;

                    queryRecordContext->StartQueryRecord(asn,
                                                         &version,
                                                         &disposition,
                                                         &ioBufferSize,
                                                         &debugInfo,
                                                         nullptr,      // ParentAsync
                                                         sync);
                    status = sync.WaitForCompletion();
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VERIFY_IS_TRUE((version == i+1) ? true : false);

                    VERIFY_IS_TRUE((disposition == KtlLogStream::eDispositionPersisted) ? true : false);

                    VERIFY_IS_TRUE((ioBufferSize == ((8 * 16 * 0x1000)+0x1000)) ? true : false);

                    // TODO; check debugInfo
                }
            }

            {
                KtlLogStream::AsyncQueryRecordsContext::SPtr queryRecordsContext;

                status = logStream->CreateAsyncQueryRecordsContext(queryRecordsContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KArray<RvdLogStream::RecordMetadata> resultsArray(*g_Allocator);

                queryRecordsContext->StartQueryRecords(1,          // LowestAsn
                                                       (24 * 25)+1,// HighestAsn
                                                       resultsArray,
                                                       nullptr,    // Parent
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (ULONG i = 0; i < numberRecords; i++)
                {
                    KtlLogAsn asn = (i * 25)+1;


                    VERIFY_IS_TRUE((resultsArray[i].Asn.Get() == asn.Get()) ? true : false);

                    VERIFY_IS_TRUE((resultsArray[i].Version == i+1) ? true : false);

                    VERIFY_IS_TRUE((resultsArray[i].Disposition == KtlLogStream::eDispositionPersisted) ? true : false);

                    VERIFY_IS_TRUE((resultsArray[i].Size == ((8 * 16 * 0x1000)+0x1000)) ? true : false);

                    // TODO; check debugInfo
                }
            }

            //
            // Test 1a: Write a set of records with different ASN and
            // same versions and sizes and query the metadata of each one to
            // verify these values. This variation has a huge number of
            // records
            //
            numberRecords = 512;
            ULONG fixedASNOffset = 10000;
            for (ULONG i = 0; i < numberRecords; i++)
            {
                KtlLogAsn asn = fixedASNOffset + (i * 25)+1;

                status = WriteRecord(logStream,
                                     1,         // version
                                     asn,
                                     4,
                                     0x1000);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            {
                KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;

                status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (ULONG i = 0; i < numberRecords; i++)
                {
                    KtlLogAsn asn = fixedASNOffset + (i * 25)+1;
                    queryRecordContext->Reuse();

                    ULONGLONG version;
                    RvdLogStream::RecordDisposition disposition;
                    ULONG ioBufferSize;
                    ULONGLONG debugInfo;

                    queryRecordContext->StartQueryRecord(asn,
                                                         &version,
                                                         &disposition,
                                                         &ioBufferSize,
                                                         &debugInfo,
                                                         nullptr,      // ParentAsync
                                                         sync);
                    status = sync.WaitForCompletion();
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VERIFY_IS_TRUE((version == 1) ? true : false);

                    VERIFY_IS_TRUE((disposition == KtlLogStream::eDispositionPersisted) ? true : false);

                    // TODO; check debugInfo
                }
            }

            {
                KtlLogStream::AsyncQueryRecordsContext::SPtr queryRecordsContext;

                status = logStream->CreateAsyncQueryRecordsContext(queryRecordsContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KArray<RvdLogStream::RecordMetadata> resultsArray(*g_Allocator);

                queryRecordsContext->StartQueryRecords(fixedASNOffset + 1,          // LowestAsn
                                                       fixedASNOffset + (numberRecords * 25)+1,// HighestAsn
                                                       resultsArray,
                                                       nullptr,    // Parent
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (ULONG i = 0; i < numberRecords; i++)
                {
                    KtlLogAsn asn = fixedASNOffset + (i * 25)+1;


                    VERIFY_IS_TRUE((resultsArray[i].Asn.Get() == asn.Get()) ? true : false);

                    VERIFY_IS_TRUE((resultsArray[i].Version == 1) ? true : false);

                    VERIFY_IS_TRUE((resultsArray[i].Disposition == KtlLogStream::eDispositionPersisted) ? true : false);

                    // TODO; check debugInfo
                }
            }

            //
            // Test 2: Query for an ASN that doesn't exist and verify
            // error code
            //
            {
                KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;
                ULONGLONG version;
                RvdLogStream::RecordDisposition disposition;
                ULONG ioBufferSize;
                ULONGLONG debugInfo;

                status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                {
                    KtlLogAsn asn = 3;
                    queryRecordContext->Reuse();

                    queryRecordContext->StartQueryRecord(asn,
                                                         &version,
                                                         &disposition,
                                                         &ioBufferSize,
                                                         &debugInfo,
                                                         nullptr,      // ParentAsync
                                                         sync);
                    status = sync.WaitForCompletion();
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VERIFY_IS_TRUE((version == 0) ? true : false);

                    VERIFY_IS_TRUE((disposition == KtlLogStream::eDispositionNone) ? true : false);

                    VERIFY_IS_TRUE((ioBufferSize == 0) ? true : false);

                    // TODO; check debugInfo
                }

                {
                    KtlLogAsn asn = 3000;
                    queryRecordContext->Reuse();

                    queryRecordContext->StartQueryRecord(asn,
                                                         &version,
                                                         &disposition,
                                                         &ioBufferSize,
                                                         &debugInfo,
                                                         nullptr,      // ParentAsync
                                                         sync);
                    status = sync.WaitForCompletion();
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VERIFY_IS_TRUE((version == 0) ? true : false);

                    VERIFY_IS_TRUE((disposition == KtlLogStream::eDispositionNone) ? true : false);

                    VERIFY_IS_TRUE((ioBufferSize == 0) ? true : false);

                    // TODO; check debugInfo
                }
            }

            //
            // Test 3: Query for a range of ASN that doesn't exist and
            // verify the error code
            //
            {
                KtlLogStream::AsyncQueryRecordsContext::SPtr queryRecordsContext;

                status = logStream->CreateAsyncQueryRecordsContext(queryRecordsContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KArray<RvdLogStream::RecordMetadata> resultsArray(*g_Allocator);

                queryRecordsContext->StartQueryRecords(3000,          // LowestAsn
                                                       4000,// HighestAsn
                                                       resultsArray,
                                                       nullptr,    // Parent
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((resultsArray.Count() == 0) ? true : false);
            }

            //
            // Test 4: Query for a range of ASN with no records and
            // verify
            //
            {
                KtlLogStream::AsyncQueryRecordsContext::SPtr queryRecordsContext;

                status = logStream->CreateAsyncQueryRecordsContext(queryRecordsContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KArray<RvdLogStream::RecordMetadata> resultsArray(*g_Allocator);

                queryRecordsContext->StartQueryRecords(3,          // LowestAsn
                                                       18,         // HighestAsn
                                                       resultsArray,
                                                       nullptr,    // Parent
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((resultsArray.Count() == 0) ? true : false);
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
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logContainer = nullptr;

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
ForcePreAllocReallocTest(
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

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x40000;   // should be more than DefaultPreAllocMetadataSize

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
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
        // Open the stream and setup for tests
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

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
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            // TODO: Verify size of metadataLength


            //
            // Write stream metadata with length 0x30000
            //

            //
            // Write record ASN 10 with length 0x200000 and metadata length 0x8000
            //

            //
            // Write record ASN 20 with length 0x210000 and metadata length 0x8000
            //

            //
            // Write record ASN 30 with length 0x210000 and metadata length 0x9000
            //

            // TODO:

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;
        }


        //
        // Open the stream and perform some tests
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

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
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            // TODO: Verify size of metadataLength


            //
            // Default values for preallocated bufers are in
            // KtlLogMarshal.h and are as follows:
            //
            // static const ULONG DefaultPreAllocRecordMetadataSize = 0x4000;    // 16K
            // static const ULONG DefaultPreAllocMetadataSize = 0x20000;         // 128K
            // static const ULONG DefaultPreAllocDataSize = 0x100000;            // 1MB
            //

            //
            // Test 1: Read stream metadata, this should force a
            // realloc of the metadata size to 0x30000
            //

            // Test 2: Write stream metadata to 0x40000 and then read
            // it again. This should not realloc since the write should
            // have updated the new metadata size
            //

            //
            // Test 3: Read ASN 10, this should cause realloc for
            // record metadata and data
            //

            //
            // Test 4: Read ASN 20, this should cause realloc for data
            //

            //
            // Test 5: Read ASN 30, this should cause realloc for
            // metadata
            //

            //
            // Test 6: Write ASN 40 with length 0x220000 and metadata
            // length 0xa000, read the record which should not realloc
            //


            //
            // Test 7: Write record to stream while holding onto
            // record metadata buffer. Try to read buffer to ensure it
            // succeeds and then try to write and see that it throws an
            // exception.
            //

            //
            // Test 8: Write record to stream while holding onto
            // record data buffer. Try to read buffer to ensure it
            // succeeds and then try to write and see that it throws an
            // exception.
            //

            // TODO:
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

        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logContainer = nullptr;

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
StreamMetadataTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    StreamCloseSynchronizer closeStreamSync;
    ULONG metadataLength = 0x10000;
    ULONG smallerMetadataLength = 0x1000;

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

        KIoBuffer::SPtr metadataWrittenIoBuffer;
        status = CreateAndFillIoBuffer(0xAB,
                                       metadataLength,
                                       metadataWrittenIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


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

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
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


            //
            // Read the empty metadata
            //
            {
                KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
                KIoBuffer::SPtr metadataReadIoBuffer;

                status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);
            }


            //
            // Write then read back the metadata
            //
            {
                KtlLogStream::AsyncWriteMetadataContext::SPtr writeMetadataAsync;
                KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
                KIoBuffer::SPtr metadataReadIoBuffer;

                status = logStream->CreateAsyncWriteMetadataContext(writeMetadataAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                writeMetadataAsync->StartWriteMetadata(metadataWrittenIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = CompareKIoBuffers(*metadataWrittenIoBuffer, *metadataReadIoBuffer);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG openedMetadataLength;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &openedMetadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(openedMetadataLength == metadataLength);

            //
            // Read and verify the metadata
            //
            {
                KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
                KIoBuffer::SPtr metadataReadIoBuffer;

                status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = CompareKIoBuffers(*metadataWrittenIoBuffer, *metadataReadIoBuffer);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
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
                                               smallerMetadataLength,
                                               metadataWrittenIoBuffer);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = logStream->CreateAsyncWriteMetadataContext(writeMetadataAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                writeMetadataAsync->StartWriteMetadata(metadataWrittenIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                                       NULL,    // ParentAsync
                                                       sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = CompareKIoBuffers(*metadataWrittenIoBuffer, *metadataReadIoBuffer);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
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

VOID
AliasTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    StreamCloseSynchronizer closeStreamSync;
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    KString::SPtr persistedAlias = KString::Create(L"PersistedAliasIsInMyHead",
                                   *g_Allocator);
    KString::CSPtr persistedAliasConst = persistedAlias.RawPtr();
                                   
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


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
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                    persistedAliasConst,
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

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;
            
            //
            // Now let's resolve the alias and verify
            //
            KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
            KtlLogStreamId resolvedLogStreamId;

            status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            resolveAsync->StartResolveAlias(persistedAliasConst,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            VERIFY_IS_TRUE((resolvedLogStreamId == logStreamId) ? true : false);


            //
            // try with a name too long
            //
            KtlLogStreamId logStreamId2;

            KGuid logStreamGuid2;
            logStreamGuid2.CreateNew();
            logStreamId2 = static_cast<KtlLogStreamId>(logStreamGuid2);
            KString::SPtr aliasTooLong = KString::Create(L"01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789",
                                                   *g_Allocator);
            KAssert(aliasTooLong != nullptr);
            KString::CSPtr aliasTooLongConst = aliasTooLong.RawPtr();

            createStreamAsync->Reuse();
            createStreamAsync->StartCreateLogStream(logStreamId2,
                                                    aliasTooLongConst,
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_NAME_TOO_LONG);

        }

        //
        // Some straight assign, resolve, remove work
        //
        {
            KtlLogContainer::AsyncAssignAliasContext::SPtr assignAsync;
            KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
            KtlLogContainer::AsyncRemoveAliasContext::SPtr removeAsync;
            KtlLogStreamId resolvedLogStreamId;

            KString::SPtr string = KString::Create(L"AliasIsInMyHead",
                                                   *g_Allocator);
            KAssert(string != nullptr);
            
            KString::CSPtr stringConst = string.RawPtr();

            status = logContainer->CreateAsyncAssignAliasContext(assignAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = logContainer->CreateAsyncRemoveAliasContext(removeAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            assignAsync->StartAssignAlias(stringConst,
                                          logStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            resolveAsync->StartResolveAlias(stringConst,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            VERIFY_IS_TRUE((resolvedLogStreamId == logStreamId) ? true : false);

            removeAsync->StartRemoveAlias(stringConst,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            resolveAsync->Reuse();
            resolveAsync->StartResolveAlias(stringConst,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

            //
            // Use max name length string
            //
            KString::SPtr aliasMaxLength = KString::Create(L"01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567",
                                                   *g_Allocator);
            KAssert(aliasMaxLength != nullptr);
            
            KString::CSPtr aliasMaxLengthConst = aliasMaxLength.RawPtr();

            assignAsync->Reuse();
            assignAsync->StartAssignAlias(aliasMaxLengthConst,
                                          logStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            resolveAsync->Reuse();
            resolveAsync->StartResolveAlias(aliasMaxLengthConst,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            VERIFY_IS_TRUE((resolvedLogStreamId == logStreamId) ? true : false);


            //
            // try with a name too long
            //
            KString::SPtr aliasTooLong = KString::Create(L"01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789",
                                                   *g_Allocator);
            KAssert(aliasTooLong != nullptr);
            
            KString::CSPtr aliasTooLongConst = aliasTooLong.RawPtr();

            assignAsync->Reuse();
            assignAsync->StartAssignAlias(aliasTooLongConst,
                                          logStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_NAME_TOO_LONG);


            //
            // Remove a non existant alias
            //
            KString::SPtr aliasDontExist = KString::Create(L"ThisAliasDoesntExist",
                                                   *g_Allocator);
            KAssert(aliasTooLong != nullptr);
            
            KString::CSPtr aliasDontExistConst = aliasDontExist.RawPtr();
            
            removeAsync->Reuse();
            removeAsync->StartRemoveAlias(aliasDontExistConst,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

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
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(persistedLogStreamId,
                                                    NULL,
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

            assignAsync->Reuse();
            assignAsync->StartAssignAlias(persistedAliasConst,
                                          persistedLogStreamId,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Re-open the just created log container
    //
    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
        KtlLogContainer::SPtr logContainer;
        KSynchronizer sync;

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
        // Now let's resolve the alias and verify
        //
        KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
        KtlLogStreamId resolvedLogStreamId;

        status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        resolveAsync->StartResolveAlias(persistedAliasConst,
                                        &resolvedLogStreamId,
                                        NULL,          // ParentAsync
                                        sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // LogStreamId should resolve back to itself
        //
        VERIFY_IS_TRUE((resolvedLogStreamId == persistedLogStreamId) ? true : false);


        //
        // Verify removed alias is gone
        //
        KString::SPtr string = KString::Create(L"AliasIsInMyHead",
                                               *g_Allocator);
        KAssert(string != nullptr);
        
        KString::CSPtr stringConst = string.RawPtr();
        
        resolveAsync->Reuse();
        resolveAsync->StartResolveAlias(stringConst,
                                        &resolvedLogStreamId,
                                        NULL,          // ParentAsync
                                        sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);



        //
        // Create 2 streams with the same alias. The alias should be
        // cleared from the first
        //
        {
            KtlLogStreamId logStreamId1;
            KtlLogStreamId logStreamId2;
            KGuid logStreamGuid1;
            KGuid logStreamGuid2;

            logStreamGuid1.CreateNew();
            logStreamId1 = static_cast<KtlLogStreamId>(logStreamGuid1);

            logStreamGuid2.CreateNew();
            logStreamId2 = static_cast<KtlLogStreamId>(logStreamGuid2);

            KString::SPtr aliasName = KString::Create(L"AliasName",
                                                   *g_Allocator);
            KAssert(aliasName != nullptr);
            
            KString::CSPtr aliasNameConst = aliasName.RawPtr();

            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength = 0x10000;
            KBuffer::SPtr securityDescriptor = nullptr;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            createStreamAsync->StartCreateLogStream(logStreamId1,
                                                    aliasNameConst,
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

            KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync2;
            KtlLogStreamId resolvedLogStreamId2;

            status = logContainer->CreateAsyncResolveAliasContext(resolveAsync2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            resolveAsync2->StartResolveAlias(aliasNameConst,
                                            &resolvedLogStreamId2,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            VERIFY_IS_TRUE((resolvedLogStreamId2 == logStreamId1) ? true : false);

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;                

            createStreamAsync->Reuse();
            createStreamAsync->StartCreateLogStream(logStreamId2,
                                                    aliasNameConst,
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

            status = logContainer->CreateAsyncResolveAliasContext(resolveAsync2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            resolveAsync2->StartResolveAlias(aliasNameConst,
                                            &resolvedLogStreamId2,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // LogStreamId should resolve back to itself
            //
            VERIFY_IS_TRUE((resolvedLogStreamId2 == logStreamId2) ? true : false);


            //
            // Now assign the alias back to logStream1 - this should
            // remove the alias from being associated with logStream2
            //
            KtlLogContainer::AsyncAssignAliasContext::SPtr assignAsync;
            status = logContainer->CreateAsyncAssignAliasContext(assignAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            assignAsync->StartAssignAlias(aliasNameConst,
                                          logStreamId1,
                                          NULL,          // ParentAsync
                                          sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            resolveAsync2->Reuse();
            resolveAsync2->StartResolveAlias(aliasNameConst,
                                            &resolvedLogStreamId2,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE((resolvedLogStreamId2 == logStreamId1) ? true : false);

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;
            
            //
            // Now delete logStream1 and verify that the alias has been
            // removed as well
            //
            KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;

            status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            deleteStreamAsync->StartDeleteLogStream(logStreamId1,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            resolveAsync2->Reuse();
            resolveAsync2->StartResolveAlias(aliasNameConst,
                                            &resolvedLogStreamId2,
                                            NULL,          // ParentAsync
                                            sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logContainer = nullptr;
    }

    //
    // Delete the log container
    //
    {
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
        KSynchronizer sync;

        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

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

        VOID
        CloseCompletion(
            __in KAsyncContextBase* const ParentAsync,
            __in KtlLogStream& LogStream,
            __in NTSTATUS Status
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
        enum FSMState { Initial, CreateStream, AssignAlias, StartVerifyAlias, VerifyAlias, RemoveAlias, CleanupStream, Completed, CloseStream };

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
        KString::CSPtr _Alias;
        KtlLogStream::SPtr _LogStream;
};


VOID AsyncAliasStressContext::AliasStressCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
)
{
    UNREFERENCED_PARAMETER(ParentAsync);

    AliasStressFSM(Async.Status());
}

VOID
AsyncAliasStressContext::CloseCompletion(
    __in KAsyncContextBase* const ParentAsync,
    __in KtlLogStream& LogStream,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    UNREFERENCED_PARAMETER(LogStream);

    AliasStressFSM(Status);
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

    if (_State == CleanupStream)
    {
        _State = CreateStream;
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
            ULONG_PTR foo = (ULONG_PTR)this;
            ULONG foo1 = (ULONG)foo;
            srand(foo1);
        }

        _State = CleanupStream;
        return;
    }

    if (_State == VerifyAlias)
    {
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

    KString::SPtr alias = KString::Create(*g_Allocator,
                             KtlLogContainer::MaxAliasLength);
    
    VERIFY_IS_TRUE(alias ? true : false);

    b = alias->FromULONGLONG( ((ULONGLONG)(this) + _RepeatCount),
                              TRUE);   // In hex
    VERIFY_IS_TRUE(b ? true : false);
    
    _Alias = KString::CSPtr(alias.RawPtr());
}

VOID AsyncAliasStressContext::AliasStressFSM(
    __in NTSTATUS Status
)
{
    KAsyncContextBase::CompletionCallback completion(this, &AsyncAliasStressContext::AliasStressCompletion);

    if (! NT_SUCCESS(Status))
    {
        _State = Completed;
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
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                _LogStream = nullptr;
                KBuffer::SPtr securityDescriptor = nullptr;
                createStreamAsync->StartCreateLogStream(_LogStreamId,
                                                        _Alias,
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
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
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

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
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                resolveAsync->StartResolveAlias(_Alias,
                                                &_ResolvedLogStreamId,
                                                this,
                                                completion);

                NextState();
                return;
            }

            case VerifyAlias:
            {
                VERIFY_IS_TRUE((_ResolvedLogStreamId == _LogStreamId) ? true : false);

                NextState();
                break;
            }

            case RemoveAlias:
            {
                KtlLogContainer::AsyncRemoveAliasContext::SPtr removeAsync;

                Status = _LogContainer->CreateAsyncRemoveAliasContext(removeAsync);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                removeAsync->StartRemoveAlias(_Alias,
                                              this,
                                              completion);

                NextState();
                return;
            }

            case CleanupStream:
            {
                NextState();
                if (_LogStream)
                {
                    KtlLogStream::CloseCompletionCallback closeCompletion(this, &AsyncAliasStressContext::CloseCompletion);
                    _LogStream->StartClose(this, closeCompletion);
                    _LogStream = nullptr;
                    return;
                }
                break;
            }
            
            case Completed:
            {
                _State = CloseStream;

                if (_LogStream)
                {
                    KtlLogStream::CloseCompletionCallback closeCompletion(this, &AsyncAliasStressContext::CloseCompletion);
                    _LogStream->StartClose(this, closeCompletion);
                    _LogStream = nullptr;
                    return;
                }

                // Fall through
            }

            case CloseStream:
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
#pragma warning(disable:4127)   // C4127: conditional expression is constant
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
    ContainerCloseSynchronizer closeContainerSync;
    KSynchronizer sync;


    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    {
#ifdef FEATURE_TEST
        const ULONG numberAsyncs = 8;
        const ULONG numberRepeats = 4;
#else
        const ULONG numberAsyncs = 4;
        const ULONG numberRepeats = 2;
#endif
        KSynchronizer syncArr[numberAsyncs];
        AsyncAliasStressContext::SPtr asyncAliasStress[numberAsyncs];

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = AsyncAliasStressContext::Create(*g_Allocator,
                                                     ALLOCATION_TAG,
                                                     asyncAliasStress[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
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
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
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
#pragma prefix(enable: 6262, "Function uses 6336 bytes of stack space")
#pragma prefix(pop)


class AsyncTruncateStressContext : public KAsyncContextBase
{
    K_FORCE_SHARED(AsyncTruncateStressContext);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG _AllocationTag,
            __out AsyncTruncateStressContext::SPtr& Context
        );

        VOID StartStress(
            __in KtlLogStream::SPtr& LogStream,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );

        static ULONG _NextAsn;

    private:
        VOID TruncateStressCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        );

        //
        // Initial -> CreateStream or AssignAlias
        //
        enum FSMState { Initial, Truncate, Completed };

        FSMState _State;

        VOID TruncateStressFSM(
            __in NTSTATUS Status
        );


        KtlLogAsn GetNextAsn();


    protected:
        VOID OnStart();

        VOID OnReuse();

    private:
        //
        // Parameters
        //
        KtlLogStream::SPtr _LogStream;

        //
        // Internal
        //

        ULONG _AllocationTag;
        KtlLogAsn _Asn;
};


ULONG AsyncTruncateStressContext::_NextAsn = 1000;

KtlLogAsn AsyncTruncateStressContext::GetNextAsn(
    )
{
    ULONG asn = InterlockedIncrement((LONG*)&_NextAsn);

    return(asn);
}


VOID AsyncTruncateStressContext::TruncateStressCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
)
{
    UNREFERENCED_PARAMETER(ParentAsync);

    TruncateStressFSM(Async.Status());
}

VOID AsyncTruncateStressContext::TruncateStressFSM(
    __in NTSTATUS Status
)
{
    KAsyncContextBase::CompletionCallback completion(this, &AsyncTruncateStressContext::TruncateStressCompletion);

    if (! NT_SUCCESS(Status))
    {
        _LogStream = nullptr;
        Complete(Status);
        return;
    }

    do
    {
        switch (_State)
        {
            case Initial:
            {
                NTSTATUS status;

                _Asn = GetNextAsn();

                if ((_Asn.Get() % 3) == 0)
                {
                    _State = Truncate;

                    KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                    status = _LogStream->CreateAsyncTruncateContext(truncateContext);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    truncateContext->Truncate(_Asn,
                                              _Asn);

                } else {
                    _State = Completed;
                    break;
                }
                return;
            }

            case Truncate:
            {
                _State = Completed;
                break;
            }

            case Completed:
            {
                _LogStream = nullptr;
                Complete(STATUS_SUCCESS);
                return;
            }

            default:
            {
                KAssert(FALSE);

                Status = STATUS_UNSUCCESSFUL;
                _LogStream = nullptr;
                Complete(Status);
                return;
            }
        }
#pragma warning(disable:4127)   // C4127: conditional expression is constant
    } while (TRUE);
}

VOID AsyncTruncateStressContext::OnStart()
{
    TruncateStressFSM(STATUS_SUCCESS);
}


VOID AsyncTruncateStressContext::StartStress(
    __in KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;

    _LogStream = LogStream;
    Start(ParentAsyncContext, CallbackPtr);
}


AsyncTruncateStressContext::AsyncTruncateStressContext()
{
}

AsyncTruncateStressContext::~AsyncTruncateStressContext()
{
}

VOID AsyncTruncateStressContext::OnReuse()
{
}

NTSTATUS AsyncTruncateStressContext::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out AsyncTruncateStressContext::SPtr& Context
    )
{
    NTSTATUS status;
    AsyncTruncateStressContext::SPtr context;

    context = _new(AllocationTag, Allocator) AsyncTruncateStressContext();
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
    context->_LogStream = nullptr;

    Context = context.RawPtr();

    return(STATUS_SUCCESS);
}


VOID
TruncateTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    ULONG testIndex;

    //
    // Create a new log container
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Simple test 1: write a bunch of records, truncate, write a new
    // record and verify that truncation occured
    //
        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            testIndex = 1;
            ULONG testAsnOffset = testIndex * 100;
            KtlLogAsn recordAsn = testAsnOffset + 4;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (ULONG i = 1; i < 5; i++)
            {
                ULONGLONG version = 1;
                KtlLogAsn asn = i+testAsnOffset;

                status = WriteRecord(logStream,
                                     version,
                                     asn);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                truncateContext->Truncate(recordAsn,
                                               recordAsn);
            }

            //
            // Write another record
            //
            ULONGLONG version = 1;
            KtlLogAsn asn = 60+testAsnOffset;

            status = WriteRecord(logStream,
                                 version,
                                 asn);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Verify truncation point occured
            //
            {
                KtlLogAsn truncationAsn;
                KtlLogStream::AsyncQueryRecordRangeContext::SPtr qRRContext;

                status = logStream->CreateAsyncQueryRecordRangeContext(qRRContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                qRRContext->StartQueryRecordRange(nullptr,          // lowAsn
                                                  nullptr,          // highAsn
                                                  &truncationAsn,
                                                  nullptr,
                                                  sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((truncationAsn.Get() == recordAsn.Get()) ? true : false);
            }

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

    //
    // Simple test 2: write a bunch of records, truncate, write a new
    // record, close log stream and reopen it, verify that truncation occured
    //
        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            testIndex = 2;
            ULONG testAsnOffset = testIndex * 100;
            KtlLogAsn recordAsn = testAsnOffset + 4;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            openStreamAsync = nullptr;

            for (ULONG i = 1; i < 5; i = i + 1)
            {
                ULONGLONG version = 1;
                KtlLogAsn asn = i+testAsnOffset;

                status = WriteRecord(logStream,
                                     version,
                                     asn);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                truncateContext->Truncate(recordAsn,
                                               recordAsn);
            }

            //
            // Write another record
            //
            ULONGLONG version = 1;
            KtlLogAsn asn = 60+testAsnOffset;

            status = WriteRecord(logStream,
                                 version,
                                 asn);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Close and reopen log stream
            //
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

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
            // Verify truncation point occured
            //
            {
                KtlLogAsn truncationAsn;
                KtlLogStream::AsyncQueryRecordRangeContext::SPtr qRRContext;

                status = logStream->CreateAsyncQueryRecordRangeContext(qRRContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                qRRContext->StartQueryRecordRange(nullptr,          // lowAsn
                                                  nullptr,          // highAsn
                                                  &truncationAsn,
                                                  nullptr,
                                                  sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((truncationAsn.Get() == recordAsn.Get()) ? true : false);
            }
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }


    //
    // Simple test 3: write a bunch of records, truncate, write a new
    // record, close log container and reopen it, verify that truncation occured
    //
        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            testIndex = 3;
            ULONG testAsnOffset = testIndex * 100;
            KtlLogAsn recordAsn = testAsnOffset + 4;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            openStreamAsync = nullptr;

            for (ULONG i = 1; i < 5; i = i + 1)
            {
                ULONGLONG version = 1;
                KtlLogAsn asn = i+testAsnOffset;

                status = WriteRecord(logStream,
                                     version,
                                     asn);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                truncateContext->Truncate(recordAsn,
                                               recordAsn);
            }

            //
            // Write another record
            //
            ULONGLONG version = 1;
            KtlLogAsn asn = 60+testAsnOffset;

            status = WriteRecord(logStream,
                                 version,
                                 asn);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Close and reopen log stream and container
            //
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            logContainer->StartClose(NULL,
                             closeContainerSync.CloseCompletionCallback());

            status = closeContainerSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logContainer = nullptr;

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
            // Verify truncation point occured
            //
            {
                KtlLogAsn truncationAsn;
                KtlLogStream::AsyncQueryRecordRangeContext::SPtr qRRContext;

                status = logStream->CreateAsyncQueryRecordRangeContext(qRRContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                qRRContext->StartQueryRecordRange(nullptr,          // lowAsn
                                                  nullptr,          // highAsn
                                                  &truncationAsn,
                                                  nullptr,
                                                  sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((truncationAsn.Get() == recordAsn.Get()) ? true : false);
            }

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

    //
    // Simple test 4: write a bunch of records, truncate, write a new
    // record, truncate at a lower ASN, close log container and reopen it, verify that truncation occured
    //
        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            testIndex = 4;
            ULONG testAsnOffset = testIndex * 100;
            KtlLogAsn recordAsn = testAsnOffset + 4;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            openStreamAsync = nullptr;

            for (ULONG i = 1; i < 5; i = i + 1)
            {
                ULONGLONG version = 1;
                KtlLogAsn asn = i+testAsnOffset;

                status = WriteRecord(logStream,
                                     version,
                                     asn);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                truncateContext->Truncate(recordAsn,
                                               recordAsn);
            }

            //
            // Write another record
            //
            ULONGLONG version = 1;
            KtlLogAsn asn = 60+testAsnOffset;

            status = WriteRecord(logStream,
                                 version,
                                 asn);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Truncates that record
            //
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                KtlLogAsn lowerAsn = testAsnOffset + 3;

                status = logStream->CreateAsyncTruncateContext(truncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                truncateContext->Truncate(lowerAsn,
                                               lowerAsn);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Close and reopen log stream and container
            //
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            logContainer->StartClose(NULL,
                             closeContainerSync.CloseCompletionCallback());

            status = closeContainerSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logContainer = nullptr;

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
            // Verify truncation point occured
            //
            {
                KtlLogAsn truncationAsn;
                KtlLogStream::AsyncQueryRecordRangeContext::SPtr qRRContext;

                status = logStream->CreateAsyncQueryRecordRangeContext(qRRContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                qRRContext->StartQueryRecordRange(nullptr,          // lowAsn
                                                  nullptr,          // highAsn
                                                  &truncationAsn,
                                                  nullptr,
                                                  sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE((truncationAsn.Get() == recordAsn.Get()) ? true : false);
            }
            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

    //
    // Multithreaded test 1: Write a whole bunch of records up to a
    // specific ASN and then start 64 asyncs where Each asnyc is
    // assigned an ASN and if that ASN divisible by 3, truncate at that ASN. When truncate async
    // completes verify that truncation point is at or beyond requested
    // truncation point.
    //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

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
            // Write a while bunch of records
            //
            const ULONG numberAsyncs = 128;

            for (ULONG i = 0; i < numberAsyncs; i++)
            {
                ULONGLONG version = 1;
                KtlLogAsn asn = i + AsyncTruncateStressContext::_NextAsn;

                status = WriteRecord(logStream,
                                     version,
                                     asn);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }


            AsyncTruncateStressContext::SPtr contexts[numberAsyncs];
            KSynchronizer syncArr[numberAsyncs];

            for (ULONG i = 0; i < numberAsyncs; i++)
            {
                status = AsyncTruncateStressContext::Create(*g_Allocator,
                                                                 ALLOCATION_TAG,
                                                                 contexts[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            for (ULONG i = 0; i < numberAsyncs; i++)
            {
                contexts[i]->StartStress(logStream,
                                              NULL,          // ParentAsync
                                              syncArr[i]);
            }

            for (ULONG i = 0; i < numberAsyncs; i++)
            {
                status = syncArr[i].WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            logStream->StartClose(NULL,
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
    deleteContainerAsync->StartDeleteLogContainer(diskId,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}

ULONGLONG RoundUpTo(
    ULONGLONG value,
    ULONGLONG k
    )
{
    value = (value + (k-1)) & ~(k-1);
    return(value);
}


ULONGLONG RoundDownTo(
    ULONGLONG value,
    ULONGLONG k
    )
{
    value = (value) & ~(k-1);
    return(value);
}

bool IsWithinRange(
    ULONGLONG value,
    ULONGLONG n,
    ULONGLONG range
    )
{
    ULONGLONG nearLow = RoundDownTo(n-1, range);
    ULONGLONG nearHigh = RoundUpTo(n+1, range);

    return( (value >= nearLow) && (value <= nearHigh) );
}

ULONGLONG RoundUpTo64K(
    ULONGLONG value
    )
{
    ULONGLONG k = 64 * 1024;

    return(RoundUpTo(value, k));
}


ULONGLONG RoundDownTo64K(
    ULONGLONG value
    )
{
    ULONGLONG k = 64 * 1024;

    return(RoundDownTo(value, k));
}

#define IsWithin64K(a,b) IsWithinRange( (a), (b), (64 * 1024) )

#define MasterBlockSize 0x1000

ULONGLONG
ToFileAddress(__in ULONGLONG Lsn, __in ULONGLONG LogFileLsnSpace)
{
    return (Lsn % LogFileLsnSpace) + MasterBlockSize; // Allow for log header
}

typedef enum
{
    Front = 0,
    End = 1,
    Middle = 2,
    Wrap = 3
} TrimmingType;

VOID
VerifyTrimmedCorrectly(
    __in TrimmingType trimmingType,
    __inout KtlLogStream::SPtr& logStream,
    __in KString::SPtr streamName,
    __in KtlLogStreamId logStreamId,
    __in KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync,
    __in KtlLogAsn
   )
{
    NTSTATUS status;
    KSynchronizer sync;
    StreamCloseSynchronizer closeStreamSync;

    //
    // Get the lowest and highest LSNs
    //
    KtlLogStream::AsyncQueryRecordRangeContext::SPtr qrrContext;
    status = logStream->CreateAsyncQueryRecordRangeContext(qrrContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogAsn lowestAsn, highestAsn, truncateAsn;
    KtlLogAsn lowestAsn2, highestAsn2, truncateAsn2;
    qrrContext->StartQueryRecordRange(&lowestAsn, &highestAsn, &truncateAsn, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogStream::AsyncQueryRecordContext::SPtr qrContext;
    status = logStream->CreateAsyncQueryRecordContext(qrContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONGLONG lowestLsn, highestLsn;
    ULONGLONG lowestLsn2, highestLsn2;
    qrContext->StartQueryRecord(lowestAsn, NULL, NULL, NULL, &lowestLsn, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    qrContext->Reuse();
    qrContext->StartQueryRecord(highestAsn, NULL, NULL, NULL, &highestLsn, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Close stream
    //
    logStream->StartClose(NULL,
        closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logStream = nullptr;

    //
    // Obtain the allocations for the stream
    //
    KBlockFile::SPtr file;
    KArray<KBlockFile::AllocatedRange> allocations(*g_Allocator);
    KWString path(*g_Allocator, (PWCHAR)*streamName);
    ULONGLONG fileLength;

    status = KBlockFile::CreateSparseFile(path, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    fileLength = file->QueryFileSize();
    status = file->QueryAllocations(0, fileLength, allocations, sync, NULL, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    file = nullptr;
    KNt::Sleep(1000);

    ULONG metadataLength;

    openStreamAsync->Reuse();
    openStreamAsync->StartOpenLogStream(logStreamId,
        &metadataLength,
        logStream,
        NULL,    // ParentAsync
        sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Get the lowest and highest LSNs after reopen
    //
    KtlLogStream::AsyncQueryRecordRangeContext::SPtr qrrContext2;
    status = logStream->CreateAsyncQueryRecordRangeContext(qrrContext2);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    qrrContext2->StartQueryRecordRange(&lowestAsn2, &highestAsn2, &truncateAsn2, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogStream::AsyncQueryRecordContext::SPtr qrContext2;
    status = logStream->CreateAsyncQueryRecordContext(qrContext2);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    qrContext2->StartQueryRecord(lowestAsn2, NULL, NULL, NULL, &lowestLsn2, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    qrContext2->Reuse();
    qrContext2->StartQueryRecord(highestAsn2, NULL, NULL, NULL, &highestLsn2, NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream->StartClose(NULL,
        closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logStream = nullptr;


    ULONGLONG lsnSpace = fileLength - 2 * MasterBlockSize;
    ULONGLONG lowestFileAddress;
    ULONGLONG highestFileAddress;

    lowestFileAddress = ToFileAddress(lowestLsn2, lsnSpace);
    highestFileAddress = ToFileAddress(highestLsn2, lsnSpace);

    printf("LowestLsn %llx, LowestLsn2 %llx, LowestFileOffset %llx\n", lowestLsn, lowestLsn2, lowestFileAddress);
    printf("HighestLsn %llx, HighestLsn2 %llx, HighestFileOffset %llx\n", highestLsn, highestLsn2, highestFileAddress);
    printf("%d allocations, %llx fileLength\n", allocations.Count(), fileLength);
    for (ULONG i = 0; i < allocations.Count(); i++)
    {
        printf("    offset: %16llx to %16llx     length: %16llx\n", allocations[i].Offset,
                                                                       allocations[i].Offset + allocations[i].Length,
                                                                       allocations[i].Length);
    }

    if (trimmingType == TrimmingType::Front)
    {
        //   Case: [Mxxxx          M]
        VERIFY_IS_TRUE(allocations.Count() == 2);
    } else if (trimmingType == TrimmingType::End) {
        //   Case: [M          xxxxxM]
        VERIFY_IS_TRUE(allocations.Count() == 2);
    } else if (trimmingType == TrimmingType::Middle) {
        //   Case: [M      xxxxx    M]
        VERIFY_IS_TRUE(allocations.Count() == 3);

    } else if (trimmingType == TrimmingType::Wrap) {
        //   Case: [Mxx     xxxM]
        VERIFY_IS_TRUE(allocations.Count() == 2);
    } else {
        VERIFY_IS_TRUE(false);
    }
}

VOID
VerifySparseTruncateTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    )
{
    UNREFERENCED_PARAMETER(logManager);

    NTSTATUS status;
    KSynchronizer sync;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    ULONG metadataLength = 0x10000;

    KtlLogAsn asn = 1;

    KtlLogContainer::SPtr logContainer;
    KtlLogStream::SPtr logStream;

    KString::SPtr containerName;
    KString::SPtr streamName;
    KtlLogContainerId logContainerId;
    KtlLogStreamId logStreamId;
    CreateStreamAndContainerPathnames(driveLetter,
                                      containerName,
                                      logContainerId,
                                      streamName,
                                      logStreamId
                                      );

    //
    // Create a new log container
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    createAsync->StartCreateLogContainer(*containerName,
        logContainerId,
        256 * 1024 * 1024,
        0,            // Max Number Streams
        1024 * 1024,            // Max Record Size
        0,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
    status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    KBuffer::SPtr securityDescriptor = nullptr;
    ULONGLONG eightGB = 0x200000000;
    createStreamAsync->StartCreateLogStream(logStreamId,
        KtlLogInformation::KtlLogDefaultStreamType(),
        nullptr,           // Alias
        KString::CSPtr(streamName.RawPtr()),
        securityDescriptor,
        metadataLength,
        eightGB,
        1024*1024,
        KtlLogManager::FlagSparseFile,
        logStream,
        NULL,    // ParentAsync
        sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream->StartClose(NULL,
        closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Test 1: Write a bunch of stuff at the front of the log under 4GB
    // and no truncation. Verify that trimming occurs appropriately
    //

    RvdLogAsn testAsn = 1;
    RvdLogAsn recordAsn;
    ULONGLONG lastLsn;
    ULONG rKB;
    {
        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
        status = logStream->CreateAsyncTruncateContext(truncateContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 1; i < 64; i++)
        {
            ULONGLONG version = 1;
            testAsn = testAsn.Get() + 1;

            rKB = 512;

            status = WriteRecord(logStream,
                version,
                testAsn,
                1, // blocks
                rKB * 1024  // block size
                );

            if (status == STATUS_LOG_FULL)
            {
                printf("Log full\n");
                VERIFY_IS_TRUE(false);
            }
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        recordAsn = 0;
        VerifyTrimmedCorrectly(TrimmingType::Front, logStream, streamName, logStreamId, openStreamAsync, recordAsn);

        // Note logStream is Closed
    }


    //
    // Test 2: Write and truncate to below 4GB. Verify that trimming
    // occurs appropriately.
    //
    {
        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
        status = logStream->CreateAsyncTruncateContext(truncateContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;
        status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        lastLsn = 0;
        ULONGLONG twoGB = 0x80000000;
        while (lastLsn < twoGB)
        {
            for (ULONG i = 0; i < 256; i++)
            {
                ULONGLONG version = 1;
                testAsn = testAsn.Get() + 1;

                rKB = 512;

                status = WriteRecord(logStream,
                    version,
                    testAsn,
                    1, // blocks
                    rKB * 1024  // block size
                    );

                if (status == STATUS_LOG_FULL)
                {
                    printf("Log full\n");
                    VERIFY_IS_TRUE(false);
                }
                VERIFY_IS_TRUE(NT_SUCCESS(status));

            }

            queryRecordContext->Reuse();
            queryRecordContext->StartQueryRecord(testAsn,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 &lastLsn,
                                                 nullptr,      // ParentAsync
                                                 sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Truncates that record
            //
            recordAsn = testAsn.Get() - 128;
            truncateContext->Reuse();
            truncateContext->Truncate(recordAsn,
                                      recordAsn);
        }

        //
        // There is a timing condition here where the truncate may not
        // happen before the file is checked. So let's wait for it.
        //
        KNt::Sleep(5 * 1000);        
        
        VerifyTrimmedCorrectly(TrimmingType::Middle, logStream, streamName, logStreamId, openStreamAsync, recordAsn);

        // Note logStream is Closed
    }

    //
    // Test 3: Write and trunctate above 4GB (file size). Verify that
    // trimming occurs appropriately
    //
    {
        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
        status = logStream->CreateAsyncTruncateContext(truncateContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;
        status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        lastLsn = 0;
        ULONGLONG sixGB = 0x183C00000;
        while (lastLsn < sixGB)
        {
            for (ULONG i = 0; i < 256; i++)
            {
                ULONGLONG version = 1;
                testAsn = testAsn.Get() + 1;

                rKB = 512;

                status = WriteRecord(logStream,
                    version,
                    testAsn,
                    1, // blocks
                    rKB * 1024  // block size
                    );

                if (status == STATUS_LOG_FULL)
                {
                    printf("Log full\n");
                    VERIFY_IS_TRUE(false);
                }
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            queryRecordContext->Reuse();
            queryRecordContext->StartQueryRecord(testAsn,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 &lastLsn,
                                                 nullptr,      // ParentAsync
                                                 sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Truncates that record
            //
            recordAsn = testAsn.Get() - 128;
            truncateContext->Reuse();
            truncateContext->Truncate(recordAsn,
                                      recordAsn);
        }

        //
        // There is a timing condition here where the truncate may not
        // happen before the file is checked. So let's wait for it.
        //
        KNt::Sleep(5 * 1000);        
            
        VerifyTrimmedCorrectly(TrimmingType::Middle, logStream, streamName, logStreamId, openStreamAsync, recordAsn);

        // Note logStream is Closed
    }


    //
    // Test 4: Write and trunctate above 8GB (file size). This is the
    // file wrap case. Verify that trimming occurs appropriately
    //
    {
        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
        status = logStream->CreateAsyncTruncateContext(truncateContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        lastLsn = 0;
        KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;
        status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONGLONG nineGB = 0x2917D0000;
        while (lastLsn < nineGB)
        {
            for (ULONG i = 0; i < 32; i++)
            {
                ULONGLONG version = 1;
                testAsn = testAsn.Get() + 1;

                rKB = 512;

                status = WriteRecord(logStream,
                    version,
                    testAsn,
                    1, // blocks
                    rKB * 1024  // block size
                    );

                if (status == STATUS_LOG_FULL)
                {
                    printf("Log full\n");
                    VERIFY_IS_TRUE(false);
                }
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                queryRecordContext->Reuse();
                queryRecordContext->StartQueryRecord(testAsn,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     &lastLsn,
                                                     nullptr,      // ParentAsync
                                                     sync);
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                if (lastLsn >= nineGB)
                {
                    break;
                }
            }

            //
            // Truncates that record
            //
            printf("Truncate at %llx\n", recordAsn.Get());
            KDbgPrintf("Truncate at %llx\n", recordAsn.Get());
            recordAsn = testAsn.Get() - 128;
            truncateContext->Reuse();
            truncateContext->Truncate(recordAsn,
                                      recordAsn);

            //
            // Give time for truncation to occur
            //
            KNt::Sleep(1000);
        }

        //
        // Wait for all dedicated writes to complete and perform a
        // final truncate
        //
        KNt::Sleep(2 * 60 * 1000);

        truncateContext->Reuse();
        truncateContext->Truncate(recordAsn,
                                  recordAsn);

        //
        // Give time for truncation to occur
        //
        KNt::Sleep(1000);

        VerifyTrimmedCorrectly(TrimmingType::Middle, logStream, streamName, logStreamId, openStreamAsync, recordAsn); // <<--

        // Note logStream is Closed
    }

    //
    // Test 4: Write and trunctate above 8GB (file size). This is the
    // file wrap case. Verify that trimming occurs appropriately
    //
    {
        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
        status = logStream->CreateAsyncTruncateContext(truncateContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        lastLsn = 0;
        KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;
        status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONGLONG thirteenGB = 0x340000000;
        while (lastLsn < thirteenGB)
        {
            for (ULONG i = 0; i < 256; i++)
            {
                ULONGLONG version = 1;
                testAsn = testAsn.Get() + 1;

                rKB = 512;

                status = WriteRecord(logStream,
                    version,
                    testAsn,
                    1, // blocks
                    rKB * 1024  // block size
                    );

                if (status == STATUS_LOG_FULL)
                {
                    printf("Log full\n");
                    VERIFY_IS_TRUE(false);
                }
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            queryRecordContext->Reuse();
            queryRecordContext->StartQueryRecord(testAsn,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 &lastLsn,
                                                 nullptr,      // ParentAsync
                                                 sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Truncates that record
            //
            recordAsn = testAsn.Get() - 128;
            truncateContext->Reuse();
            truncateContext->Truncate(recordAsn,
                                      recordAsn);
        }

        //
        // There is a timing condition here where the truncate may not
        // happen before the file is checked. So let's wait for it.
        //
        KNt::Sleep(5 * 1000);        
        
        VerifyTrimmedCorrectly(TrimmingType::Middle, logStream, streamName, logStreamId, openStreamAsync, recordAsn);

        // Note logStream is Closed
    }

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteContainerAsync->StartDeleteLogContainer(*containerName,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}



VOID
WriteTruncateAndRecoverTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    ULONG metadataLength = 0x10000;

    ULONGLONG version = 1;
    KtlLogAsn asn = 1;

    KtlLogContainer::SPtr logContainer;
    KtlLogStream::SPtr logStream;

    KString::SPtr containerName;
    KString::SPtr streamName;
    KtlLogContainerId logContainerId;
    KtlLogStreamId logStreamId;
    CreateStreamAndContainerPathnames(driveLetter,
                                      containerName,
                                      logContainerId,
                                      streamName,
                                      logStreamId
                                      );

    //
    // Create a new log container
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    createAsync->StartCreateLogContainer(*containerName,
        logContainerId,
        256 * 1024 * 1024,
        0,            // Max Number Streams
        1024 * 1024,            // Max Record Size
        KtlLogManager::FlagSparseFile,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
    status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    KBuffer::SPtr securityDescriptor = nullptr;
    createStreamAsync->StartCreateLogStream(logStreamId,
        KtlLogInformation::KtlLogDefaultStreamType(),
        nullptr,           // Alias
        KString::CSPtr(streamName.RawPtr()),
        securityDescriptor,
        metadataLength,
        96 * 1024 * 1024,
        1024*1024,
        KtlLogManager::FlagSparseFile,
        logStream,
        NULL,    // ParentAsync
        sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream->StartClose(NULL,
        closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogStream::AsyncTruncateContext::SPtr truncateContext;
    status = logStream->CreateAsyncTruncateContext(truncateContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Test 1: Recover an empty (just created) log
    //
    {
        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    // Test 2: Recover a log with data only in the front
    //            [XXXXXXX        ]
    {
        ULONG kb = 256;        // 64MB
        for (ULONG i = 0; i < 128; i++)
        {
            asn = asn.Get() + 1;
            status = WriteRecord(logStream,
                version,
                asn,
                1, // blocks
                kb * 1024  // block size
                );

            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

#if 0   // TODO: Figure out how to make these tests really work. Truncate does not seem to
        //       reliably cause trimming.
    // Test 3: Recover a log with data only in the middle
    // Test 3a:Recover a log with data only in the middle and allocations written with 0
    //            [    XXXXXX     ]
    //            [ 0  XXXXXX  0  ]
    {
        KtlLogAsn recordAsn = asn.Get() / 2;

        truncateContext->Reuse();
        truncateContext->Truncate(recordAsn,
            recordAsn);
        // Let truncation happen
        KNt::Sleep(1000);

        ULONG kb = 256;        // 64MB
        for (ULONG i = 0; i < 128; i++)
        {
            asn = asn.Get() + 1;
            status = WriteRecord(logStream,
                version,
                asn,
                1, // blocks
                kb * 1024  // block size
                );

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            recordAsn = asn.Get() / 2;
            truncateContext->Reuse();
            truncateContext->Truncate(recordAsn,
                recordAsn);
        }
        KNt::Sleep(1000);

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // TODO: Really make this work

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Write at random places in file
        //
        KIoBuffer::SPtr zeroIoBuffer;
        ULONGLONG zeroOffset;
        PUCHAR buffer;
        PVOID p;
        KBlockFile::SPtr file;
        KWString streamNameString(*g_Allocator, (PWCHAR)*streamName);

        status = KIoBuffer::CreateSimple(512 * 1024, zeroIoBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        buffer = (PUCHAR)p;

        for (ULONG i = 0; i < 512 * 1024; i++)
        {
            buffer[i] = 0;
        }

        // data is between 32MB and 64MB

        status = KBlockFile::CreateSparseFile(streamNameString, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 6 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 9 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 14 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 19 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 22 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 68 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 72 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 79 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 82 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 91 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        file = nullptr;
        //
        // Sleep to allow file to finish closing
        //
        KNt::Sleep(1000);

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    // Test 4: Recover a log with data only in the end
    //            [         XXXXXE]
    {
        // TODO: How to layout the record exactly like this

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Test 5: Recover a log with data that wraps
    //            [XXXX     XXXXXX]
    {
        ULONG kb = 512;        // 64MB
        for (ULONG i = 0; i < 128; i++)
        {
            asn = asn.Get() + 1;
            status = WriteRecord(logStream,
                version,
                asn,
                1, // blocks
                kb * 1024  // block size
                );

            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Test 6: Recover a log with data that wraps and there is
    //         an exact record at the end
    //            [XXXX     XXXXXE]
    {
        // TODO: How to layout the record exactly like this

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Test 7: Recover a log completely full
    //            [XXXXXXXXXXXXXXX]
    {
        ULONG kb = 512;
        while (status != STATUS_LOG_FULL)
        {
            asn = asn.Get() + 1;
            status = WriteRecord(logStream,
                version,
                asn,
                1, // blocks
                kb * 1024  // block size
                );

            VERIFY_IS_TRUE(NT_SUCCESS(status) || (status == STATUS_LOG_FULL));
        }

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Test 8: Recover a log that is completely truncated
    //            [               ]
    {
        KtlLogAsn recordAsn = asn.Get() + 1;

        truncateContext->Reuse();
        truncateContext->Truncate(recordAsn,
            recordAsn);
        // Let truncation happen
        KNt::Sleep(1000);

        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    // Test 9: Recover a log that has allocations of zeros
    //            [   00    00    ]
    {
        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KIoBuffer::SPtr zeroIoBuffer;
        ULONGLONG zeroOffset;
        PUCHAR buffer;
        PVOID p;
        KBlockFile::SPtr file;
        KWString streamNameString(*g_Allocator, (PWCHAR)*streamName);

        status = KIoBuffer::CreateSimple(512 * 1024, zeroIoBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        buffer = (PUCHAR)p;

        for (ULONG i = 0; i < 1024 * 1024; i++)
        {
            buffer[i] = 0;
        }

        status = KBlockFile::CreateSparseFile(streamNameString, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 6 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 9 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 14 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 19 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 22 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 68 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 72 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 79 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 82 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        zeroOffset = 91 * 1024 * 1024;
        status = file->Transfer(KBlockFile::eForeground, KBlockFile::eWrite, zeroOffset, *zeroIoBuffer, sync, NULL, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        file = nullptr;
        // Wait for file to close
        KNt::Sleep(500);

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
#endif

    //
    // Test 10: Random writes and truncates and recoveries
    //
    srand((ULONG)GetTickCount());
#ifdef FEATURE_TEST
    const ULONG testIterations = 25;
#else
    const ULONG testIterations = 10;
#endif

    for (ULONG testIndex = 1; testIndex < testIterations; testIndex++)
    {
        ULONGLONG testAsnOffset = 10000 * testIndex;
        KtlLogAsn recordAsn;
        KtlLogAsn testAsn = testAsnOffset;
        BOOLEAN hitLogFull = FALSE;

        ULONG rBlocks, rKB, rTruncateFraction;

        rBlocks = (rand() % 250);

        rTruncateFraction = (rand() % 8) + 1;

        recordAsn = testAsnOffset + rBlocks / rTruncateFraction;

        for (ULONG i = 1; i < rBlocks; i++)
        {
            ULONGLONG version2 = 1;
            testAsn = testAsn.Get() + 1;

            rKB = rand() % 512;
            rKB = (rKB + 3) & ~3;   // must be 4KB multiple
            rKB += 4;

            status = WriteRecord(logStream,
                version2,
                testAsn,
                1, // blocks
                rKB * 1024  // block size
                );

            if (status == STATUS_LOG_FULL)
            {
                printf("Log full\n");
                KDbgPrintf("Log full\n");
                hitLogFull = TRUE;
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext2;
                status = logStream->CreateAsyncTruncateContext(truncateContext2);
                truncateContext2->Reuse();
                truncateContext2->Truncate(testAsn.Get() -1,
                                          testAsn.Get() -1);
                break;
            }
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        //
        // Truncates that record
        //
        truncateContext->Reuse();
        truncateContext->Truncate(recordAsn,
                                  recordAsn);

        //
        // Sleep long enough for truncation to happen
        //
        KNt::Sleep(5 * 1000);

        //
        // Close and reopen
        //
        logStream->StartClose(NULL,
            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        {
            printf("Reopening stream %d\n", testIndex);

            KBlockFile::SPtr file;
            KArray<KBlockFile::AllocatedRange> allocations(*g_Allocator);
            KWString path(*g_Allocator, (PWCHAR)*streamName);
            ULONGLONG fileLength;

            status = KBlockFile::CreateSparseFile(path, TRUE, KBlockFile::eOpenExisting, file, sync, NULL, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            fileLength = file->QueryFileSize();
            status = file->QueryAllocations(0, fileLength, allocations, sync, NULL, NULL);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            printf("%d allocations\n", allocations.Count());
            for (ULONG i = 0; i < allocations.Count(); i++)
            {
                printf("    offset: %16llx to %16llx     length: %16llx\n", allocations[i].Offset,
                                                                               allocations[i].Offset + allocations[i].Length,
                                                                               allocations[i].Length);
            }

            file = nullptr;
            //
            // Sleep to allow file to finish closing
            //
            KNt::Sleep(1000);
        }

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync2;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync2);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync2->StartOpenLogStream(logStreamId,
            &metadataLength,
            logStream,
            NULL,    // ParentAsync
            sync);

        status = sync.WaitForCompletion();

        if (status == STATUS_LOG_FULL)
        {
            VERIFY_IS_TRUE(hitLogFull ? true : false);
            KDbgPrintf("Exit test due to log full\n");
            break;
        }       
        
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Stream reopened

    }
    truncateContext = nullptr;


    // finish test
    if (NT_SUCCESS(status))
    {
        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());
        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }
    
    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    // Do not delete for now
    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteContainerAsync->StartDeleteLogContainer(*containerName,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}




VOID
ManyKIoBufferElementsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    ULONG testIndex;

    //
    // Create a new log container
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Simple test 1: write a bunch of records, that have many, many
    // KIoBufferElements
    //
        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            testIndex = 1;
            ULONG testAsnOffset = testIndex * 100;
            KtlLogAsn recordAsn = testAsnOffset + 4;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (ULONG i = 1; i < 5; i++)
            {
                ULONGLONG version = 1;
                KtlLogAsn asn = i+testAsnOffset;

                status = WriteRecord(logStream,
                                     version,
                                     asn,
                                     1024-1,
                                     0x1000);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            logStream->StartClose(NULL,
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
    deleteContainerAsync->StartDeleteLogContainer(diskId,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}

VOID
ManyVersionsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    LONGLONG logSize = 256 * 0x100000;   // 256MB
    KtlLogContainer::SPtr logContainer;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    ULONG testIndex;

    //
    // Create a new log container
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Simple test 1: write same ASNs with increasing version number
    //
        //
        // Open the stream
        //
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            testIndex = 1;
            ULONG testAsnOffset = testIndex * 100;
            KtlLogAsn recordAsn = testAsnOffset + 4;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (ULONG loops = 0; loops < 256; loops++)
            {
                for (ULONG i = 1; i < 64; i++)
                {
                    ULONGLONG version = loops+1;
                    KtlLogAsn asn = i+testAsnOffset;

                    status = WriteRecord(logStream,
                                         version,
                                         asn,
                                         2,     // 512K total
                                         64 * 0x1000);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }
            }
            logStream->StartClose(NULL,
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
    deleteContainerAsync->StartDeleteLogContainer(diskId,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}

VOID IoctlTest(
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
    ULONG recordSize = DEFAULT_MAX_RECORD_SIZE;
    ULONG newThreshold = 100 * (1024 * 1024);

    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    KIoBuffer::SPtr emptyIoBuffer;
    KIoBuffer::SPtr metadata;
    PVOID metadataBuffer;
    PUCHAR dataWritten;
    ULONG dataSizeWritten;
    ULONG offsetToStreamBlockHeaderM;
    KBuffer::SPtr workKBuffer;
    PUCHAR workBuffer;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;

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
                                            recordSize,
                                            logStream,
                                            NULL,    // ParentAsync
                                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

    status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
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


    KtlLogStream::AsyncIoctlContext::SPtr ioctl;

    status = logStream->CreateAsyncIoctlContext(ioctl);
    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);


    //
    // Test 1: Set write only to dedicated and write a record.
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;

        inBuffer = nullptr;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::WriteOnlyToDedicatedLog, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();

        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

        dataSizeWritten = 10;
        version++;

        status = SetupAndWriteRecord(logStream,
            metadata,
            emptyIoBuffer,
            version,
            asn,
            dataWritten,
            dataSizeWritten,
            offsetToStreamBlockHeaderM);

        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        // TODO: Verify written only to dedicated
    }


    //
    // Test 2: Set write to dedicated and shared and write a record.
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;

        inBuffer = nullptr;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::WriteToSharedAndDedicatedLog, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();

        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

        dataSizeWritten = 10;
        version++;

        status = SetupAndWriteRecord(logStream,
            metadata,
            emptyIoBuffer,
            version,
            asn,
            dataWritten,
            dataSizeWritten,
            offsetToStreamBlockHeaderM);

        VERIFY_IS_TRUE(NT_SUCCESS(status));

        asn += dataSizeWritten;

        // TODO: Verify written only to dedicated

        //
        // Give a chance for both writes to complete
        //
        KNt::Sleep(1000);
    }

    //
    // Test 3: Get current write information and verify that it is
    //         correct
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;
        KLogicalLogInformation::CurrentWriteInformation* writeInformation;

        inBuffer = nullptr;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::QueryCurrentWriteInformation, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();

        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

        writeInformation = (KLogicalLogInformation::CurrentWriteInformation*)outBuffer->GetBuffer();

        VERIFY_IS_TRUE((writeInformation->MaxAsnWrittenToBothLogs >= asn - dataSizeWritten) ? true : false);
    }

    //
    // Test 4: Pass an empty inbuffer to SetWriteThrottleThreshold and
    //         verify error
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;

        inBuffer = nullptr;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
    }

    //
    // Test 5: Pass an inbuffer that is too small to SetWriteThrottleThreshold and
    //         verify error
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;

        status = KBuffer::Create(sizeof(KLogicalLogInformation::WriteThrottleThreshold) - 1,
                                 inBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
    }

    //
    // Test 6: Pass an inbuffer with an invalid value and verify error
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;
        KLogicalLogInformation::WriteThrottleThreshold* writeInformation;

        status = KBuffer::Create(sizeof(KLogicalLogInformation::WriteThrottleThreshold),
                                 inBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeInformation = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeInformation->MaximumOutstandingBytes = 1;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
    }

    //
    // Test 7: Pass an inbuffer with a valid value and verify old value
    //         is correct default
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;
        KLogicalLogInformation::WriteThrottleThreshold* writeInformation;

        status = KBuffer::Create(sizeof(KLogicalLogInformation::WriteThrottleThreshold),
                                 inBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeInformation = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeInformation->MaximumOutstandingBytes = newThreshold;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeInformation = (KLogicalLogInformation::WriteThrottleThreshold*)outBuffer->GetBuffer();
        VERIFY_IS_TRUE(writeInformation->MaximumOutstandingBytes == KLogicalLogInformation::WriteThrottleThresholdNoLimit);
    }

    //
    // Test 8: Pass an inbuffer with a valid value and verify old value
    //         is value set in test 7
    //
    {
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;
        KLogicalLogInformation::WriteThrottleThreshold* writeInformation;

        status = KBuffer::Create(sizeof(KLogicalLogInformation::WriteThrottleThreshold),
                                 inBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeInformation = (KLogicalLogInformation::WriteThrottleThreshold*)inBuffer->GetBuffer();
        writeInformation->MaximumOutstandingBytes = newThreshold / 2;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::SetWriteThrottleThreshold, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeInformation = (KLogicalLogInformation::WriteThrottleThreshold*)outBuffer->GetBuffer();
        VERIFY_IS_TRUE(writeInformation->MaximumOutstandingBytes == newThreshold);
    }


    //
    // Cleanup
    //
    ioctl = nullptr;
    logStream->StartClose(NULL,
                     closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream = nullptr;
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


VOID VersionTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    //
    // Do some validation around math with casts
    //
    {
        LONGLONG expected, newLimit;
        LONG _PerStreamAllocation = 0x200000;

        LONGLONG _TotalAllocationLimit =  0x200000000;
        expected = _TotalAllocationLimit;
        newLimit = expected + _PerStreamAllocation;
        VERIFY_IS_TRUE(newLimit == 0x200200000);
    }

    {
        LONGLONG _TotalAllocationLimit =  0x200200000;
        LONG limit = 0x200000;
        InterlockedAdd64(&_TotalAllocationLimit, -1*limit);
        VERIFY_IS_TRUE(_TotalAllocationLimit == 0x200000000);
    }


    {
        LONG Size = 0x40000;
        LONGLONG _CurrentAllocations = 0x17cb0000;
        InterlockedAdd64(&_CurrentAllocations, -1*Size);
        VERIFY_IS_TRUE(_CurrentAllocations == 0x17C70000);


        _CurrentAllocations = 0x117cb0000;
        InterlockedAdd64(&_CurrentAllocations, -1*Size);
        VERIFY_IS_TRUE(_CurrentAllocations == 0x117C70000);
    }


    {
        ULONG GetSize =  0x40000;
        LONGLONG _CurrentAllocations = 0x17cb0000;
        InterlockedAdd64(&_CurrentAllocations, GetSize);
        VERIFY_IS_TRUE(_CurrentAllocations == 0x17CF0000);

        _CurrentAllocations = 0x117cb0000;
        InterlockedAdd64(&_CurrentAllocations, GetSize);
        VERIFY_IS_TRUE(_CurrentAllocations == 0x117CF0000);
    }

    {
        ULONG GetSize =  0x40000;
        LONGLONG _CurrentAllocations = 0x17cb0000;

        // Badboy
        InterlockedAdd64(&_CurrentAllocations, -1*(LONGLONG)GetSize);
        VERIFY_IS_TRUE(_CurrentAllocations == 0x17C70000);

        _CurrentAllocations = 0x117cb0000;
        InterlockedAdd64(&_CurrentAllocations, -1*(LONGLONG)GetSize);
        VERIFY_IS_TRUE(_CurrentAllocations == 0x117C70000);
    }


    NTSTATUS status;
    KSynchronizer sync;

    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;
    ULONG metadataLength = 0x10000;

    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(diskId,
        logContainerId,
        logSize,
        0,            // Max Number Streams
        0,            // Max Record Size
        0,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    
#if defined(PLATFORM_UNIX)
    if (! NT_SUCCESS(status))
    {
        printf("**** Are you running as sudo ??  You need to do so !\n");
    }
#endif
    
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
    KtlLogStream::SPtr logStream;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KBuffer::SPtr securityDescriptor = nullptr;
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    KtlLogStream::AsyncIoctlContext::SPtr ioctl;
    ULONG result;
    KBuffer::SPtr outBuffer;
    KLogicalLogInformation::CurrentBuildInformation* currentBuildInfo;

    status = logStream->CreateAsyncIoctlContext(ioctl);
    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

    ioctl->StartIoctl(KLogicalLogInformation::QueryCurrentBuildInformation, NULL, result, outBuffer, NULL, sync);
    status = sync.WaitForCompletion();

    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

    currentBuildInfo = (KLogicalLogInformation::CurrentBuildInformation*)outBuffer->GetBuffer();

    ioctl = nullptr;
    logStream->StartClose(NULL,
        closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream = nullptr;
    logContainer->StartClose(NULL,
        closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));



    printf("Test Build Number: %d - ", VER_PRODUCTBUILD);
#if DBG
    printf("CHK\n");
#else
    printf("FRE\n");
#endif

    printf("Driver Build Number %d - %s\n", currentBuildInfo->BuildNumber, currentBuildInfo->IsFreeBuild ? "FRE" : "CHK");

    VERIFY_IS_TRUE(currentBuildInfo->BuildNumber == VER_PRODUCTBUILD);
#if DBG
    VERIFY_IS_TRUE(currentBuildInfo->IsFreeBuild == FALSE);
#else
    VERIFY_IS_TRUE(currentBuildInfo->IsFreeBuild == TRUE);
#endif
}


NTSTATUS WaitForWritesToComplete(
    KtlLogStream::SPtr logStream,
    ULONGLONG waitForAsn,
    ULONG retries,
    ULONG delay
)
{
    NTSTATUS status;
    KtlLogStream::AsyncIoctlContext::SPtr ioctl;

    status = logStream->CreateAsyncIoctlContext(ioctl);
    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

    ULONG i = 0;
    while (i++ < retries)
    {
        KSynchronizer sync;
        ULONG result;
        KBuffer::SPtr outBuffer;
        KLogicalLogInformation::CurrentWriteInformation* currentWriteInfo;

        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::QueryCurrentWriteInformation, NULL, result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();

        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

        currentWriteInfo = (KLogicalLogInformation::CurrentWriteInformation*)outBuffer->GetBuffer();

        if (currentWriteInfo->MaxAsnWrittenToBothLogs >= waitForAsn)
        {
            return(STATUS_SUCCESS);
        }

        KNt::Sleep(delay);
    }

    return(STATUS_IO_TIMEOUT);
}


LongRunningWriteStress::LongRunningWriteStress()
{
}

LongRunningWriteStress::~LongRunningWriteStress()
{
}

#if DBG
#ifdef KDRIVER
VOID
ThrottledLockedMemoryTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    const ULONG NumberStreams = 16;
    
    const ULONG NumberWriteStress = 16;
    const ULONG MaxWriteRecordSize = 512 * 1024;
    const ULONGLONG AllowedLogSpace = 0xC000000;
    const ULONGLONG MaxWriteAsn = AllowedLogSpace * 64;
    const LONGLONG StreamSize = MaxWriteAsn / 2;
    LongRunningWriteStress::SPtr writeStress[NumberWriteStress];
    KSynchronizer writeSyncs[NumberWriteStress];

    
    KtlLogStreamId logStreamId[NumberStreams];
    KtlLogStream::SPtr logStream[NumberStreams];


    NTSTATUS status;
    KSynchronizer sync;

    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    
    KGuid logStreamGuid;
    ULONG metadataLength = 0x10000;
    StreamCloseSynchronizer closeStreamSync;


    //
    // Configure to throttle
    //
    {
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
        
        memoryThrottleLimits = (KtlLogManager::MemoryThrottleLimits*)inBuffer->GetBuffer();
        memoryThrottleLimits->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMax;
        memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
        memoryThrottleLimits->WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
        memoryThrottleLimits->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_PinnedMemoryLimitMin;
        memoryThrottleLimits->PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;
        memoryThrottleLimits->PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
        memoryThrottleLimits->AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;        

        configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
        
    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = 0x200000000;  // 8GB

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(diskId,
        logContainerId,
        logSize,
        0,            // Max Number Streams
        0,            // Max Record Size
        0,
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
        logStreamGuid.CreateNew();
        logStreamId[i] = static_cast<KtlLogStreamId>(logStreamGuid);

        createStreamAsync->Reuse();
        createStreamAsync->StartCreateLogStream(logStreamId[i],
            KLogicalLogInformation::GetLogicalLogStreamType(),
            nullptr,           // Alias
            nullptr,
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


    LONG pinMemoryFailureCount;
    KBuffer::SPtr outBuffer;
    ULONG result;
    KtlLogManager::DebugUnitTestInformation* debugUnitTestInformation;
    KtlLogManager::MemoryThrottleUsage* memoryThrottleUsage; 
    
    KtlLogManager::AsyncConfigureContext::SPtr configureContext;
    status = logManager->CreateAsyncConfigureContext(configureContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    
    configureContext->Reuse();
    configureContext->StartConfigure(KtlLogManager::QueryDebugUnitTestInformation,
                                     nullptr,
                                     result,
                                     outBuffer,
                                     NULL,
                                     sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    debugUnitTestInformation = (KtlLogManager::DebugUnitTestInformation*)outBuffer->GetBuffer();
    pinMemoryFailureCount = debugUnitTestInformation->PinMemoryFailureCount + 10;
    outBuffer = nullptr;
    
    for (ULONG i = 0; i < NumberWriteStress; i++)
    {
        LongRunningWriteStress::StartParameters params;

        status = LongRunningWriteStress::Create(*g_Allocator,
                                                     KTL_TAG_TEST,
                                                     writeStress[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        params.LogStream = logStream[i];
        params.MaxRandomRecordSize = MaxWriteRecordSize;
        params.LogSpaceAllowed = AllowedLogSpace;
        params.HighestAsn = MaxWriteAsn;
        params.UseFixedRecordSize = FALSE;
        params.WaitTimerInMs = 0;
        writeStress[i]->StartIt(&params,
                                NULL,
                                writeSyncs[i]);
    }

    //
    // Time bound how long this test runs
    //
    static const ULONG timeLimit = 20;
    KTimer::SPtr timer;
    LONG lastPinMemoryFailureCount = 0;

    status = KTimer::Create(timer, *g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    for (ULONG i = 0; i < timeLimit; i++)
    {   
        timer->Reuse();
        timer->StartTimer(60 * 1000, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::QueryMemoryThrottleUsage,
                                         nullptr,
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        memoryThrottleUsage = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
        printf("pinMemoryUsage %lld pinMemoryLimit %lld\n", memoryThrottleUsage->PinnedMemoryAllocations,
                                                              memoryThrottleUsage->ConfiguredLimits.PinnedMemoryLimit);
        printf("currentAllocations %I64d\n", memoryThrottleUsage->CurrentAllocations);
        outBuffer = nullptr;
        
        configureContext->Reuse();
        configureContext->StartConfigure(KtlLogManager::QueryDebugUnitTestInformation,
                                         nullptr,
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KFinally([&] { outBuffer = nullptr; });

        debugUnitTestInformation = (KtlLogManager::DebugUnitTestInformation*)outBuffer->GetBuffer();
        printf("pinMemoryFailureCount Expected %d Actual %d\n", pinMemoryFailureCount, debugUnitTestInformation->PinMemoryFailureCount);
        lastPinMemoryFailureCount = debugUnitTestInformation->PinMemoryFailureCount;
        if (debugUnitTestInformation->PinMemoryFailureCount > pinMemoryFailureCount)
        {
            break;
        }       
    }

    VERIFY_IS_TRUE(lastPinMemoryFailureCount > pinMemoryFailureCount);
    
    for (ULONG i = 0; i < NumberWriteStress; i++)
    {
        writeStress[i]->ForceCompletion();
    }
    
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
                VERIFY_IS_TRUE(NT_SUCCESS(status) || (status == STATUS_INSUFFICIENT_RESOURCES));
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



    //
    // Restore defaults
    //
    {
        KtlLogManager::MemoryThrottleLimits* memoryThrottleLimits;
        KtlLogManager::AsyncConfigureContext::SPtr configureContext2;
        KBuffer::SPtr inBuffer;

        status = logManager->CreateAsyncConfigureContext(configureContext2);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleLimits),
                                 inBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        memoryThrottleLimits = (KtlLogManager::MemoryThrottleLimits*)inBuffer->GetBuffer();
        memoryThrottleLimits->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMax;
        memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
        memoryThrottleLimits->WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
        memoryThrottleLimits->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_DefaultPinnedMemoryLimit;
        memoryThrottleLimits->PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;
        memoryThrottleLimits->PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
        memoryThrottleLimits->AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;        

        configureContext2->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits,
                                         inBuffer.RawPtr(),
                                         result,
                                         outBuffer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
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
#endif
#endif

VOID
ReservationsTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    LONGLONG logSize = 512 * 0x100000;   // 512 MB
    KtlLogContainer::SPtr logContainer;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    ULONG testIndex;

    //
    // Create a new log container. Note that this must be larger than the dedicated container size
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Open the stream
    //
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;
        KtlLogAsn asn;

        testIndex = 1;
        ULONG testAsnOffset = testIndex * 100;
        KtlLogAsn recordAsn = testAsnOffset + 4;

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
        // Test 1: Verify reservation held is 0
        //
        {
            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0, L"Expect initial reservation space to be 0");
        }

        //
        // Test 2: Write with a reservation of 0x1000 but holding 0, expect
        // failure, verify holding 0
        //
        {
            asn = 1;
            status = WriteRecord(logStream,
                                 1,            // version
                                 asn,
                                 1,            // number blocks
                                 0x1000,       // block size
                                 0x1000);      // reservation
            VERIFY_IS_TRUE(status == K_STATUS_LOG_RESERVE_TOO_SMALL,
                           L"Expect failure when trying to use reservation space that is not held");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0, L"Expect reservation space to be 0");
        }

        //
        // Test 3: Write with reservation of 0x1000 but holding 0x800, expect
        // failure, verify still holding 0x800
        //
        {
            status = ExtendReservation(logStream,
                                       0x800);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"ExtendReservation to 0x800 failed");

            asn = 2;
            status = WriteRecord(logStream,
                                 1,                   // version
                                 asn,
                                 1,            // number blocks
                                 0x1000,       // block size
                                 0x1000);      // reservation
            VERIFY_IS_TRUE(status == K_STATUS_LOG_RESERVE_TOO_SMALL,
                           L"Expect failure when trying to use reservation space that is not held");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0x800, L"Expect reservation space to be 0x800");
        }

        //
        // Test 4: Write with reservation of 0x1000 but holding 0x1000, expect
        // success, verify holding 0
        //
        {
            status = ExtendReservation(logStream,
                                       0x800);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false, L"Expect ExtendReservation to 0x1000 to pass");

            asn = 3;
            status = WriteRecord(logStream,
                                 1,                   // version
                                 asn,
                                 1,            // number blocks
                                 0x1000,       // block size
                                 0x1000);      // reservation
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"Expect succcess when trying to use reservation space that is held");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0, L"Expect reservation space to be 0");
        }

        //
        // Test 5: Extend reservation 0x10 byte at a time until 0x800 and
        // write 0x1000, expect success and verify holding 0
        //
        {
            for (ULONG i = 0; i < 0x800; i = i + 0x10)
            {
                status = ExtendReservation(logStream,
                                           0x10);
                VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                               L"Expect ExtendReservation to succeed");

                ULONGLONG reservationSpace = logStream->QueryReservationSpace();
                VERIFY_IS_TRUE(reservationSpace == (i + 0x10), L"Expect reservation space");
            }

            asn = 4;
            status = WriteRecord(logStream,
                                 1,                   // version
                                 asn,
                                 1,            // number blocks
                                 0x1000,       // block size
                                 0x1000);      // reservation
            VERIFY_IS_TRUE(status == K_STATUS_LOG_RESERVE_TOO_SMALL ? true : false,
                           L"Expect failure when trying to use reservation space that is not held");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0x800, L"Expect reservation space to be 0x800");
        }

        //
        // Test 6: Reserve 0x1000 and contract 0x10 bytes at a time until
        // 0x800 and write 0x1000, expect success and verify holding 0
        //
        {
            status = ExtendReservation(logStream,
                                       0x1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"ExtendReservation 0x1000");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0x1800, L"Expect reservation space");

            for (ULONG i = 0; i < 0x800; i = i + 0x10)
            {
                status = ContractReservation(logStream,
                                             0x10);
                VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                               L"ExtendReservation");

                reservationSpace = logStream->QueryReservationSpace();
                VERIFY_IS_TRUE(reservationSpace == (0x1800 - (i + 0x10)), L"Expect reservation space");
            }

            asn = 5;
            status = WriteRecord(logStream,
                                 1,                   // version
                                 asn,
                                 1,            // number blocks
                                 0x1000,       // block size
                                 0x1000);      // reservation
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"Expect success when trying to use reservation space that is held");

            reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0, L"Expect reservation space to be 0");
        }

        //
        // Test 7: Reserve 0x1000 and try to release 0x2000, expect
        // failure. Verify holding 0x1000
        //
        {
            status = ExtendReservation(logStream,
                                       0x1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"ExtendReservation 0x1000");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0x1000, L"Expect reservation space");

            status = ContractReservation(logStream,
                                         0x2000);
            VERIFY_IS_TRUE(status == K_STATUS_LOG_RESERVE_TOO_SMALL ? true : false,
                           L"Expect ContractReservation 0x2000 should fail");

            reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0x1000, L"Expect reservation space 0x1000");
        }

        //
        // Test 8: Reserve 0x1000 and try to write with 0x2000 reservation.
        // Expect failure. Verify still hold 0x1000. Release 0x1000 and
        // verify holding 0.
        //
        {
            asn = 8;
            status = WriteRecord(logStream,
                                 1,                   // version
                                 asn,
                                 2,            // number blocks
                                 0x1000,       // block size
                                 0x2000);      // reservation
            VERIFY_IS_TRUE(status == K_STATUS_LOG_RESERVE_TOO_SMALL ? true : false,
                           L"Expect failure when trying to use reservation space that is not held");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0x1000, L"Expect reservation space to be 0x1000");

            status = ContractReservation(logStream,
                                         0x1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"ContractReservation 0x1000 should succeed");

            reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0, L"Expect reservation space 0");
        }

        //
        // Test 9: Write until log full and try to reserve 0x1000. Expect
        // failure.
        //
        {
            ULONG i = 9;
            do
            {
                asn = i++;
                status = WriteRecord(logStream,
                                     1,                   // version
                                     asn,
                                     16,            // number blocks
                                     0x10000,       // block size
                                     0);      // reservation

                if (! ((status == STATUS_LOG_FULL) || NT_SUCCESS(status))) VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || NT_SUCCESS(status) ? true : false,
                               L"Fill log");

            } while (NT_SUCCESS(status));

            VERIFY_IS_TRUE(status == STATUS_LOG_FULL ? true : false, L"Log file should be full");

            status = ExtendReservation(logStream,
                                       0x200000);     // has 0x2038000 Free now
            VERIFY_IS_TRUE(status == STATUS_LOG_FULL ? true : false,
                           L"Expect ExtendReservation to 0x200000 should fail on full log");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0, L"Expect reservation space 0");


            //
            // Wait for both D and S logs to have written up to the latest ASN
            //
            status = WaitForWritesToComplete(logStream, asn.Get() - 1, 60, 1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

            KNt::Sleep(5 * 1000);

            status = TruncateStream(logStream,
                                    asn.Get()-10);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"Truncate should succeed");

            //
            // Wait to ensure truncation occurs
            //
            KNt::Sleep(5 * 1000);
        }

        //
        // Test 10: Take a reservation of 0x200000 and write until log full.
        // Write 0x200000 with reservation size 0x200000 and expect success.
        //
        {
            status = ExtendReservation(logStream,
                                       0x200000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"ExtendReservation 0x200000");

            ULONGLONG reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0x200000, L"Expect reservation space 0x20000");

            ULONG i = 90000;
            do
            {
                asn = i++;
                status = WriteRecord(logStream,
                                     1,                   // version
                                     asn,
                                     16,            // number blocks
                                     0x10000,       // block size
                                     0);      // reservation
                VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || NT_SUCCESS(status) ? true : false,
                               L"Fill log");

            } while (NT_SUCCESS(status));

            VERIFY_IS_TRUE(status == STATUS_LOG_FULL ? true : false, L"Log file should be full");

            asn = 100000;
            status = WriteRecord(logStream,
                                 1,                   // version
                                 asn,
                                 16,            // number blocks
                                 0x20000,       // block size
                                 0x200000);      // reservation
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false,
                           L"Expect reserved write to succeed on full log");

            reservationSpace = logStream->QueryReservationSpace();
            VERIFY_IS_TRUE(reservationSpace == 0, L"Expect reservation space 0");
        }

        logStream->StartClose(NULL,
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
    deleteContainerAsync->StartDeleteLogContainer(diskId,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}

VOID
CloseTestVerifyStreamClosed(
    KtlLogStream::SPtr logStream
)
{
    NTSTATUS status;
    KSynchronizer sync;
    StreamCloseSynchronizer closeSync;

    {
        KtlLogAsn truncationAsn;
        KtlLogStream::AsyncQueryRecordRangeContext::SPtr qRRContext;

        status = logStream->CreateAsyncQueryRecordRangeContext(qRRContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        qRRContext->StartQueryRecordRange(nullptr,          // lowAsn
                                          nullptr,          // highAsn
                                          &truncationAsn,
                                          nullptr,
                                          sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }

    {
        status = ContractReservation(logStream,
                                     300);
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

        status = ExtendReservation(logStream,
                                   300);
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }

    {
        status = WriteRecord(logStream,
                             4,            // Version
                             12,           // Asn
                             4,            // # blocks
                             0x1000,       // Block size
                             0);           // Reservation size
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }

    {
        KtlLogStream::AsyncReadContext::SPtr readContext;
        ULONGLONG version;
        ULONG metadataSize;
        KIoBuffer::SPtr dataIoBuffer;
        KIoBuffer::SPtr metadataIoBuffer;
        KtlLogAsn asn;

        status = logStream->CreateAsyncReadContext(readContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        readContext->StartReadContaining(23,
                                 &asn,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

        readContext->Reuse();
        readContext->StartReadPrevious(23,
                                 &asn,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

        readContext->Reuse();
        readContext->StartReadNext(23,
                                 &asn,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

        readContext->Reuse();
        readContext->StartRead(23,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

    }

    {
        KtlLogStream::AsyncQueryRecordsContext::SPtr queryRecordsContext;

        status = logStream->CreateAsyncQueryRecordsContext(queryRecordsContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KArray<RvdLogStream::RecordMetadata> resultsArray(*g_Allocator);

        queryRecordsContext->StartQueryRecords(1,          // LowestAsn
                                               (24 * 25)+1,// HighestAsn
                                               resultsArray,
                                               nullptr,    // Parent
                                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

    }
    {
        KtlLogAsn asn = 25;
        ULONGLONG version;
        RvdLogStream::RecordDisposition disposition;
        ULONG ioBufferSize;
        ULONGLONG debugInfo;
        KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;

        status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        queryRecordContext->StartQueryRecord(asn,
                                             &version,
                                             &disposition,
                                             &ioBufferSize,
                                             &debugInfo,
                                             nullptr,      // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }


    {
        KIoBuffer::SPtr metadataWrittenIoBuffer = nullptr;
        KtlLogStream::AsyncWriteMetadataContext::SPtr writeMetadataAsync;
        status = logStream->CreateAsyncWriteMetadataContext(writeMetadataAsync);
#ifdef UPASSTHROUGH
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
#else
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeMetadataAsync->StartWriteMetadata(metadataWrittenIoBuffer,
                                               NULL,    // ParentAsync
                                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
#endif
    }


    {
        KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
        KIoBuffer::SPtr metadataReadIoBuffer;

        status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
#ifdef UPASSTHROUGH
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
#else
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                               NULL,    // ParentAsync
                                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
#endif
    }

    {
        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);       
    }
}

VOID
CloseTestVerifyStreamOpened(
    KtlLogStream::SPtr logStream
)
{
    NTSTATUS status;
    KSynchronizer sync;

    {
        status = WriteRecord(logStream,
                             4,            // Version
                             12,           // Asn
                             4,            // # blocks
                             0x1000,       // Block size
                             0);           // Reservation size
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteRecord(logStream,
                             4,            // Version
                             19,           // Asn
                             4,            // # blocks
                             0x1000,       // Block size
                             0);           // Reservation size
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        KtlLogAsn truncationAsn;
        KtlLogStream::AsyncQueryRecordRangeContext::SPtr qRRContext;

        status = logStream->CreateAsyncQueryRecordRangeContext(qRRContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        qRRContext->StartQueryRecordRange(nullptr,          // lowAsn
                                          nullptr,          // highAsn
                                          &truncationAsn,
                                          nullptr,
                                          sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        status = ExtendReservation(logStream,
                                   300);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = ContractReservation(logStream,
                                     300);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    {
        KtlLogStream::AsyncReadContext::SPtr readContext;
        ULONGLONG version;
        ULONG metadataSize;
        KIoBuffer::SPtr dataIoBuffer;
        KIoBuffer::SPtr metadataIoBuffer;
        KtlLogAsn asn;

        status = logStream->CreateAsyncReadContext(readContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        readContext->StartReadContaining(15,
                                 &asn,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        readContext->Reuse();
        readContext->StartReadPrevious(19,
                                 &asn,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        readContext->Reuse();
        readContext->StartReadNext(12,
                                 &asn,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        readContext->Reuse();
        readContext->StartRead(12,
                                 &version,
                                 metadataSize,
                                 metadataIoBuffer,
                                 dataIoBuffer,
                                 NULL,    // ParentAsync
                                 sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        KtlLogStream::AsyncQueryRecordsContext::SPtr queryRecordsContext;

        status = logStream->CreateAsyncQueryRecordsContext(queryRecordsContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KArray<RvdLogStream::RecordMetadata> resultsArray(*g_Allocator);

        queryRecordsContext->StartQueryRecords(12,          // LowestAsn
                                               19,// HighestAsn
                                               resultsArray,
                                               nullptr,    // Parent
                                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        KtlLogAsn asn = 19;
        ULONGLONG version;
        RvdLogStream::RecordDisposition disposition;
        ULONG ioBufferSize;
        ULONGLONG debugInfo;
        KtlLogStream::AsyncQueryRecordContext::SPtr queryRecordContext;

        status = logStream->CreateAsyncQueryRecordContext(queryRecordContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        queryRecordContext->StartQueryRecord(asn,
                                             &version,
                                             &disposition,
                                             &ioBufferSize,
                                             &debugInfo,
                                             nullptr,      // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        KIoBuffer::SPtr metadataWrittenIoBuffer = nullptr;
        KtlLogStream::AsyncWriteMetadataContext::SPtr writeMetadataAsync;
        status = logStream->CreateAsyncWriteMetadataContext(writeMetadataAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = CreateAndFillIoBuffer(0xAB,
                                       0x1000,
                                       metadataWrittenIoBuffer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        writeMetadataAsync->StartWriteMetadata(metadataWrittenIoBuffer,
                                               NULL,    // ParentAsync
                                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    {
        KtlLogStream::AsyncReadMetadataContext::SPtr readMetadataAsync;
        KIoBuffer::SPtr metadataReadIoBuffer;

        status = logStream->CreateAsyncReadMetadataContext(readMetadataAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        readMetadataAsync->StartReadMetadata(metadataReadIoBuffer,
                                               NULL,    // ParentAsync
                                               sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
}

VOID
CloseTestVerifyContainerClosed(
    KtlLogContainer::SPtr logContainer
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KtlLogStreamId logStreamId;
    KGuid logStreamGuid;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }

    {
        KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;

        status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }

    {
        KtlLogContainer::AsyncAssignAliasContext::SPtr assignAsync;
        KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
        KtlLogContainer::AsyncRemoveAliasContext::SPtr removeAsync;
        KtlLogStreamId resolvedLogStreamId;

        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

        KString::SPtr string = KString::Create(L"AliasIsInMyHead",
                                               *g_Allocator);
        VERIFY_IS_TRUE(string ? TRUE : FALSE);
        
        KString::CSPtr stringConst = string.RawPtr();

        status = logContainer->CreateAsyncAssignAliasContext(assignAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncRemoveAliasContext(removeAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        assignAsync->StartAssignAlias(stringConst,
                                      logStreamId,
                                      NULL,          // ParentAsync
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

        resolveAsync->StartResolveAlias(stringConst,
                                        &resolvedLogStreamId,
                                        NULL,          // ParentAsync
                                        sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);

        removeAsync->StartRemoveAlias(stringConst,
                                      NULL,          // ParentAsync
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);


    }

    {
        ContainerCloseSynchronizer closeSync;
        logContainer->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NO_LONGER_EXISTS);
    }
}

VOID
CloseTestVerifyContainerOpened(
    KtlLogContainer::SPtr logContainer
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KtlLogStreamId logStreamId;
    KGuid logStreamGuid;
    StreamCloseSynchronizer closeSync;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;

        status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    {
        KtlLogContainer::AsyncAssignAliasContext::SPtr assignAsync;
        KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
        KtlLogContainer::AsyncRemoveAliasContext::SPtr removeAsync;
        KtlLogStreamId resolvedLogStreamId;

        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

        KString::SPtr string = KString::Create(L"AliasIsInMyHead",
                                               *g_Allocator);
        VERIFY_IS_TRUE(string ? TRUE : FALSE);
        
        KString::CSPtr stringConst = string.RawPtr();

        status = logContainer->CreateAsyncAssignAliasContext(assignAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logContainer->CreateAsyncRemoveAliasContext(removeAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        assignAsync->StartAssignAlias(stringConst,
                                      logStreamId,
                                      NULL,          // ParentAsync
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        resolveAsync->StartResolveAlias(stringConst,
                                        &resolvedLogStreamId,
                                        NULL,          // ParentAsync
                                        sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // LogStreamId should resolve back to itself
        //
        VERIFY_IS_TRUE((resolvedLogStreamId == logStreamId) ? true : false);

        removeAsync->StartRemoveAlias(stringConst,
                                      NULL,          // ParentAsync
                                      sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
}


VOID
CloseTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    KtlLogContainer::SPtr logContainer;
    LONGLONG logSize = 256 * 0x100000;   // 256MB
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    StreamCloseSynchronizer closeSync;
    ContainerCloseSynchronizer closeContainerSync;

    //
    // Create a new log container
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        //
        // Close the newly created stream
        //
        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Test 1: Try stream apis and get correct error back
        //
        CloseTestVerifyStreamClosed(logStream);
        logStream = nullptr;

        //
        // Test 2: Try to delete stream from the log container. This
        // should succeed and not get stuck
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
        }


        //
        // Test 3: recreate, close and reopen the stream and try to use it
        //
        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#ifndef UPASSTHROUGH
        //
        // Does not support UPASSTHROUGH since same stream opened
        // multiple times
        //
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream2;
        KtlLogStream::SPtr logStream3;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream2,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = WriteRecord(logStream2,
                             1,
                             33);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream3,
                                                NULL,    // ParentAsync
                                                sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Test 4: Close stream and check apis return errors
        //
        logStream2->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        CloseTestVerifyStreamClosed(logStream2);
        logStream2 = nullptr;

        {
            logStream3->StartClose(NULL,
                             closeSync.CloseCompletionCallback());

            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }


        CloseTestVerifyStreamClosed(logStream3);
        logStream3 = nullptr;
#endif
        
        //
        // Test 5: Start a whole bunch of operations and then close. Verify
        // that all operations completed before close completes
        //
        {
#ifdef FEATURE_TEST
            const ULONG numberLoops = 16;
            const ULONG numberOperations = 16;
#else
            const ULONG numberLoops = 4;
            const ULONG numberOperations = 4;
#endif
            for (ULONG loops = 0; loops < numberLoops; loops++)
            {
                KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync2;
                KtlLogStream::SPtr logStream4;

                status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync2);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                openStreamAsync2->StartOpenLogStream(logStreamId,
                                                        &metadataLength,
                                                        logStream4,
                                                        NULL,    // ParentAsync
                                                        sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                CompletionCounter completionCounter;

                for (ULONG i = 0; i < numberOperations; i++)
                {
                    KtlLogAsn asn = i+ (loops*1000);
                    ULONG version = 1;
                    KIoBuffer::SPtr dataIoBuffer;
                    KIoBuffer::SPtr metadataIoBuffer;
                    ULONG myMetadataSize = 0x10;

                    KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext;
                    status = logStream->CreateAsyncStreamNotificationContext(notificationContext);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    notificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                                  90,                  // 90% utilization
                                                                  NULL,
                                                                  completionCounter);

                    // TODO: Other operations beside write

                    status = PrepareRecordForWrite(logStream4,
                                                   version,
                                                   asn,
                                                   0x100,
                                                   8,
                                                   0x1000,
                                                   dataIoBuffer,
                                                   metadataIoBuffer);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    KtlLogStream::AsyncWriteContext::SPtr writeContext;

                    status = logStream4->CreateAsyncWriteContext(writeContext);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    writeContext->StartWrite(asn,
                                             version,
                                             logStream->QueryReservedMetadataSize() + myMetadataSize,
                                             metadataIoBuffer,
                                             dataIoBuffer,
                                             0,    // Reservation
                                             NULL,    // ParentAsync
                                             completionCounter);



                }

                logStream4->StartClose(NULL,
                                       closeSync.CloseCompletionCallback());

                status = closeSync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logStream4 = nullptr;

#if 0
                //
                // There is still a very tiny race here. The
                // completions are synchronized in kernel mode however
                // the delivery of the completions may race.
                //
                // Another scenario is that the Close will race ahead
                // of any writes that are not yet started.
                //
                // These are no
                // consequence to the app which can move forward
                // although it messes up the counting a little bit
                //
                for (ULONG count = 0; count < numberOperations; count++)
                {
                    if (completionCounter.GetCount() == numberOperations*2)
                    {
                        break;
                    }
                    KNt::Sleep(0);
                }
                VERIFY_IS_TRUE(completionCounter.GetCount() == numberOperations*2);
#endif

            }
        }

        //
        // Test 7: Try to delete, it should not get stuck
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
        }
    }

    //
    // Test n: Close the container and try the container apis
    //
    KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
    KtlLogContainer::SPtr logContainer2;

    status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    openAsync->StartOpenLogContainer(diskId,
                                         logContainerId,
                                         logContainer2,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logContainer->StartClose(NULL,
                        closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // try to use closed log container and expect an error
    //
    CloseTestVerifyContainerClosed(logContainer);
    logContainer = nullptr;

    {
        logContainer2->StartClose(NULL,
                             closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    CloseTestVerifyContainerClosed(logContainer2);
    logContainer2 = nullptr;

    //
    // Test 7: Delete container successfully
    //
    {
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
Close2Test(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
#ifdef FEATURE_TEST
    const ULONG numberStreams = 16;
    const ULONG numberLoops = 6;
    const ULONG numberOperations = 128;
#else
    const ULONG numberStreams = 8;
    const ULONG numberLoops = 2;
    const ULONG numberOperations = 32;
#endif

    NTSTATUS status;
    KtlLogContainerId logContainerId;

    KtlLogStream::SPtr logStreams[numberStreams];
    KtlLogStreamId logStreamIds[numberStreams];
    StreamCloseSynchronizer closeSync;
    ContainerCloseSynchronizer closeContainerSync;
    KSynchronizer sync;


    //
    // Test 1: Close container while streams are open and ensure
    // container not closed until all streams closed
    //
    {

        KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
        KtlLogContainer::SPtr logContainer;
        LONGLONG logSize = 256 * 0x100000;   // 256MB
        KGuid logContainerGuid;
        BOOLEAN isCompleted;

        //
        // Create a new log container
        //
        logContainerGuid.CreateNew();
        logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

        status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        for (ULONG i = 0; i < numberStreams; i++)
        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            ULONG metadataLength = 0x10000;
            KGuid logStreamGuid;

            logStreamGuid.CreateNew();
            logStreamIds[i] = static_cast<KtlLogStreamId>(logStreamGuid);

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamIds[i],
                                                    nullptr,
                                                        nullptr,
                                                        securityDescriptor,
                                                    metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                    logStreams[i],
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        //
        // Close the container and verify that close isn't completed
        // for 5 seconds
        //
        logContainer->StartClose(NULL,
                                 closeContainerSync.CloseCompletionCallback());

        closeSync.WaitForCompletion(5000, &isCompleted);
        VERIFY_IS_TRUE(! isCompleted);

        //
        // close the streams
        //
        for (ULONG i = 0; i < numberStreams; i++)
        {
            logStreams[i]->StartClose(NULL,
                                      closeSync.CloseCompletionCallback());

            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStreams[i] = nullptr;
        }

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Test 2: Close container while a bunch of requests are
    // outstanding
    // TODO: multiple threads performing operations
    //
    for (ULONG loops = 0; loops < numberLoops; loops++)
    {
        KtlLogContainer::SPtr logContainer;

        //
        // Open up the log container
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


        CompletionCounter completionCounter;
        KtlLogStreamId logStreamId;
        KtlLogStreamId resolvedLogStreamId;

        KString::SPtr string = KString::Create(L"AliasIsInMyHead",
                                               *g_Allocator);
        VERIFY_IS_TRUE((string != nullptr) ? true : false);
        
        KString::CSPtr stringConst = string.RawPtr();

        for (ULONG i = 0; i < numberStreams; i++)
        {
            KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
            ULONG metadataLength;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamIds[i],
                                                    &metadataLength,
                                                    logStreams[i],
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

        }

        for (ULONG i = 0; i < numberOperations; i++)
        {
            KtlLogContainer::AsyncAssignAliasContext::SPtr assignAsync;
            KtlLogContainer::AsyncResolveAliasContext::SPtr resolveAsync;
            KtlLogContainer::AsyncRemoveAliasContext::SPtr removeAsync;

            status = logContainer->CreateAsyncAssignAliasContext(assignAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = logContainer->CreateAsyncResolveAliasContext(resolveAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = logContainer->CreateAsyncRemoveAliasContext(removeAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            assignAsync->StartAssignAlias(stringConst,
                                          logStreamId,
                                          NULL,          // ParentAsync
                                          completionCounter);

            resolveAsync->StartResolveAlias(stringConst,
                                            &resolvedLogStreamId,
                                            NULL,          // ParentAsync
                                            completionCounter);

            removeAsync->StartRemoveAlias(stringConst,
                                          NULL,          // ParentAsync
                                          completionCounter);

        }

        for (ULONG i = 0; i < numberStreams; i++)
        {
            logStreams[i]->StartClose(NULL,
                                      closeSync.CloseCompletionCallback());

            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStreams[i] = nullptr;
        }

        logContainer->StartClose(NULL,
                                 closeContainerSync.CloseCompletionCallback());
        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logContainer = nullptr;

#if 0
        //
        // There is still a very tiny race here. The
        // completions are synchronized in kernel mode however
        // the delivery of the completions may race. This is no
        // consequence to the app which can move forward
        // although it messes up the counting a little bit
        //
        for (ULONG count = 0; count < 8; count++)
        {
            if (completionCounter.GetCount() == numberOperations*3)
            {
                break;
            }
            KNt::Sleep(0);
        }
        VERIFY_IS_TRUE(completionCounter.GetCount() == numberOperations*3);
#endif
    }

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

typedef struct
{
    KtlLogManager::SPtr* logManager;
    KGuid diskId;
} Stress1Context;

DWORD
Stress1Thread(
    PVOID Context
    )
{
    Stress1Context* context = (Stress1Context*)Context;
    KtlLogManager::SPtr logManager = *(context->logManager);
    KGuid diskId = context->diskId;
    ULONG r;

    ULONG_PTR foo = (ULONG_PTR)context;
    ULONG foo1 = (ULONG)foo;

    srand(foo1);

    for (ULONG i = 0; i < 2; i++)
    {
        r = rand() % 11;

        switch(r)
        {
            case 0:
            {
                RWTStress(KtlSystem::GlobalNonPagedAllocator(), diskId, logManager,
                                4,
                                2,
                                4,        // NumberRecords
                                16 * 0x1000);
                break;
            }

            case 1:
            {
                LLBarrierRecoveryTest(diskId, logManager);
                break;
            }

            case 2:
            {
                ThresholdTest(diskId, logManager);
                break;
            }

            case 3:
            {
                ManagerTest(diskId, logManager);
                break;
            }

            case 4:
            {
                ContainerTest(diskId, logManager);
                break;
            }

            case 5:
            {
                StreamTest(diskId, logManager);
                break;
            }

            case 6:
            {
                ReadAsnTest(diskId, logManager);
                break;
            }

            case 7:
            {
                StreamMetadataTest(diskId, logManager);
                break;
            }

            case 8:
            {
                AliasStress(diskId, logManager);
                break;
            }

            case 9:
            {
                QueryRecordsTest(diskId, logManager);
                break;
            }

            case 10:
            {
                CloseTest(diskId, logManager);
                break;
            }

            case 11:
            {
                ReservationsTest(diskId, logManager);
                break;
            }

            default:
            {
                break;
            }
        }
    }

    return(ERROR_SUCCESS);
}

VOID
Stress1Test(
    KGuid diskId,
    ULONG logManagerCount,
    KtlLogManager::SPtr* logManagers
    )
{
    HANDLE handle[4];
    Stress1Context contexts[4];

    if (logManagerCount > 4)
    {
        logManagerCount = 4;
    }

    for (ULONG i = 0; i < logManagerCount; i++)
    {
        contexts[i].logManager = &logManagers[i];
        contexts[i].diskId = diskId;

        handle[i] = CreateThread(NULL,
                                 0,
                                 Stress1Thread,
                                 &contexts[i],
                                 0,
                                 NULL);
        VERIFY_IS_TRUE(handle[i] != NULL);

        //
        // Wait a little bit between starting threads to avoid contention
        //
        KNt::Sleep(1000);
    }

    ULONG error;
    error = WaitForMultipleObjects(logManagerCount,
                                   handle,
                                   TRUE,                // WaitAll
                                   INFINITE);
    VERIFY_IS_TRUE(error == WAIT_OBJECT_0);

    // TODO: PAL layer doesn't support CloseHandle nor does it
    //       distinguish different types of handles - it assumes that
    //       all HANDLEs are thread handles
#if !defined(PLATFORM_UNIX)
    for (ULONG i = 0; i < logManagerCount; i++)
    {
        CloseHandle(handle[i]);
    }
#endif
}

NTSTATUS
GetLogUsagePercentage(
    KtlLogStream::SPtr LogStream,
    ULONG& Percentage
)
{
    NTSTATUS status;
    KtlLogStream::AsyncIoctlContext::SPtr ioctl;
    KLogicalLogInformation::CurrentLogUsageInformation* logInfo;
    KSynchronizer sync;


    status = LogStream->CreateAsyncIoctlContext(ioctl);
    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

    KBuffer::SPtr inBuffer, outBuffer;
    ULONG result;

    inBuffer = nullptr;

    ioctl->StartIoctl(KLogicalLogInformation::QueryCurrentLogUsageInformation, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
    status = sync.WaitForCompletion();

    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

    logInfo = (KLogicalLogInformation::CurrentLogUsageInformation*)outBuffer->GetBuffer();
    Percentage = logInfo->PercentageLogUsage;
    return(status);
}

VOID
ThresholdTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    KtlLogContainer::SPtr logContainer;
    LONGLONG logSize = 512 * 0x100000;   // 512MB
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    StreamCloseSynchronizer closeSync;
    ContainerCloseSynchronizer closeContainerSync;
    ULONG recordSize =   1 * 16 * 0x10000;         // 1 MB
    ULONG streamSize = 300 * 16 * 0x10000;         // 300 MB + 100MB fudge factor (0x12C00000)

    //
    // Create a new log container
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
                                                        nullptr,           // Alias
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        streamSize,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }

    //
    // Test 1: Verify single threshold asyncs get completed when stream is
    // closed
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;
        KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext;
        KSynchronizer waitSync;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logStream->CreateAsyncStreamNotificationContext(notificationContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        notificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                      90,                  // 90% utilization
                                                      NULL,
                                                      waitSync);

        //
        // Give StartWaitForNotification a chance to finish before
        // closing the stream
        //
        KNt::Sleep(1000);

        //
        // Close the newly created stream and expect the notification
        // to fire
        //
        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        status = waitSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_CANCELLED);
    }

    //
    // Test 1a: Verify single threshold asyncs get completed when stream is
    // closed but close and threshold start race
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;
        KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext;
        KSynchronizer waitSync;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logStream->CreateAsyncStreamNotificationContext(notificationContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        notificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                      90,                  // 90% utilization
                                                      NULL,
                                                      waitSync);

        // Notification will hit later

        //
        // Close the newly created stream and expect the notification
        // to fire
        //
        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        status = waitSync.WaitForCompletion();
        VERIFY_IS_TRUE((status == STATUS_CANCELLED) || (status == STATUS_OBJECT_NO_LONGER_EXISTS) || (status == STATUS_NOT_FOUND));
    }

    //
    // Test 1b: Verify single threshold async get completed when
    // cancelled
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;
        KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext;
        KSynchronizer waitSync;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logStream->CreateAsyncStreamNotificationContext(notificationContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

#ifdef FEATURE_TEST
        const ULONG numberNotifications = 100;
#else
        const ULONG numberNotifications = 20;
#endif
        srand((ULONG)logStream);
        for (ULONG i = 0; i < numberNotifications; i++)
        {
            ULONG r;

            notificationContext->Reuse();
            notificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                          90,                  // 90% utilization
                                                          NULL,
                                                          waitSync);

            //
            // Wait for async to get settled
            //
            r = rand() % 10;
            if ((r > 3) && (r < 8))
            {
                KNt::Sleep(r*10);
            }

            notificationContext->Cancel();
            status = waitSync.WaitForCompletion();
            VERIFY_IS_TRUE(status == STATUS_CANCELLED);
        }

        //
        // Close the newly created stream and expect the notification
        // to fire
        //
        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }

    //
    // Test 2: Pass invalid values and expect errors
    //
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;
        KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext;
        KSynchronizer waitSync;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = logStream->CreateAsyncStreamNotificationContext(notificationContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // bad %
        //
        notificationContext->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                      900,                  // 90% utilization
                                                      NULL,
                                                      waitSync);
        status = waitSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        //
        // bad threshold type
        //
        notificationContext->Reuse();
        notificationContext->StartWaitForNotification((KtlLogStream::AsyncStreamNotificationContext::ThresholdTypeEnum)-1,
                                                      90,                  // 90% utilization
                                                      NULL,
                                                      waitSync);
        status = waitSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

        notificationContext = nullptr;
        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }

    //
    // Test 3: Verify a whole bunch threshold asyncs get completed when stream is
    // closed
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;
#ifdef FEATURE_TEST
        static const ULONG NumberThresholdAsyncs = 128;
#else
        static const ULONG NumberThresholdAsyncs = 32;
#endif
        KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext[NumberThresholdAsyncs];
        KSynchronizer waitSync[NumberThresholdAsyncs];

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < NumberThresholdAsyncs; i++)
        {
            status = logStream->CreateAsyncStreamNotificationContext(notificationContext[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            notificationContext[i]->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                          (i % 85)+10,
                                                          NULL,
                                                          waitSync[i]);

        }

        //
        // Give StartWaitForNotification a chance to finish before
        // closing the stream
        //
        KNt::Sleep(150);

        //
        // Close the stream and expect the notification
        // to fire
        //
        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        for (ULONG i = 0; i < NumberThresholdAsyncs; i++)
        {
            status = waitSync[i].WaitForCompletion();
            VERIFY_IS_TRUE((status == STATUS_CANCELLED) ||
                           (status == STATUS_OBJECT_NO_LONGER_EXISTS) ||
                           (status == STATUS_NOT_FOUND));
        }
    }

    //
    // Test 4: Verify async is fired when threshold is crossed
    //
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength;
        KtlLogAsn asn;
#ifdef FEATURE_TEST
        static const ULONG NumberThresholdAsyncs = 128;
#else
        static const ULONG NumberThresholdAsyncs = 50;
#endif
        KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext[NumberThresholdAsyncs];
        KSynchronizer waitSync[NumberThresholdAsyncs];

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
        // since we know that the stream size is 300MB + 100MB fudge
        // factor and the record written size is 1MB there should be
        // about 3 or 4 records per percent. However the used space
        // gets accounted for in a delayed fashion so we add a bonus
        // record.
        //
        ULONG recordsPerPercent = ((streamSize / recordSize) / 100) + 2;
#ifdef FEATURE_TEST
        static const ULONG numberLoops = 3;
#else
        static const ULONG numberLoops = 1;
#endif

        for (ULONG loops = 0; loops < numberLoops; loops++)
        {
            ULONG a = loops*90000;

            //
            // Expect this to truncate the entire stream
            //
            status = TruncateStream(logStream,
                                    a);

            //
            // Give time for truncation to take effect
            //
            KNt::Sleep(3 * 1000);
            //
            // Start 50 threshold asyncs, starting at 2%, 4%, etc
            //
            KInvariant(NumberThresholdAsyncs >= 50);

            ULONG numberAsyncs = 50;
            for (ULONG i = 0; i < numberAsyncs; i++)
            {
                status = logStream->CreateAsyncStreamNotificationContext(notificationContext[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                notificationContext[i]->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                              ((i+1) * 2)-1,
                                                              NULL,
                                                              waitSync[i]);
            }

            //
            // Give notifications a chance to start
            //
            KNt::Sleep(1000);

            //
            // Fill up the log and check if thresholds fired
            //
            ULONG thresholdIndex = 0;
            do
            {
                for (ULONG i = 0; i < recordsPerPercent*2; i++)
                {
                    asn = a++;
                    status = WriteRecord(logStream,
                                         1,                   // version
                                         asn,
                                         16,            // number blocks
                                         0x10000,       // block size
                                         0);      // reservation
                    VERIFY_IS_TRUE((status == STATUS_LOG_FULL) || NT_SUCCESS(status) ? true : false,
                                   L"Fill log");

                    if (status == STATUS_LOG_FULL)
                    {
                        break;
                    }
                }

                //
                // Expect another threshold to be fired
                //
                if (thresholdIndex < 50)
                {
                    BOOLEAN stillFiring = TRUE;

                    // Query currently used perent and ensure it is about correct
                    ULONG logUsagePercent;
                    status = GetLogUsagePercentage(logStream, logUsagePercent);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    while (stillFiring)
                    {
                        status = waitSync[thresholdIndex].WaitForCompletion(1000);
                        if (status == STATUS_IO_TIMEOUT)
                        {
                            status = STATUS_SUCCESS;
                            break;
                        }

                        VERIFY_IS_TRUE(NT_SUCCESS(status));

                        ULONG expectedPercent = ((thresholdIndex + 1) * 2) - 1;
                        KDbgPrintf("Threshold Test: Expected %d, actual %d\n", expectedPercent, logUsagePercent);
                        VERIFY_IS_TRUE(logUsagePercent >= expectedPercent);

                        thresholdIndex++;
                        if (thresholdIndex == 50)
                        {
                            break;
                        }
                    }
                }

            } while (NT_SUCCESS(status));

            VERIFY_IS_TRUE(status == STATUS_LOG_FULL ? true : false, L"Log file should be full");

            for (; thresholdIndex < 50; thresholdIndex++)
            {
                status = waitSync[thresholdIndex].WaitForCompletion(1000);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
        }

        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }

    //
    // Close the container
    //
    logContainer->StartClose(NULL,
                        closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;

    //
    // Delete container successfully
    //
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

VOID
CancelStressTest(
    ULONG NumberLoops,
    ULONG NumberAsyncs,
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    KtlLogContainer::SPtr logContainer;
    LONGLONG logSize = 256 * 0x100000;   // 256MB
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    StreamCloseSynchronizer closeSync;
    ContainerCloseSynchronizer closeContainerSync;

    //
    // Create a new log container
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Create a new stream in this container
    //
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

    {
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
        KtlLogStream::SPtr logStream;
        ULONG metadataLength = 0x10000;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KBuffer::SPtr securityDescriptor = nullptr;
        createStreamAsync->StartCreateLogStream(logStreamId,
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

        logStream->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }

    // TODO: use all kinds of asyncs, not just thresholds

    //
    // Test 1: Start up a bunch of threashold asyncs and randomly cancel
    // some. Every once in a while close and be sure all get completed.
    // This tests close racing with starting operations
    //
    {
        const ULONG MaxNumberAsyncs = 256;
        if (NumberAsyncs > MaxNumberAsyncs)
        {
            NumberAsyncs = MaxNumberAsyncs;
        }

        srand((ULONG)GetTickCount());

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG loops = 0; loops < NumberLoops; loops++)
        {
            //
            // Open up the stream
            //
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            //
            // Prime the asyncs by getting them all started
            //
            KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext[MaxNumberAsyncs];
            KSynchronizer waitSync[MaxNumberAsyncs];

            for (ULONG i = 0; i < NumberAsyncs; i++)
            {
                status = logStream->CreateAsyncStreamNotificationContext(notificationContext[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                notificationContext[i]->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                              90,                  // 90% utilization
                                                              NULL,
                                                              waitSync[i]);
            }

            //
            // Loop around canceling and restarting asyncs
            //
            for (ULONG i = 0; i < NumberAsyncs; i++)
            {
                ULONG canceled[10];
                ULONG numberToCancel;

                numberToCancel = (rand() % 12) + 1;
                if (numberToCancel > 10)
                {
                    //
                    // break out of the loop early
                    //
                    break;
                } else {
                    //
                    // Cancel a bunch of random asyncs and wait for
                    // them to complete. Next restart them.
                    //
                    for (ULONG j = 0; j < numberToCancel; j++)
                    {
                        ULONG r;
                        BOOLEAN isDupe;

                        // pick a unique index
                        do
                        {
                            isDupe = FALSE;
                            r = rand() % NumberAsyncs;

                            for (ULONG k = 0; k < j; k++)
                            {
                                if (r == canceled[k])
                                {
                                    // Hit a duplicate
                                    isDupe = TRUE;
                                    break;
                                }
                            }
                        } while (isDupe);

                        notificationContext[r]->Cancel();
                        canceled[j] = r;
                    }

                    for (ULONG j = 0; j < numberToCancel; j++)
                    {
//                      KDbgPrintf("%d: waitSync[canceled[j]] %p\n", __LINE__, notificationContext[canceled[j]].RawPtr());
                        status = waitSync[canceled[j]].WaitForCompletion();
                        VERIFY_IS_TRUE(status == STATUS_CANCELLED);
                    }

                    for (ULONG j = 0; j < numberToCancel; j++)
                    {
                        notificationContext[canceled[j]]->Reuse();
                        notificationContext[canceled[j]]->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                                      90,                  // 90% utilization
                                                                      NULL,
                                                                      waitSync[canceled[j]]);
                    }
                }
            }

            //
            // Now close the stream and expect all of the asyncs to be
            // cancelled too
            //
            logStream->StartClose(NULL,
                             closeSync.CloseCompletionCallback());

            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            for (ULONG i = 0; i < NumberAsyncs; i++)
            {
//              KDbgPrintf("%d: waitSync[i] %p\n", __LINE__, notificationContext[i].RawPtr());
                status = waitSync[i].WaitForCompletion();
                VERIFY_IS_TRUE((status == STATUS_CANCELLED) ||
                               (status == STATUS_OBJECT_NO_LONGER_EXISTS) ||
                               (status == STATUS_NOT_FOUND));
            }
        }
    }

    //
    // Test 2: Start up a bunch of threashold asyncs and randomly cancel
    // some. Every once in a while close and be sure all get completed.
    // This tests cancel racing with close.
    //
    {
        const ULONG MaxNumberAsyncs = 256;
        if (NumberAsyncs > MaxNumberAsyncs)
        {
            NumberAsyncs = MaxNumberAsyncs;
        }

        srand(GetTickCount());

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG loops = 0; loops < NumberLoops*8; loops++)
        {
            //
            // Open up the stream
            //
            KtlLogStream::SPtr logStream;
            ULONG metadataLength;

            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            //
            // Prime the asyncs by getting them all started
            //
            KtlLogStream::AsyncStreamNotificationContext::SPtr notificationContext[MaxNumberAsyncs];
            KSynchronizer waitSync[MaxNumberAsyncs];

            for (ULONG i = 0; i < NumberAsyncs; i++)
            {
                status = logStream->CreateAsyncStreamNotificationContext(notificationContext[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                notificationContext[i]->StartWaitForNotification(KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization,
                                                              90,                  // 90% utilization
                                                              NULL,
                                                              waitSync[i]);
            }

            //
            // Loop around canceling and restarting asyncs
            //
            ULONG startToCancel;
            ULONG numberToCancel = NumberAsyncs/2;

            startToCancel = (rand() % (numberToCancel-1));
            //
            // Cancel a bunch of random asyncs
            //
            for (ULONG j = startToCancel; j < (startToCancel + numberToCancel) ; j++)
            {
                notificationContext[j]->Cancel();
            }

            //
            // Now close the stream and expect all of the asyncs to be
            // cancelled too
            //
            logStream->StartClose(NULL,
                             closeSync.CloseCompletionCallback());

            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            for (ULONG i = 0; i < NumberAsyncs; i++)
            {
//              KDbgPrintf("%d: waitSync[i] %p\n", __LINE__, notificationContext[i].RawPtr());
                status = waitSync[i].WaitForCompletion();
                VERIFY_IS_TRUE((status == STATUS_CANCELLED) ||
                               (status == STATUS_OBJECT_NO_LONGER_EXISTS) ||
                               (status == STATUS_NOT_FOUND));
            }
        }
    }

    //
    // Close the container
    //
    logContainer->StartClose(NULL,
                        closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;

    //
    // Delete container successfully
    //
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


NTSTATUS ReadContainingRecord(
    KtlLogStream::SPtr logStream,
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncReadContext::SPtr readContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncReadContext(readContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    readContext->StartReadContaining(recordAsn,
                                     NULL,           // Actual ASN
                             &version,
                             metadataSize,
                             metadataIoBuffer,
                             dataIoBuffer,
                             NULL,    // ParentAsync
                             sync);

    status = sync.WaitForCompletion();

    return(status);
}


NTSTATUS ReadNextRecord(
    KtlLogStream::SPtr logStream,
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncReadContext::SPtr readContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncReadContext(readContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    readContext->StartReadNext(recordAsn,
                                     NULL,           // Actual ASN
                             &version,
                             metadataSize,
                             metadataIoBuffer,
                             dataIoBuffer,
                             NULL,    // ParentAsync
                             sync);

    status = sync.WaitForCompletion();

    return(status);
}

NTSTATUS ReadPreviousRecord(
    KtlLogStream::SPtr logStream,
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncReadContext::SPtr readContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncReadContext(readContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    readContext->StartReadPrevious(recordAsn,
                                     NULL,           // Actual ASN
                             &version,
                             metadataSize,
                             metadataIoBuffer,
                             dataIoBuffer,
                             NULL,    // ParentAsync
                             sync);

    status = sync.WaitForCompletion();

    return(status);
}

NTSTATUS ReadExactRecord(
    KtlLogStream::SPtr logStream,
    KtlLogAsn recordAsn,
    ULONGLONG& version,
    ULONG& metadataSize,
    KIoBuffer::SPtr& dataIoBuffer,
    KIoBuffer::SPtr& metadataIoBuffer
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncReadContext::SPtr readContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncReadContext(readContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    readContext->StartRead(recordAsn,
                             &version,
                             metadataSize,
                             metadataIoBuffer,
                             dataIoBuffer,
                             NULL,    // ParentAsync
                             sync);

    status = sync.WaitForCompletion();

    return(status);
}

NTSTATUS ValidateRecordData(
    ULONGLONG StreamOffsetExpected,
    ULONG DataSizeExpected,
    PUCHAR DataExpected,
    ULONGLONG VersionExpected,
    KtlLogStreamId& StreamIdExpected,
    ULONG metadataSizeRead,
    KIoBuffer::SPtr metadataIoBufferRead,
    KIoBuffer::SPtr dataIoBufferRead,
    ULONG reservedMetadataSize,
    BOOLEAN exactMatch
    )
{
    UNREFERENCED_PARAMETER(StreamIdExpected);
    UNREFERENCED_PARAMETER(metadataSizeRead);

    NTSTATUS status;
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
    ULONG dataRead;
    ULONG dataSizeRead;
    KIoBuffer::SPtr streamIoBuffer;

    KLogicalLogInformation::FindLogicalLogHeaders((PUCHAR)metadataIoBufferRead->First()->GetBuffer(),
                                                 *dataIoBufferRead,
                                                 reservedMetadataSize,
                                                 metadataBlockHeader,
                                                 streamBlockHeader,
                                                 dataSizeRead,
                                                 dataRead);

    VERIFY_IS_TRUE((streamBlockHeader->Signature == KLogicalLogInformation::StreamBlockHeader::Sig) ? true : false);

    if (exactMatch)
    {
        VERIFY_IS_TRUE((streamBlockHeader->StreamOffset == StreamOffsetExpected) ? true : false);
    }
    else
    {
        VERIFY_IS_TRUE((StreamOffsetExpected >= streamBlockHeader->StreamOffset) &&
                       (StreamOffsetExpected <= (streamBlockHeader->StreamOffset + dataSizeRead)) ? true : false);
    }
#if DBG
    ULONGLONG crc64;

//  crc64 = KChecksum::Crc64(dataRead, dataSizeRead, 0);
//    VERIFY_IS_TRUE(crc64 == streamBlockHeader->DataCRC64);

    ULONGLONG headerCrc64 = streamBlockHeader->HeaderCRC64;
    streamBlockHeader->HeaderCRC64 = 0;
    crc64 = KChecksum::Crc64(streamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader), 0);
    VERIFY_IS_TRUE((crc64 == headerCrc64) ? true : false);
#endif

    if (exactMatch)
    {
        if (DataSizeExpected != dataSizeRead)
        {
            printf("dataSizeRead %d, expected %d\n", dataSizeRead, DataSizeExpected);
        }
        VERIFY_IS_TRUE((DataSizeExpected == dataSizeRead) ? true : false);
    }
    else
    {
        if (DataSizeExpected > dataSizeRead)
        {
            printf("dataSizeRead %d\n", dataSizeRead);
        }
        VERIFY_IS_TRUE((DataSizeExpected <= dataSizeRead) ? true : false);
    }
    UNREFERENCED_PARAMETER(VersionExpected);
//    VERIFY_IS_TRUE((VersionExpected == streamBlockHeader->HighestOperationId) ? true : false);

    //
    // Build KIoBufferStream that spans the metadata and the data
    //
    status = KIoBuffer::CreateEmpty(streamIoBuffer,
                                    *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = streamIoBuffer->AddIoBufferReference(*metadataIoBufferRead,
                                                  0,
                                                  metadataIoBufferRead->QuerySize());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = streamIoBuffer->AddIoBufferReference(*dataIoBufferRead,
                                                  0,
                                                  dataIoBufferRead->QuerySize());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG dataStart;
    ULONG bytesToCheck;

    if (exactMatch)
    {
        dataStart = dataRead;
        bytesToCheck = dataSizeRead;
    }
    else
    {
        ULONGLONG offset = StreamOffsetExpected - streamBlockHeader->StreamOffset;
        dataStart = dataRead + (ULONG)offset;
        bytesToCheck = DataSizeExpected;
    }

    KIoBufferStream stream(*streamIoBuffer,
                           dataStart);

    for (ULONG i = 0; i < bytesToCheck; i++)
    {
        UCHAR byte;
        BOOLEAN b = stream.Pull(byte);
        if (! b) VERIFY_IS_TRUE(b ? true : false);
        if (byte != DataExpected[i]) VERIFY_IS_TRUE((byte == DataExpected[i]) ? true : false);
    }

    VERIFY_IS_TRUE((StreamIdExpected.Get() == streamBlockHeader->StreamId.Get()) ? true : false);

    return(STATUS_SUCCESS);
}

NTSTATUS SetupRecord(
    KtlLogStream::SPtr logStream,
    KIoBuffer::SPtr& MetadataIoBuffer,
    KIoBuffer::SPtr& DataIoBuffer,
    ULONGLONG version,
    ULONGLONG asn,
    PUCHAR data,
    ULONG dataSize,
    ULONG offsetToStreamBlockHeader,
    BOOLEAN IsBarrierRecord,
    ULONGLONG*,
    WackStreamBlockHeader WackIt
    )
{
    NTSTATUS status;
    KtlLogStreamId logStreamId;
    PVOID metadataBuffer;
    PVOID dataBuffer = NULL;
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
    PUCHAR dataPtr;
    ULONG firstBlockSize;
    ULONG secondBlockSize;
    ULONG offsetToDataLocation = offsetToStreamBlockHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);
    KIoBuffer::SPtr dataIoBufferRef;

    KIoBuffer::SPtr newMetadataBuffer;
    KIoBuffer::SPtr newDataIoBuffer;

    if (((DataIoBuffer == nullptr) || (DataIoBuffer->QuerySize() == 0)) &&
        (offsetToStreamBlockHeader >= KLogicalLogInformation::FixedMetadataSize))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Copy data into exclusive buffers
    //
    PVOID src, dest;

    status = KIoBuffer::CreateSimple(MetadataIoBuffer->QuerySize(), newMetadataBuffer, metadataBuffer, *g_Allocator);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    dest = (PVOID)newMetadataBuffer->First()->GetBuffer();
    src = (PVOID)MetadataIoBuffer->First()->GetBuffer();

    KMemCpySafe(src, MetadataIoBuffer->QuerySize(), dest, MetadataIoBuffer->QuerySize());
    MetadataIoBuffer = newMetadataBuffer;


    if ((DataIoBuffer->QuerySize() != 0) && (DataIoBuffer->QueryNumberOfIoBufferElements() == 1))
    {
        status = KIoBuffer::CreateSimple(DataIoBuffer->QuerySize(), newDataIoBuffer, dataBuffer, *g_Allocator);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        dest = (PVOID)newDataIoBuffer->First()->GetBuffer();
        src = (PVOID)DataIoBuffer->First()->GetBuffer();
        KMemCpySafe(dest, DataIoBuffer->QuerySize(), src, DataIoBuffer->QuerySize());
        DataIoBuffer = newDataIoBuffer;
    }
    else
    {
        if (DataIoBuffer->QuerySize() == 0)
        {
            dataBuffer = NULL;
        }
        else
        {
            dataBuffer = (PVOID)DataIoBuffer->First()->GetBuffer();
        }
    }

    logStream->QueryLogStreamId(logStreamId);

    metadataBlockHeader = (KLogicalLogInformation::MetadataBlockHeader*)((PUCHAR)metadataBuffer +
                                                                         logStream->QueryReservedMetadataSize());

    RtlZeroMemory(metadataBuffer, KLogicalLogInformation::FixedMetadataSize);
    metadataBlockHeader->OffsetToStreamHeader = offsetToStreamBlockHeader;
    if (IsBarrierRecord)
    {
        metadataBlockHeader->Flags = KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord;
    } else {
        metadataBlockHeader->Flags = 0;
    }

    //
    // Stream block header may not cross metadata/iodata boundary
    //
    KInvariant( (offsetToStreamBlockHeader <= (KLogicalLogInformation::FixedMetadataSize - sizeof(KLogicalLogInformation::StreamBlockHeader))) ||
                (offsetToStreamBlockHeader >= KLogicalLogInformation::FixedMetadataSize) );
    if (offsetToStreamBlockHeader >= KLogicalLogInformation::FixedMetadataSize)
    {
        if (dataBuffer == NULL)
        {
            return(STATUS_INVALID_PARAMETER);
        }

        streamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)((PUCHAR)dataBuffer +
                                                          (offsetToStreamBlockHeader - KLogicalLogInformation::FixedMetadataSize));
    } else {
        streamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)((PUCHAR)metadataBuffer + offsetToStreamBlockHeader);
    }

    RtlZeroMemory(streamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));

    streamBlockHeader->Signature = KLogicalLogInformation::StreamBlockHeader::Sig;
    streamBlockHeader->StreamId = logStreamId;
    streamBlockHeader->StreamOffset = asn;
    streamBlockHeader->HighestOperationId = version;
    streamBlockHeader->DataSize = dataSize;

    if (offsetToDataLocation < KLogicalLogInformation::FixedMetadataSize)
    {
        firstBlockSize = KLogicalLogInformation::FixedMetadataSize - offsetToDataLocation;
        if (firstBlockSize > dataSize)
        {
            firstBlockSize = dataSize;
        }

        dataPtr = (PUCHAR)metadataBuffer + offsetToDataLocation;
        KMemCpySafe(dataPtr, MetadataIoBuffer->First()->QuerySize() - offsetToDataLocation, data, firstBlockSize);

        dataPtr = (PUCHAR)dataBuffer;
    } else {
        firstBlockSize = 0;
        dataPtr = (PUCHAR)dataBuffer + (offsetToDataLocation - KLogicalLogInformation::FixedMetadataSize);
    }
    secondBlockSize = dataSize - firstBlockSize;

    KMemCpySafe(dataPtr, dataSize - firstBlockSize, (data + firstBlockSize), secondBlockSize);

    status = KIoBuffer::CreateEmpty(dataIoBufferRef, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = dataIoBufferRef->AddIoBufferReference(*DataIoBuffer,
                                          0,
                                          (secondBlockSize + 0xfff) & ~0xfff);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    streamBlockHeader->DataCRC64 = KLogicalLogInformation::ComputeDataBlockCRC(streamBlockHeader,
                                                                               *DataIoBuffer,
                                                                               offsetToDataLocation);
    streamBlockHeader->HeaderCRC64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(streamBlockHeader);

    (WackIt)(streamBlockHeader);

    DataIoBuffer = dataIoBufferRef;
    return(status);
}


NTSTATUS SetupAndWriteRecord(
    KtlLogStream::SPtr logStream,
    KIoBuffer::SPtr MetadataIoBuffer,
    KIoBuffer::SPtr DataIoBuffer,
    ULONGLONG version,
    ULONGLONG asn,
    PUCHAR data,
    ULONG dataSize,
    ULONG offsetToStreamBlockHeader,
    BOOLEAN IsBarrierRecord,
    ULONGLONG* WriteLatencyInMs,
    WackStreamBlockHeader WackIt
    )
{
    NTSTATUS status;

    status = SetupRecord(logStream,
               MetadataIoBuffer,
               DataIoBuffer,
               version,
               asn,
               data,
               dataSize,
               offsetToStreamBlockHeader,
               IsBarrierRecord,
               WriteLatencyInMs,
               WackIt);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = WriteRecord(logStream,
                         version,
                         asn,
                         DataIoBuffer,
                         0,          // no reservation
                         MetadataIoBuffer,
                         KLogicalLogInformation::FixedMetadataSize,
                         WriteLatencyInMs);
    return(status);
}

VOID VerifyTailAndVersion(
    KtlLogStream::SPtr LogStream,
    ULONGLONG ExpectedVersion,
    ULONGLONG ExpectedAsn,
    KLogicalLogInformation::StreamBlockHeader* ExpectedHeader
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncIoctlContext::SPtr ioctl;
    KBuffer::SPtr tailAndVersionBuffer;
    KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation* tailAndVersion;
    KSynchronizer sync;
    ULONG result;

    status = KBuffer::Create(sizeof(KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation),
                             tailAndVersionBuffer,
                             *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = LogStream->CreateAsyncIoctlContext(ioctl);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ioctl->StartIoctl(KLogicalLogInformation::QueryLogicalLogTailAsnAndHighestOperationControl,
                      NULL,
                      result,
                      tailAndVersionBuffer,
                      NULL,
                      sync);
    status =  sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    tailAndVersion = (KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation*)tailAndVersionBuffer->GetBuffer();
    VERIFY_IS_TRUE(tailAndVersion->HighestOperationCount == ExpectedVersion);
    VERIFY_IS_TRUE(tailAndVersion->TailAsn == ExpectedAsn);

#if 0
    // With the change to use new memory blocks for the LLDataOverwrite test in SetupAndWriteRecord, the original header
    // is no longer available to the test. To re-enable this the test will need to be update to remove
    // the assumption that write buffers can be reused.
    if (ExpectedHeader != NULL)
    {
        VERIFY_IS_TRUE(memcmp(ExpectedHeader,
                              &tailAndVersion->RecoveredLogicalLogHeader,
                              sizeof(KLogicalLogInformation::StreamBlockHeader)) == 0);
    }
#else
    UNREFERENCED_PARAMETER(ExpectedHeader);
#endif
}

VOID
LLDataOverwriteTest(
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

        {
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

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG metadataLength;

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

        //
        // Open the stream
        //
        {
            KtlLogStream::SPtr logStream;
            KLogicalLogInformation::StreamBlockHeader* lastStreamBlockHeader;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

            //
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KIoBuffer::CreateSimple(16*0x1000,
                                             iodata,
                                             iodataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                             metadata,
                                             metadataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KBuffer::Create(16*0x1000,
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

            KLogicalLogInformation::StreamBlockHeader emptyStreamBlockHeader;
            RtlZeroMemory(&emptyStreamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));
            lastStreamBlockHeader = &emptyStreamBlockHeader;

            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader
                                 );


            //
            // Test 1: Write record with good stream and metadata
            // headers with data only in metadata block and read back
            //
            {
                ULONG data = 7;
                PUCHAR dataWrittenA = (PUCHAR)&data;
                ULONG dataSizeWrittenA = sizeof(ULONG);
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWrittenA,
                                             dataSizeWrittenA,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                lastStreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)
                                               ((PUCHAR)metadata->First()->GetBuffer() + offsetToStreamBlockHeaderM);

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWrittenA);

                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            dataSizeWrittenA,
                                            dataWrittenA,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWrittenA;
            }

            //
            // Test 1a: After only a single record was written with ASN
            // of 1, close and reopen the log stream and verify that
            // the tail is recovered correctly
            //
            {
                StreamCloseSynchronizer closeSync;

                logStream->StartClose(NULL,
                                      closeSync.CloseCompletionCallback());
                status = closeSync.WaitForCompletion();
                logStream = nullptr;

                openStreamAsync->Reuse();
                openStreamAsync->StartOpenLogStream(logStreamId,
                                                        &metadataLength,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                        sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn,
                                     lastStreamBlockHeader);
            }


            //
            // Test 2: Write record with good stream and metadata
            // headers with data in metadata and io block and read back
            //
            {
                dataSizeWritten = 0x1000;
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

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;


                //
                // Complicate this test by writing a record where then
                // data is in an io buffer with a number of elements
                //
                KIoBuffer::SPtr ioBuffer;

                status = KIoBuffer::CreateEmpty(ioBuffer, *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (ULONG i = 0; i < 8; i++)
                {
                    KIoBufferElement::SPtr dataElement;
                    VOID* dataBuffer;

                    status = KIoBufferElement::CreateNew(0x2000,        // Size
                                                              dataElement,
                                                              dataBuffer,
                                                              *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    for (int j = 0; j < 0x2000; j++)
                    {
                        ((PUCHAR)dataBuffer)[j] = (UCHAR)(i*j);
                    }

                    ioBuffer->AddIoBufferElement(*dataElement);
                }

                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             ioBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;


            }

            //
            // Test 3: Write record with good stream and metadata
            // headers with data in io block and read back
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

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;
            }

            //
            //
            // Test 4: Write records up to specific ASN and then move
            // the tail of the log backward (ie, truncate newest data)
            // to a place at the boundry of a record. Verify that
            // truncated data is not returned.
            //
            {
                dataSizeWritten = 16;
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Now move tail back to before last write
                //
                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             NULL,
                                             0,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn);


                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
                // asn not updated
            }


            //
            // Test 4a: Write 5 records (offsets 100, 200, 300, 400, 500) and then truncate back to the middle of
            // the second record (250). Next write 5 more records (250, 350, 450, 650, 750). Now read back data
            // at 325 and verify that it is from the second record written and not the first.
            //
            {
                dataSizeWritten = 100;
                ULONGLONG baseAsn = asn;

                for (ULONG i = 0; i < 5; i++)
                {
                    version++;

                    memset(dataWritten, (UCHAR)('A' + i), dataSizeWritten);

                    status = SetupAndWriteRecord(logStream,
                                                    metadata,
                                                    emptyIoBuffer,
                                                    version,
                                                    asn,
                                                    dataWritten,
                                                    dataSizeWritten,
                                                    offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    asn += dataSizeWritten;
                }

                //
                // Now move tail back to 250
                //
                asn = baseAsn + 250;
                ULONGLONG expectedVersion = version+1;

                for (ULONG i = 0; i < 5; i++)
                {
                    version++;

                    memset(dataWritten, (UCHAR)('A' + i + 5), dataSizeWritten);

                    status = SetupAndWriteRecord(logStream,
                                                    metadata,
                                                    emptyIoBuffer,
                                                    version,
                                                    asn,
                                                    dataWritten,
                                                    dataSizeWritten,
                                                    offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    asn += dataSizeWritten;
                }

                //
                // Now read 325 and expect that it comes from the new record written
                // at 250 with the higher version and not the older record written at 300
                //

                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;
                ULONGLONG readAsn = baseAsn + 325;

                status = ReadContainingRecord(logStream,
                                    readAsn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                memset(dataWritten, (UCHAR)('A' + 5), dataSizeWritten);

                status = ValidateRecordData(readAsn - 75,
                                            dataSizeWritten,
                                            dataWritten,
                                            expectedVersion,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }


            //
            // Test 5: Write records up to specific ASN and then move
            // the tail of the log backward (ie, truncate newest data)
            // to a place in the middle of the last record. Verify that
            // truncated data is not returned.
            //
            {
                dataSizeWritten = 16;
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Now move tail back to before last write
                //
                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn + 8,
                                             NULL,
                                             0,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+8);


                status = ReadContainingRecord(logStream,
                                    asn+8,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            8,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += 8;
            }

            //
            // Test 6: Write records up to specific ASN and then move
            // the tail of the log backward (ie, truncate newest data)
            // to a place in the middle of not the last record. Verify that
            // truncated data is not returned.
            //
            {
                dataSizeWritten = 16;

                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;


                for (ULONG i = 0; i < 5; i++)
                {
                    version++;

                    status = SetupAndWriteRecord(logStream,
                                                 metadata,
                                                 emptyIoBuffer,
                                                 version,
                                                 asn,
                                                 dataWritten,
                                                 dataSizeWritten,
                                                 offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VerifyTailAndVersion(logStream,
                                         version,
                                         asn+dataSizeWritten);


                    status = ReadContainingRecord(logStream,
                                        asn,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    //
                    // Validate data read is correct
                    //
                    status = ValidateRecordData(asn,
                                                dataSizeWritten,
                                                dataWritten,
                                                version,
                                                logStreamId,
                                                metadataSizeRead,
                                                metadataIoBufferRead,
                                                dataIoBufferRead,
                                                logStream->QueryReservedMetadataSize(),
                                                FALSE);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    asn += dataSizeWritten;
                }

                //
                // Now move tail back to before last write
                //
                asn -= (2*dataSizeWritten) + 4;
                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             NULL,
                                             0,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn);


                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

                status = ReadContainingRecord(logStream,
                                    asn -1,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn-(dataSizeWritten-4),
                                            dataSizeWritten-4,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            //
            // Test 7: Write records up to specific ASN and then move
            // the tail of the log backward (ie, truncate newest data)
            // to a place at the boundry of a record. Write more data
            // and verify that truncated data is not returned by
            // reading at the truncation offset, at below the truncated
            // offset and beyond the truncated offset.
            //
            {
                dataSizeWritten = 16;
                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                for (ULONG i = 0; i < 10; i++)
                {
                    for (ULONG j = 0; j < dataSizeWritten; j++)
                    {
                        dataWritten[j] = i % 255;
                    }

                    version++;
                    status = SetupAndWriteRecord(logStream,
                                                 metadata,
                                                 emptyIoBuffer,
                                                 version,
                                                 asn,
                                                 dataWritten,
                                                 dataSizeWritten,
                                                 offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VerifyTailAndVersion(logStream,
                                         version,
                                         asn+dataSizeWritten);


                    status = ReadContainingRecord(logStream,
                                        asn,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    //
                    // Validate data read is correct
                    //
                    status = ValidateRecordData(asn,
                                                dataSizeWritten,
                                                dataWritten,
                                                version,
                                                logStreamId,
                                                metadataSizeRead,
                                                metadataIoBufferRead,
                                                dataIoBufferRead,
                                                logStream->QueryReservedMetadataSize(),
                                                FALSE);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));


                    asn += dataSizeWritten;
                }


                //
                // Now rewrite the last record
                //

                for (ULONG j = 0; j < dataSizeWritten; j++)
                {
                    dataWritten[j] = 0x33;
                }

                asn -= dataSizeWritten; // move back exactly 1 record
                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                //
                // Verify  error when reading beyond new tail
                //
                status = ReadContainingRecord(logStream,
                                    asn+dataSizeWritten+1,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

                //
                // Verify updated record is correct
                //
                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = ValidateRecordData(asn,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Verify earlier record at boundary is correct
                //
                for (ULONG j = 0; j < dataSizeWritten; j++)
                {
                    dataWritten[j] = 8;
                }

                status = ReadContainingRecord(logStream,
                                    asn - dataSizeWritten,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn - dataSizeWritten,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                //
                // Verify earlier record not at boundary is correct
                //
                for (ULONG j = 0; j < dataSizeWritten; j++)
                {
                    dataWritten[j] = 6;
                }

                status = ReadContainingRecord(logStream,
                                    asn - (2*dataSizeWritten + 4),
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn - (3*dataSizeWritten),
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));


                asn += dataSizeWritten;
            }


            //
            // Test 8: Write records up to specific ASN and then move
            // the tail of the log backward (ie, truncate newest data)
            // to a place in the middle of a record. Write more data
            // and verify that truncated data is not returned by
            // reading at the truncation offset, at below the truncated
            // offset and beyond the truncated offset.
            //
            {
                dataSizeWritten = 16;
                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                for (ULONG i = 0; i < 10; i++)
                {
                    for (ULONG j = 0; j < dataSizeWritten; j++)
                    {
                        dataWritten[j] = (i % 255)+99;
                    }

                    version++;
                    status = SetupAndWriteRecord(logStream,
                                                 metadata,
                                                 emptyIoBuffer,
                                                 version,
                                                 asn,
                                                 dataWritten,
                                                 dataSizeWritten,
                                                 offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VerifyTailAndVersion(logStream,
                                         version,
                                         asn+dataSizeWritten);


                    status = ReadContainingRecord(logStream,
                                        asn,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    //
                    // Validate data read is correct
                    //
                    status = ValidateRecordData(asn,
                                                dataSizeWritten,
                                                dataWritten,
                                                version,
                                                logStreamId,
                                                metadataSizeRead,
                                                metadataIoBufferRead,
                                                dataIoBufferRead,
                                                logStream->QueryReservedMetadataSize(),
                                                FALSE);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));


                    asn += dataSizeWritten;
                }


                //
                // Now rewrite the last record but make it shorter than
                // the previous incarnation
                //

                for (ULONG j = 0; j < dataSizeWritten; j++)
                {
                    dataWritten[j] = 0x88;
                }

                asn -= dataSizeWritten/2; // move back exactly a half record
                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten/2,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten/2);


                //
                // Verify  error when reading beyond new tail
                //
                status = ReadContainingRecord(logStream,
                                    asn+(dataSizeWritten/2)+1,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

                //
                // Verify updated record is correct
                //
                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = ValidateRecordData(asn,
                                            dataSizeWritten/2,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Verify earlier record at boundary is correct
                //
                for (ULONG j = 0; j < dataSizeWritten; j++)
                {
                    dataWritten[j] = 9 + 99;
                }

                status = ReadContainingRecord(logStream,
                                    asn - (dataSizeWritten/2),
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct. Since this record was
                // truncated earlier, we only read the untruncated part
                //
                status = ValidateRecordData(asn - (dataSizeWritten/2),
                                            dataSizeWritten/2,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += (dataSizeWritten/2);

            }

            //
            // Test 9a: Write records up to a specific ASN and then
            // move the tail of the log backwards multiple times. Each
            // time try to read data that should not be there and data
            // that should be
            //
            {
                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;
                KIoBuffer::SPtr data[10];
                PUCHAR dataPtrs[10];
                ULONGLONG baseAsns[10];
                ULONGLONG beginAsn;
                PVOID p;

                for (ULONG i = 0; i < 10; i++)
                {
                    status = KIoBuffer::CreateSimple(dataSizeWritten,
                                                     data[i],
                                                     p,
                                                     *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    dataPtrs[i] = (PUCHAR)p;

                    PUCHAR dataPtr = dataPtrs[i];
                    for (ULONG j = 0; j < dataSizeWritten; j++)
                    {
                        dataPtr[j] = (UCHAR)((i+1)*0x11);
                    }
                }

                beginAsn = asn;
                for (ULONG i = 0; i < 10; i++)
                {
                    baseAsns[i] = asn;

                    version++;
                    status = SetupAndWriteRecord(logStream,
                                                 metadata,
                                                 iodata,
                                                 version,
                                                 asn,
                                                 dataPtrs[i],
                                                 dataSizeWritten,
                                                 offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VerifyTailAndVersion(logStream,
                                         version,
                                         asn+dataSizeWritten);


                    status = ReadContainingRecord(logStream,
                                        asn,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    //
                    // Validate data read is correct
                    //
                    status = ValidateRecordData(asn,
                                                dataSizeWritten,
                                                dataPtrs[i],
                                                version,
                                                logStreamId,
                                                metadataSizeRead,
                                                metadataIoBufferRead,
                                                dataIoBufferRead,
                                                logStream->QueryReservedMetadataSize(),
                                                FALSE);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));


                    asn += dataSizeWritten;
                }

                //
                // Move backward in the log truncating the head at random until we get all the way back to the
                // beginning
                //
                srand((ULONG)GetTickCount());

                BOOLEAN first = TRUE;
                while (asn > beginAsn+1)
                {
                    ULONGLONG backup;

                    if (first)
                    {
                        //
                        // Test Case: TruncateTail right at the tail pointer
                        //
                        backup = 0;
                        first = FALSE;
                    } else {
                        backup = rand() % dataSizeWritten;
                    }

                    asn -= backup;
                    if (asn <= beginAsn)
                    {
                        asn = beginAsn+1;
                    }

                    //
                    // Move backward
                    //
                    version++;
                    status = SetupAndWriteRecord(logStream,
                                                 metadata,
                                                 emptyIoBuffer,
                                                 version,
                                                 asn,
                                                 NULL,
                                                 0,
                                                 offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VerifyTailAndVersion(logStream,
                                         version,
                                         asn);


                    status = ReadContainingRecord(logStream,
                                        asn+1,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

                    status = ReadContainingRecord(logStream,
                                        asn,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

                    //
                    // Now read last record
                    //
                    ULONGLONG asnRead = asn-1;
                    status = ReadContainingRecord(logStream,
                                        asnRead,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    //
                    // Validate data read is correct
                    //
                    ULONG index = (ULONG)((asnRead - beginAsn)) / dataSizeWritten;

                    status = ValidateRecordData(baseAsns[index],
                                                (ULONG)(dataSizeWritten - ((baseAsns[index] + dataSizeWritten) - asn)),
                                                dataPtrs[index],
                                                version,
                                                logStreamId,
                                                metadataSizeRead,
                                                metadataIoBufferRead,
                                                dataIoBufferRead,
                                                logStream->QueryReservedMetadataSize(),
                                                FALSE);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }
            }

            //
            // test 9b: Alternate writing forward and writing backward
            //
            {
                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                ULONGLONG beginAsn = asn;

                srand((ULONG)GetTickCount());

                for (ULONG i = 0; i < 25; i++)
                {
                    ULONG dataSize;

                    //
                    // write a record forward
                    //
                    dataSize = rand() % dataSizeWritten;

                    version++;
                    status = SetupAndWriteRecord(logStream,
                                                 metadata,
                                                 iodata,
                                                 version,
                                                 asn,
                                                 dataWritten,
                                                 dataSize,
                                                 offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VerifyTailAndVersion(logStream,
                                         version,
                                         asn+dataSize);


                    status = ReadContainingRecord(logStream,
                                        asn,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE((dataSize == 0) ? (status == STATUS_NOT_FOUND) : NT_SUCCESS(status));

                    if (NT_SUCCESS(status))
                    {
                        //
                        // Validate data read is correct
                        //
                        status = ValidateRecordData(asn,
                                                    dataSize,
                                                    dataWritten,
                                                    version,
                                                    logStreamId,
                                                    metadataSizeRead,
                                                    metadataIoBufferRead,
                                                    dataIoBufferRead,
                                                    logStream->QueryReservedMetadataSize(),
                                                    FALSE);
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }

                    asn += dataSize;

                    //
                    // Now truncate backward
                    //
                    dataSize = rand() % dataSizeWritten;

                    if (asn - dataSize < beginAsn)
                    {
                        dataSize = 0;
                    }

                    asn -= dataSize;

                    version++;
                    status = SetupAndWriteRecord(logStream,
                                                 metadata,
                                                 emptyIoBuffer,
                                                 version,
                                                 asn,
                                                 NULL,
                                                 0,
                                                 offsetToStreamBlockHeaderM);

                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VerifyTailAndVersion(logStream,
                                         version,
                                         asn);


                    status = ReadContainingRecord(logStream,
                                        asn+1,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);

                    status = ReadContainingRecord(logStream,
                                        asn,
                                        versionRead,
                                        metadataSizeRead,
                                        dataIoBufferRead,
                                        metadataIoBufferRead);
                    VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
                }
            }

            //
            // Test 9c: write records and truncate to the current tail
            //
            {
                dataSizeWritten = 16;
                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Now truncate at the current tail
                //
                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn + dataSizeWritten,
                                             dataWritten,
                                             0,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VerifyTailAndVersion(logStream,
                                     version,
                                     asn+dataSizeWritten);


                status = ReadContainingRecord(logStream,
                                    asn,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                //
                // Validate data read is correct
                //
                status = ValidateRecordData(asn,
                                            dataSizeWritten,
                                            dataWritten,
                                            version,
                                            logStreamId,
                                            metadataSizeRead,
                                            metadataIoBufferRead,
                                            dataIoBufferRead,
                                            logStream->QueryReservedMetadataSize(),
                                            FALSE);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;
            }


            // Negative cases

            //
            // Test 10: Write records with invalid stream block headers
            //
            {
                dataSizeWritten = 16;
                version++;

                //
                // Stream block header signature must be correct
                //
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE,
                                             NULL,
                                             [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader)
                                             {
                                                 streamBlockHeader->Signature = 0x18;
                                             });

                VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);


                //
                // Stream block header streamid must be correct
                //
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE,
                                             NULL,
                                             [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader)
                                             {
                                                 // {A59CAFEB-31F5-4fdb-B073-46A81DF2DF74}
                                                 static const GUID badGuid =
                                                 { 0xa59cafeb, 0x31f5, 0x4fdb, { 0xb0, 0x73, 0x46, 0xa8, 0x1d, 0xf2, 0xdf, 0x74 } };
                                                 KMemCpySafe(&streamBlockHeader->StreamId, sizeof(streamBlockHeader->StreamId), &badGuid, sizeof(GUID));
                                             });

                VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);


                //
                // Stream block header StreamOffset must be the same as ASN
                //
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE,
                                             NULL,
                                             [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader)
                                             {
                                                 streamBlockHeader->StreamOffset += 0x200;
                                             });

                VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

                //
                // Stream block header HighestOperationId must be the same
                // as Version
                //
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE,
                                             NULL,
                                             [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader)
                                             {
                                                 streamBlockHeader->HighestOperationId += 0x200;
                                             });

                VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);


#if DBG
                //
                // Stream block header CRC must be correct
                //
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE,
                                             NULL,
                                             [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader)
                                             {
                                                 streamBlockHeader->HeaderCRC64 += 0x200;
                                             });

                VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);

                //
                // Data block CRC must be correct
                //
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE,
                                             NULL,
                                             [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader)
                                             {
                                                 streamBlockHeader->DataCRC64 += 0x200;
                                             });

                VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
#endif

            }

#if 1 // Enable to test out of order writes
            //
            // Test 11: Write records where version is not increasing
            //
            {
                dataSizeWritten = 16;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(status == STATUS_REQUEST_OUT_OF_SEQUENCE);
            }

            //
            // Test 12: Write record where asn is not the expected next
            // ASN
            //
            {
                dataSizeWritten = 16;

                version++;
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;

                //
                // Invalid next asn
                //
                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version+1,
                                             asn+1,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(status == STATUS_REQUEST_OUT_OF_SEQUENCE);
            }
#endif

            //
            // Test 13: Read record using methods other than
            // ReadContaining
            {
                ULONGLONG versionRead;
                ULONG metadataSizeRead;
                KIoBuffer::SPtr dataIoBufferRead;
                KIoBuffer::SPtr metadataIoBufferRead;

                status = ReadContainingRecord(logStream,
                                    asn - 32,
                                    versionRead,
                                    metadataSizeRead,
                                    dataIoBufferRead,
                                    metadataIoBufferRead);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            StreamCloseSynchronizer closeSync;

            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            logStream = nullptr;
        }


        //
        // Test 14: Reopen the stream to verify that the latest version
        // and logical log tail have been recovered successfully
        //
        {
            KtlLogStream::SPtr logStream;

            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            // TODO: Use query records, query record range to verify
            // tail and highest versions are correct

            dataSizeWritten = 64;
            version++;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ULONGLONG versionRead;
            ULONG metadataSizeRead;
            KIoBuffer::SPtr dataIoBufferRead;
            KIoBuffer::SPtr metadataIoBufferRead;

            status = ReadContainingRecord(logStream,
                                asn,
                                versionRead,
                                metadataSizeRead,
                                dataIoBufferRead,
                                metadataIoBufferRead);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Validate data read is correct
            //
            status = ValidateRecordData(asn,
                                        dataSizeWritten,
                                        dataWritten,
                                        version,
                                        logStreamId,
                                        metadataSizeRead,
                                        metadataIoBufferRead,
                                        dataIoBufferRead,
                                        logStream->QueryReservedMetadataSize(),
                                        FALSE);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StreamCloseSynchronizer closeSync;

            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            logStream = nullptr;
        }


        //
        // Test 15:  There is a case where the core logger will return
        // an error and cause an assert
        //
        {
            KtlLogStream::SPtr logStream;

            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            dataSizeWritten = KLogicalLogInformation::FixedMetadataSize -
                                    (offsetToStreamBlockHeaderM + sizeof(KLogicalLogInformation::StreamBlockHeader));
            version++;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StreamCloseSynchronizer closeSync;

            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            logStream = nullptr;

            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
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

VOID
LLCoalesceDataTest(
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

        {
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

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG metadataLength;

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

        //
        // Open the stream
        //
        {
            KtlLogStream::SPtr logStream;
            KLogicalLogInformation::StreamBlockHeader* lastStreamBlockHeader;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                &metadataLength,
                logStream,
                NULL,    // ParentAsync
                sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

            //
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

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

            KLogicalLogInformation::StreamBlockHeader emptyStreamBlockHeader;
            RtlZeroMemory(&emptyStreamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));
            lastStreamBlockHeader = &emptyStreamBlockHeader;

            VerifyTailAndVersion(logStream,
                version,
                asn,
                lastStreamBlockHeader
                );

            StreamCloseSynchronizer closeSync;

            //
            // TEST 1: Verify that records sitting in coalesce buffer
            //         are flushed
            // Write a record that isn't coalesced, close and reopen.
            //

            for (ULONG i = 0; i < 4096; i++)
            {
                dataSizeWritten = 4;
                version++;
                status = SetupAndWriteRecord(logStream,
                    metadata,
                    emptyIoBuffer,
                    version,
                    asn,
                    dataWritten,
                    dataSizeWritten,
                    offsetToStreamBlockHeaderM);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;
            }

            logStream->StartClose(NULL,
                                 closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;
            
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                &metadataLength,
                logStream,
                NULL,    // ParentAsync
                sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (ULONG i = 0; i < 400; i++)
            {
                dataSizeWritten = 40;

                version++;
                status = SetupAndWriteRecord(logStream,
                    metadata,
                    emptyIoBuffer,
                    version,
                    asn,
                    dataWritten,
                    dataSizeWritten,
                    offsetToStreamBlockHeaderM);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                if (i == 200)
                {
                    logContainer->StartClose(NULL,
                        closeContainerSync.CloseCompletionCallback());

                }
            }


            logStream->StartClose(NULL,
                                 closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            status = closeContainerSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
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

VOID MultiRecordReadAndVerify(
    __in KtlLogStream::SPtr LogStream,
    __in KtlLogAsn Asn,
    __inout KIoBuffer::SPtr Metadata,
    __inout KIoBuffer::SPtr IoBuffer,
    __in NTSTATUS ExpectedStatus,
    __in ULONGLONG StreamOffsetExpected,
    __in ULONG DataSizeExpected,
    __in PUCHAR DataExpected,
    __in ULONGLONG VersionExpected,
    __in KtlLogStreamId& StreamIdExpected,
    __in ULONG reservedMetadataSize
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KtlLogStream::AsyncMultiRecordReadContext::SPtr multiRecordRead;

    status = LogStream->CreateAsyncMultiRecordReadContext(multiRecordRead);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    multiRecordRead->StartRead(Asn,
                               *Metadata,
                               *IoBuffer,
                               NULL,
                               sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(status == ExpectedStatus);

    if (NT_SUCCESS(status))
    {
        status = ValidateRecordData(StreamOffsetExpected,
                                    DataSizeExpected,
                                    DataExpected,
                                    VersionExpected,
                                    StreamIdExpected,
                                    0,               // MetadataSizeRead - unused
                                    Metadata,
                                    IoBuffer,
                                    reservedMetadataSize,
                                    TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
}

VOID
MultiRecordReadTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogStream::SPtr logStream;
    StreamCloseSynchronizer closeSync;

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

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;

        KIoBuffer::SPtr iodata;
        PVOID iodataBuffer;
        KIoBuffer::SPtr metadata;
        PVOID metadataBuffer;
        KBuffer::SPtr workKBuffer;
        PUCHAR workBuffer;

        KIoBuffer::SPtr emptyIoBuffer;

        KIoBuffer::SPtr readMetadata;
        PVOID readMetadataPtr;
        KIoBuffer::SPtr readIoBuffer;
        PVOID readIoBufferPtr;

        PUCHAR dataWritten;

        ULONG offsetToStreamBlockHeaderM;

        //
        // Setup for the tests
        //
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

        status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                         readMetadata,
                                         readMetadataPtr,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        status = KIoBuffer::CreateSimple(16*0x1000,
                                            iodata,
                                            iodataBuffer,
                                            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                            metadata,
                                            metadataBuffer,
                                            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KBuffer::Create(16*0x1000,
                                    workKBuffer,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        workBuffer = (PUCHAR)workKBuffer->GetBuffer();
        dataWritten = workBuffer;

        logStream->StartClose(NULL,
                               closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        ULONGLONG asn = KtlLogAsn::Min().Get();
        ULONGLONG version = 0;

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
                                &metadataLength,
                                logStream,
                                NULL,    // ParentAsync
                                sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONG metadataBufferOffset = logStream->QueryReservedMetadataSize();
        ULONG fullMDSize =  KLogicalLogInformation::FixedMetadataSize - (metadataBufferOffset + sizeof(KLogicalLogInformation::MetadataBlockHeader) +
                                                                         sizeof(KLogicalLogInformation::StreamBlockHeader));

        typedef struct
        {
            ULONGLONG BaseAsn;
            ULONG AsnOffset;
            ULONG IoSize;
            ULONGLONG Version;
        } ReadControl;

        //
        // Disable write coalescing since the point of the test is to test aggregation of the records on read
        //
        KtlLogStream::AsyncIoctlContext::SPtr ioctlContext;
        KBuffer::SPtr inBuffer, outBuffer;
        ULONG result;

        inBuffer = nullptr;

        status = logStream->CreateAsyncIoctlContext(ioctlContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::DisableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        //
        // Test 0: Write less than a full multirecord and read back
        //
        {
            RvdLogAsn testAsn = asn;
            ULONGLONG testVersion = version + 1;

            static const ULONG writeListSize = 5;
            LONG writeList[writeListSize] = { (LONG)fullMDSize, (LONG)fullMDSize, (LONG)fullMDSize, (LONG)fullMDSize, (LONG)fullMDSize };

            const ULONG readControlItems = 1;
            ReadControl readControl[readControlItems] =
            {
                { testAsn.Get(), 0, 512 * 1024, testVersion },
            };

            //
            // Write Phase
            //

            ULONG dataSizeWritten;
            ULONG totalBytesWritten = 0;
            for (ULONG i = 0; i < writeListSize; i++)
            {
                LONG absDataListSize = -1 * writeList[i];

                if (writeList[i] > 0)
                {
                    //
                    // Moving forward
                    //
                    for (int j = 0; j < writeList[i]; j++)
                    {
                        dataWritten[j+totalBytesWritten] = (UCHAR)(i * j);
                    }
                    dataSizeWritten = writeList[i];
                }
                else
                {
                    //
                    // Moving backwards
                    //
                    asn -= absDataListSize;
                    dataSizeWritten = 0;
                }

                version++;
                status = SetupAndWriteRecord(logStream,
                    metadata,
                    iodata,
                    version,
                    asn,
                    dataWritten + totalBytesWritten,
                    dataSizeWritten,
                    offsetToStreamBlockHeaderM,
                    FALSE);           //No Barrier
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;

                if (writeList[i] > 0)
                {
                    totalBytesWritten += dataSizeWritten;
                }
                else
                {
                    totalBytesWritten -= absDataListSize;
                }
            }

            //
            // Read Phase
            //
            for (ULONG i = 0; i < readControlItems; i++)
            {
                ULONG readOffset = readControl[i].AsnOffset;
                ULONG ioSize = readControl[i].IoSize;
                ULONGLONG expectedAsn = readControl[i].BaseAsn + readOffset;
                ULONG expectedSize = fullMDSize + ioSize;
                ULONGLONG expectedVersion = readControl[i].Version;

                readIoBuffer = nullptr;
                if (ioSize > 0)
                {
                    status = KIoBuffer::CreateSimple(ioSize, readIoBuffer, readIoBufferPtr, *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                } else {
                    readIoBuffer = emptyIoBuffer;
                }

                if (expectedSize > (totalBytesWritten - readOffset))
                {
                    expectedSize = (totalBytesWritten - readOffset);
                }

                MultiRecordReadAndVerify(logStream,
                                         readControl[i].BaseAsn + readOffset,
                                         readMetadata,
                                         readIoBuffer,
                                         STATUS_SUCCESS,
                                         expectedAsn,
                                         expectedSize,
                                         dataWritten + readOffset,
                                         expectedVersion,
                                         logStreamId,
                                         logStream->QueryReservedMetadataSize());
            }
        }



        //
        // Test 1: [Full MD][11K IO] -
        //         Read MD only, MD + 4K, MD + 8K, MD + 16K
        //
        {
            RvdLogAsn testAsn = asn;
            ULONGLONG testVersion = version + 1;

            static const ULONG writeListSize = 1;
            LONG writeList[writeListSize] = { (LONG)fullMDSize + 11 * 1024 };

            const ULONG readControlItems = 5;
            ReadControl readControl[readControlItems] =
            {
                { testAsn.Get(), 120, 0, testVersion },
                { testAsn.Get(), 4000, 0x1000, testVersion },
                { testAsn.Get(), 4000, 0x2000, testVersion },
                { testAsn.Get(), 120, 0x3000, testVersion },
                { testAsn.Get(), fullMDSize, 0x4000, testVersion }
            };

            //
            // Write Phase
            //

            ULONG dataSizeWritten;
            ULONG totalBytesWritten = 0;
            for (ULONG i = 0; i < writeListSize; i++)
            {
                LONG absDataListSize = -1 * writeList[i];

                if (writeList[i] > 0)
                {
                    //
                    // Moving forward
                    //
                    for (int j = 0; j < writeList[i]; j++)
                    {
                        dataWritten[j+totalBytesWritten] = (UCHAR)(i * j);
                    }
                    dataSizeWritten = writeList[i];
                }
                else
                {
                    //
                    // Moving backwards
                    //
                    asn -= absDataListSize;
                    dataSizeWritten = 0;
                }

                version++;
                status = SetupAndWriteRecord(logStream,
                    metadata,
                    iodata,
                    version,
                    asn,
                    dataWritten + totalBytesWritten,
                    dataSizeWritten,
                    offsetToStreamBlockHeaderM,
                    FALSE);           //No Barrier
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;

                if (writeList[i] > 0)
                {
                    totalBytesWritten += dataSizeWritten;
                }
                else
                {
                    totalBytesWritten -= absDataListSize;
                }
            }

            //
            // Read Phase
            //
            for (ULONG i = 0; i < readControlItems; i++)
            {
                ULONG readOffset = readControl[i].AsnOffset;
                ULONG ioSize = readControl[i].IoSize;
                ULONGLONG expectedAsn = readControl[i].BaseAsn + readOffset;
                ULONG expectedSize = fullMDSize + ioSize;
                ULONGLONG expectedVersion = readControl[i].Version;

                readIoBuffer = nullptr;
                if (ioSize > 0)
                {
                    status = KIoBuffer::CreateSimple(ioSize, readIoBuffer, readIoBufferPtr, *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                } else {
                    readIoBuffer = emptyIoBuffer;
                }

                if (expectedSize > (totalBytesWritten - readOffset))
                {
                    expectedSize = (totalBytesWritten - readOffset);
                }

                MultiRecordReadAndVerify(logStream,
                                         readControl[i].BaseAsn + readOffset,
                                         readMetadata,
                                         readIoBuffer,
                                         STATUS_SUCCESS,
                                         expectedAsn,
                                         expectedSize,
                                         dataWritten + readOffset,
                                         expectedVersion,
                                         logStreamId,
                                         logStream->QueryReservedMetadataSize());
            }
        }



        //
        // Test 2: [400 MD][] [400 MD][] [400 MD][] 8 * [2000 MD]
        //         Read: MD only, MD + 12K
        //
        {
            RvdLogAsn testAsn = asn;
            ULONGLONG testVersion = version + 1;

            static const ULONG writeListSize = 11;
            LONG writeList[writeListSize] = { 400, 400, 400, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000 };

            const ULONG readControlItems = 2;
            ReadControl readControl[readControlItems] =
            {
                { testAsn.Get(), 0, 0, testVersion },
                { testAsn.Get(), 0, 0x3000, testVersion }
            };

            //
            // Write Phase
            //

            ULONG dataSizeWritten;
            ULONG totalBytesWritten = 0;
            for (ULONG i = 0; i < writeListSize; i++)
            {
                LONG absDataListSize = -1 * writeList[i];

                if (writeList[i] > 0)
                {
                    //
                    // Moving forward
                    //
                    for (int j = 0; j < writeList[i]; j++)
                    {
                        dataWritten[j+totalBytesWritten] = (UCHAR)(i * j);
                    }
                    dataSizeWritten = writeList[i];
                }
                else
                {
                    //
                    // Moving backwards
                    //
                    asn -= absDataListSize;
                    dataSizeWritten = 0;
                }

                version++;
                status = SetupAndWriteRecord(logStream,
                    metadata,
                    iodata,
                    version,
                    asn,
                    dataWritten + totalBytesWritten,
                    dataSizeWritten,
                    offsetToStreamBlockHeaderM,
                    FALSE);           //No Barrier
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;

                if (writeList[i] > 0)
                {
                    totalBytesWritten += dataSizeWritten;
                }
                else
                {
                    totalBytesWritten -= absDataListSize;
                }
            }


            //
            // Read Phase
            //
            for (ULONG i = 0; i < readControlItems; i++)
            {
                ULONG readOffset = readControl[i].AsnOffset;
                ULONG ioSize = readControl[i].IoSize;
                ULONGLONG expectedAsn = readControl[i].BaseAsn + readOffset;
                ULONG expectedSize = fullMDSize + ioSize;
                ULONGLONG expectedVersion = readControl[i].Version;

                readIoBuffer = nullptr;
                if (ioSize > 0)
                {
                    status = KIoBuffer::CreateSimple(ioSize, readIoBuffer, readIoBufferPtr, *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                } else {
                    readIoBuffer = emptyIoBuffer;
                }

                if (expectedSize > (totalBytesWritten - readOffset))
                {
                    expectedSize = (totalBytesWritten - readOffset);
                }

                MultiRecordReadAndVerify(logStream,
                                         testAsn.Get() + readOffset,
                                         readMetadata,
                                         readIoBuffer,
                                         STATUS_SUCCESS,
                                         expectedAsn,
                                         expectedSize,
                                         dataWritten + readOffset,
                                         expectedVersion,
                                         logStreamId,
                                         logStream->QueryReservedMetadataSize());
            }
        }

        //
        // Test 3: test with truncated tails
        //         [1000][8000][40][-20][3800][12000][-6000][1100]
        //
        {
            RvdLogAsn testAsn = asn;
            ULONGLONG testVersion = version + 1;

            static const ULONG writeListSize = 8;
            LONG writeList[writeListSize] = { 1000, 8000, 40, -20, 3800, 12000, -6000, 1100 };

            const ULONG readControlItems = 5;
            ReadControl readControl[readControlItems] =
            {
                { testAsn.Get(), 0, 0x1000, testVersion },
                { testAsn.Get(), 250, 8 * 0x1000, testVersion },
                { testAsn.Get(), 1500, 16 * 0x1000, testVersion + 1 },
                { testAsn.Get(), 6000, 0, testVersion + 1 },
                { testAsn.Get(), 9050, 0, testVersion + 2 }
            };

            //
            // Write Phase
            //

            ULONG dataSizeWritten;
            ULONG totalBytesWritten = 0;
            for (ULONG i = 0; i < writeListSize; i++)
            {
                LONG absDataListSize = -1 * writeList[i];

                if (writeList[i] > 0)
                {
                    //
                    // Moving forward
                    //
                    for (int j = 0; j < writeList[i]; j++)
                    {
                        dataWritten[j + totalBytesWritten] = (UCHAR)(i * j);
                    }
                    dataSizeWritten = writeList[i];
                }
                else
                {
                    //
                    // Moving backwards
                    //
                    asn -= absDataListSize;
                    dataSizeWritten = 0;
                }

                version++;
                status = SetupAndWriteRecord(logStream,
                    metadata,
                    iodata,
                    version,
                    asn,
                    dataWritten + totalBytesWritten,
                    dataSizeWritten,
                    offsetToStreamBlockHeaderM,
                    TRUE);           //No Barrier
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;

                if (writeList[i] > 0)
                {
                    totalBytesWritten += dataSizeWritten;
                }
                else
                {
                    totalBytesWritten -= absDataListSize;
                }
            }

            KNt::Sleep(10 * 1000);

            //
            // Read Phase
            //
            for (ULONG i = 0; i < readControlItems; i++)
            {
                ULONG readOffset = readControl[i].AsnOffset;
                ULONG ioSize = readControl[i].IoSize;
                ULONGLONG expectedAsn = readControl[i].BaseAsn + readOffset;
                ULONG expectedSize = fullMDSize + ioSize;
                ULONGLONG expectedVersion = readControl[i].Version;

                readIoBuffer = nullptr;
                if (ioSize > 0)
                {
                    status = KIoBuffer::CreateSimple(ioSize, readIoBuffer, readIoBufferPtr, *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }
                else {
                    readIoBuffer = emptyIoBuffer;
                }

                if (expectedSize > (totalBytesWritten - readOffset))
                {
                    expectedSize = (totalBytesWritten - readOffset);
                }

                MultiRecordReadAndVerify(logStream,
                    testAsn.Get() + readOffset,
                    readMetadata,
                    readIoBuffer,
                    STATUS_SUCCESS,
                    expectedAsn,
                    expectedSize,
                    dataWritten + readOffset,
                    expectedVersion,
                    logStreamId,
                    logStream->QueryReservedMetadataSize());
            }
        }

        ioctlContext->Reuse();
        ioctlContext->StartIoctl(KLogicalLogInformation::EnableCoalescingWrites, NULL, result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ioctlContext = nullptr;
        logStream->StartClose(NULL,
                                closeSync.CloseCompletionCallback());
        status = closeSync.WaitForCompletion();
        logStream = nullptr;

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

VOID
LLBarrierRecoveryTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogStream::SPtr logStream;
    StreamCloseSynchronizer closeSync;

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

        //
        // Verify that log stream id is set correctly
        //
        KtlLogStreamId queriedLogStreamId;
        logStream->QueryLogStreamId(queriedLogStreamId);

        VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;

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

        //
        // Setup for the tests
        //
        KLogicalLogInformation::StreamBlockHeader* lastStreamBlockHeader;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

        status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(16*0x1000,
                                            iodata,
                                            iodataBuffer,
                                            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                            metadata,
                                            metadataBuffer,
                                            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KBuffer::Create(16*0x1000,
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

        KLogicalLogInformation::StreamBlockHeader emptyStreamBlockHeader;
        RtlZeroMemory(&emptyStreamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));
        lastStreamBlockHeader = &emptyStreamBlockHeader;

        logStream->StartClose(NULL,
                               closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        ULONGLONG asn = KtlLogAsn::Min().Get();
        ULONGLONG version = 0;

        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(logStreamId,
                                &metadataLength,
                                logStream,
                                NULL,    // ParentAsync
                                sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Test 1: Verify freshly created stream has correct tail and version
        //
        {
            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;


            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                    &metadataLength,
                                    logStream,
                                    NULL,    // ParentAsync
                                    sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VerifyTailAndVersion(logStream,
                    version,
                    asn,
                    lastStreamBlockHeader
                    );

        }

        //
        // Test 2: Write one record without barrier, reopen stream and verify correct tail and version
        //
        {
            dataSizeWritten = 16;
            version++;
            status = SetupAndWriteRecord(logStream,
                                            metadata,
                                            emptyIoBuffer,
                                            version,
                                            asn,
                                            dataWritten,
                                            dataSizeWritten,
                                            offsetToStreamBlockHeaderM,
                                            FALSE);           //No Barrier

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Stream Close
            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            // Stream Reopen
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                    &metadataLength,
                                    logStream,
                                    NULL,    // ParentAsync
                                    sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader);
        }

        //
        // Test 3: Write 16 more records without barrier, reopen stream and verify correct tail and version
        //
        {
            dataSizeWritten = 16;
            ULONGLONG writeAsn = asn;

            for (ULONG i = 0; i < 16; i++)
            {
                version++;
                status = SetupAndWriteRecord(logStream,
                                                metadata,
                                                emptyIoBuffer,
                                                version,
                                                writeAsn,
                                                dataWritten,
                                                dataSizeWritten,
                                                offsetToStreamBlockHeaderM,
                                                FALSE);           //No Barrier
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                writeAsn += dataSizeWritten;
            }

            // Stream Close
            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            // Stream Reopen
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                    &metadataLength,
                                    logStream,
                                    NULL,    // ParentAsync
                                    sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader);
        }

        //
        // Test 4: Write barrier record at tail with data, reopen stream and verify correct tail and version
        //
        {
            dataSizeWritten = 16;

            version++;
            status = SetupAndWriteRecord(logStream,
                                            metadata,
                                            emptyIoBuffer,
                                            version,
                                            asn,
                                            dataWritten,
                                            dataSizeWritten,
                                            offsetToStreamBlockHeaderM,
                                            TRUE);           // Barrier
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataSizeWritten;

            // Stream Close
            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            // Stream Reopen
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                    &metadataLength,
                                    logStream,
                                    NULL,    // ParentAsync
                                    sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            lastStreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)
                                ((PUCHAR)metadata->First()->GetBuffer() + offsetToStreamBlockHeaderM);
            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader);
        }

        //
        // Test 5: Write 32 records with barriers, reopen stream and verify correct tail and version
        //
        {
            dataSizeWritten = 16;

            for (ULONG i = 0; i < 32; i++)
            {
                version++;
                status = SetupAndWriteRecord(logStream,
                                                metadata,
                                                emptyIoBuffer,
                                                version,
                                                asn,
                                                dataWritten,
                                                dataSizeWritten,
                                                offsetToStreamBlockHeaderM,
                                                TRUE);           // Barrier
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;
            }

            // Stream Close
            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            // Stream Reopen
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                    &metadataLength,
                                    logStream,
                                    NULL,    // ParentAsync
                                    sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));


            lastStreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)
                                ((PUCHAR)metadata->First()->GetBuffer() + offsetToStreamBlockHeaderM);
            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader);
        }

#if 1 // Do not allow out of order writes
        //
        // Test 6: Overwrite ASN with one record without barrier and check for the correct error code,
        // reopen stream and verify correct tail and version
        //
        {
            dataSizeWritten = 0;
            ULONGLONG writeAsn = asn - 8;

            status = SetupAndWriteRecord(logStream,
                                            metadata,
                                            emptyIoBuffer,
                                            version+1,
                                            writeAsn,
                                            dataWritten,
                                            dataSizeWritten,
                                            offsetToStreamBlockHeaderM,
                                            FALSE);           //No Barrier
            VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER);
        }
#endif

        //
        // Test 8: Write 16 new records and overwrite ASN with one record including data with barrier,
        //         reopen stream and verify correct tail and version
        //
        {
            dataSizeWritten = 0;

            for (ULONG i = 0; i < 4; i++)
            {
                version++;
                status = SetupAndWriteRecord(logStream,
                                                metadata,
                                                emptyIoBuffer,
                                                version,
                                                asn,
                                                dataWritten,
                                                dataSizeWritten,
                                                offsetToStreamBlockHeaderM,
                                                TRUE);           // Barrier

                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn -= 8;
            }
            asn +=8;

            // Stream Close
            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            // Stream Reopen
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                    &metadataLength,
                                    logStream,
                                    NULL,    // ParentAsync
                                    sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            lastStreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)
                                ((PUCHAR)metadata->First()->GetBuffer() + offsetToStreamBlockHeaderM);
            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader);
        }

        //
        // Test 9: Write 8 of 16 new records to leave gaps and no marker, reopen and verify tail and version
        //
#if 0 // Disable since out of order writes are not allowed
        {
            ULONG dataSizeWritten = 48;
            ULONGLONG writeAsn, barrierAsn;
            KLogicalLogInformation::StreamBlockHeader streamBlockHeader;

            //
            // Place a barrier to go back to
            //
            version++;
            status = SetupAndWriteRecord(logStream,
                                            metadata,
                                            emptyIoBuffer,
                                            version,
                                            asn,
                                            dataWritten,
                                            512,
                                            offsetToStreamBlockHeaderM,
                                            FALSE);           //Barrier
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += 512;

            version++;
            status = SetupAndWriteRecord(logStream,
                                            metadata,
                                            emptyIoBuffer,
                                            version,
                                            asn,
                                            dataWritten,
                                            512,
                                            offsetToStreamBlockHeaderM,
                                            TRUE);           //Barrier
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += 512;

            lastStreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)
                                ((PUCHAR)metadata->First()->GetBuffer() + offsetToStreamBlockHeaderM);
            streamBlockHeader = *lastStreamBlockHeader;
            lastStreamBlockHeader = &streamBlockHeader;
            barrierAsn = asn;

            writeAsn = asn;
            for (ULONG i = 0; i < 4; i++)
            {
                version++;

                if (i%2 == 0)
                {
                    status = SetupAndWriteRecord(logStream,
                                                    metadata,
                                                    emptyIoBuffer,
                                                    version,
                                                    writeAsn,
                                                    dataWritten,
                                                    dataSizeWritten,
                                                    offsetToStreamBlockHeaderM,
                                                    FALSE);           //No Barrier
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }
                writeAsn += dataSizeWritten;
            }

            version++;
            status = SetupAndWriteRecord(logStream,
                                            metadata,
                                            emptyIoBuffer,
                                            version,
                                            writeAsn,
                                            dataWritten,
                                            dataSizeWritten,
                                            offsetToStreamBlockHeaderM,
                                            FALSE);           //No Barrier
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            writeAsn += dataSizeWritten;


            // Stream Close
            logStream->StartClose(NULL,
                                  closeSync.CloseCompletionCallback());
            status = closeSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;

            // Stream Reopen
            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                    &metadataLength,
                                    logStream,
                                    NULL,    // ParentAsync
                                    sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VerifyTailAndVersion(logStream,
                                 version,
                                 barrierAsn,
                                 lastStreamBlockHeader);
        }
#endif
        logStream->StartClose(NULL,
                                closeSync.CloseCompletionCallback());
        status = closeSync.WaitForCompletion();
        logStream = nullptr;

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

VOID
LLRecordMarkerRecoveryTest(
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

        {
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

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG metadataLength;

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

        ULONG offsetToStreamBlockHeaderM;

        KIoBuffer::SPtr metadataIoBufferVerify, ioBufferVerify;
        PVOID p;

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                         metadataIoBufferVerify,
                                         p,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(1024 * 1024,
                                         ioBufferVerify,
                                         p,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Open the stream
        //
        {
            KtlLogStream::SPtr logStream;
            KLogicalLogInformation::StreamBlockHeader* lastStreamBlockHeader;

            status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStream,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

            //
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            const ULONG ioBufferSize = 1024 * 1024;
            status = KIoBuffer::CreateSimple(ioBufferSize,
                                             iodata,
                                             iodataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                             metadata,
                                             metadataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KBuffer::Create(ioBufferSize,
                                     workKBuffer,
                                     *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            workBuffer = (PUCHAR)workKBuffer->GetBuffer();
            dataWritten = workBuffer;

            for (ULONG i = 0; i < ioBufferSize; i++)
            {
                dataWritten[i] = 'B';
            }

            KLogicalLogInformation::StreamBlockHeader emptyStreamBlockHeader;
            RtlZeroMemory(&emptyStreamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));
            lastStreamBlockHeader = &emptyStreamBlockHeader;

            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader
                                 );

            //
            // Write a bunch with record marker
            //
            for (ULONG i = 0; i < 16; i++)
            {
                ULONG dataSizeWritten = 128 * 1024;
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             iodata,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE);                       // IsBarrier

                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;
            }

            ULONGLONG asnAtLastMarker = asn;

            //
            // Write a bunch without record marker.
            //
            for (ULONG i = 0; i < ioBufferSize; i++)
            {
                dataWritten[i] = 'A';
            }

            for (ULONG i = 0; i < 1; i++)
            {
                ULONG dataSizeWritten = 8 * 1024;
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             iodata,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             FALSE);                       // IsBarrier

                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;
            }

            //
            // Close and reopen
            //
            {
                logStream->StartClose(NULL,
                                      closeStreamSync.CloseCompletionCallback());
                status = closeStreamSync.WaitForCompletion();
                logStream = nullptr;

                openStreamAsync->Reuse();
                openStreamAsync->StartOpenLogStream(logStreamId,
                                                        &metadataLength,
                                                        logStream,
                                                        NULL,    // ParentAsync
                                                        sync);

                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

            }

            ULONGLONG tailAsn;
            {
                KtlLogStream::AsyncIoctlContext::SPtr ioctl;
                KBuffer::SPtr tailAndVersionBuffer;
                KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation* tailAndVersion;
                ULONG result;

                status = KBuffer::Create(sizeof(KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation),
                                         tailAndVersionBuffer,
                                         *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = logStream->CreateAsyncIoctlContext(ioctl);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                ioctl->StartIoctl(KLogicalLogInformation::QueryLogicalLogTailAsnAndHighestOperationControl,
                                  NULL,
                                  result,
                                  tailAndVersionBuffer,
                                  NULL,
                                  sync);
                status =  sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                tailAndVersion = (KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation*)tailAndVersionBuffer->GetBuffer();
                tailAsn = tailAndVersion->TailAsn;

                //
                // Set the SharedTruncationAsn back so that the write
                // after the reopen will not be sent directly to the
                // dedicated log. In order for the repro to occur the
                // shared truncation ASN must be below the write so the
                // dedicated write can get coalesced.
                //
                ioctl->Reuse();
                KBuffer::SPtr setSharedTruncationAsnBuffer;
                KLogicalLogInformation::SharedTruncationAsnStruct* setSharedTruncationAsn;

                status = KBuffer::Create(sizeof(KLogicalLogInformation::SharedTruncationAsnStruct),
                                         setSharedTruncationAsnBuffer,
                                         *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                setSharedTruncationAsn = (KLogicalLogInformation::SharedTruncationAsnStruct*)setSharedTruncationAsnBuffer->GetBuffer();
                setSharedTruncationAsn->SharedTruncationAsn = asnAtLastMarker -1;

                ioctl->StartIoctl(KLogicalLogInformation::SetSharedTruncationAsn,
                                  setSharedTruncationAsnBuffer.RawPtr(),
                                  result,
                                  setSharedTruncationAsnBuffer,
                                  NULL,
                                  sync);
                status =  sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            VERIFY_IS_TRUE(tailAsn == asnAtLastMarker);
            asn = tailAsn;

            //
            // Now write the implicit truncate tail records
            //
            for (ULONG i = 0; i < ioBufferSize; i++)
            {
                dataWritten[i] = 'B';
            }

            for (ULONG i = 0; i < 2; i++)
            {
                ULONG dataSizeWritten = 48;
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             iodata,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM,
                                             TRUE);                       // IsBarrier

                VERIFY_IS_TRUE(NT_SUCCESS(status));
                asn += dataSizeWritten;
            }

            //
            // Multirecord read from before the record marker and
            // verify that none of the data that was unprotected by the
            // record marker is returned
            //

            ULONG sizeExpectedVerify = 512 * 1024;
            KtlLogAsn asnToVerify = asn - sizeExpectedVerify;
            ULONGLONG versionToVerify = version;

            MultiRecordReadAndVerify(logStream,
                                     asnToVerify,
                                     metadataIoBufferVerify,
                                     ioBufferVerify,
                                     STATUS_SUCCESS,
                                     asnToVerify.Get(),
                                     sizeExpectedVerify,
                                     dataWritten,                // TODO: fill dataWritten to verify
                                     versionToVerify,
                                     logStreamId,
                                     logStream->QueryReservedMetadataSize());


            logStream->StartClose(NULL,
                                  closeStreamSync.CloseCompletionCallback());
            status = closeStreamSync.WaitForCompletion();
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

VOID
LLDataWorkloadTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

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


    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

#ifdef FEATURE_TEST
    const ULONG timeToRunInSeconds = 240;
#else
    const ULONG timeToRunInSeconds = 15;
#endif
    LLWORKLOADSHAREDINFO llWorkloadInfo;
    StartLLWorkload(logManager,
                    diskId,
                    logContainerId,
                    timeToRunInSeconds,               // 2 minutes
                    0x800,             // Max records per cycle
                    0,                 // Default Stream size
                    0,                 // Max write latency
                    0,                 // Max average latency
                    5,                 // 5ms delay between writes
                    &llWorkloadInfo);


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

NTSTATUS GetStreamTruncationPoint(
    KtlLogStream::SPtr logStream,
    KtlLogAsn& truncationAsn
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncQueryRecordRangeContext::SPtr qRRContext;
    KSynchronizer sync;

    status = logStream->CreateAsyncQueryRecordRangeContext(qRRContext);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    qRRContext->StartQueryRecordRange(nullptr,          // lowAsn
                                      nullptr,          // highAsn
                                      &truncationAsn,
                                      nullptr,
                                      sync);
    status = sync.WaitForCompletion();

    return(status);
}

VOID
LLDataTruncateTest(
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

        {
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

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG metadataLength;

        KtlLogAsn truncationAsn;
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

        //
        // Open the stream
        //
        {
            KtlLogStream::SPtr logStream;
            KLogicalLogInformation::StreamBlockHeader* lastStreamBlockHeader;

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
            // Disable coalescence since this test counts on timing of dedicated writes
            //
            KtlLogStream::AsyncIoctlContext::SPtr ioctl;
            KBuffer::SPtr inBuffer, outBuffer;
            ULONG result;
            inBuffer = nullptr;

            status = logStream->CreateAsyncIoctlContext(ioctl);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

            ioctl->Reuse();
            ioctl->StartIoctl(KLogicalLogInformation::DisableCoalescingWrites, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);


            offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);


            //
            // Verify that log stream id is set correctly
            //
            KtlLogStreamId queriedLogStreamId;
            logStream->QueryLogStreamId(queriedLogStreamId);

            VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

            status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KIoBuffer::CreateSimple(16*0x1000,
                                             iodata,
                                             iodataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                             metadata,
                                             metadataBuffer,
                                             *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = KBuffer::Create(16*0x1000,
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

            KLogicalLogInformation::StreamBlockHeader emptyStreamBlockHeader;
            RtlZeroMemory(&emptyStreamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));
            lastStreamBlockHeader = &emptyStreamBlockHeader;

            VerifyTailAndVersion(logStream,
                                 version,
                                 asn,
                                 lastStreamBlockHeader
                                 );

            //
            // Test 1: Write records at 1, 11, 21, 31, 41 and 51.
            // Truncate head at 21 and verify that record 11 is not
            // truncated and truncation point at 11.
            //
            dataSizeWritten = 10;

            for (ULONG i = 0; i < 6; i++)
            {
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;
            }

            //
            // Wait here until all writes above have made it to the dedicated log
            // We do not know when the dedicated log will actually finish the writes
            // so this wait may need to be tweaked. The truncation relies upon the dedicated
            // log records index. In the case where the dedicated log has not caught up
            // then an optimal truncation may not be possible; this case is fine as it is valid
            // to overrecover.
            //
            status = WaitForWritesToComplete(logStream, asn - dataSizeWritten -1, 60, 1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

            KNt::Sleep(6 * 4 * 1000);

            KtlLogAsn truncatedAsn;
            ULONGLONG expectedTruncatedAsn;

            truncatedAsn = 21;
            expectedTruncatedAsn = 11;
            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);

            //
            // Test 2: Records are 21, 31, 41 and 51
            // Truncate head at 45 and verify that truncation point is
            // 31
            truncatedAsn = 45;
            expectedTruncatedAsn = 31;

            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);

            //
            // Test 3: Records are at 41 and 51
            // Truncate head at 70 and verify truncation point is still
            // 31
            truncatedAsn = 70;
            expectedTruncatedAsn = 31;

            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);


            // Test 4: Records are at 41 and 51
            // Truncate head at 35 and verify truncation point is still
            // 31
            truncatedAsn = 35;
            expectedTruncatedAsn = 31;

            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);


            // Test 5: Record are at 41 and 51, truncate head at 60,
            // truncation point should be 41
            truncatedAsn = 60;
            expectedTruncatedAsn = 41;

            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);


            // Test 5a: Record is only 51, truncate head at 55,
            // truncation point should be 41
            truncatedAsn = 55;
            expectedTruncatedAsn = 41;

            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);


            //
            // Test 6: Write record 61, 71, 81, 91, 71. Truncate at 65
            // and verify truncation point is 51.
            for (ULONG i = 0; i < 4; i++)
            {
                version++;

                status = SetupAndWriteRecord(logStream,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;
            }

            version++;

            asn = 71;
            dataSizeWritten = 14;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;

            //
            // Sleep long enough for the dedicated log to write its records
            //
            status = WaitForWritesToComplete(logStream, asn - dataSizeWritten -1, 60, 1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

            //KNt::Sleep(5 * 5 * 1000);

            truncatedAsn = 65;
            expectedTruncatedAsn = 51;

            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);


            // Test 7: Records are at 71, 81, 91, 71. Write 85, 75, 77,
            // 87, 97, 80, truncate at 82
            // and verify trucation point is 61.


            version++;
            VERIFY_IS_TRUE(asn == 85);
            dataSizeWritten = 30;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataSizeWritten;

            version++;
            asn = 75;
            dataSizeWritten = 2;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataSizeWritten;

            version++;
            VERIFY_IS_TRUE(asn == 77);
            dataSizeWritten = 10;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataSizeWritten;

            version++;
            VERIFY_IS_TRUE(asn == 87);
            dataSizeWritten = 10;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataSizeWritten;

            version++;
            VERIFY_IS_TRUE(asn == 97);
            dataSizeWritten = 10;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataSizeWritten;


            version++;
            asn = 80;
            dataSizeWritten = 5;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            asn += dataSizeWritten;

            //
            // Sleep long enough for the dedicated log to write its records
            //
            status = WaitForWritesToComplete(logStream, asn - dataSizeWritten -1, 60, 1000);
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

            //KNt::Sleep(6 * 5 * 1000);

            truncatedAsn = 82;
            expectedTruncatedAsn = 77;

            status = TruncateStream(logStream,
                                    truncatedAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            //
            // Sleep to give truncate a chance to run
            //
            KNt::Sleep(1000);

            status = GetStreamTruncationPoint(logStream,
                                              truncationAsn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(truncationAsn.Get() == expectedTruncatedAsn ? true : false);

            ioctl->Reuse();
            ioctl->StartIoctl(KLogicalLogInformation::EnableCoalescingWrites, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

            ioctl = nullptr;
            logStream->StartClose(NULL,
                                  closeStreamSync.CloseCompletionCallback());
            status = closeStreamSync.WaitForCompletion();
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

// \GLOBAL??\Volume{d498b863-8a6d-44f9-b040-48eab3b42b02} on windows
// /RvdLog on Linux
VOID DeleteAllContainersOnDisk(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KWString rootPath(*g_Allocator);

#if ! defined(PLATFORM_UNIX)
    rootPath = L"\\Global??\\Volume";
    rootPath += diskId;
    rootPath += L"\\RvdLog";
#else
    rootPath = L"/RvdLog";
#endif
    VERIFY_IS_TRUE(NT_SUCCESS(rootPath.Status()));

    //
    // Try and enum and delete any existing log files
    //
    KVolumeNamespace::NameArray files(*g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(files.Status()));

    status = KVolumeNamespace::QueryFiles(rootPath, files, *g_Allocator, sync);
    VERIFY_IS_TRUE((K_ASYNC_SUCCESS(status)));
    status = sync.WaitForCompletion();
    // Ok to keep going on failure

    if (NT_SUCCESS(status))
    {
        //
        // We have a list of files - try and delete them all
        //
        rootPath += KVolumeNamespace::PathSeparator;
        
        VERIFY_IS_TRUE(NT_SUCCESS(rootPath.Status()));

        for (ULONG ix = 0; ix < files.Count(); ix++)
        {
            KWString filePath(rootPath);
            filePath += files[ix];
            VERIFY_IS_TRUE(NT_SUCCESS(filePath.Status()));

            for (ULONG i = 0; i < 6; i++)
            {
                status = KVolumeNamespace::DeleteFileOrDirectory(filePath, *g_Allocator, sync);
                VERIFY_IS_TRUE((K_ASYNC_SUCCESS(status)));
                status = sync.WaitForCompletion();

                if (NT_SUCCESS(status))
                {
                    break;
                } else {
                    KNt::Sleep(5000);
                }
            }
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            // CONSIDER: Ok to keep going on failure ??
        }
    }
}


VOID DeleteAllContainersOnDisk(
    KGuid diskId
    )
{
    NTSTATUS status;
    KServiceSynchronizer        sync;
    KtlLogManager::SPtr logManager;

#ifdef UPASSTHROUGH
    status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, logManager);
#else
    status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, logManager);
#endif
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                             sync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    DeleteAllContainersOnDisk(diskId, logManager);

    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              sync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager = nullptr;
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}

VOID DeleteAllContainersOnDiskByEnumApi(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KArray<KtlLogContainerId> containerIds(*g_Allocator);
    KtlLogManager::AsyncEnumerateLogContainers::SPtr enumContainers;
    KSynchronizer sync;
    ULONG i;

    status = logManager->CreateAsyncEnumerateLogContainersContext(enumContainers);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    do
    {
        enumContainers->Reuse();
        enumContainers->StartEnumerateLogContainers(diskId,
                                                    containerIds,
                                                    NULL,
                                                    sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainer;
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainer);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONG sharedRetryCounter = 0;
        for (i = 0; i < containerIds.Count(); i++)
        {
            deleteContainer->Reuse();
            deleteContainer->StartDeleteLogContainer(diskId,
                                                     containerIds[i],
                                                     NULL,
                                                     sync);

            //
            // Enumerate log containers will return both the shared log
            // containers and the dedicated log containers. However
            // only shared log containers can be deleted correctly and
            // so if a dedicated log container is passed the
            // K_STATUS_LOG_STRUCTURE_FAULT will be returned. So it is
            // ok to be permissive with this error code.
            //
            // Also permit STATUS_OBJECT_NAME_NOT_FOUND in the case
            // where another process is running this test at exactly
            // the same time.
            //
            status = sync.WaitForCompletion();

            if (status == STATUS_SHARING_VIOLATION)
            {
                //
                // It is possible that the rundown for a previous test
                // has not yet finished so we wait a bit and then try
                // again
                //
                sharedRetryCounter++;
                KNt::Sleep(20*1000);
                VERIFY_IS_TRUE(sharedRetryCounter < 4);
                break;
            }

            VERIFY_IS_TRUE(NT_SUCCESS(status) ||
                           (status == K_STATUS_LOG_STRUCTURE_FAULT) ||
                           (status == STATUS_OBJECT_NAME_NOT_FOUND));

            if (NT_SUCCESS(status))
            {
                //
                // Further, when a shared log container is successfully
                // deleted, all of the dedicated log containers
                // associated with it are also deleted. This means that
                // a log container in the containerIds array may
                // already be deleted before getting to it in the list.
                // So we need to reenumerate the list on each
                // successful delete.
                //
                break;
            }
        }
    } while (i != containerIds.Count());

    enumContainers->Reuse();
    enumContainers->StartEnumerateLogContainers(diskId,
                                                containerIds,
                                                NULL,
                                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_NOT_FOUND));

    VERIFY_IS_TRUE(containerIds.Count() == 0);
}


VOID
EnumContainersTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KArray<KtlLogContainerId> containerIds(*g_Allocator);
    KArray<KtlLogContainerId> containerIdsCreated(*g_Allocator);
    KtlLogManager::AsyncEnumerateLogContainers::SPtr enumContainers;
    ContainerCloseSynchronizer closeContainerSync;
    KSynchronizer sync;

    //
    // First step is to delete all containers on the diskid to estabish
    // a baseline.
    //
    DeleteAllContainersOnDisk(diskId,
                              logManager);

    //
    // Now create a finite set of containers and then enumerate them to
    // verify the api
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainerId logContainerId;
    KGuid guid;

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    const ULONG numberContainers = 4;

    for (ULONG i = 0; i < numberContainers; i++)
    {
        KtlLogContainer::SPtr logContainer;

        guid.CreateNew();
        logContainerId = static_cast<KtlLogContainerId>(guid);

        containerIdsCreated.Append(logContainerId);

        createContainerAsync->Reuse();
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

        logContainer->StartClose(NULL,
        closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    status = logManager->CreateAsyncEnumerateLogContainersContext(enumContainers);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    enumContainers->StartEnumerateLogContainers(diskId,
                                                containerIds,
                                                NULL,
                                                sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    VERIFY_IS_TRUE(containerIds.Count() == containerIdsCreated.Count());

    for (ULONG i = 0; i < containerIds.Count(); i++)
    {
        for (ULONG j = 0; j < containerIdsCreated.Count(); j++)
        {
            if (containerIds[i] == containerIdsCreated[j])
            {
                containerIdsCreated.Remove(j);
                break;
            }
        }
    }

    VERIFY_IS_TRUE(containerIdsCreated.Count() == 0);

    DeleteAllContainersOnDisk(diskId,
                              logManager);
}

VOID
DeletedDedicatedLogTest(
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
    KGuid logStreamGuid2;
    KGuid logStreamGuid3;
    KtlLogStreamId logStreamId;
    KtlLogStreamId logStreamId2;
    KtlLogStreamId logStreamId3;
    ULONG metadataLength = 0x10000;
    KtlLogStream::SPtr logStream;
    KtlLogStream::SPtr logStream2;
    KtlLogStream::SPtr logStream3;

    //
    // Test is to create a container and two stream within it. Delete the file that contains the
    // dedicated log for one of the streams and make sure operations work ok
    //
    //

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it. Be sure the shared log
    // is larger than the stream
    //
    KInvariant(DefaultTestLogFileSize > DEFAULT_STREAM_SIZE);

    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;


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
    KBuffer::SPtr securityDescriptor = nullptr;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
    createStreamAsync->StartCreateLogStream(logStreamId,
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

    logStreamGuid2.CreateNew();
    logStreamId2 = static_cast<KtlLogStreamId>(logStreamGuid2);
    createStreamAsync->Reuse();

    createStreamAsync->StartCreateLogStream(logStreamId2,
                                                nullptr,           // Alias
                                                nullptr,
                                                securityDescriptor,
                                                metadataLength,
                                                DEFAULT_STREAM_SIZE,
                                                DEFAULT_MAX_RECORD_SIZE,
                                                logStream2,
                                                NULL,    // ParentAsync
                                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStreamGuid3.CreateNew();
    logStreamId3 = static_cast<KtlLogStreamId>(logStreamGuid3);
    createStreamAsync->Reuse();

    createStreamAsync->StartCreateLogStream(logStreamId3,
                                                nullptr,           // Alias
                                                nullptr,
                                                securityDescriptor,
                                                metadataLength,
                                                DEFAULT_STREAM_SIZE,
                                                DEFAULT_MAX_RECORD_SIZE,
                                                logStream3,
                                                NULL,    // ParentAsync
                                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    logStream->StartClose(NULL, closeStreamSync.CloseCompletionCallback());
    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logStream = nullptr;

    logStream2->StartClose(NULL, closeStreamSync.CloseCompletionCallback());
    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logStream2 = nullptr;

    logStream3->StartClose(NULL, closeStreamSync.CloseCompletionCallback());
    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logStream3 = nullptr;

    logContainer->StartClose(NULL,
                    closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Now deleted the dedicated log for the unlucky logStream2
    //
#if !defined(PLATFORM_UNIX) 
    KWString rootPath(*g_Allocator, L"\\Global??\\Volume");
    rootPath += diskId;
    rootPath += L"\\RvdLog\\Log";
#else
    KWString rootPath(*g_Allocator, L"/RvdLog/Log");
#endif
    rootPath += logStreamGuid2;
    rootPath += L".log";
    VERIFY_IS_TRUE(NT_SUCCESS(rootPath.Status()));

    status = KVolumeNamespace::DeleteFileOrDirectory(rootPath, *g_Allocator, sync);
    VERIFY_IS_TRUE((K_ASYNC_SUCCESS(status)));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // TEST 1. Reopen the log container and verify that the container opens successfully
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
    // 2. Verify that the good log stream can be opened and written to
    //
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStreamA;
        ULONG metadataLengthA;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLengthA,
                                                logStreamA,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Verify that log stream id is set correctly
        //
        KtlLogStreamId queriedLogStreamId;
        logStreamA->QueryLogStreamId(queriedLogStreamId);

        VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

        for (ULONG i = 0; i < 5; i++)
        {
            ULONGLONG version = 1;
            RvdLogAsn asn = 23+i;
            status = WriteRecord(logStreamA, version, asn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        logStreamA->StartClose(NULL, closeStreamSync.CloseCompletionCallback());
        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;                
    }

    //
    // 3. Verify that the bad log stream fails open with correct error code
    //
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG metadataLengthA;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId2,
                                                &metadataLengthA,
                                                logStream2,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
    }


    //
    // 6. Verify that after deleting the bad log can be recreated
    //
    {
        createStreamAsync = NULL;

        status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        createStreamAsync->StartCreateLogStream(logStreamId2,
                                                    nullptr,           // Alias
                                                    nullptr,
                                                    securityDescriptor,
                                                    metadataLength,
                                                    DEFAULT_STREAM_SIZE,
                                                    DEFAULT_MAX_RECORD_SIZE,
                                                    logStream2,
                                                    NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < 5; i++)
        {
            ULONGLONG version = 1;
            RvdLogAsn asn = 23+i;
            status = WriteRecord(logStream2, version, asn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        logStream2->StartClose(NULL, closeStreamSync.CloseCompletionCallback());
        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream2 = nullptr;
    }

    //
    // 7. Verify that the container can be opened
    //
    logContainer->StartClose(NULL,
    closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    openAsync->Reuse();
    openAsync->StartOpenLogContainer(diskId,
                                        logContainerId,
                                        logContainer,
                                        NULL,         // ParentAsync
                                        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // 8. Verify that the newly created log can be opened and written
    //
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        KtlLogStream::SPtr logStream2A;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId2,
                                                &metadataLength,
                                                logStream2A,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Verify that log stream id is set correctly
        //
        KtlLogStreamId queriedLogStreamId;
        logStream2A->QueryLogStreamId(queriedLogStreamId);

        VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId2.Get() ? true : false);

        for (ULONG i = 0; i < 5; i++)
        {
            ULONGLONG version = 2;
            RvdLogAsn asn = 23+i;
            status = WriteRecord(logStream2A, version, asn);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        logStream2A->StartClose(NULL, closeStreamSync.CloseCompletionCallback());
        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // 9. Delete the dedicated log again and then reopen just the
    //    stream and verify it is cleaned up correctly.
    //
    status = KVolumeNamespace::DeleteFileOrDirectory(rootPath, *g_Allocator, sync);
    VERIFY_IS_TRUE((K_ASYNC_SUCCESS(status)));
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG metadataLengthA;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId2,
                                                &metadataLengthA,
                                                logStream2,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);
    }

    //
    // 10. Verify that the bad log stream fails open with correct error code
    //
    {
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        ULONG metadataLengthA;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId2,
                                                &metadataLengthA,
                                                logStream2,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_OBJECT_NAME_NOT_FOUND);
    }   

    //
    // Delete the log container
    //
    logContainer->StartClose(NULL, closeContainerSync.CloseCompletionCallback());
    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;

    status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteAsync->StartDeleteLogContainer(diskId,
                                            logContainerId,
                                            NULL,         // ParentAsync
                                            sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
}


VOID
DLogReservationTest(
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

    //
    // This test verifies that the DLog space is reserved correctly
    // when writing
    //

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it. Be sure the shared log
    // is larger than the stream
    //
    {
        KInvariant(DefaultTestLogFileSize > DEFAULT_STREAM_SIZE);

        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;

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

        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            KtlLogStream::SPtr logStream;

            status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
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


            KtlLogStream::SPtr logStream2;
            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
            createStreamAsync->Reuse();

            createStreamAsync->StartCreateLogStream(logStreamId,
                                                        nullptr,           // Alias
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        logStream2,
                                                        NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // First step is to reserve some space in the log
            //
            ULONG reservationSize =  0x10000 + 0x1000;
            status = ExtendReservation(logStream, reservationSize);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ULONGLONG asn = 1;
            ULONGLONG latency;
            ULONGLONG bytesWritten = 0;
            ULONGLONG startTime = GetTickCount64();
            while (NT_SUCCESS(status))
            {
                status = WriteRecord(logStream,
                                     1,              // version
                                     asn,
                                     1,              // 1 block
                                     0x10000,        // this size
                                     0,              // No reservation
                                     nullptr,        // no metadata
                                     0,
                                     &latency
                                    );
                if (NT_SUCCESS(status))
                {
                    bytesWritten += 0x10000 + 0x1000;
                    asn++;
                }
            }
            ULONGLONG endTime = GetTickCount64();

            ULONGLONG throughput = (bytesWritten / ( (endTime - startTime) / 1000)) / 0x400;

#if !defined(PLATFORM_UNIX)
            LOG_OUTPUT(L"Aggregate throughput: %ld KB/sec", throughput);
#endif

            VERIFY_IS_TRUE(status == STATUS_LOG_FULL);

            //
            // Write into second log stream to verify that it was the
            // dedicated log that was full and not the shared
            //
            status = WriteRecord(logStream2,
                                 1,              // version
                                 asn,
                                 1,              // 1 block
                                 0x10000,        // this size
                                 0,              // No reservation
                                 nullptr,        // no metadata
                                 0,
                                 &latency
                                );

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            //
            // Now finally we should be able to write to the first log
            // stream using the reservation
            //
            status = WriteRecord(logStream,
                                 1,              // version
                                 asn,
                                 1,              // 1 block
                                 0x10000,        // this size
                                 reservationSize,
                                 nullptr,        // no metadata
                                 0,
                                 &latency
                                );

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logStream->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream = nullptr;
            
            logStream2->StartClose(NULL,
                             closeStreamSync.CloseCompletionCallback());

            status = closeStreamSync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            logStream2 = nullptr;
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Delete the log container
    //
    {
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;

        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
}

WorkerAsync::WorkerAsync()
{
}

WorkerAsync::~WorkerAsync()
{
}


class ServiceWrapperStressAsync : public WorkerAsync
{
    K_FORCE_SHARED(ServiceWrapperStressAsync);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out ServiceWrapperStressAsync::SPtr& Context
        )
        {
            NTSTATUS status;
            ServiceWrapperStressAsync::SPtr context;

            context = _new(AllocationTag, Allocator) ServiceWrapperStressAsync();
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

            context->_LogContainer = nullptr;
            context->_LogStream = nullptr;
            context->_CreateStreamContext = nullptr;
            context->_DeleteStreamContext = nullptr;
            context->_OpenStreamContext = nullptr;

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
        again:
            switch(_State)
            {
                case Initial:
                {
                    ULONG_PTR foo = (ULONG_PTR)this;
                    ULONG foo1 = (ULONG)foo;
                    srand(foo1);

                    Status = _LogContainer->CreateAsyncCreateLogStreamContext(_CreateStreamContext);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    Status = _LogContainer->CreateAsyncDeleteLogStreamContext(_DeleteStreamContext);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    Status = _LogContainer->CreateAsyncOpenLogStreamContext(_OpenStreamContext);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));


                    // Fall through
                }

                case CreateStream:
                {
                    KBuffer::SPtr securityDescriptor = nullptr;
                    ULONG metadataLength = 0x10000;
                    _CreateStreamContext->Reuse();
                    _CreateStreamContext->StartCreateLogStream(_StreamId,
                                                                nullptr,           // Alias
                                                                nullptr,
                                                                securityDescriptor,
                                                                metadataLength,
                                                                DEFAULT_STREAM_SIZE,
                                                                DEFAULT_MAX_RECORD_SIZE,
                                                                _LogStream,
                                                                this,    // ParentAsync
                                                                _Completion);

                    break;
                }

                case DeleteStream:
                {
                    _DeleteStreamContext->Reuse();
                    _DeleteStreamContext->StartDeleteLogStream(_StreamId,
                                                             this,    // ParentAsync
                                                             _Completion);

                    break;
                }

                case OpenStream:
                {
                    ULONG openedMetadataLength;
                    _OpenStreamContext->Reuse();
                    _OpenStreamContext->StartOpenLogStream(_StreamId,
                                                            &openedMetadataLength,
                                                            _LogStream,
                                                            this,    // ParentAsync
                                                            _Completion);
                    break;
                }

                case CloseStream:
                {
                    _LogStream = nullptr;
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

            FSMState states[5] = { CreateStream, DeleteStream, OpenStream, CloseStream, Completed};

            _LastState = _State;
            _State = states[(rand() % 5)];

            if (_LastState == CloseStream)
            {
                goto again;
            }
        }

        VOID OnReuse() override
        {
        }

        VOID OnCompleted() override
        {
            _LogContainer = nullptr;
            _LogStream = nullptr;
            _CreateStreamContext = nullptr;
            _DeleteStreamContext = nullptr;
            _OpenStreamContext = nullptr;
        }

    private:
        enum  FSMState { Initial=0, CreateStream=1, DeleteStream=2, OpenStream=3, CloseStream=4, Completed=5 };
        FSMState _State, _LastState;

        //
        // Parameters
        //
        KtlLogContainer::SPtr _LogContainer;
        KtlLogStreamId _StreamId;

        //
        // Internal
        //
        KtlLogContainer::AsyncCreateLogStreamContext::SPtr _CreateStreamContext;
        KtlLogContainer::AsyncDeleteLogStreamContext::SPtr _DeleteStreamContext;
        KtlLogContainer::AsyncOpenLogStreamContext::SPtr _OpenStreamContext;
        KtlLogStream::SPtr _LogStream;
};

ServiceWrapperStressAsync::ServiceWrapperStressAsync()
{
}

ServiceWrapperStressAsync::~ServiceWrapperStressAsync()
{
}

VOID
ServiceWrapperStressTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it. Be sure the shared log
    // is larger than the stream
    //
    {
        KInvariant(DefaultTestLogFileSize > DEFAULT_STREAM_SIZE);

        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        LONGLONG logSize = DefaultTestLogFileSize;

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

        {
            const ULONG NumberConcurrentAsyncs = 32;
            ServiceWrapperStressAsync::SPtr asyncs[NumberConcurrentAsyncs];
            KSynchronizer syncArr[NumberConcurrentAsyncs];
            ULONG i;
            ServiceWrapperStressAsync::StartParameters startParameters;

            startParameters.LogContainer = logContainer;
            startParameters.StreamId = logStreamId;

            for (i = 0; i <  NumberConcurrentAsyncs; i++)
            {
                status = ServiceWrapperStressAsync::Create(*g_Allocator, ALLOCATION_TAG, asyncs[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);
            }

            for (i = 0; i <  NumberConcurrentAsyncs; i++)
            {
                asyncs[i]->StartIt(&startParameters, NULL, syncArr[i]);
            }

            for (i = 0; i <  NumberConcurrentAsyncs; i++)
            {
                status = syncArr[i].WaitForCompletion();
            }
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }
}

#ifdef INCLUDE_OPEN_SPECIFIC_STREAM_TEST
// Enable this when debugging a log recovery failure
VOID
OpenSpecificStreamTest(
    KGuid,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;

    KGuid logContainerGuid1(0x01cf395e,0xde32,0xc53e,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01);
    KString::SPtr logContainerPath1 = KString::Create(L"\\??\\C:\\ProgramData\\Windows Fabric\\Public5\\Fabric\\work\\Applications\\StateManagerTestServiceApplication_App0\\work\\Shared_d3566658-153a-4c04-b7b8-d91a3c0c5e0d_01cf395e-de32-c53e-0807-060504030201.log", *g_Allocator, TRUE);

    KString::SPtr logContainerPath;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    ContainerCloseSynchronizer closeSync;

    KGuid logStreamGuid1(0x49fdfa2c,0x15aa,0x4ebe, 0x9b, 0xa3, 0x4d, 0x1c, 0x45, 0xfa, 0xcf, 0x6f);
    KGuid logStreamGuid2(0x333dfb8a,0x2209,0x4290,0xa5,0x3b,0x71,0xa2,0x0b,0x11,0x53,0x06);
    KtlLogStreamId logStreamId;
    KtlLogStream::SPtr logStream;
    ULONG openedMetadataLength;
    StreamCloseSynchronizer closeStreamSync;
    KSynchronizer sync;

    logContainerPath = logContainerPath1;
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid1);

    //
    // Open container and a stream within it.
    //
    {
        KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;

        status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openAsync->StartOpenLogContainer(*logContainerPath,
                                             logContainerId,
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;

        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid2);
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &openedMetadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogAsn lowAsn, highAsn, truncateAsn;
        KtlLogStream::AsyncQueryRecordRangeContext::SPtr qrrContext;

        status = logStream->CreateAsyncQueryRecordRangeContext(qrrContext);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        qrrContext->StartQueryRecordRange(&lowAsn, &highAsn, &truncateAsn, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KtlLogStream::AsyncTruncateContext::SPtr truncateAsync;
        status = logStream->CreateAsyncTruncateContext(truncateAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        truncateAsync->Truncate(truncateAsn, truncateAsn);
        KNt::Sleep(500);

        logStream->StartClose(NULL,
                            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        openStreamAsync->Reuse();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid1);
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &openedMetadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
                            closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        logContainer->StartClose(NULL,
                         closeSync.CloseCompletionCallback());

        status = closeSync.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_SUCCESS);

    }
}
#endif

#if defined(UDRIVER) || defined(UPASSTHROUGH)
#include "../../ktlshim/KtlLogShimKernel.h"
#endif

VOID SetupTests(
    KGuid& DiskId,
    UCHAR& DriveLetter,
    ULONG LogManagerCount,
    KtlLogManager::SPtr* LogManagers,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    NTSTATUS status;
    KServiceSynchronizer        sync;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    System->SetStrictAllocationChecks(TRUE);

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

#if defined(UDRIVER) 
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
#if defined(KDRIVER) || defined(UPASSTHROUGH) || defined(DAEMON)
    // For these, we assume "kernel" side is already setup
#endif

    for (ULONG i = 0; i < LogManagerCount; i++)
    {
        //
        // Access the log manager
        //
#ifdef UPASSTHROUGH
        status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, LogManagers[i]);
#else
        status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, LogManagers[i]);
#endif
        
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = LogManagers[i]->StartOpenLogManager(NULL, // ParentAsync
                                                 sync.OpenCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    DeleteAllContainersOnDisk(DiskId, LogManagers[0]);
}

VOID CleanupTests(
    KGuid& DiskId,
    ULONG LogManagerCount,
    KtlLogManager::SPtr* LogManagers,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    UNREFERENCED_PARAMETER(DiskId);
    UNREFERENCED_PARAMETER(System);
    UNREFERENCED_PARAMETER(StartingAllocs);

    NTSTATUS status;
    KServiceSynchronizer        sync;

    for (ULONG i = 0; i < LogManagerCount; i++)
    {
        status = LogManagers[i]->StartCloseLogManager(NULL, // ParentAsync
                                                  sync.CloseCompletionCallback());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        LogManagers[i] = nullptr;
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

#if defined(UDRIVER)
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    KInvariant(NT_SUCCESS(status));
#endif
    
#if defined(KDRIVER)  || defined(UPASSTHROUGH) || defined(DAEMON)
    // For kernel, we assume "kernel" side is already configured
#endif

    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

//
// Raw Logger tests
//

class OpenCloseLogManagerStressAsync : public WorkerAsync
{
    K_FORCE_SHARED(OpenCloseLogManagerStressAsync);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out OpenCloseLogManagerStressAsync::SPtr& Context
        )
    {
        NTSTATUS status;
        OpenCloseLogManagerStressAsync::SPtr context;

        context = _new(AllocationTag, Allocator) OpenCloseLogManagerStressAsync();
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

        Context = context.RawPtr();

        return(STATUS_SUCCESS);
    }

private:
    //
    // Parameters
    //
    ULONG _Loops;

public:
    struct StartParameters
    {
        ULONG Loops;
    };


    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;

        _LoopCounter = 0;
        _Loops = startParameters->Loops;        

        Start(ParentAsyncContext, CallbackPtr);
    }

protected:
    VOID OpenServiceCompletion(
        KAsyncContextBase* const,           // Parent; can be nullptr
        KAsyncServiceBase&,                 // BeginOpen()'s Service instance
        NTSTATUS Status
    )
    {
        FSMContinue(Status);
    }
                               
    VOID CloseServiceCompletion(
        KAsyncContextBase* const,           // Parent; can be nullptr
        KAsyncServiceBase&,                 // BeginOpen()'s Service instance
        NTSTATUS Status
    )
    {
        FSMContinue(Status);
    }                              

    VOID FSMContinue(
        __in NTSTATUS Status
        ) override
    {
        if (! NT_SUCCESS(Status))
        {
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            VERIFY_IS_TRUE(FALSE);
            Complete(Status);
            return;
        }
        
        switch (_State)
        {
            case Initial:
            {
Initial:
#ifdef UPASSTHROUGH
                Status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, _LogManager);
#else
                Status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, _LogManager);
#endif
                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                
                _State = OpenLogManager;
                _LogManager->StartOpenLogManager(NULL,
                                                 KAsyncServiceBase::OpenCompletionCallback(this, &OpenCloseLogManagerStressAsync::OpenServiceCompletion));
                break;
            }

            case OpenLogManager:
            {
                _State = CloseLogManager;
                _LogManager->StartCloseLogManager(NULL, 
                                                  KAsyncServiceBase::CloseCompletionCallback(this, &OpenCloseLogManagerStressAsync::CloseServiceCompletion));
                break;
            }

            case CloseLogManager:
            {
                _LogManager = nullptr;
                
                _LoopCounter++;
                if (_LoopCounter >= _Loops)
                {
                    Complete(STATUS_SUCCESS);
                } else {
                    goto Initial;
                }
                break;
            }

            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               KTraceFailedAsyncRequest(Status, this, _State, 0);
               VERIFY_IS_TRUE(FALSE);
               Complete(Status);
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
        _LogManager = nullptr;
    }

private:
    enum  FSMState { Initial = 0, OpenLogManager = 1, CloseLogManager = 2 };
    FSMState _State;

    //
    // Internal
    //
    KtlLogManager::SPtr _LogManager;
    ULONG _LoopCounter;
};

OpenCloseLogManagerStressAsync::OpenCloseLogManagerStressAsync()
{
}

OpenCloseLogManagerStressAsync::~OpenCloseLogManagerStressAsync()
{
}
   

VOID OpenCloseLogManagerStressTest(
    )
{
    NTSTATUS status;
    static const ULONG concurrent = 16;
    KSynchronizer syncs[concurrent];
    OpenCloseLogManagerStressAsync::SPtr asyncs[concurrent];

    struct OpenCloseLogManagerStressAsync::StartParameters startParameters;
    startParameters.Loops = 16;
    for (ULONG i = 0; i < concurrent; i++)
    {
        status = OpenCloseLogManagerStressAsync::Create(*g_Allocator,
                                                        KTL_TAG_TEST,
                                                        asyncs[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    for (ULONG i = 0; i < concurrent; i++)
    {
        asyncs[i]->StartIt(&startParameters,
                           NULL,
                           syncs[i]);
    }

    for (ULONG i = 0; i < concurrent; i++)
    {
        status = syncs[i].WaitForCompletion();      
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }       
}


VOID ReuseLogManagerTest(
    KGuid& diskId,
    ULONG LogManagerCount
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    KtlLogManager::SPtr logManager;
    KServiceSynchronizer        serviceSync;
    ContainerCloseSynchronizer closeContainerSync;

    //
    // Test 1: Start a new log manager, create a container, stop log
    //         manager, restart log manager then open container
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
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    {
        KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
        KtlLogContainer::SPtr logContainer;
        LONGLONG logSize = 1024 * 0x100000;   // 1GB

        status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        createContainerAsync->StartCreateLogContainer(diskId,
                                             logContainerId,
                                             logSize,
                                             0,            // Max Number Streams
                                             64 * 1024 * 1024,            // Max Record Size
                                             logContainer,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager= nullptr;
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Reopen container on a new log manager
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
    // reopen a log container
    //
    {
        KtlLogContainer::SPtr logContainer;
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
    // Test 2: Open many log managers and have each open the same
    //         container and then create a different stream within each
    //

        const ULONG MaxLogManagers = 16;
        KtlLogManager::SPtr logManagers[MaxLogManagers];
        KServiceSynchronizer        serviceSyncs[MaxLogManagers];
        KtlLogStream::SPtr logStreams[MaxLogManagers];
        KSynchronizer syncs[MaxLogManagers];
        KtlLogContainer::SPtr logContainers[MaxLogManagers];
        ContainerCloseSynchronizer closeContainerSyncs[MaxLogManagers];
        StreamCloseSynchronizer closeStreamSyncs[MaxLogManagers];

        if (LogManagerCount > MaxLogManagers)
        {
            LogManagerCount = MaxLogManagers;
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
#ifdef UPASSTHROUGH
            status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, logManagers[i]);
#else
            status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, logManagers[i]);
#endif
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = logManagers[i]->StartOpenLogManager(NULL, // ParentAsync
                                                     serviceSyncs[i].OpenCompletionCallback());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            status = serviceSyncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            KtlLogManager::AsyncOpenLogContainer::SPtr openAsyncA;

            status = logManagers[i]->CreateAsyncOpenLogContainerContext(openAsyncA);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            openAsyncA->StartOpenLogContainer(diskId,
                                                 logContainerId,
                                                 logContainers[i],
                                                 NULL,         // ParentAsync
                                                 syncs[i]);
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            status = syncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
            ULONG metadataLength = 0x10000;
            KGuid logStreamGuid;
            KtlLogStreamId logStreamId;

            logStreamGuid.CreateNew();
            logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

            status = logContainers[i]->CreateAsyncCreateLogStreamContext(createStreamAsync);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KBuffer::SPtr securityDescriptor = nullptr;
            createStreamAsync->StartCreateLogStream(logStreamId,
                                                        nullptr,           // Alias
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        logStreams[i],
                                                        NULL,    // ParentAsync
                                                    syncs[i]);
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            status = syncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            logStreams[i]->StartClose(NULL,
                             closeStreamSyncs[i].CloseCompletionCallback());
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            status = closeStreamSyncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            logContainers[i]->StartClose(NULL,
                             closeContainerSyncs[i].CloseCompletionCallback());
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            status = closeContainerSyncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            status = logManagers[i]->StartCloseLogManager(NULL, // ParentAsync
                                                      serviceSyncs[i].CloseCompletionCallback());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            logManagers[i] = nullptr;
        }

        for (ULONG i = 0; i < LogManagerCount; i++)
        {
            status = serviceSyncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Cleanup by deleting the container
    //
    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
    status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteAsync->StartDeleteLogContainer(diskId,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logManager= nullptr;
    status = serviceSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    // TODO: verify no files in c:\rvdlog
}

VOID
OneBitLogCorruptionTest(
    UCHAR driveLetter,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    StreamCloseSynchronizer closeStreamSync;
    ContainerCloseSynchronizer closeContainerSync;

    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;
    
    KString::SPtr containerName;
    KString::SPtr streamName;
    KtlLogContainerId logContainerId;
    KtlLogStreamId logStreamId;
    CreateStreamAndContainerPathnames(driveLetter,
                                      containerName,
                                      logContainerId,
                                      streamName,
                                      logStreamId
                                      );

    
    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(*containerName,
        logContainerId,
        256 * 1024 * 1024,
        0,            // Max Number Streams
        1024 * 1024,            // Max Record Size
        0,
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
        KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();

        createStreamAsync->StartCreateLogStream(logStreamId,
                                                logStreamType,
                                                nullptr,           // Alias
                                                 KString::CSPtr(streamName.RawPtr()),
                                                securityDescriptor,
                                                metadataLength,
                                                DEFAULT_STREAM_SIZE,
                                                DEFAULT_MAX_RECORD_SIZE,
                                                0,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
                         closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }

    KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
    ULONG metadataLength;

    ULONGLONG asn = KtlLogAsn::Min().Get();
    ULONGLONG version = 0;
    KIoBuffer::SPtr iodata;
    PVOID iodataBuffer;
    KIoBuffer::SPtr metadata;
    PVOID metadataBuffer;
    KBuffer::SPtr workKBuffer;
    PUCHAR workBuffer;

    PUCHAR dataWritten;
    ULONG dataSizeWritten;

    ULONG offsetToStreamBlockHeaderM;

    //
    // Open the stream and write a bunch of stuff
    //
    {
        KtlLogStream::SPtr logStream;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

        //
        // Verify that log stream id is set correctly
        //
        KtlLogStreamId queriedLogStreamId;
        logStream->QueryLogStreamId(queriedLogStreamId);

        VERIFY_IS_TRUE(queriedLogStreamId.Get() == logStreamId.Get() ? true : false);

        status = KIoBuffer::CreateSimple(16*0x1000,
                                         iodata,
                                         iodataBuffer,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                         metadata,
                                         metadataBuffer,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KBuffer::Create(16*0x1000,
                                 workKBuffer,
                                 *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        workBuffer = (PUCHAR)workKBuffer->GetBuffer();
        dataWritten = workBuffer;
        dataSizeWritten = 16*0x1000;

        for (ULONG j = 0; j < 4096; j++)
        {
            for (ULONG i = 0; i < dataSizeWritten; i++)
            {
                dataWritten[i] = (UCHAR)(0x10 + j);
            }

            version++;
            status = SetupAndWriteRecord(logStream,
                                         metadata,
                                         iodata,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);
            asn += dataSizeWritten;

            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        logStream->StartClose(NULL,
                              closeStreamSync.CloseCompletionCallback());
        status = closeStreamSync.WaitForCompletion();
        logStream = nullptr;                
    }

    //
    // Corrupt record data within the stream
    //
    {
        KBlockFile::SPtr file;
        KWString path(*g_Allocator, (PWCHAR)*streamName);

        status = KBlockFile::Create(path,
                                    TRUE,
                                    KBlockFile::eOpenExisting,
                                    file,
                                    sync,
                                    NULL,
                                    *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONGLONG magicOffset = 4 * 0x1000;
        KIoBuffer::SPtr corruptBuffer;
        PVOID p;
        PUCHAR pp;

        status = KIoBuffer::CreateSimple(0x1000, corruptBuffer, p, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = file->Transfer(KBlockFile::IoPriority::eForeground,
                                KBlockFile::TransferType::eRead,
                                magicOffset,
                                *corruptBuffer,
                                sync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        pp = (PUCHAR)p;

        pp[765]++;            // one bit corruption !!!

        
        status = file->Transfer(KBlockFile::IoPriority::eForeground,
                                KBlockFile::TransferType::eWrite,
                                magicOffset,
                                *corruptBuffer,
                                sync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        file->Close();
    }

    //
    // Reopen the stream and read it while verifying the data
    //
    {
        KtlLogStream::SPtr logStream;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStream,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        offsetToStreamBlockHeaderM = logStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);



        //
        // Do multirecord reads
        //
        KtlLogStream::AsyncMultiRecordReadContext::SPtr multiRecordRead;
        KIoBuffer::SPtr readMetadata;
        PVOID readMetadataPtr;
        KIoBuffer::SPtr iodata1;
        PVOID iodata1Ptr;
        KtlLogAsn asn1 = 1;

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                         readMetadata,
                                         readMetadataPtr,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        status = KIoBuffer::CreateSimple(8*0x1000,
                                            iodata1,
                                            iodata1Ptr,
                                            *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        
        status = logStream->CreateAsyncMultiRecordReadContext(multiRecordRead);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        multiRecordRead->StartRead(asn1,
                                   *readMetadata,
                                   *iodata1,
                                   NULL,
                                   sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(status == K_STATUS_LOG_STRUCTURE_FAULT);       

        multiRecordRead = nullptr;

        

        logStream->StartClose(NULL,
                              closeStreamSync.CloseCompletionCallback());
        status = closeStreamSync.WaitForCompletion();
        logStream = nullptr;                
        
    }

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KtlLogManager::AsyncDeleteLogContainer::SPtr deleteContainerAsync;

    status = logManager->CreateAsyncDeleteLogContainerContext(deleteContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    deleteContainerAsync->StartDeleteLogContainer(*containerName,
                                         logContainerId,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
}
   

VOID CreateLogContainerWithBadPathTest(
    UCHAR driveLetter
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    KtlLogManager::SPtr logManager;
    KServiceSynchronizer        serviceSync;
    ContainerCloseSynchronizer closeContainerSync;
    KString::SPtr containerPath;
    BOOLEAN b;
    WCHAR drive[7];

    //
    // Create a log manager and then try to create a container with a
    // bad pathname. Verify that the log manager will close properly.
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
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    containerPath = KString::Create(*g_Allocator,
                                    MAX_PATH);
    VERIFY_IS_TRUE((containerPath != nullptr) ? true : false);

    //
    // Build up container and stream paths
    //
    drive[0] = L'\\';
    drive[1] = L'?';
    drive[2] = L'?';
    drive[3] = L'\\';
    drive[4] = (WCHAR)driveLetter;
    drive[5] = 0;
    drive[6] = 0;

    b = containerPath->CopyFrom(drive, TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);

    b = containerPath->Concat(KStringView(L"\\PathThatShouldNotExist\\Filename.log"), TRUE);
    VERIFY_IS_TRUE(b ? true : FALSE);
    
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    KtlLogContainer::SPtr logContainer;
    LONGLONG logSize = 1024 * 0x100000;   // 1GB

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // try to create a log container where the path doesn't exist and
    // verify the error code
    //
    createContainerAsync->StartCreateLogContainer(*containerPath,
                                            logContainerId,
                                            logSize,
                                             0,            // Max Number Streams
                                             64 * 1024 * 1024,            // Max Record Size
                                            logContainer,
                                            NULL,         // ParentAsync
                                            sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((status == STATUS_OBJECT_PATH_INVALID) || (status == STATUS_INVALID_PARAMETER));

    //
    // Now close the log manager and verify that it closes promptly.
    // The bug that is being regressed causes the close to stop responding.
    //
    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = serviceSync.WaitForCompletion(30 * 1000);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logManager= nullptr;
}

#if defined(UDRIVER) || defined(UPASSTHROUGH)
class ThrottledAllocStress : public WorkerAsync
{
    K_FORCE_SHARED(ThrottledAllocStress);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out ThrottledAllocStress::SPtr& Context
        )
    {
            NTSTATUS status;
            ThrottledAllocStress::SPtr context;

            context = _new(AllocationTag, Allocator) ThrottledAllocStress();
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

            context->_IoBuffer = nullptr;
            context->_Allocator = nullptr;
            context->_Alloc = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);
        }

    struct StartParameters
    {
        ThrottledKIoBufferAllocator::SPtr Allocator;
        ULONG IterationCount;
        ULONG Size;
        ULONG MinimumSize;
    };

    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;
        _Allocator = startParameters->Allocator;
        _IterationCount = startParameters->IterationCount;
        _Size = startParameters->Size;
        _MinimumSize = startParameters->MinimumSize;

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
                Status = _Allocator->CreateAsyncAllocateKIoBufferContext(_Alloc);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }

                _Counter = 0;

                _State = Allocate;
                // fall through
            }

            case Allocate:
            {
Allocate:
                 _State = Free;
                 _DesiredSize = KLoggerUtilities::RoundUpTo4K((rand() % _Size)+ _MinimumSize);
                 _Alloc->Reuse();
                 _Alloc->StartAllocateKIoBuffer(_DesiredSize,
                                                KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                                _IoBuffer, this, _Completion);
                 break;
            }

            case Free:
            {
                 _Allocator->FreeKIoBuffer(0, _IoBuffer->QuerySize());
                 VERIFY_IS_TRUE(_DesiredSize == _IoBuffer->QuerySize());
                 _IoBuffer = nullptr;

                 _Counter++;
                 if (_Counter < _IterationCount)
                 {
                     _State = Allocate;
                     goto Allocate;
                 }

                 _State = Completed;
                 // fall through
            }

            case Completed:
            {
                Complete(STATUS_SUCCESS);
                return;
            }

            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               KTraceFailedAsyncRequest(Status, this, _State, 0);
               Complete(Status);
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
        _Alloc = nullptr;
        _IoBuffer = nullptr;
        _Allocator = nullptr;
    }

private:
    enum  FSMState { Initial = 0, Allocate = 1, Free = 2, Completed = 3 };
    FSMState _State;

    //
    // Parameters
    //
    ThrottledKIoBufferAllocator::SPtr _Allocator;
    ULONG _IterationCount;
    ULONG _Size;
    ULONG _MinimumSize;

    //
    // Internal
    //
    KIoBuffer::SPtr _IoBuffer;
    ULONG _Counter;
    ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr _Alloc;
    ULONG _DesiredSize;
};

ThrottledAllocStress::ThrottledAllocStress()
{
}

ThrottledAllocStress::~ThrottledAllocStress()
{
}

VOID ThrottledAllocatorTest(
    KGuid&
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KSynchronizer largeSync;


    //
    // Test 0: Alloc to limit, alloc and wait, free enough to release
    //         waiting alloc and verify alloc
    //
    {
        ULONG numberAllocs = 1000;
        ULONG ioBufferSize = 0x2000;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        KIoBuffer::SPtr ioBuffer;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = numberAllocs * ioBufferSize;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = numberAllocs * ioBufferSize;
        memoryThrottleLimits.AllocationTimeoutInMs = 5 * 60 * 1000;   // 5 minutes
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(alloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < numberAllocs; i++)
        {
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                          KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                          ioBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);

            ioBuffer = nullptr;
        }

        alloc->Reuse();
        alloc->StartAllocateKIoBuffer(ioBufferSize,
                                      KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                      ioBuffer, NULL, sync);
        status = sync.WaitForCompletion(2000);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

        throttledAllocator->FreeKIoBuffer(0, ioBufferSize);
        KNt::Sleep(250);

        status = sync.WaitForCompletion(0);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);
        ioBuffer = nullptr;

        throttledAllocator->FreeKIoBuffer(0, numberAllocs * ioBufferSize);

        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }


    {
        ULONG numberAllocs = 1000;
        ULONG ioBufferSize = 0x2000;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        KIoBuffer::SPtr ioBuffer;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = numberAllocs * ioBufferSize;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = numberAllocs * ioBufferSize;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(alloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < numberAllocs; i++)
        {
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                          KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                          ioBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);

            ioBuffer = nullptr;
        }

        alloc->Reuse();
        alloc->StartAllocateKIoBuffer(ioBufferSize,
                                      5 * 60 * 1000,
                                      ioBuffer, NULL, sync);
        status = sync.WaitForCompletion(2000);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

        throttledAllocator->FreeKIoBuffer(0, ioBufferSize);
        KNt::Sleep(250);

        status = sync.WaitForCompletion(0);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);
        ioBuffer = nullptr;

        throttledAllocator->FreeKIoBuffer(0, numberAllocs * ioBufferSize);

        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

    //
    // Test 1: Alloc and free with no limit many times in a single
    // thread and verify no leaks
    //
    {
        ULONG ioBufferSize = 0x2000;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;

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

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(alloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < 1000; i++)
        {
            KIoBuffer::SPtr ioBuffer;
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                                   KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                          ioBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);

            ioBuffer = nullptr;
            throttledAllocator->FreeKIoBuffer(0, ioBufferSize);
        }
        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

    //
    // Test 2: Alloc to limit, alloc and wait, free enough to release
    //         waiting alloc and verify alloc
    //
    {
        ULONG numberAllocs = 1000;
        ULONG ioBufferSize = 0x2000;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        KIoBuffer::SPtr ioBuffer;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = numberAllocs * ioBufferSize;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = numberAllocs * ioBufferSize;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(alloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < numberAllocs; i++)
        {
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                          KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                          ioBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);

            ioBuffer = nullptr;
        }

        alloc->Reuse();
        alloc->StartAllocateKIoBuffer(ioBufferSize,
                                      KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                      ioBuffer, NULL, sync);
        status = sync.WaitForCompletion(2000);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

        throttledAllocator->FreeKIoBuffer(0, ioBufferSize);
        KNt::Sleep(250);

        status = sync.WaitForCompletion(0);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);
        ioBuffer = nullptr;

        throttledAllocator->FreeKIoBuffer(0, numberAllocs * ioBufferSize);

        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

    //
    // Test 3: Alloc to limit, alloc large buffer, loop smaller
    //         alloc and free, free enough for large and verify large
    //         alloc succeeds
    //
    {
        ULONG numberAllocs = 1000;
        ULONG ioBufferSize = 0x2000;
        ULONG largeAllocMultiplier = 10;
        ULONG largeIoBufferSize = largeAllocMultiplier * 0x2000;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr largeAlloc;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr largeIoBuffer;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = numberAllocs * ioBufferSize;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = numberAllocs * ioBufferSize;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(alloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(largeAlloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Allocate up to almost the full pool size
        //
        for (ULONG i = 0; i < (numberAllocs - 1); i++)
        {
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                                   KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                          ioBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);

            ioBuffer = nullptr;
        }

        //
        // Perform a large allocation and expect this to wait at the
        // top of the list
        //
        largeAlloc->Reuse();
        largeAlloc->StartAllocateKIoBuffer(largeIoBufferSize,
                                                   KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                           largeIoBuffer, NULL, largeSync);
        status = largeSync.WaitForCompletion(250);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

        //
        // Try to alloc a small alloc. Even though there is space in
        // the pool for the small alloc, the alloc should fail since it
        // is not at the top of the list. However free up just enough
        // a bit at a time to satisfy the large waiting alloc eventually
        //
        for (ULONG i = 0; i < (largeAllocMultiplier - 1); i++)
        {
            //
            // Set the alloc to timeout after 3 seconds. Check that it
            // doesn't after 2 and certainly after 5. The 2 second
            // extra "slop" time is needed since the timer that checks
            // the list for timeouts only runs once a second
            //
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                          3000,
                                          ioBuffer, NULL, sync);

            status = sync.WaitForCompletion(2000);
            VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

            status = sync.WaitForCompletion(3000);
            VERIFY_IS_TRUE(status == STATUS_INSUFFICIENT_RESOURCES);

            throttledAllocator->FreeKIoBuffer(0, ioBufferSize);
        }

        //
        // Now verify that the large alloc suceeds
        //
        status = largeSync.WaitForCompletion(250);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(largeIoBuffer->QuerySize() == largeIoBufferSize);

        //
        // Since we know the full pool is allocated, we can free in one
        // shot
        //
        throttledAllocator->FreeKIoBuffer(0, numberAllocs * ioBufferSize);

        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

    //
    // Test 3a: Alloc to limit, alloc large buffer, loop smaller
    //         alloc and free, don't free enough for large and verify large
    //         alloc fails
    //
    {
        ULONG numberAllocs = 1000;
        ULONG ioBufferSize = 0x2000;
        ULONG largeAllocMultiplier = 10;
        ULONG largeIoBufferSize = largeAllocMultiplier * 0x2000;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr largeAlloc;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr largeIoBuffer;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = numberAllocs * ioBufferSize;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = numberAllocs * ioBufferSize;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(alloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(largeAlloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < (numberAllocs - 1); i++)
        {
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                                   KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                          ioBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);

            ioBuffer = nullptr;
        }

        largeAlloc->Reuse();
        largeAlloc->StartAllocateKIoBuffer(largeIoBufferSize,
                                           5000,
                                           largeIoBuffer, NULL, largeSync);
        status = largeSync.WaitForCompletion(250);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

        for (ULONG i = 0; i < (largeAllocMultiplier - 1); i++)
        {
            alloc->Reuse();
            alloc->StartAllocateKIoBuffer(ioBufferSize,
                                                   KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                          ioBuffer, NULL, sync);
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(ioBuffer->QuerySize() == ioBufferSize);

            ioBuffer = nullptr;
            throttledAllocator->FreeKIoBuffer(0, ioBufferSize);
        }
        status = largeSync.WaitForCompletion(30 * 1000);
        VERIFY_IS_TRUE(status == STATUS_INSUFFICIENT_RESOURCES);

        throttledAllocator->FreeKIoBuffer(0, (numberAllocs-1) * ioBufferSize);

        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

    //
    // Test 4: Stress many threads with limit alloc and free random
    //         sizes
    //
    {
        const ULONG numberAsyncs = 1000;
        const ULONG iterations = 128;
        ULONG ioBufferSize = 0x2000;
        LONG allocatorLimit = (numberAsyncs / 4) * ioBufferSize;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        ThrottledAllocStress::SPtr asyncs[numberAsyncs];
        KSynchronizer syncs[numberAsyncs];

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = allocatorLimit;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = allocatorLimit;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = ThrottledAllocStress::Create(*g_Allocator, KTL_TAG_TEST, asyncs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        struct ThrottledAllocStress::StartParameters params;
        params.Allocator = throttledAllocator;
        params.IterationCount = iterations;
        params.Size = ioBufferSize;
        params.MinimumSize = 0;

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            asyncs[i]->StartIt(&params, NULL, syncs[i]);
        }
        params.Allocator = nullptr;

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = syncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

    //
    // Test 5: Stress many threads with limit alloc and free random
    //         sizes and a few with very large allocs
    //
    {
#ifdef FEATURE_TEST
        const ULONG numberAsyncs = 1000;
        const ULONG iterations = 128;
#else
        const ULONG numberAsyncs = 100;
        const ULONG iterations = 32;
#endif
        ULONG ioBufferSize = 0x2000;
        ULONG largeIoBufferSize = ioBufferSize * 10;
        const ULONG numberLargeAsyncs = 10;
        LONG allocatorLimit = (numberAsyncs / 4) * ioBufferSize;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        ThrottledAllocStress::SPtr asyncs[numberAsyncs];
        KSynchronizer syncs[numberAsyncs];
        ThrottledAllocStress::SPtr largeAsyncs[numberLargeAsyncs];
        KSynchronizer largeSyncs[numberLargeAsyncs];

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = allocatorLimit;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = allocatorLimit;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = ThrottledAllocStress::Create(*g_Allocator, KTL_TAG_TEST, asyncs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < numberLargeAsyncs; i++)
        {
            status = ThrottledAllocStress::Create(*g_Allocator, KTL_TAG_TEST, largeAsyncs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        struct ThrottledAllocStress::StartParameters params;
        params.Allocator = throttledAllocator;
        params.IterationCount = iterations;
        params.Size = ioBufferSize;
        params.MinimumSize = 0;

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            asyncs[i]->StartIt(&params, NULL, syncs[i]);
        }

        params.MinimumSize = largeIoBufferSize;
        for (ULONG i = 0; i < numberLargeAsyncs; i++)
        {
            largeAsyncs[i]->StartIt(&params, NULL, largeSyncs[i]);
        }

        params.Allocator = nullptr;

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = syncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < numberLargeAsyncs; i++)
        {
            status = largeSyncs[i].WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

    //
    // Test 6: allocate larger than limit and expect failure
    //
    {
        ULONG ioBufferSize = 0x2000;
        ULONG largeIoBufferSize = 0x3000;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;
        ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr largeAlloc;
        ThrottledKIoBufferAllocator::SPtr throttledAllocator;
        KIoBuffer::SPtr ioBuffer;
        KIoBuffer::SPtr largeIoBuffer;

        KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
        memoryThrottleLimits.WriteBufferMemoryPoolPerStream = largeIoBufferSize - ioBufferSize;
        memoryThrottleLimits.WriteBufferMemoryPoolMax = largeIoBufferSize;
        memoryThrottleLimits.WriteBufferMemoryPoolMin = ioBufferSize;
        memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs;
        memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_NoSharedLogThrottleLimit;

        status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
            memoryThrottleLimits,
            *g_Allocator,
            KTL_TAG_TEST,
            throttledAllocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(alloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = throttledAllocator->CreateAsyncAllocateKIoBufferContext(largeAlloc);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        largeAlloc->Reuse();
        largeAlloc->StartAllocateKIoBuffer(largeIoBufferSize,
            KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
            largeIoBuffer, NULL, largeSync);
        status = largeSync.WaitForCompletion(250);
        VERIFY_IS_TRUE(status == STATUS_INVALID_BUFFER_SIZE);

        //
        // Increase limit to allow the larger alloc to start then lower limit to force it to fail
        //
        LONG size = throttledAllocator->AddToLimit();

        alloc->Reuse();
        alloc->StartAllocateKIoBuffer(ioBufferSize,
            KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
            ioBuffer, NULL, sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        largeAlloc->Reuse();
        largeAlloc->StartAllocateKIoBuffer(largeIoBufferSize,
            KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
            ioBuffer, NULL, largeSync);
        status = largeSync.WaitForCompletion(2000);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

        throttledAllocator->RemoveFromLimit(size);
        status = largeSync.WaitForCompletion(2000);
        VERIFY_IS_TRUE(status == STATUS_INVALID_BUFFER_SIZE);

        throttledAllocator->FreeKIoBuffer(0, ioBufferSize);
        throttledAllocator->Shutdown();
        throttledAllocator.Reset();
    }

}
#endif

//
VOID
OpenRightAfterCloseTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;

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

    static const ULONG LoopCount = 8;
    static const ULONG ReopenCount = 16;
    static const ULONG WriteCount = 64;


    KtlLogStream::SPtr logStream;
    KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG iLoops = 0; iLoops < LoopCount; iLoops++)
    {
        ULONG metadataLength = 0x10000;
        KGuid logStreamGuid;
        KtlLogStreamId logStreamId;
        StreamCloseSynchronizer closeStreamWaitSync;
        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
        KBuffer::SPtr securityDescriptor = nullptr;

        createStreamAsync->Reuse();
        createStreamAsync->StartCreateLogStream(logStreamId,
                                                    logStreamType,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
                         closeStreamWaitSync.CloseCompletionCallback());

        status = closeStreamWaitSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        //
        // Open the stream
        //
        StreamCloseSynchronizer closeStreamNoWaitSync;

        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        KIoBuffer::SPtr metadata;
        PVOID metadataBuffer;
        KIoBuffer::SPtr emptyIoBuffer;

        status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                         metadata,
                                         metadataBuffer,
                                         *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        ULONG offsetToStreamBlockHeaderM;

        ULONGLONG version = 0;
        ULONGLONG asn = KtlLogAsn::Min().Get();

        for (ULONG iReopen = 0; iReopen < ReopenCount; iReopen++)
        {
            KtlLogStream::SPtr logStreamA;

            openStreamAsync->Reuse();
            openStreamAsync->StartOpenLogStream(logStreamId,
                                                    &metadataLength,
                                                    logStreamA,
                                                    NULL,    // ParentAsync
                                                    sync);

            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            offsetToStreamBlockHeaderM = logStreamA->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

            //
            // Write a record
            //
            for (ULONG iWrites = 0; iWrites < WriteCount; iWrites++)
            {
                ULONG data = iWrites;
                PUCHAR dataWritten = (PUCHAR)&data;
                ULONG dataSizeWritten = sizeof(ULONG);
                version++;

                status = SetupAndWriteRecord(logStreamA,
                                             metadata,
                                             emptyIoBuffer,
                                             version,
                                             asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             offsetToStreamBlockHeaderM);

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                asn += dataSizeWritten;
            }

            //
            // Do not wait for completion
            //
            logStreamA->StartClose(NULL,
                             closeStreamNoWaitSync.CloseCompletionCallback());
            logStream = nullptr;
        }

        //
        // Delete the stream
        //
        KtlLogContainer::AsyncDeleteLogStreamContext::SPtr deleteStreamAsync;

        status = logContainer->CreateAsyncDeleteLogStreamContext(deleteStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        deleteStreamAsync->StartDeleteLogStream(logStreamId,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Cleanup the log
    //
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
//


#if defined(UDRIVER) || defined(UPASSTHROUGH)

NTSTATUS
VerifyRawRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const MetaDataBuffer,
    __in ULONG MetaDataBufferSize,
    __in const KIoBuffer::SPtr& IoBuffer
)
{
    UNREFERENCED_PARAMETER(MetaDataBuffer);
    UNREFERENCED_PARAMETER(MetaDataBufferSize);
    UNREFERENCED_PARAMETER(IoBuffer);

    return(STATUS_SUCCESS);
}


VOID QueryLogContainerUsage(
    KGuid diskId,
    KtlLogContainerId logContainerId,
    ULONGLONG& totalSpace,
    ULONGLONG& freeSpace
   )
{
    NTSTATUS status;
    RvdLogManager::SPtr rawLogManager;
    KSynchronizer activateSync;
    KSynchronizer sync;

    RvdLogManager::AsyncOpenLog::SPtr openLogOp;
    RvdLog::SPtr rawLogContainer;
    KAsyncEvent closeEvent;
    KAsyncEvent::WaitContext::SPtr waitForClose;

    status = closeEvent.CreateWaitContext(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), waitForClose);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), rawLogManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = rawLogManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status));

    rawLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(), &VerifyRawRecordCallback);


    status = rawLogManager->CreateAsyncOpenLogContext(openLogOp);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    openLogOp->StartOpenLog(diskId,
                            logContainerId,
                            rawLogContainer,
                            NULL,
                            sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rawLogContainer->QuerySpaceInformation(&totalSpace,
                                           &freeSpace);

    openLogOp.Reset();
    rawLogContainer->SetShutdownEvent(&closeEvent);
    rawLogContainer.Reset();

    waitForClose->StartWaitUntilSet(NULL, sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    rawLogManager->Deactivate();
    activateSync.WaitForCompletion();
    rawLogManager.Reset();
}

VOID
OpenWriteCloseTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;


    //
    // This test opens the stream, writes to it and then immediately
    // closes it. Once closed it verifies that the stream has been
    // completely destaged from the shared log
    //

    //
    // Create container and a stream within it
    //
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;

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

    static const ULONG ReopenCount = 32;
    static const ULONG WriteCount = 32;


    KtlLogStream::SPtr logStream;
    KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG metadataLength = 0x10000;
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;
    StreamCloseSynchronizer closeStreamWaitSync;
    logStreamGuid.CreateNew();
    logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
    KBuffer::SPtr securityDescriptor = nullptr;

    createStreamAsync->Reuse();
    createStreamAsync->StartCreateLogStream(logStreamId,
                                                logStreamType,
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
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    logStream->StartClose(NULL,
                     closeStreamWaitSync.CloseCompletionCallback());

    status = closeStreamWaitSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logStream = nullptr;

    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    //
    // Prepare for the loops
    //
    KtlLogManager::AsyncOpenLogContainer::SPtr openContainerAsync;
    status = logManager->CreateAsyncOpenLogContainerContext(openContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KIoBuffer::SPtr metadata;
    PVOID metadataBuffer;
    KIoBuffer::SPtr emptyIoBuffer;

    status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                     metadata,
                                     metadataBuffer,
                                     *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG offsetToStreamBlockHeaderM;

    ULONGLONG version = 0;
    ULONGLONG asn = KtlLogAsn::Min().Get();

    BOOLEAN firstTime = TRUE;
    for (ULONG iReopen = 0; iReopen < ReopenCount; iReopen++)
    {
        KtlLogStream::SPtr logStreamA;
        ULONGLONG totalSpaceBefore, freeSpaceBefore;
        ULONGLONG totalSpaceAfter, freeSpaceAfter;

        QueryLogContainerUsage(diskId, logContainerId, totalSpaceBefore, freeSpaceBefore);

        openContainerAsync->Reuse();
        openContainerAsync->StartOpenLogContainer(diskId,
                                         logContainerId,
                                         logContainer,
                                         NULL,
                                         sync);
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
        status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        openStreamAsync->StartOpenLogStream(logStreamId,
                                                &metadataLength,
                                                logStreamA,
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        offsetToStreamBlockHeaderM = logStreamA->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

        //
        // Write a record
        //
        for (ULONG iWrites = 0; iWrites < WriteCount; iWrites++)
        {
            ULONG data = iWrites;
            PUCHAR dataWritten = (PUCHAR)&data;
            ULONG dataSizeWritten = sizeof(ULONG);
            version++;

            status = SetupAndWriteRecord(logStreamA,
                                         metadata,
                                         emptyIoBuffer,
                                         version,
                                         asn,
                                         dataWritten,
                                         dataSizeWritten,
                                         offsetToStreamBlockHeaderM);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            asn += dataSizeWritten;
        }

        logStreamA->StartClose(NULL,
                         closeStreamWaitSync.CloseCompletionCallback());

        status = closeStreamWaitSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;

        logContainer->StartClose(NULL,
                         closeContainerSync.CloseCompletionCallback());

        status = closeContainerSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        

        //
        // TODO: Verify that the stream has been completely destaged from
        // the shared log
        //

        QueryLogContainerUsage(diskId, logContainerId, totalSpaceAfter, freeSpaceAfter);

        if ((freeSpaceBefore > freeSpaceAfter) && (! firstTime))
        {
            //
            // Do not verify the first time through since there will be
            // a checkpoint record that will eat space that we do not
            // want to account for.
            //
            printf("Before %lld, %lld\n", totalSpaceBefore, freeSpaceBefore);
            printf("After %lld, %lld\n", totalSpaceAfter, freeSpaceAfter);
            VERIFY_IS_TRUE(FALSE);
        }
        firstTime = FALSE;
    }

    //
    // Cleanup the log
    //

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
#endif


class OpenCloseRaceAsync : public WorkerAsync
{
    K_FORCE_SHARED(OpenCloseRaceAsync);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out OpenCloseRaceAsync::SPtr& Context
        )
    {
        NTSTATUS status;
        OpenCloseRaceAsync::SPtr context;

        context = _new(AllocationTag, Allocator) OpenCloseRaceAsync();
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

        Context = context.RawPtr();

        return(STATUS_SUCCESS);
    }

private:
    //
    // Parameters
    //
    KAsyncEvent* _OpenEventAsync;
    KtlLogContainer::SPtr _LogContainer;
    KtlLogStreamId _LogStreamId;
    ULONG _LoopCount;
    ULONG _WriteCount;
    ULONGLONG* _Version;
    ULONGLONG* _Asn;

public:
    struct StartParameters
    {
        KAsyncEvent* OpenEventAsync;
        KtlLogContainer::SPtr LogContainer;
        KtlLogStreamId LogStreamId;
        ULONG LoopCount;
        ULONG WriteCount;
        ULONGLONG* Version;
        ULONGLONG* Asn;
    };


    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;

        _OpenEventAsync = startParameters->OpenEventAsync;
        _LogContainer = startParameters->LogContainer;
        _LogStreamId = startParameters->LogStreamId;
        _LoopCount = startParameters->LoopCount;
        _WriteCount = startParameters->WriteCount;
        _Asn = startParameters->Asn;
        _Version = startParameters->Version;

        Start(ParentAsyncContext, CallbackPtr);
    }

protected:

    VOID
    CloseStreamCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogStream& LogStream,
        __in NTSTATUS Status)
    {
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(LogStream);
        VERIFY_IS_TRUE(NT_SUCCESS(Status));
    }

    VOID
    FinalCloseStreamCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogStream& LogStream,
        __in NTSTATUS Status)
    {
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(LogStream);
        VERIFY_IS_TRUE(NT_SUCCESS(Status));
        FSMContinue(Status);
    }


    VOID FSMContinue(
        __in NTSTATUS Status
        ) override
    {
        switch (_State)
        {
            case Initial:
            {
                srand(GetTickCount());

                _CloseStreamCompletion.Bind(this, &OpenCloseRaceAsync::CloseStreamCompletion);
                _FinalCloseStreamCompletion.Bind(this, &OpenCloseRaceAsync::FinalCloseStreamCompletion);

                Status = _OpenEventAsync->CreateWaitContext(KTL_TAG_TEST,
                                                           *g_Allocator,
                                                           _WaitAsync);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }

                Status = _LogContainer->CreateAsyncOpenLogStreamContext(_OpenStreamAsync);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }

                Status = _LogContainer->CreateAsyncResolveAliasContext(_ResolveAliasContext);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }

                KString::SPtr alias;
                Status = KString::Create(alias, *g_Allocator, L"foo");
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }
                _Alias = KString::CSPtr(alias.RawPtr());

                _LoopCounter = 0;

                _State = WaitForOpen;
                // fall through
            }

            case WaitForOpen:
            {
WaitForOpen:
                _State = OpenStream;
                _WaitAsync->Reuse();
                _WaitAsync->StartWaitUntilSet(this, _Completion);
                break;
            }

            case OpenStream:
            {
                ULONG metadataSize;

                _WriteCounter = 0;
                _State = StartWriteRecords;

                _OpenStreamAsync->Reuse();
                _OpenStreamAsync->StartOpenLogStream(_LogStreamId,
                                                     &metadataSize,
                                                     _LogStream,
                                                     this,
                                                     _Completion);
                break;
            }

            case StartWriteRecords:
            {
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }

                Status = _LogStream->CreateAsyncWriteContext(_WriteContext);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }
                // fall through
            }

            case WriteRecords:
            {
                ULONG offsetToStreamBlockHeaderM = _LogStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

                if (_WriteCounter < _WriteCount)
                {
                    ULONG data = _WriteCounter;
                    PUCHAR dataWritten = (PUCHAR)&data;
                    ULONG dataSizeWritten = sizeof(ULONG);

                    (*_Version) = (*_Version) + 1;

                    PVOID metadataBuffer;
                    KIoBuffer::SPtr metadata;
                    KIoBuffer::SPtr emptyIoBuffer;
                    Status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                                     metadata,
                                                     metadataBuffer,
                                                     *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    Status = KIoBuffer::CreateEmpty(emptyIoBuffer, *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    Status = SetupRecord(_LogStream,
                                                 metadata,
                                                 emptyIoBuffer,
                                                 (*_Version),
                                                 (*_Asn),
                                                 dataWritten,
                                                 dataSizeWritten,
                                                 offsetToStreamBlockHeaderM,
                                                 TRUE,
                                                 NULL,
                                                 [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader){ UNREFERENCED_PARAMETER(streamBlockHeader);});

                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    ULONG metadataIoSize = KLogicalLogInformation::FixedMetadataSize;
                    _WriteContext->Reuse();
                    _WriteContext->StartWrite((*_Asn),
                                             (*_Version),
                                             metadataIoSize,
                                             metadata,
                                             emptyIoBuffer,
                                             0,    // Reservation
                                             this,    // ParentAsync
                                             _Completion);

                    (*_Asn) = (*_Asn) + dataSizeWritten;

                    _WriteCounter++;
                    return;
                } else {
                    _LoopCounter++;

                    _WriteContext = nullptr;
                    if (_LoopCounter >= _LoopCount)
                    {
                        _OpenEventAsync->SetEvent();
                        goto Completed;
                    }

                    ULONG r = rand() % 1;
                    if (r == 0)
                    {
                        //
                        // Let Open race ahead of this close
                        //
                        _OpenEventAsync->SetEvent();

                        _State = WaitToClose;
                        _ResolveAliasContext->Reuse();
                        _ResolveAliasContext->StartResolveAlias(_Alias, &_ResolvedLogStreamId, NULL, _Completion);

                        return;
                    } else {
                        //
                        // Start close before open
                        //
                        _LogStream->StartClose(NULL, _CloseStreamCompletion);
                        _OpenEventAsync->SetEvent();
                    }

                    goto WaitForOpen;
                }

                break;
            }

            case WaitToClose:
            {

                _LogStream->StartClose(NULL, _CloseStreamCompletion);

                goto WaitForOpen;
            }

            case Completed:
            {
Completed:
                _State = CloseCompleted;
                _LogStream->StartClose(NULL, _FinalCloseStreamCompletion);
                break;
            }

            case CloseCompleted:
            {
                Complete(STATUS_SUCCESS);
                return;             
            }

            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               KTraceFailedAsyncRequest(Status, this, _State, 0);
               VERIFY_IS_TRUE(FALSE);
               Complete(Status);
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
        _LogStream = nullptr;
        _WaitAsync = nullptr;
        _LogContainer = nullptr;
        _OpenStreamAsync = nullptr;
        _WriteContext = nullptr;
        _Alias = nullptr;
        _ResolveAliasContext = nullptr;
    }

private:
    enum  FSMState { Initial = 0, WaitForOpen = 1, OpenStream = 2, StartWriteRecords = 3, WriteRecords = 4, WaitToClose = 5, Completed = 6, CloseCompleted = 7 };
    FSMState _State;

    //
    // Internal
    //
    KtlLogStream::AsyncWriteContext::SPtr _WriteContext;
    KtlLogStream::CloseCompletionCallback _CloseStreamCompletion;
    KtlLogStream::CloseCompletionCallback _FinalCloseStreamCompletion;
    KtlLogContainer::AsyncOpenLogStreamContext::SPtr _OpenStreamAsync;
    KtlLogStream::SPtr _LogStream;
    KAsyncEvent::WaitContext::SPtr _WaitAsync;
    ULONG _WriteCounter;
    ULONG _LoopCounter;
    KtlLogContainer::AsyncResolveAliasContext::SPtr _ResolveAliasContext;
    KString::CSPtr _Alias;
    KtlLogStreamId _ResolvedLogStreamId;
};

OpenCloseRaceAsync::OpenCloseRaceAsync()
{
}

OpenCloseRaceAsync::~OpenCloseRaceAsync()
{
}


VOID
CloseOpenRaceTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;

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


    static const ULONG loopCount = 8;
    static const ULONG writeCount = 64;
    static const ULONG streamsCount = 1;
    static const ULONG contention = 1;

    KtlLogStreamId streamsId[streamsCount];


    KtlLogStream::SPtr logStream;
    KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG i = 0; i < streamsCount; i++)
    {
        ULONG metadataLength = 0x10000;
        KGuid logStreamGuid;
        KtlLogStreamId logStreamId;
        StreamCloseSynchronizer closeStreamWaitSync;
        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
        KBuffer::SPtr securityDescriptor = nullptr;

        streamsId[i] = logStreamId;

        createStreamAsync->Reuse();
        createStreamAsync->StartCreateLogStream(logStreamId,
                                                    logStreamType,
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
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        logStream->StartClose(NULL,
                         closeStreamWaitSync.CloseCompletionCallback());

        status = closeStreamWaitSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        logStream = nullptr;
    }

    KAsyncEvent openEventAsync(FALSE, TRUE);

    KSynchronizer syncs[streamsCount * contention];
    OpenCloseRaceAsync::SPtr openCloseAsync[streamsCount * contention];
    ULONGLONG asns[streamsCount];
    ULONGLONG versions[streamsCount];

    struct OpenCloseRaceAsync::StartParameters parameters;
    parameters.OpenEventAsync = &openEventAsync;
    parameters.LogContainer = logContainer;
    parameters.LoopCount = loopCount;
    parameters.WriteCount = writeCount;

    ULONG k = 0;
    for (ULONG i = 0; i < streamsCount; i++)
    {
        asns[i] = KtlLogAsn::Min().Get();
        versions[i] = 0;

        parameters.LogStreamId = streamsId[i];
        parameters.Asn = &asns[i];
        parameters.Version = &versions[i];

        for (ULONG j = 0; j < contention; j++)
        {
            status = OpenCloseRaceAsync::Create(*g_Allocator, KTL_TAG_TEST, openCloseAsync[k]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            openCloseAsync[k]->StartIt(&parameters, NULL, syncs[k]);
            k++;
        }
    }

    for (k = 0; k < streamsCount * contention; k++)
    {
        status = syncs[k].WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Cleanup the log
    //
    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;

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


////
//
// Async will write small records infrequently but more frequently that
// the periodic flush time to force the scenario where accelerated
// flushing needs to be invoked.
//
class AccelerateFlushAsync : public WorkerAsync
{
    K_FORCE_SHARED(AccelerateFlushAsync);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out AccelerateFlushAsync::SPtr& Context
        )
    {
        NTSTATUS status;
        AccelerateFlushAsync::SPtr context;

        context = _new(AllocationTag, Allocator) AccelerateFlushAsync();
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

        Context = context.RawPtr();

        return(STATUS_SUCCESS);
    }

private:
    //
    // Parameters
    //
    KtlLogStream::SPtr _LogStream;
    KBuffer::SPtr _Data;
    ULONG _WriteDelay;
    BOOLEAN* _AllDone;

public:
    struct StartParameters
    {
        KtlLogStream::SPtr LogStream;
        KBuffer::SPtr Data;
        ULONG WriteDelay;
        BOOLEAN* AllDone;
    };


    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;

        _LogStream = startParameters->LogStream;
        _Data = startParameters->Data;
        _WriteDelay = startParameters->WriteDelay;
        _AllDone = startParameters->AllDone;

        Start(ParentAsyncContext, CallbackPtr);
    }


private:
    enum  FSMState { Initial = 0, PerformWrite = 1, PerformDelay = 2, Completed = 3 };
    FSMState _State;
    
protected:
    VOID FSMContinue(
        __in NTSTATUS Status
        ) override
    {
        switch (_State)
        {
            case Initial:
            {
                _OffsetToStreamBlockHeaderM = _LogStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);
                _Asn = KtlLogAsn::Min().Get();
                _Version = 0;

                
                Status = KTimer::Create(_Timer, GetThisAllocator(), GetThisAllocationTag());
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }
                
                Status = _LogStream->CreateAsyncWriteContext(_WriteContext);
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }
                
                // fall through
            }

            case PerformWrite:
            {
                if (*_AllDone)
                {
                    Complete(STATUS_SUCCESS);
                    return;
                }

                _State = PerformDelay;
                _WriteContext->Reuse();

                PVOID metadataBuffer;
                KIoBuffer::SPtr metadata;
                KIoBuffer::SPtr emptyIoBuffer;
                ULONG dataSizeWritten = _Data->QuerySize();
                PUCHAR dataWritten = (PUCHAR)_Data->GetBuffer();
                Status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                                 metadata,
                                                 metadataBuffer,
                                                 GetThisAllocator());
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }

                Status = KIoBuffer::CreateEmpty(emptyIoBuffer, GetThisAllocator());
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }

                _Version++;
                Status = SetupRecord(_LogStream,
                                             metadata,
                                             emptyIoBuffer,
                                             _Version,
                                             _Asn,
                                             dataWritten,
                                             dataSizeWritten,
                                             _OffsetToStreamBlockHeaderM,
                                             TRUE,
                                             NULL,
                                             [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader){ UNREFERENCED_PARAMETER(streamBlockHeader);});

                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                ULONG metadataIoSize = KLogicalLogInformation::FixedMetadataSize;
                _WriteContext->Reuse();
                _WriteContext->StartWrite(_Asn,
                                         _Version,
                                         metadataIoSize,
                                         metadata,
                                         emptyIoBuffer,
                                         0,    // Reservation
                                         this,    // ParentAsync
                                         _Completion);

                _Asn = _Asn + dataSizeWritten;

                
                break;
            }

            case PerformDelay:
            {
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    VERIFY_IS_TRUE(FALSE);
                    Complete(Status);
                    return;
                }
                
                if (*_AllDone)
                {
                    Complete(STATUS_SUCCESS);
                    return;
                }

                _State = PerformWrite;
                _Timer->Reuse();
                _Timer->StartTimer(_WriteDelay, this, _Completion);
                
                break;
            }

            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               KTraceFailedAsyncRequest(Status, this, _State, 0);
               VERIFY_IS_TRUE(FALSE);
               Complete(Status);
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
        _LogStream = nullptr;
        _WriteContext = nullptr;
        _Timer = nullptr;
        _Data = nullptr;
    }   
    
private:
    //
    // Internal
    //
    KtlLogStream::AsyncWriteContext::SPtr _WriteContext;
    KTimer::SPtr _Timer;
    ULONG _OffsetToStreamBlockHeaderM;
    ULONGLONG _Asn;
    ULONGLONG _Version;
};

AccelerateFlushAsync::AccelerateFlushAsync()
{
}

AccelerateFlushAsync::~AccelerateFlushAsync()
{
}


VOID
AccelerateFlushTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    //
    // Configuration Parameters
    LONGLONG logSize = 64 * 1024 * 1024;
    ULONG writeSize = 64;
    ULONG writeDelay = 1 * 1000;
    static const ULONG streamsCount = 256;
    ULONG runTime = 300;

    
    NTSTATUS status;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    StreamCloseSynchronizer closeStreamWaitSync;
    ContainerCloseSynchronizer closeContainerSync;

    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    //
    // Create container and a stream within it
    //
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    KtlLogContainer::SPtr logContainer;
    KSynchronizer sync;

    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    createContainerAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                         0,            // Max Number Streams
                                         4 * 1024 * 1024,            // Max Record Size
                                         0,
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    KtlLogStream::SPtr streams[streamsCount];

    KtlLogStreamType logStreamType = KLogicalLogInformation::GetLogicalLogStreamType();
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;

    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    for (ULONG i = 0; i < streamsCount; i++)
    {
        ULONG metadataLength = 0x10000;
        KGuid logStreamGuid;
        KtlLogStreamId logStreamId;
        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
        KBuffer::SPtr securityDescriptor = nullptr;

        createStreamAsync->Reuse();
        createStreamAsync->StartCreateLogStream(logStreamId,
                                                    logStreamType,
                                                    nullptr,           // Alias
                                                    nullptr,
                                                    securityDescriptor,
                                                    metadataLength,
                                                    DEFAULT_STREAM_SIZE,
                                                    DEFAULT_MAX_RECORD_SIZE,
                                                    KtlLogManager::FlagSparseFile,
                                                    streams[i],
                                                    NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    KBuffer::SPtr data;
    BOOLEAN allDone = FALSE;
    KSynchronizer syncs[streamsCount];
    AccelerateFlushAsync::SPtr doWritesAsync[streamsCount];
    struct AccelerateFlushAsync::StartParameters parameters;
    
    status = KBuffer::Create(writeSize, data, *g_Allocator, KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    parameters.AllDone = &allDone;
    parameters.Data = data;
    parameters.WriteDelay = writeDelay;
    
    for (ULONG i = 0; i < streamsCount; i++)
    {
        parameters.LogStream = streams[i];

        status = AccelerateFlushAsync::Create(*g_Allocator, KTL_TAG_TEST, doWritesAsync[i]);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        doWritesAsync[i]->StartIt(&parameters, NULL, syncs[i]);
    }

    //
    // Poll for accelerated flush active/passive modes
    //

    KtlLogStream::AsyncIoctlContext::SPtr ioctl;

    status = streams[0]->CreateAsyncIoctlContext(ioctl);
    VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);
    
    
    ULONG activeCount = 0;
    ULONG passiveCount = 0;
    BOOLEAN isActiveMode = FALSE;
    struct KLogicalLogInformation::AcceleratedFlushMode* flushMode;
    KBuffer::SPtr inBuffer, outBuffer;
    ULONG result;
        
    inBuffer = nullptr;
    for (ULONG i = 0; i < runTime; i++)
    {
        KNt::Sleep(1000);
        ioctl->Reuse();
        ioctl->StartIoctl(KLogicalLogInformation::QueryAcceleratedFlushMode, inBuffer.RawPtr(), result, outBuffer, NULL, sync);
        status = sync.WaitForCompletion();

        VERIFY_IS_TRUE(NT_SUCCESS(status) ? true : false);

        flushMode = (KLogicalLogInformation::AcceleratedFlushMode*)outBuffer->GetBuffer();

        if (flushMode->IsAccelerateInActiveMode)
        {
            if (! isActiveMode)
            {
                isActiveMode = TRUE;
                activeCount++;
                printf("Enter active mode %d active %d passive\n", activeCount, passiveCount);
            }
        } else {
            if (isActiveMode)
            {
                isActiveMode = FALSE;
                passiveCount++;
                printf("Enter passive mode %d active %d passive\n", activeCount, passiveCount);
            }
        }    
    }
    ioctl = nullptr;

    VERIFY_IS_TRUE(activeCount > 0);
    VERIFY_IS_TRUE(passiveCount > 0);
    
    printf("Shutting down\n");
    allDone = TRUE;
    
    for (ULONG i = 0; i < streamsCount; i++)
    {
        status = syncs[i].WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    //
    // Cleanup
    //
    for (ULONG i = 0; i < streamsCount; i++)
    {
        streams[i]->StartClose(NULL,
                         closeStreamWaitSync.CloseCompletionCallback());

        status = closeStreamWaitSync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        streams[i] = nullptr;
    }
    
    //
    // Cleanup the log
    //
    logContainer->StartClose(NULL,
                     closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    logContainer = nullptr;

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
////


VOID SetupRawKtlLoggerTests(
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

    System->SetStrictAllocationChecks(TRUE);

    StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    KDbgMirrorToDebugger = TRUE;
    KDbgMirrorToDebugger = 1;
    
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

#if defined(UDRIVER) 
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#endif
    
#if defined(KDRIVER) || defined(UPASSTHROUGH) || defined(DAEMON)
    // For kernel, we assume "kernel" side already configured
#endif

    DeleteAllContainersOnDisk(DiskId);
}

VOID CleanupRawKtlLoggerTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    UNREFERENCED_PARAMETER(DiskId);
    UNREFERENCED_PARAMETER(System);
    UNREFERENCED_PARAMETER(StartingAllocs);

#if defined(UDRIVER) 
    //
    // For UDRIVER need to perform work done in PNP RemoveDevice
    //
    NTSTATUS status;

    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    KInvariant(NT_SUCCESS(status));
#endif
#if defined(KDRIVER) || defined(UPASSTHROUGH) || defined(DAEMON)
    // For kernel, we assume driver already installed by InstallForCITs
#endif

    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}
