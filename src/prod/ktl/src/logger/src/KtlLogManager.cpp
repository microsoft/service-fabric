/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogManager.cpp

    Description:
      RvdLogManager implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

//** RvdLogManager implementation
RvdLogManager::RvdLogManager()
{
}

RvdLogManager::~RvdLogManager()
{
}

NTSTATUS
RvdLogManager::Create(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out RvdLogManager::SPtr& Result)
{
    Result = _new(AllocationTag, Allocator) RvdLogManagerImp(32);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }

    return status;
}

//** RvdLogManagerImp Implementation
RvdLogManagerImp::RvdLogManagerImp(__in ULONG MaxNumberOfLogsHint)
    :   _OnDiskLogObjectArray(GetThisAllocator(), MaxNumberOfLogsHint),
        _ActivityCount(0),
        _IsActive(FALSE),
        _CallbackArray(100, &K_DefaultHashFunction, GetThisAllocator()),
        _DiskRecoveryGates(GetThisAllocator()),
        _Lock(1, 1)            // only allow 1 thread at a time
{
    NTSTATUS status;

    status = _OnDiskLogObjectArray.Status();
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = _CallbackArray.Status();
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

RvdLogManagerImp::~RvdLogManagerImp()
{
    KInvariant(_ActivityCount == 0);

    // Validate that all gates have been opened if present - guarentees no waitors
    for (ULONG ix = 0; ix < _DiskRecoveryGates.Count(); ix++)
    {
        KInvariant(_DiskRecoveryGates[ix]->_Event.IsSignaled());
    }
}

NTSTATUS
RvdLogManagerImp::Activate(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
//
//  Description: This method starts an operation on an RvdLogManager instance. A RvdLogManager
//               acts as the "root" of a complete Log Service and hosts the all other objects
//               making up such a service. As long as the operation started by this method is
//               active, the corresponding Log Service instance is available - assuming no other
//               fault conditions. This active state is terminated via a call to Deactivate().
//               As with any KAsyncContext derivation, this will eventually cause the supplied
//               completion callback (CallbackPtr) to be invoked - optionally within a parent
//               KAsyncContext's dispatch apartment.
//
//  Parameters:  ParentAsyncContext - Optional parent KAsyncContext
//               CallbackPtr        - Optional completion (Deactivation complete) callback
//
//  Return:      STATUS_PENDING     - Service is operational
//
{
    KAssert(!_IsActive);
    _IsActive = TRUE;

    // A ref count representing that this instance is active; the API is "locked"
    // while _ActivityCount == 0  || !_IsActive. So this also unlocks the API.
    KInvariant(_InterlockedCompareExchange(&_ActivityCount, 1, 0) == 0);

    Start(ParentAsyncContext, CallbackPtr);
    return STATUS_PENDING;
}

VOID
RvdLogManagerImp::OnStart()
{
}

VOID
RvdLogManagerImp::Deactivate()
{
    KAssert(_IsActive);
    _IsActive = FALSE;              // Lock the API

    Cancel();
}

VOID
RvdLogManagerImp::OnCancel()
{
    KInvariant(!_IsActive);             // trap Cancel calls not from Deactivate()
    ReleaseActivity();              // Allow Eventual Complete() call
}

VOID
RvdLogManagerImp::DiskLogDestructed()
//
// Called by "child" RvdOnDiskLog as they destruct - they are no longer an "activity"
{
    ReleaseActivity();
}

NTSTATUS
RvdLogManagerImp::FindOrCreateLog(
    __in BOOLEAN const MustCreate,
    __in KGuid& DiskId,
    __in RvdLogId& LogId,
    __in KWString& OptionalLoggerPath,
    __out KSharedPtr<RvdOnDiskLog>& ResultRvdLog)
//
//  Common logic for maintaining _OnDiskLogObjectArray for log open, create, and delete Async Contexts
//
//  Returns:
//      STATUS_SUCCESS:                 A ref counted RvdOnDiskLog* is returned via ResultRvdLog
//      STATUS_INSUFFICIENT_RESOURCES:  RvdOnDiskLog could not be allocated - out of memory
//      STATUS_OBJECT_NAME_COLLISION:   Attempt to Create an existing log
//
{
    KAssert(_ActivityCount > 0);
    ResultRvdLog.Reset();

    KGuid               diskId;
    RvdLogId            logId;
    RvdOnDiskLog::SPtr  logObject;
    KWString            fullyQualifiedLogName(GetThisAllocator());
    NTSTATUS            status;

    if (OptionalLoggerPath.Length() == 0)
    {
        status = RvdDiskLogConstants::BuildFullyQualifiedLogName(DiskId, LogId, fullyQualifiedLogName);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    } else {
#if !defined(PLATFORM_UNIX)
        status = KVolumeNamespace::CreateFullyQualifiedRootDirectoryName(
                    DiskId,
                    fullyQualifiedLogName);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
#endif

        fullyQualifiedLogName += OptionalLoggerPath;
    }

    status = fullyQualifiedLogName.Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    _Lock.Subtract();
    KFinally([this]()
    {
        _Lock.Add();
    });
    {
        // Try to find an existing log object.
        LONG lastFreeElement = -1;
        for (ULONG index = 0; index < _OnDiskLogObjectArray.Count(); index++)
        {
            if (_OnDiskLogObjectArray[index].WRef)
            {
                // Have WeakRef::SPtr @ _OnDiskLogObjectArray[index].WRef for potentially matching LogId
                logObject = _OnDiskLogObjectArray[index].WRef->TryGetTarget();
                if (logObject)
                {
                    // Have logObject
                    logObject->QueryLogId(diskId, logId);
                    if ((diskId == DiskId) && (logId == LogId))
                    {
                        // At this point we have found an existing RvdOnDiskLog matching the requested log.
                        // And we own a reference on that instance - meaning it will not go away so we may leave
                        // this lock block.
                        break;
                    }
                    else
                    {
#if KTL_USER_MODE
#else
                        //
                        // Verify that this is not being called at raised IRQL
                        //
                        KInvariant(KeGetCurrentIrql() <= APC_LEVEL);
#endif

                        logObject.Reset();      // May cause destruction
                    }
                }
                else
                {
                    // Found a stranded weak ref - free it
                    _OnDiskLogObjectArray[index].WRef.Reset();
                    lastFreeElement = index;
                }
            }
            else
            {
                lastFreeElement = index;
            }
        }

        if (!logObject)
        {
            // BUG, richhas, xxxxx, limit the number of total logs to be <= MaxNumberOfDisksPerNode*MaxNumberOfLogsPerDisk

            // Create new log object
            logObject = _new(KTL_TAG_LOGGER, GetThisAllocator()) RvdOnDiskLog(this, DiskId, LogId, fullyQualifiedLogName);
            if (!logObject)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            // Account for the created RvdOnDiskLog instance. Beyond this point DiskLogDestructed()
            // will be called for the ctor'd RvdOnDiskLog; which in turn calls ReleaseActivity() which
            // closes this call to TryAcquireActivity().
            KInvariant(TryAcquireActivity());

            NTSTATUS status1 = logObject->Status();
            if (!NT_SUCCESS(status1))
            {
                logObject.Reset();          // dtor called
                return status1;
            }

            // Store weak ref sptr in the array
            DiskLogWRef newDiskLogWRef = logObject->GetWeakRef();
            if (lastFreeElement != -1)
            {
                // Reuse an empty array slot
                KAssert(!_OnDiskLogObjectArray[lastFreeElement].WRef);
                _OnDiskLogObjectArray[lastFreeElement].WRef = Ktl::Move(newDiskLogWRef);
                _OnDiskLogObjectArray[lastFreeElement].Key = LogId;
            }
            else
            {
                KeyedDiskLogWRef item;
                item.Key = LogId;
                item.WRef = Ktl::Move(newDiskLogWRef);
                status1 = _OnDiskLogObjectArray.Append(Ktl::Move(item));
                if (!NT_SUCCESS(status1))
                {
                    return status1;
                }
            }
        }
        else if (MustCreate)
        {
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }

    ResultRvdLog = Ktl::Move(logObject);
    return STATUS_SUCCESS;
}

//** RegisterVerificationCallback
NTSTATUS
RvdLogManagerImp::RegisterVerificationCallback(
    __in const RvdLogStreamType& LogStreamType,
    __in VerificationCallback Callback)
{
    return _CallbackArray.Put(LogStreamType.Get(), Callback);
}

//** UnRegisterVerificationCallback
RvdLogManager::VerificationCallback
RvdLogManagerImp::UnRegisterVerificationCallback(__in const RvdLogStreamType& LogStreamType)
{
    RvdLogManager::VerificationCallback result;
    NTSTATUS    status = _CallbackArray.Remove(LogStreamType.Get(), &result);

    if (!NT_SUCCESS(status))
    {
        result.Reset();
    }

    return result;
}

//** QueryVerificationCallback
RvdLogManager::VerificationCallback
RvdLogManagerImp::QueryVerificationCallback(__in const RvdLogStreamType& LogStreamType)
{
    RvdLogManager::VerificationCallback result;
    NTSTATUS    status = _CallbackArray.Get(LogStreamType.Get(), result);

    if (!NT_SUCCESS(status))
    {
        result.Reset();
    }

    return result;
}

