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

#include "records.h"

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

BOOLEAN DumpAllData = FALSE;

// TODO: Separate into .h and .c


const LONGLONG DefaultTestLogFileSize = (LONGLONG)1024 * 1024 * 1024;
const LONGLONG DEFAULT_STREAM_SIZE = (LONGLONG)256 * 1024 * 1024;
const ULONG DEFAULT_MAX_RECORD_SIZE = (ULONG)16 * 1024 * 1024;

using namespace LogRecords;

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

VOID SetupRawCoreLoggerTests(
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

VOID DumpStreamInfo(
    RvdLog::SPtr logContainer,
    RvdLogStreamId streamId
    )
{
    NTSTATUS status;
    KSynchronizer sync;

#if 0
    if (streamId == RvdDiskLogConstants::CheckpointStreamId())
    {
        printf("        <Checkpoint Stream>\n");
        printf("\n");
        return;
    }
#endif

    RvdLog::AsyncOpenLogStreamContext::SPtr openStream;
    RvdLogStream::SPtr logStream;

    status = logContainer->CreateAsyncOpenLogStreamContext(openStream);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncOpenLogStreamContext");

    openStream->StartOpenLogStream(streamId, logStream, NULL, sync);
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        RvdLogUtility::AsciiGuid g(streamId.Get());
        printf("        Error 0x%x opening stream %s\n", status, g.Get());
        printf("\n");
        return;
    }

    RvdLogUtility::AsciiGuid g;
    RvdLogStreamType logStreamType;
    RvdLogStreamId logStreamId;

    logStream->QueryLogStreamType(logStreamType);   
    logStream->QueryLogStreamId(logStreamId);

    g.Set(logStreamId.Get());
    printf("        LogStreamId:   %s\n", g.Get());
    g.Set(logStreamType.Get());
    printf("        LogStreamType: %s\n", g.Get());
    printf("\n");

    ULONGLONG currentReservation = logStream->QueryCurrentReservation();
    LONGLONG logStreamUsage = logStream->QueryLogStreamUsage();
    printf("        LogStreamUsage:     0x%llx\n", logStreamUsage);
    printf("        CurrentReservation: 0x%llx\n", currentReservation);

    RvdLogAsn lowestAsn;
    RvdLogAsn highestAsn;
    RvdLogAsn truncationAsn;
    status = logStream->QueryRecordRange(&lowestAsn, &highestAsn, &truncationAsn);
    if (NT_SUCCESS(status))
    {
        printf("        Low ASN:            %llu 0x%llx\n", lowestAsn.Get(), lowestAsn.Get());
        printf("        High ASN:           %llu 0x%llx\n", highestAsn.Get(), highestAsn.Get());
        printf("        Truncation ASN:     %llu 0x%llx\n", truncationAsn.Get(), truncationAsn.Get());
    } else {
        printf("        QueryRecordRange:   0x%x\n", status);
    }
    printf("\n");


    ULONGLONG totalOnDiskSize = 0;             // How much disk space used by records
    ULONGLONG totalLogicalLogSize = 0;         // How much logical log space used
    ULONGLONG totalWastedSize = 0;             // How much unused buffers

    ULONGLONG minimumRecordSize = 0xffffffff;
    ULONGLONG maximumRecordSize = 0;

    ULONG number4KRecords = 0;
    ULONGLONG totalLogicalLogSizeOf4KRecords = 0;

    KArray<RvdLogStream::RecordMetadata> records(*g_Allocator);
    logStream->QueryRecords(lowestAsn, highestAsn, records);
    for (ULONG i = 0; i < records.Count(); i++)
    {
        ULONGLONG wastedSpace = 0;
        ULONGLONG diskSpaceUsed = 0;
        ULONGLONG logicalLogSpaceUsed = 0;

        if (i != (records.Count()-1))
        {
            diskSpaceUsed = records[i].Size;
            logicalLogSpaceUsed = records[i + 1].Asn.Get() - records[i].Asn.Get();

            if (diskSpaceUsed < logicalLogSpaceUsed)
            {
                printf("***** diskSpaceUsed %lld (%llx) < logicalLogSpaceUsed %lld (%llx)\n", diskSpaceUsed, diskSpaceUsed, logicalLogSpaceUsed, logicalLogSpaceUsed);
            }
            wastedSpace = diskSpaceUsed - logicalLogSpaceUsed;
        }

        if (diskSpaceUsed > maximumRecordSize)
        {
            maximumRecordSize = diskSpaceUsed;
        }

        if ((diskSpaceUsed != 0) && (diskSpaceUsed < minimumRecordSize))
        {
            minimumRecordSize = diskSpaceUsed;
        }

        totalOnDiskSize += diskSpaceUsed;
        totalLogicalLogSize += logicalLogSpaceUsed;
        totalWastedSize += wastedSpace;

        if (records[i].Size == 4096)
        {
            number4KRecords++;
            totalLogicalLogSizeOf4KRecords += logicalLogSpaceUsed;
        }

        printf("        Record %llu 0x%llx, Version %llu 0x%llx, Size %d 0x%x, Unused %lld 0x%llx, Disposition %2d, Debug_LSN 0x%llx\n",
            records[i].Asn.Get(),
            records[i].Asn.Get(),
            records[i].Version,
            records[i].Version,
            records[i].Size,
            records[i].Size,
            wastedSpace,
            wastedSpace,
            records[i].Disposition,
            records[i].Debug_LsnValue);
    }

    printf("\n");
    printf("        TotalOnDiskSpace:     %lld %llx\n", totalOnDiskSize, totalOnDiskSize);
    printf("        TotalLogicalLogSpace: %lld %llx\n", totalLogicalLogSize, totalLogicalLogSize);
    printf("        UnusedSpace:          %lld %llx\n", totalWastedSize, totalWastedSize);

    float percentUnusedSpace = ((float)totalWastedSize / (float)totalOnDiskSize) * 100;
    printf("        %% UnusedSpace       : %f\n", percentUnusedSpace);

    float percentUsedSpace = ((float)totalLogicalLogSize / (float)totalOnDiskSize) * 100;
    printf("        %% UsedSpace         : %f\n", percentUsedSpace);

    printf("\n");
    printf("        Total Number of records:            %d\n", records.Count());
    printf("        Number of 4K records:               %d\n", number4KRecords);

    if (records.Count() != 0)
    {
        float percent4KRecords = ((float)number4KRecords / (float)records.Count()) * 100;
        printf("        %% of records are 4K:                %f\n", percent4KRecords);
    }
    printf("        Total size of 4K records:           %lld 0x%llx\n", (ULONGLONG)(number4KRecords * 4096), (ULONGLONG)(number4KRecords * 4096));
    printf("        LogicalLogSpace used in 4K records: %lld 0x%llx\n", totalLogicalLogSizeOf4KRecords, totalLogicalLogSizeOf4KRecords);

    if (number4KRecords != 0)
    {
        float percentUsedIn4K = ((float)totalLogicalLogSizeOf4KRecords / (float)(number4KRecords * 4096)) * 100;
        printf("        %% used in 4K records:               %f\n", percentUsedIn4K);
    }

    printf("\n");
    printf("        Minimum Record Size:      0x%llx %lld\n", minimumRecordSize, minimumRecordSize);
    printf("        Maximum Record Size:      0x%llx %lld\n", maximumRecordSize, maximumRecordSize);
    if (records.Count() != 0)
    {
        float averageRecordSize = ((float)totalOnDiskSize / (float)records.Count());
        printf("        Average Record Size:      %f\n", averageRecordSize);
        float averageWastedRecordSize = ((float)totalWastedSize / (float)records.Count());
        printf("        Average Unused Size:      %f\n", averageWastedRecordSize);
    }
}

VOID DumpCoreContainerInfo(
    LPCWSTR ContainerPath,
    ULONG& NumberStreams
    )
{
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;

    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "RvdLogManager::Create");

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status), "logManager->Activate");

    status = logManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                      &KtlLogVerify::VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "logManager->RegisterVerificationCallback");

    status = logManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                      &KtlLogVerify::VerifyRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "logManager->RegisterVerificationCallback");

    {
        RvdLog::SPtr logContainer;
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncOpenLogContext");

        openLogOp->StartOpenLog(KStringView(ContainerPath), logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
#if !defined(PLATFORM_UNIX)
            printf("Failed to open log container %ws 0x%x\n", ContainerPath, status);
#else
            printf("Failed to open log container %s 0x%x\n", Utf16To8(ContainerPath).c_str(), status);
#endif
            ExitProcess(static_cast<UINT>(-1));
        }

        KGuid diskId;
        RvdLogId logId;
        logContainer->QueryLogId(diskId, logId);
        RvdLogUtility::AsciiGuid g(diskId);
        printf("DiskId: %s\n", g.Get());
        g.Set(logId.Get());
        printf("LogId:  %s\n", g.Get());
        printf("\n");

        ULONGLONG totalSpace;
        ULONGLONG freeSpace;
        logContainer->QuerySpaceInformation(&totalSpace,
                                            &freeSpace);

        ULONGLONG currentReservation;
        currentReservation = logContainer->QueryCurrentReservation();
        printf("TotalSpace:         0x%llx %lld\n", totalSpace, totalSpace);
        printf("FreeSpace :         0x%llx %lld\n", freeSpace, freeSpace);

        float freeSpacePercent = (((float)freeSpace / (float)totalSpace)) * 100;
        printf("%% Free    :         %f\n", freeSpacePercent);

        printf("CurrentReservation: 0x%llx\n", currentReservation);
        printf("\n");

        ULONG maxAllowedStreams;
        maxAllowedStreams = logContainer->QueryMaxAllowedStreams();
        printf("MaxAlowedStreams: %u\n", maxAllowedStreams);
        printf("\n");
        NumberStreams = maxAllowedStreams;

        ULONG maxRecordSize;
        maxRecordSize = logContainer->QueryMaxRecordSize();
        ULONG maxUserRecordSize;
        maxUserRecordSize = logContainer->QueryMaxUserRecordSize();
        ULONG userRecordSystemMetadataOverhead;
        userRecordSystemMetadataOverhead = logContainer->QueryUserRecordSystemMetadataOverhead();

        printf("MaxRecordSize:                    0x%x\n", maxRecordSize);
        printf("MaxUserRecordSize:                0x%x\n", maxUserRecordSize);
        printf("UserRecordSystemMetadataOverhead: 0x%x\n", userRecordSystemMetadataOverhead);
        printf("\n");

        LONGLONG readCacheSizeLimit;
        LONGLONG readCacheUsage;
        logContainer->QueryCacheSize(&readCacheSizeLimit, &readCacheUsage);
        BOOLEAN isLogFlushed;
        isLogFlushed = logContainer->IsLogFlushed();

        printf("ReadCacheSizeLimit: %lld\n", readCacheSizeLimit);
        printf("ReadCacheUsage:     %lld\n", readCacheUsage);
        printf("IsLogFlushed:       %s\n", isLogFlushed ? "True" : "False");
        printf("\n");

        ULONG numberOfStreams;
        numberOfStreams = logContainer->GetNumberOfStreams();
        printf("NumberOfStreams: %u\n", numberOfStreams);

        #define ALLOCATION_TAG 'LLKT'

        HRESULT hr;
        ULONG sizeNeeded;
        hr = ULongMult(numberOfStreams, sizeof(RvdLogStreamId), &sizeNeeded);
        KInvariant(SUCCEEDED(hr));
        KBuffer::SPtr buffer;
        status = KBuffer::Create(sizeNeeded, buffer, *g_Allocator, ALLOCATION_TAG);

        RvdLogStreamId* streamIds = (RvdLogStreamId*)buffer->GetBuffer();
        ULONG numberEntries;
        numberEntries = logContainer->GetStreams(numberOfStreams, streamIds);

        for (ULONG i = 0; i < numberEntries; i++)
        {
            BOOLEAN isValid;
            isValid = logContainer->IsStreamIdValid(streamIds[i]);

            BOOLEAN isOpen;
            BOOLEAN isClosed;
            BOOLEAN isDeleted;
            BOOLEAN result;
            result = logContainer->GetStreamState(streamIds[i], &isOpen, &isClosed, &isDeleted);
            if (result)
            {
                g.Set(streamIds[i].Get());
                printf("    %s  isStreamIdValid %s  isOpen %s  isClosed %s  isDeleted %s\n",
                    g.Get(),
                    isValid ? "True" : "False",
                    isOpen ? "True" : "False",
                    isClosed ? "True" : "False",
                    isDeleted ? "True" : "False");
            } else {
                g.Set(streamIds[i].Get());
                printf("    %s  isStreamIdValid %s  isOpen %s  isClosed %s  isDeleted %s\n",
                    g.Get(),
                    isValid ? "True" : "False",
                    "<Unknown>",
                    "<Unknown>",
                    "<Unknown>");
            }

            DumpStreamInfo(logContainer, streamIds[i]);
        }

        //
        // All done, cleanup
        //
        logContainer.Reset();
    }


    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();
}

