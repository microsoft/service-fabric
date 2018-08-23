/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLog.AsyncCreateLog.cpp

    Description:
      RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

#include "ktrace.h"


//** CreateLog Implementation
NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::CreateAsyncCreateLog(__out AsyncCreateLog::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncCreateLog(this);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;    
}

RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::AsyncCreateLog(
    RvdLogManagerImp::RvdOnDiskLog::SPtr Owner)
    :   _Owner(Ktl::Move(Owner))
{
}

RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::~AsyncCreateLog()
{
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::StartCreateLog(
    __in RvdLogConfig& Config,
    __in KWString& LogType,
    __in LONGLONG const LogSize,
    __in KStringView& OptionalLogFileLocation,
    __in DWORD CreationFlags,
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    // BUG, richhas, xxxxx, add log type support
    _LogType = &LogType;
    _LogSize = LogSize;
    _ConfigParams = Config;
    _CreationFlags = CreationFlags;
    _OptionalLogFileLocation = nullptr;
    if (OptionalLogFileLocation.Length() > 0)
    {
        _OptionalLogFileLocation = &OptionalLogFileLocation;
    }

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::DoComplete(__in NTSTATUS Status)
{
    if (!_Owner->_IsOpen)
    {
        // Only perserve an underlying open _BlockDevice if we achieved "mounted"
        // state - see FlushComplete() for the (only) successful creation path exit.
        _Owner->_BlockDevice.Reset();
    }

    ReleaseLock(_Owner->_CreateOpenDeleteGate);
    Complete(Status);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::OnStart()
//
//  Continued from StartCreateLog
{
    // Validate creation parameters
    //
    if (!NT_SUCCESS(_ConfigParams.Status()))
    {
        KTraceFailedAsyncRequest(_ConfigParams.Status(), this, 0, 0);
        Complete(_ConfigParams.Status());
        return;
    }

    if ((_LogSize < 0) || ((ULONGLONG)_LogSize < _ConfigParams.GetMinFileSize()))
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, this, _LogSize, _ConfigParams.GetMinFileSize());
        Complete(STATUS_INVALID_PARAMETER);
        return;
    }

    if ((_LogSize % RvdDiskLogConstants::BlockSize) != 0)
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, this, _LogSize, RvdDiskLogConstants::BlockSize);
        Complete(STATUS_INVALID_PARAMETER);
        return;
    }

    _Owner->_CreationFlags = _CreationFlags;

    _LogSize = (((_LogSize + _ConfigParams.GetMaxQueuedWriteDepth() - 1) 
                / _ConfigParams.GetMaxQueuedWriteDepth()) 
                * _ConfigParams.GetMaxQueuedWriteDepth())
                    + (2 * RvdDiskLogConstants::MasterBlockSize);

    AcquireLock(
        _Owner->_CreateOpenDeleteGate, 
        LockAcquiredCallback(this, &AsyncCreateLog::GateAcquireComplete));
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::GateAcquireComplete(
    __in BOOLEAN IsAcquired,
    __in KAsyncLock& LockAttempted)
