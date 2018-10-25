/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLoggerTestHelpers.cpp

Abstract:

    This file contains test helper functions

--*/
#pragma once
#include "RvdLogTestHelpers.h"

#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif

ULONG RandomGenerator::_Seed = 0;

NTSTATUS
GetLogDriveRootPath(
    __in WCHAR const DriveLetter,
    __out KWString& RootPath)
{
#if ! defined(PLATFORM_UNIX)
    // Make the directory if needed
    WCHAR       driveLetterWStr[2];
    driveLetterWStr[0] = DriveLetter;
    driveLetterWStr[1] = 0;

    RootPath = L"\\??\\";
    RootPath += driveLetterWStr;
    RootPath += L":";
#else
    RootPath = L"/";
#endif

    RootPath += &RvdDiskLogConstants::DirectoryName();
    KDbgPrintf("GetLogDriveRootPath: Rootpath constructed is : %ws\n", (WCHAR*)RootPath);
    return RootPath.Status();
}

NTSTATUS
GetLogFilesOnLogDrive(
    __in WCHAR const DriveLetter,
    __out KVolumeNamespace::NameArray& LogFiles)
{
    NTSTATUS status = STATUS_SUCCESS;

    KWString rootPath(KtlSystem::GlobalNonPagedAllocator());
    status = GetLogDriveRootPath(DriveLetter, rootPath);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("GetLogFilesOnLogDrive: GetLogDriveRootPath() failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    KEvent compEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback opDoneCallback;
#ifdef _WIN64
    opDoneCallback.Bind((PVOID)(&compEvent), &StaticTestCallback);
#endif

    status = KVolumeNamespace::QueryFiles(rootPath, LogFiles, KtlSystem::GlobalNonPagedAllocator(), opDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("GetLogFilesOnLogDrive: QueryFiles() start failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    compEvent.WaitUntilSet();
    status = LastStaticTestCallbackStatus;
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("GetLogFilesOnLogDrive: QueryFiles() failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
GetLogDriveGuid(
    __in WCHAR const LogDriveLetter,
    __out KGuid& LogDriveGuid)
{
    KDbgPrintf("GetLogDriveGuid: called for drive %c : %i\n", (UCHAR)LogDriveLetter, __LINE__);
    NTSTATUS status = STATUS_SUCCESS;
    GUID        volGuid;

#if ! defined(PLATFORM_UNIX)
    KEvent getVolInfoCompleteEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback getVolInfoDoneCallback;
#ifdef _WIN64
    getVolInfoDoneCallback.Bind((PVOID)(&getVolInfoCompleteEvent), &StaticTestCallback);
#endif

    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), getVolInfoDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("GetLogDriveGuid: KVolumeNamespace::QueryVolumeListEx() start failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    getVolInfoCompleteEvent.WaitUntilSet();
    status = LastStaticTestCallbackStatus;
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("GetLogDriveGuid: KVolumeNamespace::QueryVolumeListEx() failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    if (volInfo.Count() == 0)
    {
        KDbgPrintf("GetLogDriveGuid: KVolumeNamespace::QueryVolumeIdFromDriveLetter() did not return any volume guids: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG ix = 0; ix < volInfo.Count(); ix++)
    {
        KDbgPrintf("GetLogDriveGuid: KVolumeNamespace::QueryVolumeIdFromDriveLetter() returned volume guid for drive %c : %i\n", volInfo[ix].DriveLetter, __LINE__);
    }

    // Find Log Drive volume GUID
    if (!KVolumeNamespace::QueryVolumeIdFromDriveLetter(volInfo, (UCHAR)LogDriveLetter, volGuid))
    {
        KDbgPrintf("GetLogDriveGuid: KVolumeNamespace::QueryVolumeIdFromDriveLetter() did not return volume guid for drive %c: Line: %i\n", (UCHAR)LogDriveLetter, __LINE__);
        return STATUS_UNSUCCESSFUL;
    }
#else
    volGuid = RvdDiskLogConstants::HardCodedVolumeGuid();
#endif
    
    LogDriveGuid.Set(volGuid);
    return STATUS_SUCCESS;
}

NTSTATUS
GetLogDriveLetter(
    __in KGuid const& LogDriveGuid,
    __out WCHAR* LogDriveLetter)
{
    NTSTATUS status = STATUS_SUCCESS;

#if ! defined(PLATFORM_UNIX)
    KEvent getVolInfoCompleteEvent(FALSE, FALSE);
    KAsyncContextBase::CompletionCallback getVolInfoDoneCallback;
#ifdef _WIN64
    getVolInfoDoneCallback.Bind((PVOID)(&getVolInfoCompleteEvent), &StaticTestCallback);
#endif

    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), getVolInfoDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("GetLogDriveGuid: KVolumeNamespace::QueryVolumeListEx() start failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    getVolInfoCompleteEvent.WaitUntilSet();
    status = LastStaticTestCallbackStatus;
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("GetLogDriveGuid: KVolumeNamespace::QueryVolumeListEx() failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    // Find Log Drive volume GUID
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].VolumeId == LogDriveGuid)
        {
            break;
        }
    }

    if (i == volInfo.Count())
    {
        KDbgPrintf(
            "GetLogDriveGuid: KVolumeNamespace::QueryVolumeListEx() did not return volume drive letter for drive %ws : %i\n",
            (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), LogDriveGuid), __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    *LogDriveLetter = (WCHAR)volInfo[i].DriveLetter;
#else
    *LogDriveLetter = L'c';
#endif
    return STATUS_SUCCESS;
}

NTSTATUS
CheckIfLogExists(
    __in KGuid const& LogId,
    __in KGuid const& LogDriveId)
{
    NTSTATUS status = STATUS_SUCCESS;
    KWString logFilename(KtlSystem::GlobalNonPagedAllocator());
    logFilename += L"Log";
    logFilename += LogId;
    logFilename += L".log";

    WCHAR logDriveLetter;
    status = GetLogDriveLetter(LogDriveId, &logDriveLetter);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("CheckIfLogExists: GetLogDriveLetter() failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    KVolumeNamespace::NameArray logFiles(KtlSystem::GlobalNonPagedAllocator());
    status = GetLogFilesOnLogDrive(logDriveLetter, logFiles);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("CheckIfLogExists: GetLogFilesOnLogDrive() failed with status 0x%0x. Line : %i\n", status, __LINE__);
        return status;
    }

    BOOLEAN logFound = FALSE;
    for(ULONG i = 0; i < logFiles.Count(); i++)
    {
        if(logFilename.CompareTo(logFiles[i], TRUE) == 0)
        {
            logFound = TRUE;
        }
    }

    if(!logFound)
    {
        KDbgPrintf("CheckIfLogExists: Failed to find log %ws. Line : %i\n", (WCHAR*)logFilename, __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CheckLogAttributes(
    __in RvdLog::SPtr& Log,
    __in KGuid const& LogId,
    __in KGuid const& DiskId,
    __in KWString const& LogType,
    __in LONGLONG LogSize)
{
    UNREFERENCED_PARAMETER(LogType);
    UNREFERENCED_PARAMETER(LogSize);

    KGuid diskId;
    RvdLogId logId;
    Log->QueryLogId(diskId, logId);
    if(logId.Get() != LogId || diskId != DiskId)
    {
        KDbgPrintf(
            "CheckLogAttributes: Supplied DiskId %ws and LogId %ws did not match queried DiskId %ws and LogId %ws. Line : %i\n",
            (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), DiskId),
            (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), LogId),
            (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), diskId),
            (WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), logId.Get()), __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // TODO: Add these checks when the queries are implemented

/*
    KWString logType;
    Log->QueryLogType(logType);
    if(logType.CompareTo(LogType) != 0)
    {
        KDbgPrintf("CheckLogAttributes: Supplied LogType %ws did not match queried LogType %ws. Line : %i\n", (WCHAR*)KWString(LogType), (WCHAR*)KWString(logType), __LINE__);
        return STATUS_UNSUCCESSFUL;
    }


    LONGLONG logSize;
    Log->QuerySpaceInformation(&logSize, NULL);
    if(logSize != LogSize)
    {
        KDbgPrintf("CheckLogAttributes: Supplied LogSize %ull did not match queried LogSize %ull. Line : %i\n", LogSize, logSize, __LINE__);
        return STATUS_UNSUCCESSFUL;
    }
*/
    return STATUS_SUCCESS;
}


NTSTATUS
GetDriveAndPathOfLogFile(
    __in WCHAR DriveLetter,
    __in RvdLogId& LogId,
    __out KGuid& DriveGuid,
    __out KWString& FullyQualifiedDiskFileName)
{
    NTSTATUS                                status;
#if ! defined(PLATFORM_UNIX)    
    KInvariant((DriveLetter < 256) && (DriveLetter >= 0));
    UCHAR const                             driveLetter = (UCHAR)DriveLetter;

    CompletionEvent                         getVolInfoCompleteEvent;
    KAsyncContextBase::CompletionCallback   getVolInfoDoneCallback;
    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());

    getVolInfoDoneCallback.Bind(&getVolInfoCompleteEvent, &CompletionEvent::EventCallback);
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), getVolInfoDoneCallback);
    if (!K_ASYNC_SUCCESS(status))
    {
        KDbgPrintf("GetDriveAndPathOfLogFile: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        return status;
    }

    status = getVolInfoCompleteEvent.WaitUntilSet();
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("GetDriveAndPathOfLogFile: KVolumeNamespace::QueryVolumeListEx Failed: %i\n", __LINE__);
        return status;
    }

    // Find Drive *DriveLetterStr: volume GUID
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].DriveLetter == driveLetter)
        {
            break;
        }
    }

    if (i == volInfo.Count())
    {
        KDbgPrintf("GetDriveAndPathOfLogFile: KVolumeNamespace::QueryVolumeListEx did not return volume guid for drive: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }
    DriveGuid = volInfo[i].VolumeId;
#else
    DriveGuid = RvdDiskLogConstants::HardCodedVolumeGuid();
#endif
    status = RvdDiskLogConstants::BuildFullyQualifiedLogName(DriveGuid, LogId, FullyQualifiedDiskFileName);

    return status;
}

BOOLEAN
ToULonglong(__in KWString& FromStr, __out ULONGLONG& Result)
//
// Returns FALSE if there is a conversion error of some sort
{
    Result = 0;
    ULONGLONG       result = 0;
    WCHAR*          currDigPtr = (WCHAR*)FromStr;
    ULONG           fromStrLen = FromStr.Length();
    KAssert((fromStrLen & 0x01) == 0);

    while (fromStrLen > 0)
    {
        WCHAR       currDig = *currDigPtr;

        if ((currDig < '0') || (currDig > '9'))
        {
            return FALSE;
        }

        currDig -= '0';
        ULONGLONG   oldValue = result;
        result *= 10;
        result += currDig;

        if (result < oldValue)
        {
            // Overflow
            return FALSE;
        }

        fromStrLen -= 2;
        currDigPtr++;
    }

    Result = result;
    return TRUE;
}

BOOLEAN
ToULong(__in KWString& FromStr, __out ULONG& Result)
//
// Returns FALSE if there is a conversion error of some sort
{
    ULONGLONG       longValue;

    if (!ToULonglong(FromStr, longValue))
    {
        return FALSE;
    }

    if (longValue > MAXULONG)
    {
        return FALSE;
    }

    Result = (ULONG)longValue;
    return TRUE;
}



//** KInjectorBlockDevice implementation
FAILABLE
KInjectorBlockDevice::KInjectorBlockDevice(KBlockDevice::SPtr UnderlyingDevice, RvdLog::SPtr Log)
    :   _UnderlyingDevice(UnderlyingDevice),
        _IsInterceptWrite(FALSE),
        _FaultedRegions(GetThisAllocator()),
        _FaultedRecords(GetThisAllocator()),
        _State(0),
        _Log(*((RvdLogManagerImp::RvdOnDiskLog*)Log.RawPtr()))
{
}

KInjectorBlockDevice::~KInjectorBlockDevice()
{
}

VOID
KInjectorBlockDevice::SetWriteIntercept(BOOLEAN To)
{
    _IsInterceptWrite = To;
}

BOOLEAN
KInjectorBlockDevice::GetWriteIntercept()
{
    return _IsInterceptWrite;
}

VOID
KInjectorBlockDevice::SetInjectionConstraint(InjectionConstraint To)
{
    _InjectionConstraint = To;
}

KArray<KInjectorBlockDevice::FaultedRegionDescriptor>&
KInjectorBlockDevice::GetFaultedRegions()
{
    return _FaultedRegions;
}

KArray<KInjectorBlockDevice::FaultedRecordHeader::SPtr>&
KInjectorBlockDevice::GetFaultedRecords()
{
    return _FaultedRecords;
}

KBlockDevice::SPtr
KInjectorBlockDevice::GetUnderlyingDevice()
{
    return _UnderlyingDevice;
}

VOID
KInjectorBlockDevice::QueryDeviceAttributes(__out DeviceAttributes& Attributes)
{
    _UnderlyingDevice->QueryDeviceAttributes(Attributes);
}

VOID
KInjectorBlockDevice::QueryDeviceHandle(__out HANDLE& Handle)
{
    _UnderlyingDevice->QueryDeviceHandle(Handle);
}

VOID
KInjectorBlockDevice::Close()
{
    _UnderlyingDevice->Close();
}

NTSTATUS
KInjectorBlockDevice::AllocateReadContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag)
{
    return _UnderlyingDevice->AllocateReadContext(Async, AllocationTag);
}

NTSTATUS
KInjectorBlockDevice::AllocateNonContiguousReadContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag)
{
    return _UnderlyingDevice->AllocateNonContiguousReadContext(Async, AllocationTag);
}

NTSTATUS
KInjectorBlockDevice::AllocateWriteContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag)
{
    return _UnderlyingDevice->AllocateWriteContext(Async, AllocationTag);
}

NTSTATUS
KInjectorBlockDevice::AllocateFlushContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag)
{
    return _UnderlyingDevice->AllocateFlushContext(Async, AllocationTag);
}