NTSTATUS DumpEachEntryEnumCallback(
    __in SharedLCMBInfoAccess::LCEntryEnumAsync& Async
    )
{
    GUID nullGUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
    SharedLCMBInfoAccess::SharedLCEntry& entry = Async.GetEntry();
    RvdLogUtility::AsciiGuid g;
    
    if (! IsEqualGUID(entry.LogStreamId.Get(), nullGUID))
    {
        PCHAR dispositionFlagsText[] = {
            "FlagDeleted",
            "FlagCreating",
            "FlagCreated",
            "FlagDeleting"
        };
        PCHAR flags = "Unknown";

        if (entry.Flags > SharedLCMBInfoAccess::DispositionFlags::FlagDeleting)
        {
            flags = "Unknown";
        } else {
            if ((entry.Flags >= 0) && (entry.Flags <= 3))
            {
                flags = dispositionFlagsText[entry.Flags];
            }
        }

        g.Set(entry.LogStreamId.Get());     
        printf("    %d:  %s\n",
            entry.Index,
            g.Get());

        printf("        Flags: %s(%d)  MaxLLMetadataSize: 0x%x  MaxRecordSize:0x%x  StreamSize: 0x%llx\n",
            flags,
            entry.Flags,
            entry.MaxLLMetadataSize,
            entry.MaxRecordSize,
            entry.StreamSize
            );

#if !defined(PLATFORM_UNIX)
        printf("        Alias: %ws\n", entry.AliasName);
        printf("        PathToDedicated: %ws\n", entry.PathToDedicatedContainer);
#else
        printf("        Alias: %s\n", Utf16To8(entry.AliasName).c_str());
        printf("        PathToDedicated: %s\n", Utf16To8(entry.PathToDedicatedContainer).c_str());
#endif
        printf("\n");
    }

    return(STATUS_SUCCESS);
}


