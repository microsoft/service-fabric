// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

AliasToStreamEntry::AliasToStreamEntry(
    __in KtlLogStreamId& LogStreamId
    ) : _StreamId(LogStreamId)
{
}

AliasToStreamEntry::~AliasToStreamEntry()
{
}

ContainerAliasToStreamTable::ContainerAliasToStreamTable(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    ) :
        GlobalTable<AliasToStreamEntry, KString::CSPtr>(Allocator, AllocationTag)
{
}

ContainerAliasToStreamTable::~ContainerAliasToStreamTable()
{
    Clear();
}

VOID
ContainerAliasToStreamTable::Clear()
{
    NTSTATUS status = STATUS_SUCCESS;
    AliasToStreamEntry::SPtr aliasEntry;
    KString::CSPtr alias;
    
    K_LOCK_BLOCK(GetLock())
    {
        while (NT_SUCCESS(status))
        {
            PVOID index;
            
            status = GetFirstObjectNoLock(aliasEntry, alias, index);
            if (NT_SUCCESS(status))
            {
                RemoveObjectNoLock(aliasEntry, alias);
            }
        }
    }
}

StreamToAliasEntry::StreamToAliasEntry(
    __in KString const & Alias
    ) : _Alias(&Alias)
{
}

StreamToAliasEntry::~StreamToAliasEntry()
{
}

ContainerStreamToAliasTable::ContainerStreamToAliasTable(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    ) :
        GlobalTable<StreamToAliasEntry, KtlLogStreamId>(Allocator, AllocationTag)
{
}

ContainerStreamToAliasTable::~ContainerStreamToAliasTable()
{
    Clear();
}

VOID
ContainerStreamToAliasTable::Clear()
{
    NTSTATUS status = STATUS_SUCCESS;
    StreamToAliasEntry::SPtr aliasEntry;
    KtlLogStreamId streamId;
    
    K_LOCK_BLOCK(GetLock())
    {
        while (NT_SUCCESS(status))
        {
            PVOID index;
            
            status = GetFirstObjectNoLock(aliasEntry, streamId, index);
            if (NT_SUCCESS(status))
            {
                RemoveObjectNoLock(aliasEntry, streamId);
            }
        }
    }
}


NTSTATUS
OverlayLog::LookupAlias(
    __in KString const & Alias,
    __out KtlLogStreamId& LogStreamId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AliasToStreamEntry::SPtr aliasEntry;
    KString::CSPtr alias = &Alias;

    K_LOCK_BLOCK(_AliasTablesLock)
    {
        status = _AliasToStreamTable.LookupObjectNoLock(aliasEntry,
                                                        alias);
        if (NT_SUCCESS(status))
        {
            LogStreamId = aliasEntry->GetStreamId();
        }
    }
    
    return(status);
}

NTSTATUS
OverlayLog::AddOrUpdateAlias(
    __in KString const & Alias,
    __in KtlLogStreamId& LogStreamId
    )
{
    NTSTATUS status;
    AliasToStreamEntry::SPtr aliasToStreamEntry;
    StreamToAliasEntry::SPtr streamToAliasEntry;
    KString::CSPtr alias = &Alias;

    aliasToStreamEntry = _new(GetThisAllocationTag(), GetThisAllocator()) AliasToStreamEntry(LogStreamId);
    if (! aliasToStreamEntry)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, 0, 0);      
        return(status);
    }

    status = aliasToStreamEntry->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    streamToAliasEntry = _new(GetThisAllocationTag(), GetThisAllocator()) StreamToAliasEntry(Alias);
    if (! streamToAliasEntry)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, 0, 0);      
        return(status);
    }

    status = streamToAliasEntry->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Clean the alias in case it previously pointed to another stream
    //
    K_LOCK_BLOCK(_AliasTablesLock)
    {
        KtlLogStreamId logStreamId;
        RemoveAliasByAliasNoLock(*alias, logStreamId);

        status = _AliasToStreamTable.AddOrUpdateObjectNoLock(aliasToStreamEntry,
                                                             alias);
        if (NT_SUCCESS(status))
        {
            status = _StreamToAliasTable.AddOrUpdateObjectNoLock(streamToAliasEntry,
                                                                 LogStreamId);
        }
    }
        
    return(status); 
}

NTSTATUS
OverlayLog::RemoveAliasByAliasNoLock(
    __in KString const & Alias,
    __out KtlLogStreamId& StreamId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AliasToStreamEntry::SPtr aliasToStreamEntry;
    StreamToAliasEntry::SPtr streamToAliasEntry;
    KString::CSPtr alias = &Alias;

	//
	// First remove alias from the alias to stream table
	//
	status = _AliasToStreamTable.RemoveObjectNoLock(aliasToStreamEntry,
													alias);

	if (! NT_SUCCESS(status))
	{
		return(status);
	}

	//
	// Grab the stream id from the entry
	//
	StreamId = aliasToStreamEntry->GetStreamId();

	//
	// Next remove streamid from the stream to alias table
	//
	status = _StreamToAliasTable.RemoveObjectNoLock(streamToAliasEntry,
													StreamId);
    
    return(status); 
}