NTSTATUS
KInjectorBlockDevice::AllocateTrimContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag)
{
    return _UnderlyingDevice->AllocateTrimContext(Async, AllocationTag);
}

NTSTATUS
KInjectorBlockDevice::AllocateQueryAllocationsContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag)
{
    return _UnderlyingDevice->AllocateQueryAllocationsContext(Async, AllocationTag);
}

NTSTATUS
KInjectorBlockDevice::AllocateSetSystemIoPriorityHintContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag)
{
    return _UnderlyingDevice->AllocateSetSystemIoPriorityHintContext(Async, AllocationTag);
}

NTSTATUS
KInjectorBlockDevice::Read(
    __in ULONGLONG Offset,
    __in ULONG Length,
    __in BOOLEAN ContiguousIoBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
    return _UnderlyingDevice->Read(
        Offset,
        Length,
        ContiguousIoBuffer,
        IoBuffer,
        Completion,
        ParentAsync,
        Async);
}

NTSTATUS
KInjectorBlockDevice::ReadNonContiguous(
        __in ULONGLONG Offset,
        __in ULONG ReadLength,
        __in ULONG BufferLength,
        __out KIoBuffer::SPtr& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async,
        __in_opt KArray<KAsyncContextBase::SPtr>* const BlockDeviceReadOps)
{
    return _UnderlyingDevice->ReadNonContiguous(
        Offset,
        ReadLength,
        BufferLength,
        IoBuffer,
        Completion,
        ParentAsync,
        Async,
        BlockDeviceReadOps);
}

