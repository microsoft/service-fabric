// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

//
// Streams table
//
GlobalStreamsTable::GlobalStreamsTable(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    ) :
        GlobalTable<OverlayStreamFreeService, KtlLogStreamId>(Allocator, AllocationTag)
{
}

GlobalStreamsTable::~GlobalStreamsTable()
{
}

NTSTATUS
GlobalStreamsTable::CreateOrGetOverlayStream(
    __in BOOLEAN ReadOnly,                                           
    __in KtlLogStreamId& StreamId,
    __in KGuid& DiskId,
    __in_opt KString const * const Path,
    __in ULONG MaxRecordSize,
    __in LONGLONG StreamSize,
    __in ULONG StreamMetadataSize,
    __out OverlayStreamFreeService::SPtr& OS,
    __out BOOLEAN& StreamCreated
    )
{
    UNREFERENCED_PARAMETER(ReadOnly);
    
    NTSTATUS status;
    OverlayStreamFreeService::SPtr os;

    StreamCreated = FALSE;
    
    K_LOCK_BLOCK(GetLock())
    {
        //
        // While under lock determine if the container
        // object already exists and if so then take a ref
        // or if it doesn't exist create it and start it up
        //
        status = LookupObjectNoLock(os,
                                    StreamId);
        if (NT_SUCCESS(status))
        {
            //
            // Stream is already on the list which means that it is in
            // use by another actor. At this point we are enforcing
            // exclusive access
            //
            // TODO: At some point allow read only access to the stream
            //
            return(STATUS_SHARING_VIOLATION);
        } else if (status == STATUS_NOT_FOUND) {
            //
            // Container doesn't exist so create one and
            // add it to the list while under lock
            //
            os = _new(_AllocationTag, _Allocator) OverlayStreamFreeService(StreamId,
                                                                           DiskId,
                                                                           Path,
                                                                           MaxRecordSize,
                                                                           StreamSize,
                                                                           StreamMetadataSize);
            if (! os)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                return(status);
            }

            status = os->Status();
            if (! NT_SUCCESS(status))
            {
                KDbgCheckpointWData(0, "CreateOrGetOverlayStream", status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);
                return(status);
            }

            //
            // As part of creation, add to the global containers list
            //
            status = AddObjectNoLock(os, StreamId);
            if (! NT_SUCCESS(status))
            {
                KDbgCheckpointWData(0, "CreateOrGetOverlayStream", status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);
                return(status);
            }

            //
            // Activate the service wrapper processing queue
            //
            status = os->ActivateQueue();
            if (! NT_SUCCESS(status))
            {
                //
                // In case of an error, pull this off the list
                //
                RemoveObjectNoLock(os, StreamId);
                
                KDbgCheckpointWData(0, "CreateOrGetOverlayStream", status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);
                return(status);
            }

            //
            // Bump up the on list refcount
            //
            os->IncrementOnListCount();
            
            StreamCreated = TRUE;
        } else {
            return(status);
        }
    }
    
    OS = Ktl::Move(os);
    
    return(STATUS_SUCCESS);
}

NTSTATUS
GlobalStreamsTable::FindOverlayStream(
    __in const KtlLogStreamId& StreamId,
    __out OverlayStreamFreeService::SPtr& OS
    )
{
    NTSTATUS status;
    OverlayStreamFreeService::SPtr os;
    
    K_LOCK_BLOCK(GetLock())
    {
        //
        // While under lock determine if the container
        // object already exists and if so then take a ref
        //
        status = LookupObjectNoLock(os,
                                    StreamId);
        if (NT_SUCCESS(status))
        {
            os->IncrementOnListCount();            
        } else {
            return(status);
        }
    }
    
    OS = Ktl::Move(os);
    
    return(STATUS_SUCCESS);
}

BOOLEAN
GlobalStreamsTable::ReleaseOverlayStream(
    __in OverlayStreamFreeService& OS
    )
{
    BOOLEAN RemoveFromList = FALSE;
    OverlayStreamFreeService::SPtr overlayStream = &OS;
    
    K_LOCK_BLOCK(GetLock())
    {
        RemoveFromList = overlayStream->DecrementOnListCount();
        if (RemoveFromList)
        {
            NTSTATUS status;
            OverlayStreamFreeService::SPtr os;

            status = RemoveObjectNoLock(os, overlayStream->GetStreamId());
#if DBG
            KAssert(NT_SUCCESS(status));
            KAssert(os.RawPtr() == overlayStream.RawPtr());
#else
            UNREFERENCED_PARAMETER(status);
#endif
        }
    }

    return(RemoveFromList);
}