void DumpSharedContainerMBInfo(
    LPCWSTR ContainerPath,
    ULONG NumberStreams
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    SharedLCMBInfoAccess::SPtr slcmb;
    KGuid diskId;
    KtlLogContainerId logContainerId;
    KString::SPtr containerPath = KString::Create(ContainerPath, *g_Allocator);

    VERIFY_IS_TRUE(containerPath, "KString::Create(ContainerPath)");

    status = SharedLCMBInfoAccess::Create(slcmb,
                                          diskId,
                                          containerPath.RawPtr(),
                                          logContainerId,
                                          NumberStreams,
                                          *g_Allocator,
                                          ALLOCATION_TAG);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SharedLCMBInfoAccess::Create");

    SharedLCMBInfoAccess::LCEntryEnumCallback dumpEachEntryEnumCallback(&DumpEachEntryEnumCallback);
    slcmb->StartOpenMetadataBlock(dumpEachEntryEnumCallback,
                                    nullptr,
                                    sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SharedLCMBInfoAccess::StartOpenMetadataBlock");

    // Open Metadata block invokes callbacks that dump
    printf("\n");

    slcmb->Reuse();
    slcmb->StartCloseMetadataBlock(nullptr,
                                    sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SharedLCMBInfoAccess::CloseMetadataBlock");
}


void DumpAllContainerInfo(
    LPCWSTR ContainerPath
    )
{
    ULONG numberStreams;
    KtlLogContainerId containerId;

    DumpCoreContainerInfo(ContainerPath, numberStreams);
    DumpSharedContainerMBInfo(ContainerPath, numberStreams);
}

void ValidateMBInfoSection(
    PVOID SectionPointer,
    KIoBuffer& SectionIoBuffer,
    BOOLEAN  IgnoreChecksum,
    BOOLEAN  IgnoreSectionSize,
    ULONG SectionSize,
    ULONGLONG FileOffset,
    BOOLEAN& IsValidSignature,
    BOOLEAN& IsValidFileOffset,
    BOOLEAN& IsValidVersion,
    BOOLEAN& IsValidSectionSize,
    BOOLEAN& IsValidChecksum                           
    )
{
    KIoBuffer::SPtr sectionIoBuffer = &SectionIoBuffer;
    MBInfoAccess::MBInfoHeader* header = static_cast<MBInfoAccess::MBInfoHeader*>(SectionPointer);
    
    IsValidSignature = (header->Signature == MBInfoAccess::MBInfoHeader::Sig);
    IsValidFileOffset = (header->FileOffset == FileOffset);
    IsValidVersion = (header->Version == MBInfoAccess::MBInfoHeader::CurrentVersion);

    if (! IgnoreSectionSize)
    {
        IsValidSectionSize = (header->SectionSize == SectionSize);
    } else {
        IsValidSectionSize = TRUE;
    }

    if (! IgnoreChecksum)
    {
        ULONGLONG headerChecksum = header->BlockChecksum;
        KFinally([&] { header->BlockChecksum = headerChecksum; });

        header->BlockChecksum = 0;
        ULONGLONG checksum = 0;
        KtlLogVerify::ComputeCrc64ForIoBuffer(sectionIoBuffer,
                                              header->ChecksumSize,
                                              checksum);
        IsValidChecksum = (checksum == headerChecksum);
    } else {
        IsValidChecksum = TRUE;
    }
}

void ValidateAndDisplaySection(
    PVOID SectionPointer,
    KIoBuffer& SectionIoBuffer,
    ULONGLONG FileOffset,
    ULONG SectionSize
    )
{
    BOOLEAN isValidSignature;
    BOOLEAN isValidFileOffset;
    BOOLEAN isValidVersion;
    BOOLEAN isValidSectionSize;
    BOOLEAN isValidChecksum;

    MBInfoAccess::MBInfoHeader* header = static_cast<MBInfoAccess::MBInfoHeader*>(SectionPointer);
    
    ValidateMBInfoSection(SectionPointer,
                          SectionIoBuffer,
                          FALSE,
                          FALSE,
                          SectionSize,
                          FileOffset,
                          isValidSignature,
                          isValidFileOffset,
                          isValidVersion,
                          isValidSectionSize,
                          isValidChecksum);
    
    printf("    Signature %llx %s valid\n",
                header->Signature,
                isValidSignature ? "Is" : "IS NOT");
    printf("    File Offset %lld (0x%llx) %s valid\n",
                header->FileOffset, header->FileOffset,
                isValidFileOffset ? "Is" : "IS NOT");
           
    printf("    Generation %lld (0x%llx)\n", header->Generation, header->Generation);

    printf("    BlockChecksum %llx %s valid\n",
                header->BlockChecksum,
                isValidChecksum ? "Is" : "IS NOT");

    printf("    SectionSize %d (0x%x) %s valid\n",
                header->SectionSize, header->SectionSize,
                isValidSectionSize ? "Is" : "IS NOT");
    printf("    Version %d (0x%x) %s valid\n",
                header->Version, header->Version,
                isValidVersion ? "Is" : "IS NOT");
    printf("    ChecksumSize %d (0x%x)\n", header->ChecksumSize, header->ChecksumSize);

    SharedLCMBInfoAccess::SharedLCSection* sharedLCSection = (SharedLCMBInfoAccess::SharedLCSection*)header;
    printf("    SectionIndex: %d\n", sharedLCSection->SectionIndex);
    printf("    TotalNumberSections: %d\n", sharedLCSection->TotalNumberSections);
    printf("    TotalMaxNumberEntries: %d\n", sharedLCSection->TotalMaxNumberEntries);
    
    PUCHAR p = (PUCHAR)header + FIELD_OFFSET(SharedLCMBInfoAccess::SharedLCSection, Entries);
    
    PCHAR dispositionFlagsText[] = {
        "FlagDeleted",
        "FlagCreating",
        "FlagCreated",
        "FlagDeleting"
    };
    
    for (ULONG i = 0; i < SharedLCMBInfoAccess::_EntriesPerSection; i++)
    {
        SharedLCMBInfoAccess::SharedLCEntry* entry;
        PCHAR flags = "Invalid";

        entry = (SharedLCMBInfoAccess::SharedLCEntry*)(p);

        printf("    Entry %d:\n", i);
        RvdLogUtility::AsciiGuid g(entry->LogStreamId.Get());
        printf("        LogStreamId: %s\n", g.Get());
        printf("        Index: %d\n", entry->Index);
        printf("        MaxLLMetadataSize: %d\n", entry->MaxLLMetadataSize);
        printf("        MaxRecordSize: %d (0x%x)\n", entry->MaxRecordSize, entry->MaxRecordSize);
        printf("        StreamSize: %lld (0x%llx)\n", entry->StreamSize, entry->StreamSize);

        if ((entry->Flags >= 0) && (entry->Flags <= 3))
        {
            flags = dispositionFlagsText[entry->Flags];
        }
        printf("        Disposition: %s\n", flags);

#if !defined(PLATFORM_UNIX)
        printf("        AliasName: %ws\n", entry->AliasName);
        printf("        PathToDedicatedContainer: %ws\n", entry->PathToDedicatedContainer);
#else
        printf("        AliasName: %s\n", Utf16To8(entry->AliasName).c_str());
        printf("        PathToDedicatedContainer: %s\n", Utf16To8(entry->PathToDedicatedContainer).c_str());
#endif
        p = p + sizeof(SharedLCMBInfoAccess::SharedLCEntry);
    }
    printf("\n");
}

void DumpSharedContainerMBInfoRaw(
    LPCWSTR ContainerPath
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KBlockFile::SPtr file;
    KWString fileName(*g_Allocator);    

#if !defined(PLATFORM_UNIX)
    fileName = L"\\??\\";
#else
    fileName = L"";
#endif
    
    fileName += ContainerPath;
    VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()), "Open raw MBInfo");

    ULONG createOptions = static_cast<ULONG>(KBlockFile::eReadOnly) |
                          static_cast<ULONG>(KBlockFile::eShareRead) |
                          static_cast<ULONG>(KBlockFile::eShareWrite);
    
    status = KBlockFile::Create(fileName,
                                FALSE,
                                KBlockFile::eOpenExisting,
                                static_cast<KBlockFile::CreateOptions>(createOptions),
                                file,
                                sync,
                                NULL,
                                *g_Allocator,
                                KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");

    static const ULONG fourKB = 4 * 1024;
    MBInfoAccess::MBInfoHeader* header;
    KIoBuffer::SPtr headerIoBuffer;
    PVOID p;
    ULONGLONG fileSize;
    ULONG sectionSize;
    ULONG sectionCount;

    status = KIoBuffer::CreateSimple(fourKB, headerIoBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");

    status = file->Transfer(KBlockFile::IoPriority::eForeground,
                            KBlockFile::eRead,
                            0,
                            *headerIoBuffer,
                            sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");

    fileSize = file->QueryFileSize();
    
    //
    // Validate and gather info from header
    //
    header = static_cast<MBInfoAccess::MBInfoHeader*>(p);

    BOOLEAN isValidSignature;
    BOOLEAN isValidFileOffset;
    BOOLEAN isValidVersion;
    BOOLEAN isValidSectionSize;
    BOOLEAN isValidChecksum;

    ValidateMBInfoSection(p,
                          *headerIoBuffer,
                          TRUE,
                          TRUE,
                          0,
                          0,
                          isValidSignature,
                          isValidFileOffset,
                          isValidVersion,
                          isValidSectionSize,
                          isValidChecksum);
    if (isValidSignature && isValidFileOffset && isValidVersion)
    {
        //
        // This may be close enough to assume the section size
        //
        sectionSize = header->SectionSize;
        sectionCount = static_cast<ULONG>(fileSize / static_cast<ULONGLONG>(sectionSize));
        if ((sectionCount * sectionSize) != fileSize)
        {
            printf("LCMetadata appears to be corrupted\n");
            return;
        }
    } else {
        printf("LCMetadata appears to be corrupted\n");
        return;
    }

    headerIoBuffer = nullptr;
    
    KIoBuffer::SPtr sectionIoBuffer;
    status = KIoBuffer::CreateSimple(sectionSize, sectionIoBuffer, p, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");
    
    ULONGLONG fileOffset;
    for (ULONG i = 0; i < sectionCount; i++)
    {
        fileOffset = i * sectionSize;
        status = file->Transfer(KBlockFile::IoPriority::eForeground,
                                KBlockFile::eRead,
                                fileOffset,
                                *sectionIoBuffer,
                                sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Open raw MBInfo");

        ValidateAndDisplaySection(p,
                                  *sectionIoBuffer,
                                  fileOffset,
                                  sectionSize);
    }
}

void DumpBuffer(
    PVOID Buffer,
    ULONG Length
    )
{
    PUCHAR b = (PUCHAR)Buffer;
    ULONG i = 0;

    if (! DumpAllData)
    {
        if (Length > 256)
        {
            Length = 256;
        }
    }

    while (i < Length)
    {
        if ((i % 0x10) == 0)
        {
            printf("\n    %08x0:", i/0x10);
        }
        printf(" %02x", b[i]);
        i++;
    }
    printf("\n");
}

void DumpHeaderAndStreamData(
    KBuffer::SPtr MetadataBuffer,
    KIoBuffer::SPtr IoBuffer,
    ULONG CoreLoggerOverhead,
    KtlLogStreamId ExpectedStreamId
    )
{
    NTSTATUS status;
    KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
    KLogicalLogInformation::StreamBlockHeader* streamHeader;
    ULONG dataSize;
    ULONG dataOffset;

    status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(MetadataBuffer,
                                                                               CoreLoggerOverhead,
                                                                               *IoBuffer,
                                                                               sizeof(KtlLogVerify::KtlMetadataHeader),
                                                                               metadataHeader,
                                                                               streamHeader,
                                                                               dataSize,
                                                                               dataOffset);
    if (!NT_SUCCESS(status))
    {
        printf("KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset failed 0x%x\n", status);
        return;
    }

    printf("    Metadata Header\n");
    printf("      Offset to stream: 0x%x\n", metadataHeader->OffsetToStreamHeader);
    printf("      RecordMarkerOffsetPlusOne: %d  (0x%x)",
        metadataHeader->RecordMarkerOffsetPlusOne, metadataHeader->RecordMarkerOffsetPlusOne);

    printf("    \n");

    RvdLogUtility::AsciiGuid g(streamHeader->StreamId.Get());
    printf("    StreamHeader:\n");
    printf("      Signature %llx %s\n", streamHeader->Signature, streamHeader->Signature == streamHeader->Sig ? "is correct" : "is not correct");
    printf("      StreamId %s %s\n", g.Get(), (streamHeader->StreamId == ExpectedStreamId) ? "is correct" : "is not correct");
    printf("      StreamOffset %lld (0x%llx)\n", streamHeader->StreamOffset, streamHeader->StreamOffset);
    printf("      HighestOperationId %lld (0x%llx)\n", streamHeader->HighestOperationId, streamHeader->HighestOperationId);
    printf("      HeadTruncationPoint %lld (0x%llx)\n", streamHeader->HeadTruncationPoint, streamHeader->HeadTruncationPoint);
    printf("      Datasize %d (0x%x)\n", streamHeader->DataSize, streamHeader->DataSize);
    printf("      Reserved %d (0x%x)\n", streamHeader->Reserved, streamHeader->Reserved);

#if 0
    // TODO: Fix CRCs
    ULONGLONG ComputedHeaderCRC64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(streamHeader);
    printf("      HeaderCRC %lld (0x%llx) %s\n", streamHeader->HeaderCRC64, streamHeader->HeaderCRC64, (streamHeader->HeaderCRC64 == ComputedHeaderCRC64) ? "is correct" : "is not correct");

    ULONGLONG ComputedDataCRC64 = KLogicalLogInformation::ComputeDataBlockCRC(streamHeader, *IoBuffer, dataOffset-CoreLoggerOverhead);
    printf("      DataCRC %ld (0x%llx) %s\n", streamHeader->DataCRC64, streamHeader->DataCRC64, (streamHeader->DataCRC64 == ComputedDataCRC64) ? "is correct" : "is not correct");
#endif

    PUCHAR dataPtr = (PUCHAR)(MetadataBuffer->GetBuffer()) + dataOffset - CoreLoggerOverhead;
    ULONG metadataToDump = KLogicalLogInformation::FixedMetadataSize - dataOffset;
    if (metadataToDump > dataSize)
    {
        metadataToDump = dataSize;
    }

    PUCHAR metadataBuffer = (PUCHAR)MetadataBuffer->GetBuffer();
    ULONG metadataBufferLength = KLogicalLogInformation::FixedMetadataSize;
    status = KtlLogVerify::VerifyRecordCallback(metadataBuffer,
                                                metadataBufferLength,
                                                IoBuffer);
    if (! NT_SUCCESS(status))
    {
        printf("    ****** Record is not verified correct\n");
    }

    printf("    Stream Data (in metadata) %d 0x%x bytes\n", metadataToDump, metadataToDump);
    DumpBuffer(dataPtr, metadataToDump);

    if (IoBuffer->QuerySize() > 0)
    {
        dataPtr = (PUCHAR)IoBuffer->First()->GetBuffer();
        ULONG iodataToDump = dataSize - metadataToDump;
        printf("    Stream Data (in IoBuffer) %d 0x%x bytes\n", iodataToDump, iodataToDump);
        DumpBuffer(dataPtr, iodataToDump);
    }
    else
    {
        printf("    No data in IoBuffer\n");
    }
}

#define IsReplicatorPhysicalRecord(r) (((r)->RecordType >= BeginCheckpoint) && ((r)->RecordType <= CompleteCheckpoint))
#define IsValidReplicatorRecordType(r) (((r)->RecordType >= BeginTransaction) && ((r)->RecordType <= CompleteCheckpoint))

void DumpReplicatorPhysicalRecord(
    PREPLICATOR_PHYSICAL_RECORD Record
    )
{
    printf("    LinkedPhysicalRecordOffset: %lld\t0x%llx\n", Record->LinkedPhysicalRecordOffset, Record->LinkedPhysicalRecordOffset);
}

void DumpReplicatorTransactionRecord(
    PREPLICATOR_TRANSACTION_RECORD Record
    )
{
    KString::SPtr s = KString::Create(*g_Allocator, MAX_PATH);

    printf("    TransactionId:           %lld\t0x%llx\n", Record->TransactionId, Record->TransactionId);
    printf("    ParentTransactionOffset: %lld\t0x%llx\n", Record->ParentTransactionOffset, Record->ParentTransactionOffset);
}

void DumpReplicatorLogicalRecord(
    PREPLICATOR_LOGICAL_RECORD Record
    )
{
    UNREFERENCED_PARAMETER(Record);
}

void DumpReplicatorBeginTransactionRecord(
    PREPLICATOR_BEGINTRANSACTION Record
    )
{
    DumpReplicatorLogicalRecord(&Record->Logical);
    DumpReplicatorTransactionRecord(&Record->Transaction);

    printf("    IsSingleOperationTxn:   %s\n", (Record->IsSingleOperationTransaction != 0) ? "TRUE" : "FALSE");
    printf("    #Segments (MOpData):    %d\n", Record->NumberOfArraySegmentsInMetadataOperationData);
    // TODO: Dump the rest of the data.
}

void DumpReplicatorOperationRecord(
    PREPLICATOR_OPERATION Record
    )
{
    DumpReplicatorLogicalRecord(&Record->Logical);
    DumpReplicatorTransactionRecord(&Record->Transaction);
    printf("    IsRedoOnly:             %s\n", (Record->IsRedoOnly != 0) ? "TRUE" : "FALSE");
    // TODO: Dump redo and undo bytes
}

void DumpReplicatorEndTransactionRecord(
    PREPLICATOR_ENDTRANSACTION Record
    )
{
    DumpReplicatorLogicalRecord(&Record->Logical);
    DumpReplicatorTransactionRecord(&Record->Transaction);
    printf("    IsCommitted:            %s\n", (Record->IsCommitted != 0) ? "TRUE" : "FALSE");
}

void DumpReplicatorBarrierRecord(
    PREPLICATOR_BARRIER Record
    )
{
    DumpReplicatorLogicalRecord(&Record->Logical);
    printf("    LastStableLSN:          %lld\t0x%llx\n", Record->LastStableLSN, Record->LastStableLSN);
}

void DumpReplicatorUpdateEpochRecord(
    PREPLICATOR_UPDATEEPOCH Record
    )
{
    DumpReplicatorLogicalRecord(&Record->Logical);
    printf("    Dataloss Number:        %lld\t0x%llx\n", Record->DataLossNumber, Record->DataLossNumber);
    printf("    ConfigurationNumber:    %lld\t0x%llx\n", Record->ConfigurationNumber, Record->ConfigurationNumber);
    printf("    ReplicaId:              %lld\t0x%llx\n", Record->ReplicaId, Record->ReplicaId);
    printf("    Timestamp:              %x %x %x %x %x %x %x %x\n",
        Record->Timestamp[0],
        Record->Timestamp[1],
        Record->Timestamp[2],
        Record->Timestamp[3],
        Record->Timestamp[4],
        Record->Timestamp[5],
        Record->Timestamp[6],
        Record->Timestamp[7]);
}

void DumpReplicatorBeginCheckpointRecord(
    PREPLICATOR_BEGINCHECKPOINT Record
    )
{
    DumpReplicatorPhysicalRecord(&Record->PhysicalRecord);
    printf("    ProgressVectorCount:    %d\n", Record->ProgressVectorCount);
    // TODO: dump rest of record
}

void DumpReplicatorLogHeadRecord(
    PREPLICATOR_LOGHEAD_RECORD Record
    )
{
    printf("    Dataloss Number:        %lld\t0x%llx\n", Record->DataLossNumber, Record->DataLossNumber);
    printf("    ConfigurationNumber:    %lld\t0x%llx\n", Record->ConfigurationNumber, Record->ConfigurationNumber);
    printf("    LSN:                    %lld\t0x%llx\n", Record->LSN, Record->LSN);
    printf("    PSN:                    %lld\t0x%llx\n", Record->PSN, Record->PSN);
    printf("    LogHeadRecordOffset:    %d\t0x%x\n", Record->LogHeadRecordOffset, Record->LogHeadRecordOffset);
}

void DumpReplicatorEndCheckpointRecord(
    PREPLICATOR_ENDCHECKPOINT Record
    )
{
    DumpReplicatorLogHeadRecord(&Record->LogHead);
    DumpReplicatorPhysicalRecord(&Record->PhysicalRecord);
    printf("    LastStableLSN:          %lld\t0x%llx\n", Record->LastStableLSN, Record->LastStableLSN);
    printf("    LastCompletedBCptOffset:%d %x\n", Record->LastCompletedBeginCheckpointOffset, Record->LastCompletedBeginCheckpointOffset);
}

void DumpReplicatorIndexingRecord(
    PREPLICATOR_INDEXING Record
    )
{
    DumpReplicatorPhysicalRecord(&Record->PhysicalRecord);
    printf("    Dataloss Number:        %lld\t0x%llx\n", Record->DataLossNumber, Record->DataLossNumber);
    printf("    Configuration Number:   %lld\t0x%llx\n", Record->ConfigurationNumber, Record->ConfigurationNumber);
}

void DumpReplicatorTruncateHeadRecord(
    PREPLICATOR_TRUNCATEHEAD Record
    )
{
    DumpReplicatorLogHeadRecord(&Record->LogHead);
    DumpReplicatorPhysicalRecord(&Record->PhysicalRecord);
}

void DumpReplicatorTruncateTailRecord(
    PREPLICATOR_TRUNCATETAIL Record
    )
{
    DumpReplicatorPhysicalRecord(&Record->PhysicalRecord);
}

void DumpReplicatorInformationRecord(
    PREPLICATOR_INFORMATION Record
    )
{
    DumpReplicatorPhysicalRecord(&Record->PhysicalRecord);

    if (Record->InformationEvent > RestoredFromBackup)
    {
        return;
    }

    CHAR* const eventText = InformationEvent[Record->InformationEvent];
    printf("    Information Event:      %d\t%s\n", Record->InformationEvent, eventText);
}

void DumpReplicatorHeader(
    PREPLICATOR_RECORD_HEADER recordHeader
    )
{
    ULONG recordLength = recordHeader->RecordLength;

    //
    // The recorded record length is the size of the record excluding the ULONG that preceed the record and
    // the ULONG that is immediately after the record.
    //
    printf("    Replicator record data size is %d 0x%x\n", recordLength, recordLength);
    printf("\n");
    printf("    RecordLength:            %d\n", recordHeader->RecordLength);
    printf("    RecordType:              %d\t%s\n", recordHeader->RecordType, (recordHeader->RecordType <= 13) ? ReplicatorRecordTypes[recordHeader->RecordType] : "Invalid");
    printf("    LSN:                     %lld\t0x%llx\n", recordHeader->LSN, recordHeader->LSN);
    printf("    PSN:                     %lld\t0x%llx\n", recordHeader->PSN, recordHeader->PSN);
    printf("    PreviousRecordOffset:    %lld\t0x%llx\n", recordHeader->PreviousRecordOffset, recordHeader->PreviousRecordOffset);
}

void DumpSpecificReplicatorRecord(
    PREPLICATOR_RECORD_HEADER recordHeader
    )
{
    switch(recordHeader->RecordType)
    {
        case BeginTransaction:
        {
            PREPLICATOR_BEGINTRANSACTION record = (PREPLICATOR_BEGINTRANSACTION)recordHeader;
            DumpReplicatorBeginTransactionRecord(record);
            break;
        }

        case Operation:
        {
            PREPLICATOR_OPERATION record = (PREPLICATOR_OPERATION)recordHeader;
            DumpReplicatorOperationRecord(record);
            break;
        }

        case EndTransaction:
        {
            PREPLICATOR_ENDTRANSACTION record = (PREPLICATOR_ENDTRANSACTION)recordHeader;
            DumpReplicatorEndTransactionRecord(record);

            break;
        }

        case Barrier:
        {
            PREPLICATOR_BARRIER record = (PREPLICATOR_BARRIER)recordHeader;
            DumpReplicatorBarrierRecord(record);

            break;
        }

        case UpdateEpoch:
        {
            PREPLICATOR_UPDATEEPOCH record = (PREPLICATOR_UPDATEEPOCH)recordHeader;
            DumpReplicatorUpdateEpochRecord(record);

            break;
        }

        case BeginCheckpoint:
        {
            PREPLICATOR_BEGINCHECKPOINT record = (PREPLICATOR_BEGINCHECKPOINT)recordHeader;
            DumpReplicatorBeginCheckpointRecord(record);

            break;
        }

        case EndCheckpoint:
        {
            PREPLICATOR_ENDCHECKPOINT record = (PREPLICATOR_ENDCHECKPOINT)recordHeader;
            DumpReplicatorEndCheckpointRecord(record);

            break;
        }

        case Indexing:
        {
            PREPLICATOR_INDEXING record = (PREPLICATOR_INDEXING)recordHeader;

            DumpReplicatorIndexingRecord(record);
            break;
        }

        case TruncateHead:
        {
            PREPLICATOR_TRUNCATEHEAD record = (PREPLICATOR_TRUNCATEHEAD)recordHeader;
            DumpReplicatorTruncateHeadRecord(record);

            break;
        }

        case TruncateTail:
        {
            PREPLICATOR_TRUNCATETAIL record = (PREPLICATOR_TRUNCATETAIL)recordHeader;
            DumpReplicatorTruncateTailRecord(record);

            break;
        }

        case Information:
        {
            PREPLICATOR_INFORMATION record = (PREPLICATOR_INFORMATION)recordHeader;
            DumpReplicatorInformationRecord(record);

            break;
        }

        default:
        {
            break;
        }
    }

    printf("\n");

    printf("    Replicator Record Data\n");
    DumpBuffer((PVOID)recordHeader, recordHeader->RecordLength + (2 * sizeof(ULONG)));
}

PVOID CopyBytesFromRecord(
    PUCHAR MetadataBuffer,
    ULONG MetadataAvailable,
    PUCHAR IoBufferPtr,
    ULONG IoBufferAvailable,
    ULONG OffsetToStart,
    ULONG NumberOfBytes,
    PUCHAR CopyBufferPtr
    )
{
    PUCHAR dataPtr;

    if ((OffsetToStart + NumberOfBytes) > (MetadataAvailable + IoBufferAvailable))
    {
        //
        // The replicator record crosses a physical record boundary so
        // for now, give up on dumping it.
        // TODO: Handle this situation by reading the next record and
        // stitching these together.
        //
        return(NULL);
    }

    if (OffsetToStart > MetadataAvailable)
    {
        //
        // All data resides in IoBuffer
        //
        ULONG offsetToStartInIoBuffer = OffsetToStart - MetadataAvailable;
        dataPtr = IoBufferPtr + offsetToStartInIoBuffer;
    }
    else if ( (OffsetToStart+NumberOfBytes) <= MetadataAvailable)
    {
        //
        // All data resides in metadata
        //
        dataPtr = MetadataBuffer + OffsetToStart;
    }
    else
    {
        //
        // Part of the data is in the metadata and part in the IoBuffer. Copy to temp buffer.
        //
        PUCHAR dataPtrInMetadata = MetadataBuffer + OffsetToStart;
        PUCHAR dataPtrInIoBuffer = IoBufferPtr;
        ULONG bytesInMetadata = MetadataAvailable - OffsetToStart;
        ULONG bytesInIoBuffer = NumberOfBytes - bytesInMetadata;

        memcpy(CopyBufferPtr, dataPtrInMetadata, bytesInMetadata);
        memcpy(CopyBufferPtr+bytesInMetadata, dataPtrInIoBuffer, bytesInIoBuffer);
        dataPtr = CopyBufferPtr;
    }

    return(dataPtr);
}


void DumpReplicatorRecord(
    RvdLogAsn Asn,
    KBuffer::SPtr MetadataBuffer,
    KIoBuffer::SPtr IoBuffer,
    ULONG CoreLoggerOverhead,
    KtlLogStreamId ExpectedStreamId,
    ULONG* ReplicatorRecordLength,
    ULONGLONG* ReplicatorPreviousLength
    )
{
    NTSTATUS status;
    KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
    KLogicalLogInformation::StreamBlockHeader* streamHeader;
    ULONG dataSize;
    ULONG dataOffset;

    status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(MetadataBuffer,
        CoreLoggerOverhead,
        *IoBuffer,
        sizeof(KtlLogVerify::KtlMetadataHeader),
        metadataHeader,
        streamHeader,
        dataSize,
        dataOffset);
    if (!NT_SUCCESS(status))
    {
        printf("KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset failed 0x%x\n", status);
        return;
    }

    if (!(streamHeader->Signature == streamHeader->Sig))
    {
        printf("Record %lld (0x%llx) signature 0x%llx is not correct 0x%llx\n", Asn.Get(), Asn.Get(), streamHeader->Signature, streamHeader->Sig);
    }

    if (!(streamHeader->StreamId == ExpectedStreamId))
    {
        KString::SPtr sActual = KString::Create(*g_Allocator, MAX_PATH);
        KString::SPtr sExpected = KString::Create(*g_Allocator, MAX_PATH);

        RvdLogUtility::AsciiGuid gActual(streamHeader->StreamId.Get());
        RvdLogUtility::AsciiGuid gExpected(ExpectedStreamId.Get());
        
        printf("Record %lld (0x%llx) streamId %s is not correct, expected %s\n", Asn.Get(), Asn.Get(), gActual.Get(), gExpected.Get());
    }

    printf("    Record %lld (0x%llx) is shown as Position %lld (0x%llx) in replicator traces\n",
           Asn.Get(), Asn.Get(), Asn.Get()-1, Asn.Get()-1);

    if (streamHeader->StreamOffset != Asn.Get())
    {
        printf("    Record %lld (0x%llx) is contained within logical record %lld (0x%llx)\n", Asn.Get(), Asn.Get(), streamHeader->StreamOffset, streamHeader->StreamOffset);
    }
    else
    {
        printf("    ********** Record %lld (0x%llx) is first record in logical record %lld (0x%llx)\n", Asn.Get(), Asn.Get(), streamHeader->StreamOffset, streamHeader->StreamOffset);
    }

    if (metadataHeader->RecordMarkerOffsetPlusOne != 0)
    {
        printf("    Record %lld (0x%llx) has a end of replicator record marker at record offset %d (0x%x) and stream offset %lld (0x%llx)\n",
               streamHeader->StreamOffset, streamHeader->StreamOffset,
               metadataHeader->RecordMarkerOffsetPlusOne, metadataHeader->RecordMarkerOffsetPlusOne,
               metadataHeader->RecordMarkerOffsetPlusOne + streamHeader->StreamOffset - 1,
               metadataHeader->RecordMarkerOffsetPlusOne + streamHeader->StreamOffset - 1);
    }

    if (DumpAllData)
    {
        DumpHeaderAndStreamData(
                                MetadataBuffer,
                                IoBuffer,
                                CoreLoggerOverhead,
                                ExpectedStreamId
                               );
    }


    ULONGLONG offsetToStartUL = Asn.Get() - streamHeader->StreamOffset;
    ULONG offsetToStart = (ULONG)offsetToStartUL;
    PUCHAR metadataBuffer = (PUCHAR)(MetadataBuffer->GetBuffer()) + dataOffset - CoreLoggerOverhead;
    ULONG metadataAvailable = KLogicalLogInformation::FixedMetadataSize - dataOffset;

    //
    // Extract the record length from the first ULONG
    //
    ULONG staticRecordSize;
    ULONG* recordSizePtr;

    recordSizePtr = (ULONG*)CopyBytesFromRecord(metadataBuffer, metadataAvailable,
        IoBuffer->First() ? (PUCHAR)(IoBuffer->First()->GetBuffer()) : NULL,
        IoBuffer->First() ? (IoBuffer->First()->QuerySize()) : 0,
        offsetToStart,
        sizeof(ULONG),
        (PUCHAR)&staticRecordSize);
    KInvariant(recordSizePtr != NULL);

    //
    // Set this directly from the front of the record in the case the
    // replicator record crosses physical records
    //
    *ReplicatorRecordLength = (*recordSizePtr);


    //
    // First extract the record header and dump that
    //
    REPLICATOR_RECORD_HEADER staticRecordHeader;
    PREPLICATOR_RECORD_HEADER recordHeader;

    recordHeader = (PREPLICATOR_RECORD_HEADER)CopyBytesFromRecord(metadataBuffer, metadataAvailable,
        IoBuffer->First() ? (PUCHAR)(IoBuffer->First()->GetBuffer()) : NULL,
        IoBuffer->First() ? (IoBuffer->First()->QuerySize()) : 0,
        offsetToStart,
        sizeof(REPLICATOR_RECORD_HEADER),
        (PUCHAR)&staticRecordHeader);

    if (recordHeader == NULL)
    {
        printf("    ##### Replicator Record crosses physical records #####\n");
        return;
    }

    DumpReplicatorHeader(recordHeader);

    //
    // Now extract the whole record
    //
    ULONG tempRecordSize = recordHeader->RecordLength;
    PUCHAR tempBuffer;

    // Add 2*sizeof(ULONG) to account for size at the beginning of record and at end of record
    tempRecordSize += 2 * sizeof(ULONG);

//    if (!DumpAllData)
//    {
//        tempRecordSize = 256;
//    }

    tempBuffer = (PUCHAR)(g_Allocator->Alloc(tempRecordSize));
    if (tempBuffer == NULL)
    {
        printf("Error allocating %d 0x%x bytes for record\n", tempRecordSize, tempRecordSize);
        ExitProcess((UINT)-1);
    }
    KFinally([&] { g_Allocator->Free(tempBuffer); });

    recordHeader = (PREPLICATOR_RECORD_HEADER)CopyBytesFromRecord(metadataBuffer, metadataAvailable,
                                                                  IoBuffer->First() ? (PUCHAR)(IoBuffer->First()->GetBuffer()) : NULL,
                                                                  IoBuffer->First() ? (IoBuffer->First()->QuerySize()) : 0,
                                                                  offsetToStart,
                                                                  tempRecordSize,
                                                                  tempBuffer);
    if (recordHeader == NULL)
    {
        printf("    ##### Replicator Record crosses physical records #####\n");
        return;
    }

    DumpSpecificReplicatorRecord(recordHeader);

    if (IsValidReplicatorRecordType(recordHeader))
    {
        *ReplicatorRecordLength = recordHeader->RecordLength;
        *ReplicatorPreviousLength = recordHeader->PreviousRecordOffset;
    }
}

void ReadAndDumpRecord(
    RvdLog::SPtr LogContainer,
    RvdLogStream::SPtr LogStream,
    RvdLogStreamId StreamId,
    RvdLogAsn Asn,
    BOOLEAN DumpRawData,
    BOOLEAN DumpLogicalLogInfo,
    BOOLEAN DumpReplicatorRecordHeader,
    ULONG* ReplicatorRecordLength,
    ULONGLONG* ReplicatorPreviousLength
    )
{
    NTSTATUS status;

    RvdLogStream::AsyncReadContext::SPtr readRecord;
    status = LogStream->CreateAsyncReadContext(readRecord);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "logStream->CreateReadContext");

    ULONGLONG version;
    KIoBuffer::SPtr ioBuffer;
    KBuffer::SPtr metadata;
    RvdLogAsn actualAsn;
    KSynchronizer sync;

    *ReplicatorRecordLength = 0;
    *ReplicatorPreviousLength = 0;

    printf("---------------------------------------------------------------------------------------------------------------------------------\n");
    printf(" Record %llu 0x%llx\n", Asn.Get(), Asn.Get());
    printf("---------------------------------------------------------------------------------------------------------------------------------\n");

    readRecord->StartRead(Asn, RvdLogStream::AsyncReadContext::ReadContainingRecord, &actualAsn, &version, metadata, ioBuffer, NULL, sync);
    status = sync.WaitForCompletion();
    if (!NT_SUCCESS(status))
    {
        printf("Error %x reading ASN %lld (0x%llx)\n", status, Asn.Get(), Asn.Get());
    }


    if (DumpRawData)
    {
        if (Asn.Get() != actualAsn.Get())
        {
            printf("Read ASN %lld (0x%llx) is within record %lld (0x%llx)\n", Asn.Get(), Asn.Get(), actualAsn.Get(), actualAsn.Get());
        }
        printf("Read containing %lld (0x%llx) is record %lld (0x%llx)\n", Asn.Get(), Asn.Get(), actualAsn.Get(), actualAsn.Get());
        printf("Version %lld\n", version);
        printf("Metadata Size is %d (0x%x), IO buffer size is %d (0x%x)\n", metadata->QuerySize(), metadata->QuerySize(), ioBuffer->QuerySize(), ioBuffer->QuerySize());
        ULONG dataOffset = sizeof(KtlLogVerify::KtlMetadataHeader) + sizeof(KLogicalLogInformation::MetadataBlockHeader) +
                           sizeof(KLogicalLogInformation::StreamBlockHeader);
        printf("Raw Metadata (note core logger header removed). Data begins at %d (0x%x):\n", dataOffset, dataOffset);
        DumpBuffer(metadata->GetBuffer(), metadata->QuerySize());
        printf("Raw IO Buffers (%d elements, first shown)\n", ioBuffer->QueryNumberOfIoBufferElements());
        if (ioBuffer->QueryNumberOfIoBufferElements() > 0)
        {
            DumpBuffer((PVOID)ioBuffer->First()->GetBuffer(), ioBuffer->First()->QuerySize());
        }
    }

    if (DumpLogicalLogInfo)
    {
        printf("Logical log data:\n");
        DumpHeaderAndStreamData(metadata,
            ioBuffer,
            LogContainer->QueryUserRecordSystemMetadataOverhead(),
            StreamId
            );
    }

    if (DumpReplicatorRecordHeader)
    {
        DumpReplicatorRecord(Asn, metadata, ioBuffer, LogContainer->QueryUserRecordSystemMetadataOverhead(),
                             StreamId, ReplicatorRecordLength, ReplicatorPreviousLength);
    }
}

void DumpRecords(
    LPCWSTR ContainerPath,
    RvdLogStreamId StreamId,
    RvdLogAsn Asn,
    BOOLEAN Forward,
    ULONGLONG ForwardMax,
    BOOLEAN Backward,
    ULONGLONG BackwardMin
    )
{
    NTSTATUS status;
    KSynchronizer activateSync;
    KSynchronizer sync;
    RvdLogManager::SPtr logManager;
    RvdLogUtility::AsciiGuid g;
    
    //
    // Get a log manager
    //
    status = RvdLogManager::Create(KTL_TAG_LOGGER, KtlSystem::GlobalNonPagedAllocator(), logManager);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "RvdLogManager::Create");

    status = logManager->Activate(nullptr, activateSync);
    VERIFY_IS_TRUE(K_ASYNC_SUCCESS(status), "logManager->Activate");

    status = logManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
        &VerifyRawRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "logManager->RegisterVerificationCallback");

    status = logManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
        &VerifyRawRecordCallback);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "logManager->RegisterVerificationCallback");

    {
        RvdLog::SPtr logContainer;
        RvdLogManager::AsyncOpenLog::SPtr openLogOp;
        status = logManager->CreateAsyncOpenLogContext(openLogOp);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncOpenLogContext");

        openLogOp->StartOpenLog(KStringView(ContainerPath), logContainer, nullptr, sync);
        status = sync.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
#if !defined(PLATFORM_UNIX)
            printf("Failed to open log container %ws 0x%x\n", ContainerPath, status);
#else
            printf("Failed to open log container %s 0x%x\n", Utf16To8(ContainerPath).c_str(), status);
#endif
            ExitProcess(static_cast<UINT>(-1));
        }

        RvdLog::AsyncOpenLogStreamContext::SPtr openStream;
        RvdLogStream::SPtr logStream;

        status = logContainer->CreateAsyncOpenLogStreamContext(openStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateAsyncOpenLogStreamContext");

        openStream->StartOpenLogStream(StreamId, logStream, NULL, sync);
        status = sync.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            g.Set(StreamId.Get());
            printf("        Error 0x%x opening stream %s\n", status, g.Get());
            printf("\n");
            return;
        }
		
        ULONG thisLen;
        ULONGLONG prevLen;

        if (Forward)
        {
            ULONGLONG forwardAsn = Asn.Get();

            do
            {
                ReadAndDumpRecord(logContainer, logStream, StreamId, forwardAsn, FALSE, FALSE, TRUE, &thisLen, &prevLen);
                //
                // Add extra 2 ULONGs since record size persisted does not include length ULONG before and after the record
                //
                forwardAsn += (ULONGLONG)thisLen + 2 * sizeof(ULONG);
            } while ((thisLen != 0) && (forwardAsn < ForwardMax));
        }
        else if (Backward)
        {
            ULONGLONG backwardAsn = Asn.Get();

            do
            {
                // TODO: Figure out exactly how the read backward should work
                ReadAndDumpRecord(logContainer, logStream, StreamId, backwardAsn, FALSE, FALSE, TRUE, &thisLen, &prevLen);
                //
                // Add extra 2 ULONGs since record size persisted does not include length ULONG before and after the record
                //
                backwardAsn = ((ULONGLONG)prevLen);
            } while ((prevLen != 0) && (backwardAsn > BackwardMin));
        }
        else
        {
            ReadAndDumpRecord(logContainer, logStream, StreamId, Asn, TRUE, TRUE, TRUE, &thisLen, &prevLen);
        }

		
        logStream.Reset();
        //
        // All done, cleanup
        //
        logContainer.Reset();
    }

    logManager->Deactivate();
    activateSync.WaitForCompletion();
    logManager.Reset();
}