NTSTATUS
KInjectorBlockDevice::Write(
__in ULONGLONG Offset,
__in KIoBuffer& IoBuffer,
__in KAsyncContextBase::CompletionCallback Completion,
__in_opt KAsyncContextBase* const ParentAsync,
__in_opt KAsyncContextBase::SPtr Async)
{
    NTSTATUS status;

    status = Write(KBlockFile::IoPriority::eForeground,
        Offset,
        IoBuffer,
        Completion,
        ParentAsync,
        Async);

    return(status);
}


NTSTATUS
KInjectorBlockDevice::Write(
    __in KBlockFile::IoPriority IoPriority,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
    NTSTATUS status;

    status = Write(IoPriority,
                   KBlockFile::SystemIoPriorityHint::eNotDefined,
                   Offset,
                   IoBuffer,
                   Completion,
                   ParentAsync,
                   Async);

    return(status);
}

NTSTATUS
KInjectorBlockDevice::Write(
    __in KBlockFile::IoPriority IoPriority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
    if (!_IsInterceptWrite || (Offset == RvdDiskLogConstants::BlockSize))
    {
        return _UnderlyingDevice->Write(
            IoPriority,
            IoPriorityHint,
            Offset,
            IoBuffer,
            Completion,
            ParentAsync,
            Async);
    }

    // To simulate power failure during the writing of the log, the StateMachine() logic will drop
    // random blocks from the write request; recording those dropped blocks so that the runtime
    // and recovery logic aimed at limiting the exposed region of the log tail to no more than
    // RvdDiskLogConstants::MaxQueuedWriteDepth can be proven.
    //
    if (StateMachine(Offset, IoBuffer, Completion, ParentAsync, Async))
    {
        // state machine is processing the write
        return STATUS_SUCCESS;
    }

    return _UnderlyingDevice->Write(
        IoPriority,
        Offset,
        IoBuffer,
        Completion,
        ParentAsync,
        Async);
}