NTSTATUS
OverlayLog::RemoveAliasByAlias(
    __in KString const & Alias,
    __out KtlLogStreamId& StreamId
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_AliasTablesLock)
    {
		status = RemoveAliasByAliasNoLock(Alias, StreamId);
    }
    
    return(status); 
}

NTSTATUS
OverlayLog::RemoveAliasByStreamId(
    __in KtlLogStreamId& StreamId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    AliasToStreamEntry::SPtr aliasToStreamEntry;
    StreamToAliasEntry::SPtr streamToAliasEntry;
    KString::CSPtr alias;

    K_LOCK_BLOCK(_AliasTablesLock)
    {
        //
        // First remove alias from the stream to alias table
        //
        status = _StreamToAliasTable.RemoveObjectNoLock(streamToAliasEntry,
                                                        StreamId);

        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        //
        // Grab the stream id from the entry
        //
        alias = streamToAliasEntry->GetAlias();

        //
        // Next remove alias to stream table
        //
        status = _AliasToStreamTable.RemoveObjectNoLock(aliasToStreamEntry,
                                                        alias);
    }

    return(status); 
}


VOID
OverlayLog::AliasOperationContext::OnCompleted(
    )
{
    _Alias = nullptr;
}

VOID OverlayLog::AliasOperationContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLog::AliasOperationContext::OperationCompletion);
    KStringView const emptyAlias(L"");
    KStringView const * alias = NULL;

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _State = CompletedWithError;
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case InitialRemove:
        {
            //
            // Remove alias from the in memory table
            //
            Status = _OverlayLog->RemoveAliasByAlias(*_Alias,
                                                     _StreamId);
            
            
            if (! NT_SUCCESS(Status))
            {
                //
                // Alias not found
                //
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);               
                return;
            }

            //
            // Removing alias means updating the alias value to empty
            //
            alias = &emptyAlias;
            
            // Fall through
        }

        case InitialAdd:
        {
            OverlayStreamFreeService::SPtr osfs;
            ULONG entryIndex;

            //
            // Find the entry in the shared metadata so it can be
            // updated for both the add and remove cases
            //
            Status = _OverlayLog->_StreamsTable.FindOverlayStream(_StreamId,
                                                                  osfs);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }

            entryIndex = osfs->GetLCMBEntryIndex();

            BOOLEAN freed = _OverlayLog->_StreamsTable.ReleaseOverlayStream(*osfs);
            KInvariant( ! freed);       
            
            if (_State == InitialAdd)
            {
                //
                // Clean any other aliases pointing to our stream id
                //
                Status = _OverlayLog->RemoveAliasByStreamId(_StreamId);


                if (! NT_SUCCESS(Status))
                {
                    //
                    // StreamId not found
                    //
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                }
                
                //
                // Add alias to in memory table
                //
                Status = _OverlayLog->AddOrUpdateAlias(*_Alias,
                                                       _StreamId);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);               
                    return;
                }
                
                alias = _Alias.RawPtr();
            }
            
            _State = UpdateAliasEntry;
                                    
            _UpdateLCMBEntry->StartUpdateEntry(_StreamId,
                                               NULL,                     // No change to container path
                                               alias,          // TODO: Pass KString ??
                                               (SharedLCMBInfoAccess::DispositionFlags)-1,
                                               FALSE,                    // Do not remove alias
                                               entryIndex,              
                                               this,
                                               completion);                                            
            
            break;
        }

        case UpdateAliasEntry:
        {
            Complete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID OverlayLog::AliasOperationContext::StartUpdateAlias(
    __in KString const & Alias,
    __in KtlLogStreamId LogStreamId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    KInvariant(_State == Unassigned);
    
    _State = InitialAdd;
    _Alias = &Alias;
    _StreamId = LogStreamId;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID OverlayLog::AliasOperationContext::StartRemoveAlias(
    __in KString const & Alias,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    KInvariant(_State == Unassigned);

    _State = InitialRemove;
    _Alias = &Alias;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayLog::AliasOperationContext::OnStart(
    )
{
    KInvariant((_State == InitialAdd) || (_State == InitialRemove));

    FSMContinue(STATUS_SUCCESS);   
}

VOID OverlayLog::AliasOperationContext::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}
                
                
VOID
OverlayLog::AliasOperationContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLog::AliasOperationContext::OnReuse(
    )
{
    _State = Unassigned;
	_UpdateLCMBEntry->Reuse();
}

OverlayLog::AliasOperationContext::AliasOperationContext(
    __in OverlayLog& OC
    ) : _OverlayLog(&OC),
        _State(Unassigned)
{
    NTSTATUS status;
    
    status = _OverlayLog->_LCMBInfo->CreateAsyncUpdateEntryContext(_UpdateLCMBEntry);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

OverlayLog::AliasOperationContext::~AliasOperationContext()
{
}


NTSTATUS
OverlayLog::CreateAsyncAliasOperationContext(
    __out OverlayLog::AliasOperationContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLog::AliasOperationContext::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AliasOperationContext(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, 0, 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}
