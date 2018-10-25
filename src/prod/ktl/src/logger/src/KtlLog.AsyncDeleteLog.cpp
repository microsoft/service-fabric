/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLog.AsyncDeleteLog.cpp

    Description:
      RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"


//** Delete Implementation
NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::CreateAsyncDeleteLog(__out AsyncDeleteLog::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncDeleteLog(this);
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

RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::AsyncDeleteLog(
    __in RvdLogManagerImp::RvdOnDiskLog::SPtr Owner)
    :   _Owner(Ktl::Move(Owner))
{
}

RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::~AsyncDeleteLog()
{
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::StartDelete(
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::OnStart()
//
// Continued from StartMount()
{
	KDbgCheckpointWDataInformational((KActivityId)_Owner->_LogId.Get().Data1, "AsyncDeleteLog Start", STATUS_SUCCESS,
		(ULONGLONG)0,
		(ULONGLONG)this,
		(ULONGLONG)0,
		(ULONGLONG)0);
	
    AcquireLock(
        _Owner->_CreateOpenDeleteGate, 
        LockAcquiredCallback(this, &AsyncDeleteLog::GateAcquireComplete));
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::DoComplete(__in NTSTATUS Status)
{
	KDbgCheckpointWDataInformational((KActivityId)_Owner->_LogId.Get().Data1, "AsyncDeleteLog Complete", STATUS_SUCCESS,
		(ULONGLONG)0,
		(ULONGLONG)this,
		(ULONGLONG)0,
		(ULONGLONG)0);
	
    ReleaseLock(_Owner->_CreateOpenDeleteGate);
    Complete(Status);
}

VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::GateAcquireComplete(
    __in BOOLEAN IsAcquired,
    __in KAsyncLock& LockAttempted)
//
// Continues from OnStart()
{
    UNREFERENCED_PARAMETER(LockAttempted);
    
    NTSTATUS status;

    if (!IsAcquired)
    {
        // Reacquire attempt
        AcquireLock(
            _Owner->_CreateOpenDeleteGate, 
            LockAcquiredCallback(this, &AsyncDeleteLog::GateAcquireComplete));

        return;
    }

    if (_Owner->_IsOpen)
    {
        // Underlying RvdLogOnDisk has been opened already - indicate file busy
        DoComplete(STATUS_SHARING_VIOLATION);
        return;
    }
 
    // Start the delete process
    status = KVolumeNamespace::DeleteFileOrDirectory(
        _Owner->_FullyQualifiedLogName,
        GetThisAllocator(),
        CompletionCallback(this, &AsyncDeleteLog::LogFileDeleteComplete),
        this);

    if (!K_ASYNC_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // Continued @ LogFileDeleteComplete()
}
    
VOID
RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::LogFileDeleteComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continues from GateAcquireComplete()
{
    UNREFERENCED_PARAMETER(Parent);
    
    KAssert(!_Owner->_IsOpen);
    KAssert(!_Owner->_BlockDevice);
    DoComplete(CompletingSubOp.Status());
}