NTSTATUS
KInjectorBlockDevice::Flush(
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
    return _UnderlyingDevice->Flush(Completion, ParentAsync, Async);
}

NTSTATUS
KInjectorBlockDevice::Trim(
    __in ULONGLONG TrimFrom,
    __in ULONGLONG TrimToPlusOne,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
    return _UnderlyingDevice->Trim(TrimFrom, TrimToPlusOne, Completion, ParentAsync, Async);
}

NTSTATUS
KInjectorBlockDevice::QueryAllocations(
    __in ULONGLONG QueryFrom,
    __in ULONGLONG Length,
    __inout KArray<KBlockFile::AllocatedRange>& Results,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
    return _UnderlyingDevice->QueryAllocations(QueryFrom, Length, Results, Completion, ParentAsync, Async);
}

NTSTATUS
KInjectorBlockDevice::SetSystemIoPriorityHint(
    __in KBlockFile::SystemIoPriorityHint Hint,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
    )
{
    return _UnderlyingDevice->SetSystemIoPriorityHint(Hint, Completion, ParentAsync, Async);
}

NTSTATUS
KInjectorBlockDevice::SetSparseFile(
    __in BOOLEAN IsSparseFile
    )
{
    return _UnderlyingDevice->SetSparseFile(IsSparseFile);
}