void Usage()
{
    printf("Dumps the contents of a KTL core log container\n");
    printf("\n");
    printf("    Usage: DumpContainerInfo -l:<filename of log container> -s:<stream id> -r:<record number> -f:<max record to move forward> -b:<min record to move backward>\n");
    printf("        -l specifies the filename that holds the log container\n");
    printf("        -s specifies the stream id for the stream from which to read records\n");
    printf("        -r specifies the record offset whole data should be dumped\n");
    printf("        -f specifies that each replicator record should be dumped moving forward from the value of -r\n");
    printf("        -f with -p: specifies the file offset from which to start the raw record search and dump\n");
    printf("        -b specifies that each replicator record should be dumped moving backward from the value of -r\n");
    printf("        -a specifies that full record information should be dumped\n");
    printf("\n");
    printf("        -k specifies that the stream should be open via the KTL logger\n");
    printf("        -d specifies the disk id for the KTL stream\n");
    printf("        -c specifies the container id for the KTL stream or container id for raw file open\n");
    printf("\n");
    printf("        -p specifies that the file should be opened without core logger and core logger records dumped. Use -r to specify the LSN within the file or -r:0 to search for records starting at offset specified in -f:");

    printf("    Note: filename must be a fully qualifed path, ie, c:\\wf_n.b.chk\\WinFabricTest\\MCFQueue-CIT.test.Data\\ReplicatorShared.Log\n");
    printf("           stream id must be of the form {54d8a551-fcff-4004-a3c3-6758fc2633f8}\n");
    printf("           -b and -f may not be both set at the same time\n");

    // TODO: Examples

    printf(" To dump all records try this: \n");
    printf("      dumpcontainer -l:c:\\citfail\\3303762\\MS.Test.WinFabric.Cases.TAEFTestCaseWrapper.ScriptTestMethod\\N0030\\Fabric\\work\\Applications\\QueueApp_App0\\work\\968b1b07-0478-48a4-b2be-b2a42c1f0fb3.log -s:{968b1b07-0478-48a4-b2be-b2a42c1f0fb3} -f:0 -r:0\n");
    printf("\n");
    printf(" To dump a stream via KTL logger:\n");
    printf("      dumpcontainer -k: -d:{54d8a551-fcff-4004-a3c3-6758fc2633f8} -c:{54d8a551-fcff-4004-a3c3-6758fc2633f8} -s:{54d8a551-fcff-4004-a3c3-6758fc2633f8}\n");
    printf("\n");
    printf(" To dump a log using raw reads at specific LSN:\n");
    printf("      dumpcontainer -p: -c:{54d8a551-fcff-4004-a3c3-6758fc2633f8} -r:152944640 -l:c:\\citfail\\tpcc\\MS.Test.WinFabric.Cases.TAEFTestCaseWrapper.ScriptTestMethod\\N0050\\Fabric\\work\\Applications\\TpccApp_App0\\work\\b71930f3-6f4f-4a04-b458-20b751a25195.log");
    printf("\n");
    printf(" To search a log using raw reads from a specific file offset:\n");
    printf("      dumpcontainer -p: -c:{54d8a551-fcff-4004-a3c3-6758fc2633f8} -r:0 -f:4096 -l:c:\\citfail\\tpcc\\MS.Test.WinFabric.Cases.TAEFTestCaseWrapper.ScriptTestMethod\\N0050\\Fabric\\work\\Applications\\TpccApp_App0\\work\\b71930f3-6f4f-4a04-b458-20b751a25195.log");
}

