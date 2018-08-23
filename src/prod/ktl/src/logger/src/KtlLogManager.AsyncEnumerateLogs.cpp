/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogManager.AsyncEnumerateLogs.cpp

    Description:
      RvdLogManager::AsyncEnumerateLogs implementation.

--*/

#include "InternalKtlLogger.h"


//** EnumerateLogs
class RvdLogManagerImp::AsyncEnumerateLogsImp : public RvdLogManager::AsyncEnumerateLogs
{
public:
    AsyncEnumerateLogsImp(RvdLogManagerImp* Owner);

    VOID
    StartEnumerateLogs(
        __in const KGuid& DiskId,
        __out KArray<RvdLogId>& LogIdArray,
        __in KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

private:
    VOID
    DoComplete(NTSTATUS Status);

    VOID
    OnStart();

    VOID
    EnumCompletionCallback(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);
    
private:
    RvdLogManagerImp::SPtr      _LogManager;

    // Per operation state
    KGuid                       _DiskId;
    KArray<RvdLogId>*           _LogIdArray;
    KWString                    _DirectoryName;
    KVolumeNamespace::NameArray _NameArray;
};

NTSTATUS
RvdLogManagerImp::CreateAsyncEnumerateLogsContext(__out AsyncEnumerateLogs::SPtr& Context)
//
// Public Factory
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncEnumerateLogsImp(this);
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

RvdLogManager::AsyncEnumerateLogs::AsyncEnumerateLogs()
{
}

RvdLogManager::AsyncEnumerateLogs::~AsyncEnumerateLogs()
{
}

RvdLogManagerImp::AsyncEnumerateLogsImp::AsyncEnumerateLogsImp(RvdLogManagerImp* Owner)
    : _LogManager(Owner),
      _NameArray(GetThisAllocator()),
      _DirectoryName(GetThisAllocator())
{
}

VOID
RvdLogManagerImp::AsyncEnumerateLogsImp::StartEnumerateLogs(
    __in const KGuid& DiskId,
    __out KArray<RvdLogId>& LogIdArray,
    __in KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    // BUG, richhas, xxxxx, validate _out params as !IsOnStack()...
    _DiskId = DiskId;
    _LogIdArray = &LogIdArray;

    LogIdArray.Clear();
    _NameArray.Clear();

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::AsyncEnumerateLogsImp::DoComplete(NTSTATUS Status)
{
    _LogManager->ReleaseActivity();     // Tell hosting log manager we are done with it
                                        // See OnStart()
    _LogIdArray = nullptr;
    Complete(Status);
}

VOID
RvdLogManagerImp::AsyncEnumerateLogsImp::OnStart()
//
// Continued from StartEnumerateLogs()
{
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

	NTSTATUS status;
#if !defined(PLATFORM_UNIX)     
    status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(
        _DiskId,
        _DirectoryName);
#else
    _DirectoryName = L"";
#endif

    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    _DirectoryName += &RvdDiskLogConstants::DirectoryName();
    if (!NT_SUCCESS(_DirectoryName.Status()))
    {
        DoComplete(_DirectoryName.Status());
        return;
    }

    status = KVolumeNamespace::QueryFiles(
        _DirectoryName,
        _NameArray,
        GetThisAllocator(),
        KAsyncContextBase::CompletionCallback(this, &AsyncEnumerateLogsImp::EnumCompletionCallback),
        this);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoComplete(status);
    }
}

VOID
RvdLogManagerImp::AsyncEnumerateLogsImp::EnumCompletionCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
//
// Continuation from OnStart()
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletingSubOp.Status();
    if (NT_SUCCESS(status))
    {
        // BUG, richhas, xxxxx, review with others to find a more direct way if possible of parsing...
        //                      From Peng: "You could first skip the prefix to the GUID part, call
        //                                  RtlGUIDFromString() to get the GUID, then construct a valid
        //                                  log file name based on the log GUID, then compare if the two
        //                                  name strings match."

        // Walk thru each _NameArray item (unicode filename) and extract those that are
        // formatted as valid log file names into the the result array, _LogIdArray.
        for (ULONG ix = 0; ix < _NameArray.Count(); ix++)
        {
            UNICODE_STRING s = _NameArray[ix];
            if (s.Length == (RvdDiskLogConstants::LogFileNameLength * sizeof(WCHAR)))
            {
                // Overall length of strings match - verify proper prefix
                UNICODE_STRING t;
                t.Buffer = (PWCH)(&RvdDiskLogConstants::NamePrefix());
                t.Length = (s.Length = (RvdDiskLogConstants::NamePrefixSize * sizeof(WCHAR)));
                t.MaximumLength = t.Length + sizeof(WCHAR);

                if (RtlCompareUnicodeString(&t, &s, TRUE) == 0)
                {
                    // Prefix matches - verify proper extention
                    PWCH sBuffer = s.Buffer;        // saved buffer base*

                    t.Buffer = (PWCH)(&RvdDiskLogConstants::NameExtension());
                    s.Buffer += RvdDiskLogConstants::LogFileNameExtensionOffset;
                    t.Length = (s.Length = RvdDiskLogConstants::NameExtensionSize);
                    t.MaximumLength = t.Length + sizeof(WCHAR);

                    if (RtlCompareUnicodeString(&t, &s, TRUE) == 0)
                    {
                        // extension matches - try and extract a {GUID}
                        s.Buffer = sBuffer + RvdDiskLogConstants::LogFileNameGuidOffset;
                        s.Length = (RvdDiskLogConstants::LogFileNameGuidStrLength * sizeof(WCHAR));
                        s.MaximumLength = s.Length + sizeof(WCHAR);
                        sBuffer[RvdDiskLogConstants::LogFileNameExtensionOffset] = UNICODE_NULL;

                        GUID logGuid;
                        if (NT_SUCCESS(RtlGUIDFromString(&s, &logGuid)))
                        {
                            // Have a valid {GUID} - try and append into the result set
                            _LogIdArray->Append(RvdLogId(KGuid(logGuid)));
                            status = _LogIdArray->Status();
                            if (!NT_SUCCESS(status))
                            {
                                // Either everything or nothing returned
                                _LogIdArray->Clear();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    DoComplete(status);
}