BOOLEAN
KInjectorBlockDevice::StateMachine(
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
//    ULONG               ioBufferSize = IoBuffer.QuerySize();   //    TODO: deleteme
    RvdLogRecordHeader* recHdr = (RvdLogRecordHeader*)IoBuffer.First()->GetBuffer();

    K_LOCK_BLOCK(_ThisLock)
    {
        if (_State == 0)
        {
            //* State 0: Validate state and select lowest possible fault location LSN if user record is to be faulted
            _State = 1;

            if ((_InjectionConstraint == InjectionConstraint::UserRecordHeaderOnly) ||
                (_InjectionConstraint == InjectionConstraint::UserRecordIoBufferDataOnly))
            {
                // Select a random log lsn offset from the current location within which to trigger injection of fault (at or after)
                // - limited by MaxQueuedWriteDepth
                LONGLONG    lowest;
                LONGLONG    next;

                _Log.GetPhysicalExtent(lowest, next);
                _RecordLocationToFault = RandomGenerator::Get(_Log.GetConfig().GetMaxQueuedWriteDepth()) + next;
            }
            else
            {
                KInvariant(_InjectionConstraint == InjectionConstraint::CheckpointOnly);
            }
        }

        if (_State == 1)
        {
            //* State 1: Logically "wait" for the next record header to be written
            NTSTATUS status;

            status = recHdr->ChecksumIsValid(IoBuffer.First()->QuerySize());
            if (! NT_SUCCESS(status))
            {
                // Can't be a header
                return FALSE;
            }

            // Have a valid record header and metadata
            if (_InjectionConstraint == InjectionConstraint::CheckpointOnly)
            {
                if (recHdr->LogStreamId != RvdDiskLogConstants::CheckpointStreamId())
                {
                    return FALSE;
                }

                // We have a trigger for injecting a checkpoint fault
                _State = 2;

                // Compute lsn block address to be dropped with in the current cp record
                _RecordLocationToFault = recHdr->Lsn.Get()
                    + ((RvdDiskLogConstants::ToNumberOfBlocks(RandomGenerator::Get(IoBuffer.QuerySize())) - 1)
                        * RvdDiskLogConstants::BlockSize);

                _FileLocationToFault = RvdLogRecordHeader::ToFileAddress(_RecordLocationToFault, _Log.GetAvailableLsnSpace());
            }
            else
            {
                // We have a user stream
                KAssert((_InjectionConstraint == InjectionConstraint::UserRecordHeaderOnly) ||
                        (_InjectionConstraint == InjectionConstraint::UserRecordIoBufferDataOnly));

                // "Waiting" for user record at or beyond _RecordLocationToFault
                RvdLogUserStreamRecordHeader*   userStreamRec = (RvdLogUserStreamRecordHeader*)recHdr;
                if (userStreamRec->RecordType != RvdLogUserStreamRecordHeader::Type::UserRecord)
                {
                    // Not a user record - not interested in stream CP records
                    return FALSE;
                }

                if (recHdr->Lsn < _RecordLocationToFault)
                {
                    // Not at trigger point yet
                    return FALSE;
                }

                // We have a trigger on the subject user record
                _State = 2;

                if (_InjectionConstraint == InjectionConstraint::UserRecordHeaderOnly)
                {
                    _RecordLocationToFault = recHdr->Lsn;
                    _FileLocationToFault = Offset;
                }
                else
                {
                    // Compute offset into I/O buffer (if there is one) to actually fault - in LSN-space
                    if (recHdr->IoBufferSize == 0)
                    {
                        // Special case: no I/O buffer to fault - wait for next user record
                        _State = 1;
                        return FALSE;
                    }

                    // Compute block lsn address within the page-aligned user data to be dropped
                    _RecordLocationToFault = recHdr->Lsn.Get()
                        + ((RvdDiskLogConstants::ToNumberOfBlocks(recHdr->ThisHeaderSize + RandomGenerator::Get(recHdr->IoBufferSize)) - 1)
                            *  RvdDiskLogConstants::BlockSize);

                    _FileLocationToFault = RvdLogRecordHeader::ToFileAddress(_RecordLocationToFault, _Log.GetAvailableLsnSpace());
                }
            }

            // Capture the header for the injected fault
            FaultedRecordHeader::SPtr   faultedRecord = _new(KTL_TAG_TEST, GetThisAllocator()) FaultedRecordHeader();
            KMemCpySafe(&faultedRecord->_Blk[0], sizeof(faultedRecord->_Blk), recHdr, RvdDiskLogConstants::BlockSize);
            _FaultedRecords.Append(faultedRecord);
        }

        if (_State == 2)
        {
            //* State 2: Wait for _FileLocationToFault to be in the range of the current write

            // Determine if the _FileLocationToFault is within the current write range
            if ((_FileLocationToFault < Offset) || (_FileLocationToFault >= (Offset + IoBuffer.QuerySize())))
            {
                // Not in range
                return FALSE;
            }

            // The trigger point is within the current write range
            UnsafeComputeAndStartWrite(Offset, IoBuffer, Completion, ParentAsync, Async);
            _State = 99;
            return TRUE;
        }

        KInvariant(_State == 99);
    }

    return FALSE;
}