typedef struct
{
    PWCHAR LogFileName;
    RvdLogStreamId StreamId;
    RvdLogAsn RecordNumber;
    BOOLEAN DumpForward;
    BOOLEAN DumpBackward;
    BOOLEAN DumpAllData;
    BOOLEAN RawLCMBInfo;
    ULONGLONG ForwardMax;
    ULONGLONG BackwardMin;
    BOOLEAN OpenViaKtlLogger;
    BOOLEAN OpenViaRawFile;
    KGuid DiskId;
    RvdLogId ContainerId;
} COMMAND_OPTIONS, *PCOMMAND_OPTIONS;

ULONG ParseCommandLine(__in int argc, __in wchar_t** args, __out PCOMMAND_OPTIONS Options)
{
    Options->LogFileName = NULL;
    Options->StreamId = { 0 };
    Options->RecordNumber = (ULONGLONG)-1;
    Options->RawLCMBInfo = FALSE;
    Options->DumpBackward = FALSE;
    Options->DumpForward = FALSE;
    Options->OpenViaKtlLogger = FALSE;
    Options->OpenViaRawFile = FALSE;
    Options->DumpAllData = FALSE;

    WCHAR flag;
    size_t len;
    PWCHAR arg;

    for (int i = 1; i < argc; i++)
    {
        arg = args[i];

        len = wcslen(arg);
        if (len < 3)
        {
            Usage();
            return(ERROR_INVALID_PARAMETER);
        }

        if ((arg[0] != L'-') || (arg[2] != L':'))
        {
            Usage();
            return(ERROR_INVALID_PARAMETER);
        }

        flag = (WCHAR)tolower(arg[1]);

        switch (flag)
        {
            case L'l':
            {
                Options->LogFileName = arg + 3;
                break;
            }

            case L's':
            {
                KStringView guidText(arg+3);
                RvdLogStreamId streamId;
                GUID guid;

                guidText.ToGUID(guid);
                KGuid kg(guid);
                streamId = kg;
                Options->StreamId = kg;
                break;
            }

            case L'r':
            {
                KStringView asnText(arg+3);
                RvdLogAsn asn;
                ULONGLONG asnValue;
                asnText.ToULONGLONG(asnValue);
                Options->RecordNumber.Set(asnValue);
                break;
            }

            case L'f':
            {
                if (!Options->DumpBackward)
                {
                    KStringView text(arg + 3);
                    ULONGLONG value;

                    Options->DumpForward = TRUE;
                    text.ToULONGLONG(value);
                    if (value == 0)
                    {
                        value = MAXULONGLONG;
                    }
                    Options->ForwardMax = value;
                }
                else {
                    Usage();
                    return(ERROR_INVALID_PARAMETER);
                }
                break;
            }

            case L'b':
            {
                if (!Options->DumpForward)
                {
                    KStringView text(arg + 3);
                    ULONGLONG value;

                    Options->DumpBackward = TRUE;
                    text.ToULONGLONG(value);
                    Options->BackwardMin = value;
                }
                else {
                    Usage();
                    return(ERROR_INVALID_PARAMETER);
                }
                break;
            }

            case L'a':
            {
                Options->DumpAllData = TRUE;
                break;
            }

            case L'k':
            {
                Options->OpenViaKtlLogger = TRUE;
                break;
            }

            case L'p':
            {
                Options->OpenViaRawFile = TRUE;
                break;
            }

            case L'm':
            {
                Options->RawLCMBInfo = TRUE;
                break;
            }

            case L'c':
            {
                KStringView guidText(arg + 3);
                GUID guid;

                guidText.ToGUID(guid);
                KGuid kg(guid);
                Options->ContainerId = kg;
                break;
            }

            case L'd':
            {
                KStringView guidText(arg + 3);
                GUID guid;

                guidText.ToGUID(guid);
                KGuid kg(guid);
                Options->DiskId = kg;
                break;
            }

            default:
            {
                Usage();
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    return(ERROR_SUCCESS);
}

void OpenViaKtl(
    __in KGuid DiskId,
    __in RvdLogId ContainerId,
    __in RvdLogStreamId StreamId
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KtlLogManager::SPtr logManager;
    KtlLogContainer::SPtr logContainer;
    KtlLogStream::SPtr logStream;
    KServiceSynchronizer        ssync;
    ContainerCloseSynchronizer closeSync;
    StreamCloseSynchronizer closeStreamSync;


#ifdef UPASSTHROUGH
    status = KtlLogManager::CreateInproc(ALLOCATION_TAG, *g_Allocator, logManager);
#else
    status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, logManager);
#endif
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KtlLogManager::Create");

    status = logManager->StartOpenLogManager(NULL, // ParentAsync
        ssync.OpenCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "StartOpenLogManager sync");
    status = ssync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "StartOpenLogManager async");

    KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
    status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateOpenLogCOntinar");
    openAsync->StartOpenLogContainer(DiskId,
        ContainerId,
        logContainer,
        NULL,         // ParentAsync
        sync);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "StartOpenLogCOnt");

    KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;
    ULONG metadataLength;

    status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Createasyncopenstream");
    openStreamAsync->StartOpenLogStream(StreamId,
        &metadataLength,
        logStream,
        NULL,    // ParentAsync
        sync);

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");


    // Stream is open

    logStream->StartClose(NULL,
        closeStreamSync.CloseCompletionCallback());

    status = closeStreamSync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

    logContainer->StartClose(NULL,
        closeSync.CloseCompletionCallback());

    status = closeSync.WaitForCompletion();
    VERIFY_IS_TRUE(status == STATUS_SUCCESS, "");


    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                   ssync.CloseCompletionCallback());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

    logManager = nullptr;
    status = ssync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

}