//
// Continues from OnStart()
{
    UNREFERENCED_PARAMETER(LockAttempted);

    if (!IsAcquired)
    {
        // Reacquire attempt
        AcquireLock(
            _Owner->_CreateOpenDeleteGate, 
            LockAcquiredCallback(this, &AsyncCreateLog::GateAcquireComplete));

        return;
    }

    // NOTE: Beyond this point all completion MUST be done thru DoComplete() - must release _Owner->_CreateOpenDeleteGate
    if (_Owner->_IsOpen)
    {
        DoComplete(STATUS_OBJECT_NAME_COLLISION);
        return;
    }

    KAssert(_Owner->_Streams.IsEmpty());


    //
    // Try and create the Log directory
    //
    NTSTATUS            status = STATUS_SUCCESS;
    KWString            fullyQualifiedDirName(GetThisAllocator());

#if !defined(PLATFORM_UNIX)
    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(_Owner->_DiskId, fullyQualifiedDirName);
#else
    fullyQualifiedDirName = L"";
    status = fullyQualifiedDirName.Status();
#endif
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    if (_OptionalLogFileLocation != nullptr)
    {
        KWString path(GetThisAllocator());
        KWString filename(GetThisAllocator());
        status = KVolumeNamespace::SplitObjectPathInPathAndFilename(GetThisAllocator(),
                                                  *_OptionalLogFileLocation,
                                                  path,
                                                  filename);
        if (NT_SUCCESS(status))
        {
            
            KWString rootPath(GetThisAllocator(), KVolumeNamespace::PathSeparator);
        
            status = rootPath.Status();        
            if (! NT_SUCCESS(status))
            {
                DoComplete(status);
                return;
            }
            
            if (path == rootPath)
            {
                //
                // Logs in the root directory are a special case
                //
                OnLogDirCreateComplete();
                return;
            }
            
            fullyQualifiedDirName += path;
            status = fullyQualifiedDirName.Status();
        }
    } else {
        fullyQualifiedDirName += &RvdDiskLogConstants::DirectoryName();
        status = fullyQualifiedDirName.Status();
    }
        
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }
        
    status = KVolumeNamespace::CreateDirectory(
        fullyQualifiedDirName, 
        GetThisAllocator(),
        CompletionCallback(this, &AsyncCreateLog::LogDirCreateComplete),
        this);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoComplete(status);
    }

    // Continued @LogDirCreateComplete()
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::LogDirCreateComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continues from GateAcquireComplete()
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS            status = CompletingSubOp.Status();

    if (!NT_SUCCESS(status))
    {
        if (status != STATUS_OBJECT_NAME_COLLISION)
        {
             DoComplete(status);
             return;
        }

        // Directory was already present
    }
    // Continued @OnLogDirCreateComplete
    OnLogDirCreateComplete();
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::OnLogDirCreateComplete(
)
{
    NTSTATUS status;

    KAssert(!_Owner->_IsOpen);
    // BUG, richhas, xxxxx, consider making sure that there is zero chance that an old (deleted)
    //                      log file could be re-allocated into the exactly the file being created
    //                      below. We should make sure that the header of the file is zeroed (or written)
    //                      before the length is set; allocating the entire file.

    // Start the creation process
    status = _Owner->CompleteActivation(_ConfigParams);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    BOOLEAN useSparseFile = ((_CreationFlags & RvdLogManager::AsyncCreateLog::FlagSparseFile) == RvdLogManager::AsyncCreateLog::FlagSparseFile) ? TRUE : FALSE;
    ULONG createOptions = (useSparseFile ? static_cast<ULONG>(KBlockFile::eNoIntermediateBuffering) : 0) |
                                           static_cast<ULONG>(KBlockFile::CreateOptions::eShareRead);
    
    // Start creation of the actual log file
    status = KBlockDevice::CreateDiskBlockDevice(
        _Owner->_DiskId,
        _LogSize,
        TRUE,
        TRUE,
        _Owner->_FullyQualifiedLogName,
        _Owner->_BlockDevice,
        CompletionCallback(this, &AsyncCreateLog::LogFileCreateComplete),
        this,
        GetThisAllocator(),
        KTL_TAG_LOGGER,
        NULL,
        FALSE,
        useSparseFile,
        RvdLogManagerImp::RvdOnDiskLog::ReadAheadSize,
        static_cast<KBlockFile::CreateOptions>(createOptions));

    if (!K_ASYNC_SUCCESS(status))
    {
        DoComplete(status);
    }

    // Continued @LogFileCreateComplete
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::LogFileCreateComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continues from LogDirCreateComplete()
{
   UNREFERENCED_PARAMETER(Parent);

    KAssert(!_Owner->_IsOpen);
    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        // CreateDiskBlockDevice() failed
        DoComplete(status);
        return;
    }

    KAssert(_Owner->_BlockDevice);
    union
    {
        void*               masterBlkBuffer;
        RvdLogMasterBlock*  masterBlk;
    };

    status = KIoBuffer::CreateSimple(
        RvdDiskLogConstants::MasterBlockSize, 
       _MasterBlkIoBuffer,
        masterBlkBuffer,
        GetThisAllocator(),
        KTL_TAG_LOGGER);

    if (!NT_SUCCESS(status))
    {
        DoCompleteWithDelete(status);
        return;
    }

    // BUG, richhas, xxxxx, consider writing both master blocks with all zeros (and RaW Verify) before
    //                      we attempt to write the two master blocks. This is aimed at destroying any
    //                      old master blocks before a partial failure could occur writing the two new
    //                      ones.

    // Build up a master block; also capture the unique log file sig locally
    // which is needed to stamp all log records
    RtlZeroMemory(masterBlkBuffer, RvdDiskLogConstants::MasterBlockSize);
    masterBlk->MajorVersion = RvdDiskLogConstants::MajorFormatVersion;
    masterBlk->MinorVersion = RvdDiskLogConstants::MinorFormatVersion; 
    masterBlk->LogFormatGuid = RvdDiskLogConstants::LogFormatGuid();
    masterBlk->LogId = _Owner->_LogId;
    masterBlk->LogFileSize = _LogSize;
    masterBlk->CreationFlags = _CreationFlags;

    new(&masterBlk->GeometryConfig) RvdLogOnDiskConfig(_Owner->_Config);
    masterBlk->MasterBlockLocation = 0;
    masterBlk->CreationDirectory[0] = 0;        // Only default location supported at this point

    if (_OptionalLogFileLocation != nullptr)
    {
        // Aliased log file path name is provided
        // Once Header::CreationDirectory contains an alias, all attempts will be made to verify its existance
        // with auto-deletion of the corresponding log occuring if it does not exist.
        KAssert((_Owner->_FullyQualifiedLogName.Length() > 0) && NT_SUCCESS(_Owner->_FullyQualifiedLogName.Status()));
        KMemCpySafe(
            &masterBlk->CreationDirectory[0], 
            sizeof(masterBlk->CreationDirectory), 
            (WCHAR*)_Owner->_FullyQualifiedLogName, 
            _Owner->_FullyQualifiedLogName.Length());
    }
    
    _Owner->_LogFileSize = _LogSize;
    _Owner->_LogFileLsnSpace = _LogSize - (2 * RvdDiskLogConstants::BlockSize);
    _Owner->_LogFileFreeSpace = _Owner->_LogFileLsnSpace;
    _Owner->_LogFileReservedSpace = 0;
    _Owner->_LogFileMinFreeSpace = _Owner->_Config.GetMinFreeSpace();
    KAssert(_Owner->_LogFileMinFreeSpace < _Owner->_LogFileFreeSpace);

    // Write LogType into master block
    ULONG lenSrc = _LogType->Length();
    ULONG lenDest = (RvdLogManager::AsyncCreateLog::MaxLogTypeLength) * sizeof(WCHAR);
    if (lenSrc > lenDest)
    {
        lenSrc = lenDest;
    }

    KMemCpySafe(
        &masterBlk->LogType[0],
        lenDest,
        _LogType->Buffer(),
        lenSrc);
    masterBlk->LogType[(lenDest-1)/sizeof(WCHAR)] = 0;

    KMemCpySafe(
        &_Owner->_LogType[0], 
        sizeof(_Owner->_LogType), 
        &masterBlk->LogType[0], 
        RvdLogManager::AsyncCreateLog::MaxLogTypeLength * sizeof(WCHAR));

    ULONG seed = (ULONG)KNt::GetTickCount64();
    for (int i = 0; i < RvdDiskLogConstants::SignatureULongs; i++) 
    {
        ULONG r = RtlRandomEx(&seed);
        masterBlk->LogSignature[i] = r;
        _Owner->_LogSignature[i] = r;
    }

    // Checksum of this header.
    masterBlk->ThisBlockChecksum = 0;
    ULONGLONG chksum = KChecksum::Crc64(masterBlkBuffer, RvdDiskLogConstants::MasterBlockSize, 0);
    masterBlk->ThisBlockChecksum = chksum;

    // Write master block at start of the log file
    status = _Owner->_BlockDevice->Write(
        0, 
        *_MasterBlkIoBuffer, 
        KAsyncContextBase::CompletionCallback(this, &AsyncCreateLog::HeaderWriteComplete),
        this);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoCompleteWithDelete(status);
        return;
    }
    // Continued @HeaderWriteComplete
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::HeaderWriteComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
//  Continued from LogFileCreateComplete
{
   UNREFERENCED_PARAMETER(Parent);

    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
        DoCompleteWithDelete(CompletingSubOp.Status());
        return;
    }

    // Build up a master block copy
    union
    {
        void const*         masterBlkBuffer;
        RvdLogMasterBlock*  masterBlk;
    };

    masterBlkBuffer = _MasterBlkIoBuffer->First()->GetBuffer();
    masterBlk->MasterBlockLocation = _LogSize - RvdDiskLogConstants::BlockSize;

    // Checksum of this header.
    masterBlk->ThisBlockChecksum = 0;
    ULONGLONG chksum = KChecksum::Crc64(masterBlkBuffer, RvdDiskLogConstants::MasterBlockSize, 0);
    masterBlk->ThisBlockChecksum = chksum;

    // Fire off write of the master block copy at the end of the file
    NTSTATUS status = _Owner->_BlockDevice->Write(
        masterBlk->MasterBlockLocation, 
        *_MasterBlkIoBuffer, 
        KAsyncContextBase::CompletionCallback(this, &AsyncCreateLog::TrailerWriteComplete),
        this);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoCompleteWithDelete(status);
        return;
    }
    // Continued @TrailerWriteComplete
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::TrailerWriteComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
//  Continued from HeaderWriteComplete
{
   UNREFERENCED_PARAMETER(Parent);

    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
        DoCompleteWithDelete(CompletingSubOp.Status());
        return;
    }

    // Finish the Create
    _Owner->_LowestLsn = RvdLogLsn::Min();
    _Owner->_NextLsnToWrite = RvdLogLsn::Min();
    _Owner->_HighestCompletedLsn = RvdLogLsn::Null();

    // Create the dedicated checkpoint stream
    RvdLogStreamImp::SPtr       resultStream;
    LogStreamDescriptor*        resultStreamDesc;
    NTSTATUS                    status = _Owner->FindOrCreateStream(
        TRUE, 
        RvdDiskLogConstants::CheckpointStreamId(),
        RvdDiskLogConstants::CheckpointStreamType(), 
        resultStream, 
        resultStreamDesc);

    if (!NT_SUCCESS(status))
    {
        DoCompleteWithDelete(status);
        return;
    }

    // The checkpoint stream has been opened - snap the address of the underlying
    // checkpoint stream's LogStreamDescriptor for Log. NOTE: once opened, this
    // LogStreamDescriptor is never closed per-se.
    _Owner->_CheckPointStream = &(static_cast<RvdLogStreamImp*>(resultStream.RawPtr()))->GetLogStreamDescriptor();

    // We have to steal the underlying stream state but drop the stream api object...
    resultStream.Reset();

    // Allocate the async op instance used to write physical log record into _CheckPointStream
    status = RvdLogStreamImp::AsyncWriteStream::CreateCheckpointStreamAsyncWriteContext(
        GetThisAllocator(),
        *(_Owner.RawPtr()),
        _Owner->_PhysicalCheckPointWriteOp);

    if (!NT_SUCCESS(status))
    {
        DoCompleteWithDelete(CompletingSubOp.Status());
        return;
    }

    // The Log has been successfully initialized on disk and mounted.
    // NOTE: By setting _IsOpen = TRUE we unlock this instance for all other
    //       operations.
    _Owner->_IsOpen = TRUE;
    DoComplete(STATUS_SUCCESS);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::DoCompleteWithDelete(__in NTSTATUS Status)
//
//  Complete the current op with Status AFTER the underlying file and optional hardlink is proven to have
//  been deleted
{
    KAssert(_Owner->_BlockDevice);
    _DoCompleteWithDeleteStatus = Status;

    // Cause closure of the underlying file object
    _Owner->_BlockDevice.Reset();

    NTSTATUS        status = STATUS_SUCCESS;

    // Start the delete process
    status = KVolumeNamespace::DeleteFileOrDirectory(
        _Owner->_FullyQualifiedLogName,
        GetThisAllocator(),
        CompletionCallback(this, &AsyncCreateLog::LogFileDeleteComplete),
        this);

    if (!K_ASYNC_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(_DoCompleteWithDeleteStatus);
        return;
    }
    // Continued @LogFileDeleteComplete
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::LogFileDeleteComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
//  Continued from DoCompleteWithDelete() or LogFileDeleteComplete()
{
   UNREFERENCED_PARAMETER(Parent);

    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
        KTraceFailedAsyncRequest(CompletingSubOp.Status(), &CompletingSubOp, 0, 0);
        DoComplete(_DoCompleteWithDeleteStatus);

        return;
    }

    DoComplete(_DoCompleteWithDeleteStatus);
}