NTSTATUS
GlobalStreamsTable::GetFirstStream(
    __out OverlayStreamFreeService::SPtr& OS
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;

    K_LOCK_BLOCK(GetLock())
    {
        KtlLogStreamId key;
        PVOID index;
        
        status = GetFirstObjectNoLock(OS, key, index);
    }

    return(status);
}


//
// Containers table
//
GlobalContainersTable::GlobalContainersTable(
    __in OverlayManager& OM,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    ) :
        GlobalTable<OverlayLogFreeService, KtlLogContainerId>(Allocator, AllocationTag),
        _OverlayManager(OM)
{
}

GlobalContainersTable::~GlobalContainersTable()
{
}

NTSTATUS
GlobalContainersTable::CreateOrGetOverlayContainer(
    __in const KGuid& DiskId,
    __in const KtlLogContainerId& ContainerId,
    __out OverlayLogFreeService::SPtr& OC,
    __out BOOLEAN& ContainerCreated
    )
{
    NTSTATUS status;
    OverlayLogFreeService::SPtr oc;

    ContainerCreated = FALSE;
    
    K_LOCK_BLOCK(GetLock())
    {
        //
        // While under lock determine if the container
        // object already exists and if so then take a ref
        // or if it doesn't exist create it and start it up
        //
        status = LookupObjectNoLock(oc,
                                    ContainerId);
        if (NT_SUCCESS(status))
        {
            //
            // Container already exists
            //
            oc->IncrementOnListCount();
        } else if (status == STATUS_NOT_FOUND) {
            //
            // Container doesn't exist so create one and
            // add it to the list while under lock
            //
            oc = _new(_AllocationTag, _Allocator) OverlayLogFreeService(_OverlayManager,
                                                                        *_BaseLogManager,
                                                                        DiskId,
                                                                        ContainerId);
            if (! oc)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                return(status);
            }

            status = oc->Status();
            if (! NT_SUCCESS(status))
            {
                KDbgCheckpointWData(0, "CreateOrGetOverlayContainer", status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);
                return(status);
            }

            //
            // As part of creation, add to the global containers list
            //
            status = AddObjectNoLock(oc, ContainerId);
            if (! NT_SUCCESS(status))
            {
                KDbgCheckpointWData(0, "CreateOrGetOverlayContainer", status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);
                return(status);
            }

            //
            // Activate the service wrapper processing queue
            //
            status = oc->ActivateQueue();
            if (! NT_SUCCESS(status))
            {
                //
                // In case of an error, pull this off the list
                //
                RemoveObjectNoLock(oc, ContainerId);
                
                KDbgCheckpointWData(0, "CreateOrGetOverlayContainer", status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);
                return(status);
            }

            //
            // Bump up the on list refcount
            //
            oc->IncrementOnListCount();

            ContainerCreated = TRUE;
        } else {
            return(status);
        }
    }
    
    OC = Ktl::Move(oc);
    
    return(STATUS_SUCCESS);
}

BOOLEAN
GlobalContainersTable::ReleaseOverlayContainer(
    __in OverlayLogFreeService& OC
    )
{
    BOOLEAN RemoveFromList = FALSE;
    OverlayLogFreeService::SPtr overlayContainer = &OC;
    
    K_LOCK_BLOCK(GetLock())
    {
        RemoveFromList = overlayContainer->DecrementOnListCount();
        if (RemoveFromList)
        {
            NTSTATUS status;
            OverlayLogFreeService::SPtr oc;

            status = RemoveObjectNoLock(oc, overlayContainer->GetContainerId());
#if DBG
            KAssert(NT_SUCCESS(status));
            KAssert(oc.RawPtr() == overlayContainer.RawPtr());
#else
            UNREFERENCED_PARAMETER(status);
#endif
        }
    }

    return(RemoveFromList);
}

NTSTATUS
GlobalContainersTable::GetFirstContainer(
    __out OverlayLogFreeService::SPtr& OC
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;

    K_LOCK_BLOCK(GetLock())
    {
        KtlLogContainerId key;
        PVOID index;
        
        status = GetFirstObjectNoLock(OC, key, index);
        if (NT_SUCCESS(status))
        {
            OC->IncrementOnListCount();
        }
    }

    return(status);
}

const ULONG ActiveContext::_LinkOffset = FIELD_OFFSET(ActiveContext, _ListEntry);

ActiveContext::ActiveContext(
    ) :
    _Marshaller(NULL),
    _IsCancelled(FALSE),
    _IsStarted(FALSE),
    _Parent(NULL)
{
}

ActiveContext::~ActiveContext()
{
}