#if !defined(PLATFORM_UNIX)
NTSTATUS FindDiskIdForDriveLetter(
    __in WCHAR DriveLetter,
    __out GUID& DiskIdGuid
    )
{
    NTSTATUS status;
    KSynchronizer sync;

    UCHAR driveLetter = (UCHAR)toupper(DriveLetter);

    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), sync);
    if (!K_ASYNC_SUCCESS(status))
    {
        return status;
    }
    status = sync.WaitForCompletion();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Find Drive volume GUID
    //
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].DriveLetter == driveLetter)
        {
            DiskIdGuid = volInfo[i].VolumeId;
            return(STATUS_SUCCESS);
        }
    }

    return(STATUS_UNSUCCESSFUL);
}

NTSTATUS GenerateFileName(
    __out KWString& FileName,
    __in KGuid DiskId,
    __in WCHAR* inputFileName
    )
{
    KString::SPtr guidString;
    BOOLEAN b;
    static const ULONG GUID_STRING_LENGTH = 40;

    //
    // Caller us using the default file name as only specified the disk
    //
    // Filename expected to be of the form:
    //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\Testfile.dat"


    guidString = KString::Create(KtlSystem::GlobalNonPagedAllocator(),
        GUID_STRING_LENGTH);
    if (!guidString)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    b = guidString->FromGUID(DiskId);
    guidString->SetNullTerminator();
    if (!b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (!b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    FileName = L"\\GLOBAL??\\Volume";
    FileName += static_cast<WCHAR*>(*guidString);

    FileName += inputFileName;
    return(FileName.Status());
}
#endif

VOID
ValidateAndDumpMasterBlock(
    __in PCHAR Label,
    __in RvdLogId& LogId,
    __in ULONGLONG LogFileSize,
    __in ULONGLONG MasterBlockLocation,
    __in PVOID MasterBlockPtr
)
{
    NTSTATUS status;
    RvdLogMasterBlock* masterBlock = (RvdLogMasterBlock*)MasterBlockPtr;

    status = masterBlock->Validate(LogId, LogFileSize, MasterBlockLocation);
    if (NT_SUCCESS(status))
    {
        printf("%s Master block is valid\n", Label);
    }
    else {
        printf("%s Master block is NOT valid: %x\n", Label, status);
    }

    printf("    MajorVersion: %d", masterBlock->MajorVersion);
    if (masterBlock->MajorVersion > RvdDiskLogConstants::MajorFormatVersion)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    MinorVersion: %d", masterBlock->MinorVersion);
    if (masterBlock->MinorVersion > RvdDiskLogConstants::MinorFormatVersion)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    Reserved1: %d", masterBlock->Reserved1);
    printf("\n");

    RvdLogUtility::AsciiGuid g(masterBlock->LogFormatGuid.Get());
    printf("    LogFormatGuid: %s", g.Get());
    if (masterBlock->LogFormatGuid != RvdDiskLogConstants::LogFormatGuid())
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    RvdLogUtility::AsciiGuid g1(masterBlock->LogId.Get());
    printf("    LogId: %s", g1.Get());
    if (masterBlock->LogId != LogId)
    {
        RvdLogUtility::AsciiGuid g2(LogId.Get());
        printf("   IS NOT VALID. Expecting %s", g2.Get());
    }
    printf("\n");

    printf("    LogSignature: ");
    for (ULONG i = 0; i < RvdDiskLogConstants::SignatureULongs; i++)
    {
        printf(" %0x  ", masterBlock->LogSignature[i]);
    }
    printf("\n");

    printf("    LogFileSize: %lld (%llx)", masterBlock->LogFileSize, masterBlock->LogFileSize);
    if (masterBlock->LogFileSize != LogFileSize)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    MasterBlockLocation: %lld (%llx)", masterBlock->MasterBlockLocation, masterBlock->MasterBlockLocation);
    if (masterBlock->MasterBlockLocation != MasterBlockLocation)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");


    // TODO: Logfile configured geometry
    // RvdLogOnDiskConfig  GeometryConfig;
    printf("    GeometryConfig");
    if (! masterBlock->GeometryConfig.IsValidConfig(masterBlock->LogFileSize))
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

#if !defined(PLATFORM_UNIX)
    printf("    CreationDirectory: %ws", masterBlock->CreationDirectory);
    printf("    LogType: %ws", masterBlock->LogType);
#else
    printf("    CreationDirectory: %s", Utf16To8(masterBlock->CreationDirectory).c_str());
    printf("    LogType: %s", Utf16To8(masterBlock->LogType).c_str());
#endif

    printf("\n");

    // Checksum of this header.
    printf("    ThisBlockChecksum: %lld (%llx)", masterBlock->ThisBlockChecksum, masterBlock->ThisBlockChecksum);
    ULONGLONG savedChksum = masterBlock->ThisBlockChecksum;
    masterBlock->ThisBlockChecksum = 0;
    BOOLEAN checksumWorked = (KChecksum::Crc64(masterBlock, RvdDiskLogConstants::MasterBlockSize, 0) == savedChksum);
    masterBlock->ThisBlockChecksum = savedChksum;
    if (!checksumWorked)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    // Creation flags
    printf("    CreationFlags: %d (%x)", masterBlock->CreationFlags, masterBlock->CreationFlags);
    printf("\n");
}

BOOLEAN IsRecordChecksumValid(
    __in KBlockFile::SPtr File,
    __in ULONGLONG CurrentFileOffset,
    __in ULONG HeaderSize
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    KIoBuffer::SPtr checksumBuffer;
    PVOID checksumPtr;
    RvdLogRecordHeader* fullRecordHeader;
    status = KIoBuffer::CreateSimple(HeaderSize, checksumBuffer, checksumPtr, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");
    fullRecordHeader = (RvdLogRecordHeader*)checksumPtr;

    status = File->Transfer(KBlockFile::IoPriority::eForeground, KBlockFile::eRead, CurrentFileOffset, *checksumBuffer, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

	status = fullRecordHeader->ChecksumIsValid(HeaderSize);
	
    return(NT_SUCCESS(status));
}

VOID
ValidateAndDumpRecordHeader(
    __in KBlockFile::SPtr File,
    __in PVOID RecordHeader,
    __in RvdLogConfig& Config,
    __in ULONGLONG CurrentFileOffset,
    __in RvdLogId& LogId,
    __in ULONG(&LogSignature)[RvdDiskLogConstants::SignatureULongs],
    __in ULONGLONG LogFileLsnSpace,
    __in_opt RvdLogLsn* const LsnToValidate,
    __out ULONG& MetadataSize,
    __out ULONG& IoBufferSize
    )
{
    RvdLogRecordHeader* recordHeader = (RvdLogRecordHeader*)RecordHeader;

    BOOLEAN isValid;

    isValid = recordHeader->IsBasicallyValid(Config, CurrentFileOffset, LogId, LogSignature, LogFileLsnSpace, LsnToValidate);
    if (isValid)
    {
        printf("Record is valid\n");
    }
    else
    {
        printf("Record IS NOT VALID\n");
    }

    IoBufferSize = recordHeader->IoBufferSize;
    MetadataSize = recordHeader->ThisHeaderSize;

    if ((recordHeader->ThisHeaderSize + recordHeader->IoBufferSize) > Config.GetMaxRecordSize())
    {
        printf("*** Record Invalid since Header plus IoBufferSize is larger than MaxRecordSize ***\n");
    }

    printf("    LSN: %lld (%llx)", recordHeader->Lsn.Get(), recordHeader->Lsn.Get());
    if (LsnToValidate != NULL)
    {
        if (LsnToValidate->Get() != recordHeader->Lsn.Get())
        {
            printf("   IS NOT VALID due to Invalid LSN");
        }
    }
    printf("    Highest Complete LSN: %lld (%llx)\n", recordHeader->HighestCompletedLsn.Get(), recordHeader->HighestCompletedLsn.Get());
    printf("    Last checkpoint LSN: %lld (%llx)\n", recordHeader->LastCheckPointLsn.Get(), recordHeader->LastCheckPointLsn.Get());
    printf("    Previous LSN in stream: %lld (%llx)\n", recordHeader->PrevLsnInLogStream.Get(), recordHeader->PrevLsnInLogStream.Get());

    if (RvdLogRecordHeader::ToFileAddress(recordHeader->Lsn, LogFileLsnSpace) != CurrentFileOffset)
    {
        printf("   IS NOT VALID due to bad file address");
    }

    printf("\n");


    RvdLogUtility::AsciiGuid g(recordHeader->LogId.Get());
    printf("    LogId: %s", g.Get());
    if (recordHeader->LogId != LogId)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");


    printf("    MajorVersion: %d", recordHeader->MajorVersion);
    if (recordHeader->MajorVersion > RvdDiskLogConstants::MajorFormatVersion)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    MinorVersion: %d", recordHeader->MinorVersion);
    if (recordHeader->MinorVersion > RvdDiskLogConstants::MinorFormatVersion)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    Reserved1: %d", recordHeader->Reserved1);
    printf("\n");


    printf("    LogSignature: ");
    for (ULONG i = 0; i < RvdDiskLogConstants::SignatureULongs; i++)
    {
        printf(" %0x  ", recordHeader->LogSignature[i]);
    }
    if ((RtlCompareMemory(&recordHeader->LogSignature[0], &LogSignature[0], RvdLogRecordHeader::LogSignatureSize)
                        !=  RvdLogRecordHeader::LogSignatureSize))
    {
        printf("   IS NOT VALID");
    }
    printf("\n");


    RvdLogUtility::AsciiGuid g1(recordHeader->LogStreamId.Get());
    printf("    LogStreamId: %s", g1.Get());
    printf("\n");


    BOOLEAN isUserStream = TRUE;
    RvdLogUtility::AsciiGuid g2(recordHeader->LogStreamType.Get());
    printf("    LogStreamType: %s", g2.Get());
    if (recordHeader->LogStreamType == RvdDiskLogConstants::DeletingStreamType())
    {
        printf("  DeletingStreamType");
        isUserStream = FALSE;
    }

    if (recordHeader->LogStreamType == RvdDiskLogConstants::CheckpointStreamType())
    {
        printf("  CheckpointStreamType");
        isUserStream = FALSE;
    }

    if (recordHeader->LogStreamType == RvdDiskLogConstants::DummyStreamType())
    {
        printf("  DummyStreamType");
        isUserStream = FALSE;
    }

    printf("\n");


    printf("    ThisHeaderSize: %d", recordHeader->ThisHeaderSize);
    if ((recordHeader->ThisHeaderSize % RvdDiskLogConstants::BlockSize) != 0)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    IoBufferSize: %d", recordHeader->IoBufferSize);
    if ((recordHeader->IoBufferSize % RvdDiskLogConstants::BlockSize) != 0)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    MetaDataSize: %d", recordHeader->MetaDataSize);
    if (recordHeader->MetaDataSize > recordHeader->ThisHeaderSize)
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    printf("    Reserved2: %d", recordHeader->Reserved2);
    printf("\n");

    printf("    ThisBlockChecksum: %lld (%llx)", recordHeader->ThisBlockChecksum, recordHeader->ThisBlockChecksum);
    if (!IsRecordChecksumValid(File, CurrentFileOffset, recordHeader->ThisHeaderSize))
    {
        printf("   IS NOT VALID");
    }
    printf("\n");

    if (isUserStream)
    {
        printf("\n");
        RvdLogUserStreamRecordHeader* userRecordStreamHeader = (RvdLogUserStreamRecordHeader*)recordHeader;

        CHAR *recordType = "Unknown";
        BOOLEAN isValidRecordType = FALSE;
        if (userRecordStreamHeader->RecordType == RvdLogUserStreamRecordHeader::Type::CheckPointRecord)
        {
            recordType = "CheckPointRecord";
            isValidRecordType = TRUE;
        }
        else if (userRecordStreamHeader->RecordType == RvdLogUserStreamRecordHeader::Type::UserRecord)
        {
            recordType = "UserRecord";
            isValidRecordType = TRUE;
        }

        printf("    RecordType: %s", recordType);
        if (!isValidRecordType)
        {
            printf("   IS NOT VALID");
        }
        printf("\n");

        printf("    Reserved1: %d\n", userRecordStreamHeader->Reserved1);
        printf("\n");

        printf("    TruncationPoint: %lld (%llx)", userRecordStreamHeader->TruncationPoint.Get(), userRecordStreamHeader->TruncationPoint.Get());
        printf("\n");
        printf("    CopyRecordLsn: %lld (%llx)", userRecordStreamHeader->CopyRecordLsn.Get(), userRecordStreamHeader->CopyRecordLsn.Get());
        printf("\n");

        if (userRecordStreamHeader->RecordType == RvdLogUserStreamRecordHeader::Type::UserRecord)
        {
            RvdLogUserRecordHeader* userRecordHeader = (RvdLogUserRecordHeader*)userRecordStreamHeader;

            printf("    Asn: %lld (%llx)", userRecordHeader->Asn.Get(), userRecordHeader->Asn.Get());
            printf("\n");

            printf("    Asn Version: %lld (%llx)", userRecordHeader->AsnVersion, userRecordHeader->AsnVersion);

            // TODO: Dump headers within metadata
        }
    }
}

class CoreRecordInfo : public KObject<CoreRecordInfo>, public KShared<CoreRecordInfo>
{
public:
    K_FORCE_SHARED(CoreRecordInfo);

public:
    CoreRecordInfo(__in RvdLogRecordHeader* RecordHeader);

    static LONG
        Comparator(CoreRecordInfo& Left, CoreRecordInfo& Right) {
            // -1 if Left < Right
            // +1 if Left > Right
            //  0 if Left == Right
            if (Left._RecordHeader.Lsn < Right._RecordHeader.Lsn)
            {
                return(-1);
            }
            else if (Left._RecordHeader.Lsn > Right._RecordHeader.Lsn) {
                return(+1);
            }
            return(0);
        };

    static ULONG
        GetLinksOffset() { return FIELD_OFFSET(CoreRecordInfo, _TableEntry); };

    KTableEntry _TableEntry;
    RvdLogRecordHeader _RecordHeader;
};

CoreRecordInfo::CoreRecordInfo()
{
}

CoreRecordInfo::CoreRecordInfo(
    __in RvdLogRecordHeader* RecordHeader
    )
{
    KMemCpySafe(&_RecordHeader, sizeof(RvdLogRecordHeader), RecordHeader, sizeof(RvdLogRecordHeader));
}

CoreRecordInfo::~CoreRecordInfo()
{
}


BOOLEAN
IsInRecordList(
    __in ULONGLONG Lsn,
    __in KNodeTable<CoreRecordInfo>& Records
)
{
    RvdLogRecordHeader recordHeader;
    CoreRecordInfo::SPtr recordInfo;
    CoreRecordInfo* foundRecordInfo;

    recordHeader.Lsn = Lsn;
    recordInfo = _new(KTL_TAG_TEST, *g_Allocator) CoreRecordInfo(&recordHeader);

    foundRecordInfo = Records.Lookup(*recordInfo.RawPtr());

    return(foundRecordInfo != NULL);
}

BOOLEAN
IsBasicallyValid(
__in RvdLogRecordHeader* RecordHeader,
__in RvdLogConfig& Config,
__in ULONGLONG const CurrentFileOffset,
__in RvdLogId& LogId,
__in ULONG(&LogSignature)[RvdDiskLogConstants::SignatureULongs],
__in ULONGLONG LogFileLsnSpace,
__in_opt RvdLogLsn* const LsnToValidate = nullptr)
{
    if ((RecordHeader->MajorVersion > RvdDiskLogConstants::MajorFormatVersion) ||
        (RecordHeader->MinorVersion > RvdDiskLogConstants::MinorFormatVersion) ||
        (RecordHeader->LogId != LogId) ||
        (RtlCompareMemory(&RecordHeader->LogSignature[0], &LogSignature[0], RvdLogRecordHeader::LogSignatureSize)
                                                                              != RvdLogRecordHeader::LogSignatureSize) ||
        (RvdLogRecordHeader::ToFileAddress(RecordHeader->Lsn, LogFileLsnSpace) != CurrentFileOffset) ||
        (RecordHeader->PrevLsnInLogStream >= RecordHeader->Lsn) ||
        (RecordHeader->LastCheckPointLsn >= RecordHeader->Lsn) ||
        (RecordHeader->HighestCompletedLsn >= RecordHeader->Lsn) ||
        ((RecordHeader->ThisHeaderSize % RvdDiskLogConstants::BlockSize) != 0) ||
        ((RecordHeader->IoBufferSize % RvdDiskLogConstants::BlockSize) != 0) ||
        (RecordHeader->MetaDataSize > RecordHeader->ThisHeaderSize) ||
        ((RecordHeader->ThisHeaderSize + RecordHeader->IoBufferSize) > Config.GetMaxRecordSize()) ||
        ((LsnToValidate != nullptr) && ((*LsnToValidate) != RecordHeader->Lsn)))
    {
        return FALSE;
    }

    return TRUE;
}

VOID
ScanForRecordsAndDump(
    __in KBlockFile::SPtr File,
    __in PVOID ScanStartPtr,
    __in ULONG ScanLength,
    __in ULONGLONG ScanFileOffset,
    __in RvdLogConfig& Config,
    __in RvdLogId& LogId,
    __in ULONG(&LogSignature)[RvdDiskLogConstants::SignatureULongs],
    __in ULONGLONG LogFileLsnSpace,
    __out KNodeTable<CoreRecordInfo>& Records
    )
{
    ULONG leftToScan = ScanLength;
    ULONG minimumScanSize = sizeof(RvdLogRecordHeader);
    ULONG scanInterval = 0x1000;
    PUCHAR scanPtr = (PUCHAR)ScanStartPtr;
    ULONGLONG fileOffset = ScanFileOffset;
    ULONGLONG expectNext = 0;
    ULONGLONG lastCheckpoint = 0;

    //
    // TODO: This routine does not cleanly handle the case where log wraps
    //
    while (leftToScan > minimumScanSize)
    {
        RvdLogRecordHeader* recordHeader = (RvdLogRecordHeader*)scanPtr;

        BOOLEAN isValid = IsBasicallyValid(recordHeader, Config, fileOffset, LogId, LogSignature, LogFileLsnSpace, NULL);
        if (isValid)
        {
            ULONG metadataSize = recordHeader->ThisHeaderSize;
            ULONG ioBufferSize = recordHeader->IoBufferSize;
            ULONG recordSize = metadataSize + ioBufferSize;
            ULONGLONG lsn = recordHeader->Lsn.Get();
            ULONGLONG highestCompletedLsn = recordHeader->HighestCompletedLsn.Get();
            ULONGLONG lastCheckpointLsn = recordHeader->LastCheckPointLsn.Get();
            ULONGLONG prevLSNInStream = recordHeader->PrevLsnInLogStream.Get();

            BOOLEAN isChecksumValid = IsRecordChecksumValid(File, fileOffset, recordHeader->ThisHeaderSize);
            printf("%c", isChecksumValid ? '-' : 'I');

            BOOLEAN isExpected = (lsn == expectNext);
            printf("%c", isExpected ? '-' : 'U');

            BOOLEAN areLinksWrong = FALSE;
            PCHAR expectNextFormat = " expectnext: %12llx";
            PCHAR highLsnFormat = " highlsn: %12llx";
            PCHAR chkptLsnFormat = " chkptlsn: %12llx";
            PCHAR prevLsnFormat = " prevlsn: %12llx";

            if (!isExpected)
            {
                expectNextFormat = " EXPECTNEXT: %12llX";
            }

            if ((lsn <= highestCompletedLsn) || (!IsInRecordList(highestCompletedLsn, Records)))
            {
                highLsnFormat = " HIGHLSN: %12llX";
                areLinksWrong = TRUE;
            }

            if ((lsn <= lastCheckpointLsn) || (!IsInRecordList(lastCheckpointLsn, Records)) || (lastCheckpointLsn != lastCheckpoint))
            {
                chkptLsnFormat = " CHKPTLSN: %12llX";
                areLinksWrong = TRUE;
            }

            if ((lsn <= prevLSNInStream) || (!IsInRecordList(prevLSNInStream, Records)))
            {
                prevLsnFormat = " PREVLSN: %12llX";
                areLinksWrong = TRUE;
            }

            printf("%c", areLinksWrong ? 'L' : '-');

            CHAR logStreamType = '-';
            if (recordHeader->LogStreamType == RvdDiskLogConstants::DeletingStreamType())
            {
                logStreamType = 'D';
            }

            if (recordHeader->LogStreamType == RvdDiskLogConstants::CheckpointStreamType())
            {
                logStreamType = 'C';
                lastCheckpoint = lsn;
            }

            if (recordHeader->LogStreamType == RvdDiskLogConstants::DummyStreamType())
            {
                logStreamType = 'M';
            }

            if (logStreamType == '-')
            {
                RvdLogUserStreamRecordHeader* userRecordStreamHeader = (RvdLogUserStreamRecordHeader*)recordHeader;
                if (userRecordStreamHeader->RecordType == RvdLogUserStreamRecordHeader::Type::CheckPointRecord)
                {
                    logStreamType = 'S';
                }
                else if (userRecordStreamHeader->RecordType == RvdLogUserStreamRecordHeader::Type::UserRecord)
                {
                    logStreamType = 'R';
                }
            }

            printf("%c", logStreamType);

            expectNext = lsn + recordSize;

            CoreRecordInfo::SPtr recordInfo;
            recordInfo = _new(KTL_TAG_TEST, *g_Allocator) CoreRecordInfo(recordHeader);
            recordInfo->AddRef();                // Add for placing on list
            Records.Insert(*recordInfo);

            printf("%12llx length %12x",
                lsn, recordSize);

            printf(expectNextFormat, expectNext);
            printf(highLsnFormat, highestCompletedLsn);
            printf(chkptLsnFormat, lastCheckpointLsn);
            printf(prevLsnFormat, prevLSNInStream);


            printf("\n");


            if (DumpAllData)
            {
                ValidateAndDumpRecordHeader(File, recordHeader, Config, fileOffset, LogId, LogSignature, LogFileLsnSpace, NULL, metadataSize, ioBufferSize);
            }

        }

        leftToScan -= scanInterval;
        scanPtr += scanInterval;
        fileOffset += scanInterval;
    }
}


VOID
OpenViaRawFile(
    __in PWCHAR Filename,
    __in RvdLogId& LogId,
    __in ULONGLONG RecordLSN,
    __in ULONGLONG StartOffset,
    __in BOOLEAN DumpAllDataX
)
{
    NTSTATUS status;
    KSynchronizer sync;
    KBlockFile::SPtr file;
    KWString fileName(*g_Allocator);

    printf("RecordLsn %llx, StartOffset %llx\n", RecordLSN, StartOffset);

#if !defined(PLATFORM_UNIX)
    GUID diskId;
    status = FindDiskIdForDriveLetter(*Filename, diskId);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

    status = GenerateFileName(fileName, diskId, Filename + 2);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");
#else
    fileName = Filename;
    VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()), "");  
#endif

    ULONG createOptions = static_cast<ULONG>(KBlockFile::eReadOnly) |
                          static_cast<ULONG>(KBlockFile::eShareRead) |
                          static_cast<ULONG>(KBlockFile::eShareWrite);
    
    status = KBlockFile::Create(fileName,
                                TRUE,
                                KBlockFile::eOpenExisting,
                                static_cast<KBlockFile::CreateOptions>(createOptions),
                                file,
                                sync,
                                NULL,
                                *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

    const ULONG masterBlockSize = RvdDiskLogConstants::MasterBlockSize;
    KIoBuffer::SPtr masterBlockBuffer;
    PVOID masterBlockPtr;
    RvdLogMasterBlock* masterBlock;
    ULONGLONG offset = 0;
    ULONGLONG fileSize;

    fileSize = file->QueryFileSize();
    ULONGLONG logFileLsnSpace = fileSize - (2 * RvdDiskLogConstants::MasterBlockSize);

#if !defined(PLATFORM_UNIX)
    printf("%ws\n\n", (PWCHAR)fileName);
#else
    printf("%s\n\n", Utf16To8((PWCHAR)fileName).c_str());
#endif
    printf("    File Size: %lld (%llx)\n", fileSize, fileSize);
    //
    // Read and verify master blocks
    //
    status = KIoBuffer::CreateSimple(masterBlockSize, masterBlockBuffer, masterBlockPtr, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

    status = file->Transfer(KBlockFile::IoPriority::eForeground, KBlockFile::eRead, offset, *masterBlockBuffer, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

    printf("\n");
    ValidateAndDumpMasterBlock("First", LogId, fileSize, offset, masterBlockPtr);


    offset = fileSize - masterBlockSize;
    status = file->Transfer(KBlockFile::IoPriority::eForeground, KBlockFile::eRead, offset, *masterBlockBuffer, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "");

    printf("\n");
    ValidateAndDumpMasterBlock("Last", LogId, fileSize, offset, masterBlockPtr);

    masterBlock = (RvdLogMasterBlock*)masterBlockPtr;
    RvdLogConfig config(masterBlock->GeometryConfig);

    if (RecordLSN == 0)
    {
        //
        // Scan for records
        //
        KIoBuffer::SPtr scanBuffer;
        PVOID scanPtr;
        ULONG scanLength = 64 * 0x1000 * 0x1000;
        ULONGLONG scanLengthL = scanLength;
        ULONGLONG scanOffset = StartOffset == 0 ? masterBlockSize : StartOffset;
        ULONGLONG highFileOffset = fileSize - masterBlockSize;
        KNodeTable<CoreRecordInfo> records(CoreRecordInfo::GetLinksOffset(),
                                           KNodeTable<CoreRecordInfo>::CompareFunction(&CoreRecordInfo::Comparator));

        // TODO: Routine would be more easily written using memory
        //       mapped file
        printf("\n");

        printf("I - Invalid Chksum, U - unexpected next record, L - LSN links wrong, D - deleted stream, C - log chkpt, M - dummy stream, S - stream chkpt, R - user record\n");
        status = KIoBuffer::CreateSimple(scanLength, scanBuffer, scanPtr, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "");

//      printf("scanOffset %llx, highFileOffset %llx\n", scanOffset, highFileOffset);
        while (scanOffset < highFileOffset)
        {
            //printf("Offset %llx\n", scanOffset);
            ULONGLONG leftInFile = (highFileOffset - scanOffset);
//          printf("scanOffset %llx, leftInFile %llx\n", scanOffset, leftInFile);
            if (leftInFile < scanLengthL)
            {
                scanLength = (ULONG)leftInFile;
//              printf("** scanLength %x\n", scanLength);
            }

//          printf("Reading %llx for %x\n", scanOffset, scanLength);
            status = file->Transfer(KBlockFile::IoPriority::eForeground, KBlockFile::eRead, scanOffset, scanPtr, scanLength, sync, NULL);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "");
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "");

            ScanForRecordsAndDump(file, scanPtr, scanLength, scanOffset, config, LogId, masterBlock->LogSignature, logFileLsnSpace, records);

            scanOffset += scanLength;
        }

        CoreRecordInfo* entry;
        do
        {

            entry = records.First();
            if (entry != NULL)
            {
                records.Remove(*entry);
            }

            if (entry)
            {
                //
                // Release ref taken when inserted in table
                //
                entry->Release();
            }

        } while (entry != NULL);


    }
    else
    {
        //
        // Read specific record and header
        //
        ULONG recordHeaderSize = 0x1000;
        KIoBuffer::SPtr recordHeaderBuffer;
        PVOID recordHeaderPtr;
        RvdLogRecordHeader* recordHeader;
        status = KIoBuffer::CreateSimple(recordHeaderSize, recordHeaderBuffer, recordHeaderPtr, *g_Allocator);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "");

        offset = RvdLogRecordHeader::ToFileAddress(RecordLSN, logFileLsnSpace);
        printf("\n");
        printf("    LSN: %lld (%llx) Offset %llx (%llx)", RecordLSN, RecordLSN, offset, offset);
        status = file->Transfer(KBlockFile::IoPriority::eForeground, KBlockFile::eRead, offset, *recordHeaderBuffer, sync, NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "");
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "");

        recordHeader = (RvdLogRecordHeader*)recordHeaderPtr;
        ULONG ioBufferSize, metadataSize;
        RvdLogLsn recordLSN = RecordLSN;
        ValidateAndDumpRecordHeader(file, recordHeaderPtr, config, offset, LogId, masterBlock->LogSignature, logFileLsnSpace, &recordLSN, metadataSize, ioBufferSize);

        if (DumpAllDataX)
        {
            //
            // Read and dump metadata
            //
            KIoBuffer::SPtr metadataBuffer;
            PVOID metadataPtr;
            status = KIoBuffer::CreateSimple(metadataSize, metadataBuffer, metadataPtr, *g_Allocator);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "");
            // offset same as record header offset
            status = file->Transfer(KBlockFile::IoPriority::eForeground, KBlockFile::eRead, offset, *metadataBuffer, sync, NULL);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "");
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "");
            printf("\n");
            printf("Metadata\n");
            DumpBuffer(metadataPtr, metadataSize);

            if (ioBufferSize != 0)
            {
                //
                // Read and dump IoBuffer
                //
                KIoBuffer::SPtr ioBuffer;
                PVOID ioPtr;
                status = KIoBuffer::CreateSimple(ioBufferSize, ioBuffer, ioPtr, *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(status), "");
                offset += metadataSize;
                status = file->Transfer(KBlockFile::IoPriority::eForeground, KBlockFile::eRead, offset, *metadataBuffer, sync, NULL);
                VERIFY_IS_TRUE(NT_SUCCESS(status), "");
                status = sync.WaitForCompletion();
                VERIFY_IS_TRUE(NT_SUCCESS(status), "");
                printf("\n");
                printf("IoBuffer\n");
                DumpBuffer(ioPtr, ioBufferSize);
            }
        }
    }
}