VOID
KInjectorBlockDevice::UnsafeComputeAndStartWrite(
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async)
{
    KInvariant(((_RecordLocationToFault.Get() / RvdDiskLogConstants::BlockSize) * RvdDiskLogConstants::BlockSize) == _RecordLocationToFault.Get());
    KInvariant(RvdLogRecordHeader::ToFileAddress(_RecordLocationToFault, _Log.GetAvailableLsnSpace()) == _FileLocationToFault);

    InterceptedWriteOp::SPtr    interceptOp = _new(KTL_TAG_TEST, GetThisAllocator()) InterceptedWriteOp(*this);
    KInvariant(interceptOp != nullptr);

    FaultedRegionDescriptor d;
    d._FaultType = FaultedRegionDescriptor::Type::DroppedWrite;
    d._StartOfRegion = _RecordLocationToFault;
    d._RegionSize = RvdDiskLogConstants::BlockSize;
    _FaultedRegions.Append(d);

    // Compute the write plan to drop a blk @_FileLocationToFault
    KInvariant((_FileLocationToFault - Offset) <= MAXULONG);
    ULONG                   offsetInIoBufferToDrop = (ULONG)(_FileLocationToFault - Offset);
    ULONGLONG               writeOffset0 = 0;
    KIoBufferView::SPtr     partialBufferView0;
    ULONGLONG               writeOffset1 = 0;
    KIoBufferView::SPtr     partialBufferView1;
    NTSTATUS                status;

    KInvariant(((offsetInIoBufferToDrop / RvdDiskLogConstants::BlockSize) * RvdDiskLogConstants::BlockSize) == offsetInIoBufferToDrop);
    KInvariant(offsetInIoBufferToDrop < IoBuffer.QuerySize());

    if (offsetInIoBufferToDrop > 0)
    {
        // First block not being dropped - write from 1st blk out to the drop point
        status = KIoBufferView::Create(partialBufferView0, GetThisAllocator(), KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));

        // Map 1st block to write and remaining offset
        writeOffset0 = Offset;
        partialBufferView0->SetView(IoBuffer, 0, offsetInIoBufferToDrop);

        // Write any remaining blks after the drop point
        writeOffset1 = Offset + offsetInIoBufferToDrop + RvdDiskLogConstants::BlockSize;
        ULONG       sizeToWrite = IoBuffer.QuerySize() - (offsetInIoBufferToDrop + RvdDiskLogConstants::BlockSize);
        if (sizeToWrite > 0)
        {
            // There's more data after the dropped blk @_FileLocationToFault
            status = KIoBufferView::Create(partialBufferView1, GetThisAllocator(), KTL_TAG_TEST);
            KInvariant(NT_SUCCESS(status));

            partialBufferView1->SetView(IoBuffer, offsetInIoBufferToDrop + RvdDiskLogConstants::BlockSize, sizeToWrite);
        }
    }
    else
    {
        // Dropping 1st blk - write any blks after that
        KAssert(offsetInIoBufferToDrop == 0);

        ULONG   sizeToWrite = IoBuffer.QuerySize() - RvdDiskLogConstants::BlockSize;

        if (sizeToWrite > 0)
        {
            writeOffset0 = Offset + RvdDiskLogConstants::BlockSize;
            status = KIoBufferView::Create(partialBufferView0, GetThisAllocator(), KTL_TAG_TEST);
            KInvariant(NT_SUCCESS(status));

            partialBufferView0->SetView(IoBuffer, RvdDiskLogConstants::BlockSize, sizeToWrite);
        }
    }

    interceptOp->StartInterceptedWrite(
        Async,
        Completion,
        ParentAsync,
        writeOffset0,
        partialBufferView0,
        writeOffset1,
        partialBufferView1,
        KAsyncContextBase::CompletionCallback(this, &KInjectorBlockDevice::OnInterceptedWriteComplete),
        nullptr);
}

