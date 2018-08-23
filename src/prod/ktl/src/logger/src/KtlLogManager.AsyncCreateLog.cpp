/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogManager.AsyncCreateLog.cpp

    Description:
      RvdLogManager::AsyncCreateLog implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"



//** CreateLog
class RvdLogManagerImp::AsyncCreateLogImp : public RvdLogManager::AsyncCreateLog
{
    K_FORCE_SHARED(AsyncCreateLogImp);

public:
    AsyncCreateLogImp(RvdLogManagerImp* Owner);

    VOID
    StartCreateLog(
        __in KGuid const& DiskId,
        __in RvdLogId const& LogId,
        __in KWString& LogType,
        __in LONGLONG LogSize,
        __out RvdLog::SPtr& Result,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;
    
    VOID
    StartCreateLog(
        __in KGuid const& DiskId,
        __in RvdLogId const& LogId,
        __in KWString& LogType,
        __in LONGLONG LogSize,
        __in ULONG MaxAllowedStreams,
        __in ULONG MaxRecordSize,
        __out RvdLog::SPtr& Log,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    VOID
    StartCreateLog(
        __in KStringView const& FullyQualifiedLogFilePath,
        __in RvdLogId const& LogId,
        __in KWString& LogType,
        __in LONGLONG LogSize,
        __in ULONG MaxAllowedStreams,
        __in ULONG MaxRecordSize,
        __out RvdLog::SPtr& Log,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    VOID
    StartCreateLog(
        __in KGuid const& DiskId,
        __in RvdLogId const& LogId,
        __in KWString& LogType,
        __in LONGLONG LogSize,
        __in ULONG MaxAllowedStreams,
        __in ULONG MaxRecordSize,
        __in DWORD CreationFlags,
        __out RvdLog::SPtr& Log,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;
    
    VOID
    StartCreateLog(
        __in KStringView const& FullyQualifiedLogFilePath,
        __in RvdLogId const& LogId,
        __in KWString& LogType,
        __in LONGLONG LogSize,
        __in ULONG MaxAllowedStreams,
        __in ULONG MaxRecordSize,
        __in DWORD CreationFlags,
        __out RvdLog::SPtr& Log,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

private:
    VOID
    DoComplete(NTSTATUS Status);

    VOID
    OnStart() override;

    VOID
    OnStartContinue();

	NTSTATUS
	VerifyNotReservedDirectory(
		__out KWString& Path
		);
	
#if !defined(PLATFORM_UNIX)
    VOID
    OnQueryVolumeIdCompleted(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);
#endif
    
    VOID
    CreateCompletionCallback(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);

private:
    RvdLogManagerImp::SPtr                  _LogManager;
    NTSTATUS                                _StartStatus;

    // Per operation state
    DWORD                                   _CreationFlags;
    RvdLogConfig                            _Config;
    KGuid                                   _DiskId;
    RvdLogId                                _LogId;
    KWString*                               _LogType;
    LONGLONG                                _LogSize;
    KWString                                _FilePath;
    KStringView                             _RelativeFilePath;
    RvdLog::SPtr*                           _ResultLogPtr;
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _OnDiskLog;
};

NTSTATUS
RvdLogManagerImp::CreateAsyncCreateLogContext(__out AsyncCreateLog::SPtr& Context)
//
// Public Factory
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncCreateLogImp(this);
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

RvdLogManager::AsyncCreateLog::AsyncCreateLog()
{
}

RvdLogManager::AsyncCreateLog::~AsyncCreateLog()
{
}

RvdLogManagerImp::AsyncCreateLogImp::AsyncCreateLogImp(RvdLogManagerImp* Owner)
    :   _LogManager(Owner),
        _FilePath(GetThisAllocator())
{
}

RvdLogManagerImp::AsyncCreateLogImp::~AsyncCreateLogImp()
{
}

VOID
RvdLogManagerImp::AsyncCreateLogImp::StartCreateLog(
    __in KGuid const& DiskId,
    __in RvdLogId const& LogId,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __out RvdLog::SPtr& Result,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)

{
    StartCreateLog(DiskId, LogId, LogType, LogSize, 0, 0, Result, ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::AsyncCreateLogImp::StartCreateLog(
    __in KGuid const& DiskId,
    __in RvdLogId const& LogId,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __in ULONG MaxAllowedStreams,
    __in ULONG MaxRecordSize,
    __in DWORD CreationFlags,
    __out RvdLog::SPtr& Result,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _StartStatus = STATUS_SUCCESS;

    if (MaxAllowedStreams == 0)
    {
        MaxAllowedStreams = RvdLogConfig::DefaultMaxNumberOfStreams;
    }

    if (MaxRecordSize == 0)
    {
        MaxRecordSize = RvdLogConfig::DefaultMaxRecordSize;
    }

    RvdLogConfig    config(MaxAllowedStreams, MaxRecordSize);

#ifdef ALWAYS_SPARSE_FILES
    CreationFlags |= FlagSparseFile;
#endif
    _Config = config;
    _DiskId = DiskId;
    _LogId = LogId;
    _LogType = &LogType;
    _LogSize = LogSize;
    _CreationFlags = CreationFlags;
    _ResultLogPtr = &Result;
    Result.Reset();
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::AsyncCreateLogImp::StartCreateLog(
    __in KGuid const& DiskId,
    __in RvdLogId const& LogId,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __in ULONG MaxAllowedStreams,
    __in ULONG MaxRecordSize,
    __out RvdLog::SPtr& Result,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _StartStatus = STATUS_SUCCESS;

    if (MaxAllowedStreams == 0)
    {
        MaxAllowedStreams = RvdLogConfig::DefaultMaxNumberOfStreams;
    }

    if (MaxRecordSize == 0)
    {
        MaxRecordSize = RvdLogConfig::DefaultMaxRecordSize;
    }

    RvdLogConfig    config(MaxAllowedStreams, MaxRecordSize);

    DWORD creationFlags = 0;
#ifdef ALWAYS_SPARSE_FILES
    creationFlags |= FlagSparseFile;
#endif
    _CreationFlags = creationFlags;
    _Config = config;
    _DiskId = DiskId;
    _LogId = LogId;
    _LogType = &LogType;
    _LogSize = LogSize;
    _ResultLogPtr = &Result;
    Result.Reset();
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::AsyncCreateLogImp::StartCreateLog(
    __in KStringView const& FullyQualifiedLogFilePath,
    __in RvdLogId const& LogId,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __in ULONG MaxAllowedStreams,
    __in ULONG MaxRecordSize,
    __out RvdLog::SPtr& Result,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _StartStatus = STATUS_SUCCESS;

    if (MaxAllowedStreams == 0)
    {
        MaxAllowedStreams = RvdLogConfig::DefaultMaxNumberOfStreams;
    }

    if (MaxRecordSize == 0)
    {
        MaxRecordSize = RvdLogConfig::DefaultMaxRecordSize;
    }

    RvdLogConfig    config(MaxAllowedStreams, MaxRecordSize);

    _FilePath = FullyQualifiedLogFilePath;
    _StartStatus = _FilePath.Status();
    if (NT_SUCCESS(_StartStatus))
    {
        _FilePath += WCHAR(0);
        _StartStatus = _FilePath.Status();
    }

    DWORD creationFlags = 0;
#ifdef ALWAYS_SPARSE_FILES
    creationFlags |= FlagSparseFile;
#endif
    _CreationFlags = creationFlags;
    _Config = config;
    _LogId = LogId;
    _LogType = &LogType;
    _LogSize = LogSize;
    _ResultLogPtr = &Result;
    Result.Reset();
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::AsyncCreateLogImp::StartCreateLog(
    __in KStringView const& FullyQualifiedLogFilePath,
    __in RvdLogId const& LogId,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __in ULONG MaxAllowedStreams,
    __in ULONG MaxRecordSize,
    __in DWORD CreationFlags,
    __out RvdLog::SPtr& Result,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _StartStatus = STATUS_SUCCESS;

    if (MaxAllowedStreams == 0)
    {
        MaxAllowedStreams = RvdLogConfig::DefaultMaxNumberOfStreams;
    }

    if (MaxRecordSize == 0)
    {
        MaxRecordSize = RvdLogConfig::DefaultMaxRecordSize;
    }

    RvdLogConfig    config(MaxAllowedStreams, MaxRecordSize);

    _FilePath = FullyQualifiedLogFilePath;
    _StartStatus = _FilePath.Status();
    if (NT_SUCCESS(_StartStatus))
    {
        _FilePath += WCHAR(0);
        _StartStatus = _FilePath.Status();
    }

#ifdef ALWAYS_SPARSE_FILES
    CreationFlags |= FlagSparseFile;
#endif

    _Config = config;
    _LogId = LogId;
    _LogType = &LogType;
    _LogSize = LogSize;
    _CreationFlags = CreationFlags;
    _ResultLogPtr = &Result;
    Result.Reset();
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}


VOID
RvdLogManagerImp::AsyncCreateLogImp::DoComplete(NTSTATUS Status)
{
    _LogManager->ReleaseActivity();     // Tell hosting log manager we are done with it
                                        // See OnStart()
    _OnDiskLog.Reset();
    Complete(Status);
}

VOID
RvdLogManagerImp::AsyncCreateLogImp::OnStart()
{
    if (!NT_SUCCESS(_StartStatus))
    {
        Complete(_StartStatus);
        return;
    }

    if (!NT_SUCCESS(_Config.Status()))
    {
        // Bad Config
        Complete(_Config.Status());
        return;
    }

    // Acquire an activity slot (if possible) - keeps _LogManager from
    // completing if successful - otherwise there's a shutdown condition occuring
    // in the _LogManager.
    if (!_LogManager->TryAcquireActivity())
    {
        Complete(K_STATUS_SHUTDOWN_PENDING);
        return;
    }
    // NOTE: Beyond this point all completion is done thru DoComplete() to
    //       Release the acquired activity slot for this operation.
    NTSTATUS status = STATUS_SUCCESS;
    
    if (_FilePath.Length() > 0)
    {
#if !defined(PLATFORM_UNIX)
        // Must extract the DiskId from FQN and store the volume-relative back into _VolumeRelativeFilePath
        status = KVolumeNamespace::QueryVolumeIdFromRootDirectoryName(
            _FilePath,
            GetThisAllocator(),
            _DiskId,
            CompletionCallback(this, &AsyncCreateLogImp::OnQueryVolumeIdCompleted),
            this);

        if (!K_ASYNC_SUCCESS(status))
        {
            DoComplete(status);
            return;
        }

        // Continued @ OnQueryVolumeIdCompleted
        return;
#else
        _RelativeFilePath.KStringView::KStringView((UNICODE_STRING)_FilePath);
        _DiskId = RvdDiskLogConstants::HardCodedVolumeGuid();

        KWString            path(GetThisAllocator());
        status = VerifyNotReservedDirectory(path);
        if (! NT_SUCCESS(status))
        {
            DoComplete(status);
            return;
        }       
#endif
    }

    OnStartContinue();
}

NTSTATUS
RvdLogManagerImp::AsyncCreateLogImp::VerifyNotReservedDirectory(
    __out KWString& Path
    )
{
	NTSTATUS status;
    KWString            vol(GetThisAllocator());
    status = KVolumeNamespace::SplitAndNormalizeObjectPath(_FilePath, vol, Path);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    // Disallow use of reserved directory
    if (RvdDiskLogConstants::IsReservedDirectory(KStringView((UNICODE_STRING)Path)))
    {
        return(STATUS_ACCESS_DENIED);
    }
    return(STATUS_SUCCESS);
}

#if !defined(PLATFORM_UNIX)
VOID
RvdLogManagerImp::AsyncCreateLogImp::OnQueryVolumeIdCompleted(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletingSubOp.Status();     // capture completion status locally
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // Have DiskId - now get volume relative path
    KWString            path(GetThisAllocator());
    status = VerifyNotReservedDirectory(path);
    if (! NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }
    
    _FilePath = path;
    KAssert(NT_SUCCESS(_FilePath.Status()));
    _RelativeFilePath.KStringView::KStringView((UNICODE_STRING)_FilePath);
    OnStartContinue();
}
#endif

VOID
RvdLogManagerImp::AsyncCreateLogImp::OnStartContinue()
{
    RvdOnDiskLog::SPtr logObject;
    NTSTATUS status = _LogManager->FindOrCreateLog(TRUE, _DiskId, _LogId, _FilePath, logObject);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // Create the log asynchronously.
    RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLog::SPtr createContext;
    status = logObject->CreateAsyncCreateLog(createContext);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // We are going to start the async create - pass along the constructed RvdOnDiskLog sptr
    _OnDiskLog = Ktl::Move(logObject);

    KAsyncContextBase::CompletionCallback callback;
    callback.Bind(this, &AsyncCreateLogImp::CreateCompletionCallback);

    createContext->StartCreateLog(_Config, *_LogType, _LogSize, _RelativeFilePath, _CreationFlags, this, callback);
}

VOID
RvdLogManagerImp::AsyncCreateLogImp::CreateCompletionCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletingSubOp.Status();     // capture completion status locally
    if (NT_SUCCESS(status))
    {
        (*_ResultLogPtr) = Ktl::Move(reinterpret_cast<RvdLog::SPtr&>(_OnDiskLog));       // return resulting sptr to RvdLog
    }

    DoComplete(status);
}