int
TheMain(__in int argc, __in wchar_t** args)
{
    ULONG error;
    COMMAND_OPTIONS options;

    SetupRawCoreLoggerTests();
    KFinally([&] { CleanupRawCoreLoggerTests(); });

    KString::SPtr s = KString::Create(*g_Allocator);

    if (! s)
    {
        printf("Couldn't alloc kstring\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }


    error = ParseCommandLine(argc, args, &options);
    if (error != ERROR_SUCCESS)
    {
        return(error);
    }

    if (options.DumpAllData)
    {
        DumpAllData = TRUE;
    }

    if ((options.LogFileName == NULL) && (!options.OpenViaKtlLogger))
    {
        Usage();
        return(ERROR_INVALID_PARAMETER);
    }

    if (options.RawLCMBInfo)
    {
        DumpSharedContainerMBInfoRaw(options.LogFileName);
        return(0);
    }
    
    if (options.OpenViaKtlLogger)
    {
        OpenViaKtl(options.DiskId, options.ContainerId, options.StreamId);
        return(0);
    }

    if (options.OpenViaRawFile)
    {
        OpenViaRawFile(options.LogFileName, options.ContainerId, options.RecordNumber.Get(), options.ForwardMax, options.DumpAllData);
        return(0);
    }

    if ((options.DumpBackward || options.DumpForward) && (options.RecordNumber == (ULONGLONG)-1))
    {
        Usage();
        return(ERROR_INVALID_PARAMETER);
    }
    
    if (options.RecordNumber == 0)
    {
        options.RecordNumber = 1;
    }

    if (options.RecordNumber == (ULONGLONG)-1)
    {
#if !defined(PLATFORM_UNIX)
        printf("LogContainer: %ws\n\n", options.LogFileName);
#else
        printf("LogContainer: %s\n\n", Utf16To8(options.LogFileName).c_str());
#endif
        DumpAllContainerInfo(options.LogFileName);
    }
    else
    {
        s->FromGUID(options.StreamId.Get());
        s->SetNullTerminator();

        DumpRecords(options.LogFileName, options.StreamId, options.RecordNumber, options.DumpForward, options.ForwardMax, options.DumpBackward, options.BackwardMin);
    }

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