VOID
KInjectorBlockDevice::OnInterceptedWriteComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    KInvariant(NT_SUCCESS(CompletingSubOp.Status()));
    KInvariant(_IsInterceptWrite);
    _IsInterceptWrite = FALSE;
}

//* KInjectorBlockDevice::WriteOpInceptor implementation
KInjectorBlockDevice::WriteOpInceptor::WriteOpInceptor()
{
}

KInjectorBlockDevice::WriteOpInceptor::~WriteOpInceptor()
{
}

NTSTATUS
KInjectorBlockDevice::WriteOpInceptor::Create(ULONG Tag, KAllocator& Allocator, KInjectorBlockDevice::WriteOpInceptor::SPtr& Result)
{
    UNREFERENCED_PARAMETER(Tag);

    Result = _new(KTL_TAG_TEST, Allocator) WriteOpInceptor();
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

VOID
KInjectorBlockDevice::WriteOpInceptor::OnCompletedIntercepted()
{
    DisableIntercept();
}

//* KInjectorBlockDevice::InterceptedWriteOp implemewntation
KInjectorBlockDevice::InterceptedWriteOp::InterceptedWriteOp(KInjectorBlockDevice& Owner)
    :   _Owner(Owner)
{
}

KInjectorBlockDevice::InterceptedWriteOp::~InterceptedWriteOp()
{
}

VOID
KInjectorBlockDevice::InterceptedWriteOp::StartInterceptedWrite(
    __in KAsyncContextBase::SPtr InterceptedWrite,
    __in KAsyncContextBase::CompletionCallback InterceptedWriteCompletion,
    __in_opt KAsyncContextBase* const InterceptedWriteParentAsync,
    __in ULONGLONG Offset0,
    __in KIoBufferView::SPtr WriteView0,
    __in ULONGLONG Offset1,
    __in KIoBufferView::SPtr WriteView1,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    _OffsetOfWrite0 = Offset0;
    _WriteView0 = WriteView0;
    _OffsetOfWrite1 = Offset1;
    _WriteView1 = WriteView1;
    _InterceptedKAsync = InterceptedWrite;
    _InterceptedWriteCompletion = InterceptedWriteCompletion;
    _InterceptedWriteParentAsync = InterceptedWriteParentAsync;

    Start(ParentAsync, Completion);
}

VOID
KInjectorBlockDevice::InterceptedWriteOp::OnStart()
{
    NTSTATUS        status = STATUS_SUCCESS;

    status = WriteOpInceptor::Create(KTL_TAG_TEST, GetThisAllocator(), _Interceptor);
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        Complete(status);
        return;
    }

    _NumberOfWrites = 0;
    _Interceptor->EnableIntercept(*_InterceptedKAsync);

    ((KInjectorBlockDevice::InterceptedWriteOp&)(*_InterceptedKAsync)).Start(_InterceptedWriteParentAsync, _InterceptedWriteCompletion);

    if (_WriteView0 != nullptr)
    {
        status = ScheduleUnderlyingWrite(_OffsetOfWrite0, _WriteView0);
        if (!NT_SUCCESS(status))
        {
            return;
        }
    }
    else
    {
        // A null write has occured
        Complete(STATUS_SUCCESS);
        return;
    }

    if (_WriteView1 != nullptr)
    {
        status = ScheduleUnderlyingWrite(_OffsetOfWrite1, _WriteView1);
        if (!NT_SUCCESS(status))
        {
            return;
        }
    }
}

NTSTATUS
KInjectorBlockDevice::InterceptedWriteOp::ScheduleUnderlyingWrite(ULONGLONG OffsetOfWrite, KIoBufferView::SPtr& WriteView)
{
    KAssert(WriteView != nullptr);

    NTSTATUS status = _Owner._UnderlyingDevice->Write(
        OffsetOfWrite,
        *WriteView,
        KAsyncContextBase::CompletionCallback(this, &KInjectorBlockDevice::InterceptedWriteOp::OnUnderlyingWriteComplete),
        this,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return(status);
    }

    _NumberOfWrites++;
    return STATUS_SUCCESS;
}

VOID
KInjectorBlockDevice::InterceptedWriteOp::OnUnderlyingWriteComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    KAssert(_NumberOfWrites > 0);
    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        Complete(status);
    }

    _NumberOfWrites--;
    if (_NumberOfWrites == 0)
    {
        Complete(STATUS_SUCCESS);
    }
}

VOID
KInjectorBlockDevice::InterceptedWriteOp::OnCompleted()
{
    // All underlying write activity is done and Status() is the overall status of the op

    // trigger completion of intecepted async. See InterceptedWriteOp::OnCompletedIntercepted()
    ((KInjectorBlockDevice::InterceptedWriteOp&)(*_InterceptedKAsync)).Complete(Status());
}

//* KInjectorBlockDevice::FaultedRecordHeader implementation
KInjectorBlockDevice::FaultedRecordHeader::FaultedRecordHeader()
{
}

KInjectorBlockDevice::FaultedRecordHeader::~FaultedRecordHeader()
{
}

RvdLogRecordHeader&
KInjectorBlockDevice::FaultedRecordHeader::GetLogRecordHeader()
{
    return *((RvdLogRecordHeader*)&_Blk[0]);
}

BOOLEAN
KInjectorBlockDevice::FaultedRecordHeader::IsUserStreamRecord()
{
    return GetLogRecordHeader().LogStreamId != RvdDiskLogConstants::CheckpointStreamId();
}

RvdLogUserStreamRecordHeader&
KInjectorBlockDevice::FaultedRecordHeader::GetUserStreamRecordHeader()
{
    KInvariant(IsUserStreamRecord());
    return *((RvdLogUserStreamRecordHeader*)&_Blk[0]);
}

BOOLEAN
KInjectorBlockDevice::FaultedRecordHeader::IsCheckpointStreamRecord()
{
    return GetLogRecordHeader().LogStreamId == RvdDiskLogConstants::CheckpointStreamId();
}

RvdLogPhysicalCheckpointRecord&
KInjectorBlockDevice::FaultedRecordHeader::GetCheckpointRecordHeader()
{
    KInvariant(IsCheckpointStreamRecord());
    return *((RvdLogPhysicalCheckpointRecord*)&_Blk[0]);
}

BOOLEAN
KInjectorBlockDevice::FaultedRecordHeader::IsUserRecord()
{
    if (IsUserStreamRecord() && (GetUserStreamRecordHeader().RecordType == RvdLogUserStreamRecordHeader::Type::UserRecord))
    {
        return TRUE;
    }

    return FALSE;
}

RvdLogUserRecordHeader&
KInjectorBlockDevice::FaultedRecordHeader::GetUserRecordHeader()
{
    KInvariant(IsUserRecord());
    return *((RvdLogUserRecordHeader*)&_Blk[0]);
}

BOOLEAN
KInjectorBlockDevice::FaultedRecordHeader::IsUserStreamCheckpointRecord()
{
    if (IsUserStreamRecord() && (GetUserStreamRecordHeader().RecordType == RvdLogUserStreamRecordHeader::Type::CheckPointRecord))
    {
        return TRUE;
    }

    return FALSE;
}

RvdLogUserStreamCheckPointRecordHeader&
KInjectorBlockDevice::FaultedRecordHeader::GetUserStreamCheckpointRecordHeader()
{
    KInvariant(IsUserStreamCheckpointRecord());
    return *((RvdLogUserStreamCheckPointRecordHeader*)&_Blk[0]);
}
