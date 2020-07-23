// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#pragma prefast(push)
#pragma prefast(disable: 28167, "DevDiv:422165: Prefast not recognizing c++ dtor being called")

#include "KtlLogShimKernel.h"

#if defined(UDRIVER) || defined(KDRIVER)
FileObjectEntry::FileObjectEntry()
{
}

FileObjectEntry::~FileObjectEntry()
{
}

NTSTATUS FileObjectEntry::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,       
    __out FileObjectEntry::SPtr& FOE)
{
    NTSTATUS status;

    FOE = nullptr;
    
    FileObjectEntry::SPtr fileObjectEntry = _new(AllocationTag, Allocator) FileObjectEntry();

    if (! fileObjectEntry)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }

    status = fileObjectEntry->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    FOE = Ktl::Move(fileObjectEntry);
    return(STATUS_SUCCESS);
}


FileObjectTable::FileObjectTable(
    ) :
   _Table(FileObjectEntry::GetLinksOffset(),
          KNodeTable<FileObjectEntry>::CompareFunction(&FileObjectEntry::Comparator)),
   _RequestList(FIELD_OFFSET(RequestMarshallerKernel, _ListEntry)),
   _IoBufferElementList(FIELD_OFFSET(KIoBufferElementKernel, _ListEntry))
{
    _LookupKey = _new(GetThisAllocationTag(), GetThisAllocator()) FileObjectEntry();
    if (! _LookupKey)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    if (! NT_SUCCESS(_LookupKey->Status()))
    {
        SetConstructorStatus(_LookupKey->Status());     
    }        
}

FileObjectTable::~FileObjectTable()
{
    ClearObjects();
}


NTSTATUS
FileObjectTable::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out FileObjectTable::SPtr& FOT
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    FileObjectTable::SPtr fot;

    FOT = nullptr;
    
    fot = _new(AllocationTag, Allocator) FileObjectTable();
    if (fot == nullptr)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(fot->Status()))
    {
        return(fot->Status());
    }

    OverlayManager::SPtr overlayManager;

    LookupOverlayManager(Allocator, overlayManager);
    
    if (overlayManager)
    {
        fot->_OverlayManager = overlayManager;
    } else {
#if DBG
        KInvariant(FALSE);   // Don't bring machine down unless debugging
#endif
        status = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(status, NULL, 0, 0);
    }       
    
    fot->_ObjectIdIndex = 1;
    
    FOT = Ktl::Move(fot);
    
    return(status);
}

VOID
FileObjectTable::ClearObjects(
    )
{
    FileObjectEntry* entry = NULL;
    
    //
    // Walk the table and remove each entry as we need to remove the
    // reference to each of them so they are freed. Entries cannot be
    // cleared while holding the spinlock as the destructor may invoke
    // a Zw api that should be called at passive level
    //
    do
    {
        K_LOCK_BLOCK(_Lock)
        {
            entry = _Table.First();
            if (entry != NULL)
            {
                _Table.Remove(*entry);
            }
        }

        if (entry != NULL)
        {
            KDbgCheckpointWData(0, "Clear Object", STATUS_SUCCESS,
                                (ULONGLONG)this,
                                (ULONGLONG)entry->_ObjectHolder.GetObjectId(),
                                (ULONGLONG)0,
                                (ULONGLONG)0);
            
            entry->_ObjectHolder.ClearObject();
            entry->Release();
        }
    } while (entry != NULL);

}

VOID
FileObjectTable::AddObject(
    __in KSharedBase& ObjectPtr,
    __in_opt KSharedBase* const CloseContext,
    __in FileObjectEntry& FileObjectEntry,
    __in RequestMarshaller::OBJECT_TYPE ObjectType,
    __in ULONGLONG ObjectId
    )
{
    KSharedBase::SPtr objectPtr = &ObjectPtr;
    KSharedBase::SPtr closeContext = CloseContext;
    FileObjectEntry::SPtr fileObjectEntry = &FileObjectEntry;

    //
    // Object id is the address of the object itself. Entry will take a
    // refcount on the object which gets released when the entry is
    // removed or the table is cleared
    //
    fileObjectEntry->_ObjectHolder.SetObject(ObjectType,
                                             ObjectId,
                                             objectPtr.RawPtr(),
                                             closeContext);

    K_LOCK_BLOCK(_Lock)
    {
        if (_Table.Insert(*fileObjectEntry))
        {
            //
            // Entry added, take a refcount on the entry to keep it
            // alive while it is in the table. Ref is released when
            // removed from the table or when the table is destructed
            //
            fileObjectEntry->AddRef();

            KDbgCheckpointWData(0, "Add Object", STATUS_SUCCESS,
                                (ULONGLONG)this,
                                (ULONGLONG)ObjectId,
                                (ULONGLONG)0,
                                (ULONGLONG)0);
            
            if (ObjectType == RequestMarshaller::LogManager)
            {
                KInvariant(_LogManager == nullptr);
                
                _LogManager = up_cast<KtlLogManagerKernel>(objectPtr);

            }
        } else {
            //
            // Entry already exists - this shouldn't happen
            //
            KInvariant(FALSE);
        }
    }
}


NTSTATUS
FileObjectTable::RemoveObject(
    __in ULONGLONG ObjectId
    )
{
    FileObjectEntry* fileObjectEntry;
    
    K_LOCK_BLOCK(_Lock)
    {
        _LookupKey->_ObjectHolder.SetObjectId(ObjectId);
        
        fileObjectEntry = _Table.Lookup(*_LookupKey);
        
        if (fileObjectEntry == NULL)
        {
            return(STATUS_NOT_FOUND);
        }

        _Table.Remove(*fileObjectEntry);

        KDbgCheckpointWData(0, "Remove Object", STATUS_SUCCESS,
                            (ULONGLONG)this,
                            (ULONGLONG)ObjectId,
                            (ULONGLONG)0,
                            (ULONGLONG)0);
        
        if (fileObjectEntry->_ObjectHolder.GetObjectType() == RequestMarshaller::LogManager)
        {
            if (_LogManager->Status() != STATUS_PENDING)
            {
                KTraceFailedAsyncRequest(_LogManager->Status(), NULL, 0, 0);
            }
        }
        fileObjectEntry->_ObjectHolder.ClearObject();

        //
        // Release the reference taken when the entry was added to the
        // table.
        //
        fileObjectEntry->Release();
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
FileObjectTable::LookupObject(
    __in ULONGLONG ObjectId,
    __out KtlLogObjectHolder& ObjectHolder
    )
{
    FileObjectEntry* fileObjectEntry;
    
    K_LOCK_BLOCK(_Lock)
    {
        _LookupKey->_ObjectHolder.SetObjectId(ObjectId);
        
        fileObjectEntry = _Table.Lookup(*_LookupKey);
        
        if (fileObjectEntry == NULL)
        {

            KDbgCheckpointWData(0, "Object not found", STATUS_NOT_FOUND,
                                (ULONGLONG)this,
                                (ULONGLONG)ObjectId,
                                (ULONGLONG)0,
                                (ULONGLONG)0);

            return(STATUS_NOT_FOUND);
        }

        //
        // Take an extra refcount and pass it back to the caller.
        // Caller is reponsible to free it. Do not send back the close
        // context
        //
        ObjectHolder = fileObjectEntry->_ObjectHolder;
        ObjectHolder.ResetCloseContext();
    }
    return(STATUS_SUCCESS);
}

PVOID
FileObjectTable::LookupObjectPointer(
    __in ULONGLONG ObjectId
    )
{
    FileObjectEntry* fileObjectEntry = NULL;
    
    K_LOCK_BLOCK(_Lock)
    {
        _LookupKey->_ObjectHolder.SetObjectId(ObjectId);
        
        fileObjectEntry = _Table.Lookup(*_LookupKey);
        
        if (fileObjectEntry == NULL)
        {

            KDbgCheckpointWData(0, "Object not found", STATUS_NOT_FOUND,
                                (ULONGLONG)this,
                                (ULONGLONG)ObjectId,
                                (ULONGLONG)0,
                                (ULONGLONG)0);

            return(NULL);
        }
    }
    
    return(fileObjectEntry->_ObjectHolder.GetObjectPointer());
}

NTSTATUS
FileObjectTable::AddRequest(
    __in RequestMarshallerKernel* Request
)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_RequestLock)
    {
        status = _RequestList.Add(Request);
    }
    
    return(status);
}

VOID
FileObjectTable::RemoveRequest(
    __in RequestMarshallerKernel* Request
)
{
    K_LOCK_BLOCK(_RequestLock)
    {
        _RequestList.Remove(Request);
    }
}

VOID
FileObjectTable::CancelAllRequests(
)
{
    //
    // Ensure this is not invoked by a KTL thread
    //
    KInvariant(!GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
    
    RequestMarshallerKernel::SPtr request = nullptr;

    KDbgPrintfInformational("StartCancelAllRequests %p\n", this);
    
    _RequestList.SignalWhenEmpty();

    K_LOCK_BLOCK(_RequestLock)
    {
        request = _RequestList.PeekHead();
        while (request)
        {   
            request->_ObjectCancelCallback();

            request = _RequestList.Next(request.RawPtr());
        }
    }

    if (_RequestList.IsListEmpty())
    {
        _RequestList.WaitForEmpty();
    }
    KDbgPrintfInformational("EndCancelAllRequests %p\n", this);
}

VOID
FileObjectTable::FindRequest(
    __in ULONGLONG RequestId,
    __out RequestMarshallerKernel::SPtr& Request
    )
{
    K_LOCK_BLOCK(_RequestLock)
    {
        Request = _RequestList.Find(&RequestId);
    }
}

NTSTATUS
FileObjectTable::AddIoBufferElement(
    __in KIoBufferElementKernel& Element
)
{
    NTSTATUS status = STATUS_SUCCESS;

    K_LOCK_BLOCK(_IoBufferElementLock)
    {
        status = _IoBufferElementList.Add(&Element);
    }

    InterlockedAdd64(&_IoBufferElementsSize, Element.QuerySize());
    
    return(status);
}

VOID
FileObjectTable::RemoveIoBufferElement(
    __in KIoBufferElementKernel& Element
)
{
    InterlockedAdd64(&_IoBufferElementsSize, -1*Element.QuerySize());
    
    K_LOCK_BLOCK(_IoBufferElementLock)
    {
        _IoBufferElementList.Remove(&Element);
    }
    
}

VOID
FileObjectTable::WaitForAllIoBufferElements(
)
{
    //
    // Ensure this is not invoked by a KTL thread
    //
    KInvariant(!GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
    
    KDbgPrintfInformational("StartWaitForAllIoElements %p\n", this);
    
    _IoBufferElementList.SignalWhenEmpty(); 
    _IoBufferElementList.WaitForEmpty();
    
    KDbgPrintfInformational("EndWaitForAllIoElements %p\n", this);
}
#endif

KGlobalSpinLockStorage      FileObjectTable::gs_CreateStopOverlayManagerLock;
   
NTSTATUS FileObjectTable::CreateAndRegisterOverlayManager(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    OverlayManager::SPtr overlayManager;
    
    KDbgCheckpointWData(0, "CreateAndRegisterOverlayManager", STATUS_SUCCESS,
                        (ULONGLONG)0, (ULONGLONG)&Allocator.GetKtlSystem(), (ULONGLONG)0, 0);
    
    status = CreateAndRegisterOverlayManagerInternal(Allocator, AllocationTag, overlayManager);
    if (status == STATUS_OBJECT_NAME_COLLISION)
    {
        //
        // Already registered
        //
        return(STATUS_SUCCESS);
    }
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)&Allocator.GetKtlSystem(), (ULONGLONG)0);
        return(status);
    }

    //
    // Try to start the overlay manager
    //
    KServiceSynchronizer        serviceSync;
    status = overlayManager->StartServiceOpen(NULL,       // ParentAsync
                                              serviceSync.OpenCompletionCallback(),
                                              NULL);      // GlobalContextOverride
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)0);
        return(status);
    }

    KInvariant(! overlayManager->GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
    status = serviceSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)0);
        return(status);
    }

    KDbgCheckpointWData(0, "CreateAndRegisterOverlayManager created", STATUS_SUCCESS,
                        (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)&Allocator.GetKtlSystem(), (ULONGLONG)0, 0);
    
    return(STATUS_SUCCESS);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS> FileObjectTable::CreateAndRegisterOverlayManagerAsync(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    OverlayManager::SPtr overlayManager;

    status = CreateAndRegisterOverlayManagerInternal(Allocator, AllocationTag, overlayManager);
    if (status == STATUS_OBJECT_NAME_COLLISION)
    {
#if defined(UPASSTHROUGH)       
        //
        // Wait for the overlay manager to be fully opened
        //
        KAsyncEvent::WaitContext::SPtr waitContext;

        status = overlayManager->CreateOpenWaitContext(waitContext);
        if (! NT_SUCCESS(status))
        {
            co_return status;
        }

        status = co_await waitContext->StartWaitUntilSetAsync(nullptr);
#else
        status = STATUS_SUCCESS;
#endif
        co_return status;
    }
    
    if (! NT_SUCCESS(status))
    {
        co_return status;
    }

    ktl::kservice::OpenAwaiter::SPtr awaiter;
    status = ktl::kservice::OpenAwaiter::Create(
        Allocator,
        AllocationTag,
        *overlayManager, 
        awaiter,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)0);
        co_return status;
    }

    status = co_await *awaiter;

    co_return status;    
}
#endif

#define OVERLAY_MANAGER L"OVERLAY_MANAGER"

NTSTATUS FileObjectTable::CreateAndRegisterOverlayManagerInternal(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out OverlayManager::SPtr &OM
)
{
    NTSTATUS status = STATUS_SUCCESS;
    //
    // Create and register the overlay manager
    //
    KtlLogManager::MemoryThrottleLimits memoryThrottleLimits;
    KtlLogManager::SharedLogContainerSettings sharedLogContainerSettings;
    KtlLogManager::AcceleratedFlushLimits accelerateFlushLimits;

    //
    // Default memory throttle limits and settings
    //
    memoryThrottleLimits.WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMax;
    memoryThrottleLimits.WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMin;
    memoryThrottleLimits.WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
    memoryThrottleLimits.PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_DefaultPinnedMemoryLimit;
    memoryThrottleLimits.PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;
    memoryThrottleLimits.PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
    memoryThrottleLimits.AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;
    memoryThrottleLimits.MaximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_DefaultMaximumDestagingWriteOutstanding;
    memoryThrottleLimits.SharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;

    //
    // Use global default settings in the case that FabricNode does not
    // call down to configure the initial shared log container default
    // settings. This should only be the case where unit tests are
    // being run.
    //
    KGuid guidNull(0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
    sharedLogContainerSettings.Path[0] = 0;
    sharedLogContainerSettings.LogContainerId = guidNull;
    sharedLogContainerSettings.LogSize = KtlLogManager::SharedLogContainerSettings::_DefaultLogSize;
    sharedLogContainerSettings.MaximumNumberStreams = KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMax;
    sharedLogContainerSettings.MaximumRecordSize = 0;
    sharedLogContainerSettings.Flags = 0;

    accelerateFlushLimits.AccelerateFlushActiveTimerInMs = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushActiveTimerInMs;
    accelerateFlushLimits.AccelerateFlushPassiveTimerInMs = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushPassiveTimerInMs;
    accelerateFlushLimits.AccelerateFlushActivePercent = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushActivePercent;
    accelerateFlushLimits.AccelerateFlushPassivePercent = KtlLogManager::AcceleratedFlushLimits::DefaultAccelerateFlushPassivePercent;
    
    OverlayManager::SPtr overlayManager;

    status = OverlayManager::Create(overlayManager,
                                    memoryThrottleLimits,
                                    sharedLogContainerSettings,
                                    accelerateFlushLimits,
                                    Allocator,
                                    AllocationTag
                                    );

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)&Allocator.GetKtlSystem(), (ULONGLONG)0);
        return(status);
    }

#if defined(UPASSTHROUGH)
    K_LOCK_BLOCK(gs_CreateStopOverlayManagerLock.Lock())
    {
        //
        // Register the overlay manager. RegisterGlobalObject is atomic as
        // it adds the object under a lock. So it is safe to use that as a
        // way to determine which thread should start the service.
        //
        status = Allocator.GetKtlSystem().RegisterGlobalObject(OVERLAY_MANAGER,
                                                               *overlayManager);
        if (! NT_SUCCESS(status))
        {
            overlayManager = nullptr;
            LookupOverlayManager(Allocator, overlayManager);
            KInvariant(overlayManager);
        }
    
        //
        // Only the first should return STATUS_SUCCESS as that will start
        // the OverlayManager service
        //
        LONG count = overlayManager->IncrementUsageCount();
        status = (count == 1) ? STATUS_SUCCESS : STATUS_OBJECT_NAME_COLLISION;
    }
#else
    //
    // Register the overlay manager. RegisterGlobalObject is atomic as
    // it adds the object under a lock. So it is safe to use that as a
    // way to determine which thread should start the service.
    //
    status = Allocator.GetKtlSystem().RegisterGlobalObject(OVERLAY_MANAGER,
                                                           *overlayManager);
    if (! NT_SUCCESS(status))
    {
        overlayManager = nullptr;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)0);
        return(status);
    }
#endif
    
    OM = Ktl::Move(overlayManager);

    return(status);
}

NTSTATUS FileObjectTable::StopAndUnregisterOverlayManager(
    __in KAllocator& Allocator,
    __in_opt OverlayManager* OM
)
{
    //
    // Create and register the overlay manager
    //
    NTSTATUS status = STATUS_SUCCESS;
#if defined(UPASSTHROUGH)
    LONG count = 0;
#endif
    OverlayManager::SPtr overlayManager;

    if (OM == NULL)
    {
        LookupOverlayManager(Allocator, overlayManager);
    } else {
        overlayManager = OM;
    }

    if (overlayManager)
    {
#if defined(UPASSTHROUGH)
        K_LOCK_BLOCK(gs_CreateStopOverlayManagerLock.Lock())
        {
            count = overlayManager->DecrementUsageCount();

            //
            // Only last caller should do the actual cleanup
            //
            if (count == 0)
            {
                //
                // Unregister the overlay manager
                //
                Allocator.GetKtlSystem().UnregisterGlobalObject(OVERLAY_MANAGER);
            } else {
                return(STATUS_SUCCESS);
            }
        }
#else
        //
        // Unregister the overlay manager
        //
        Allocator.GetKtlSystem().UnregisterGlobalObject(OVERLAY_MANAGER);
#endif
        //
        // KAsyncService will not allow the service to be stopped
        // multiple times and so it is safe for multiple threads to
        // race and stop the service.
        //      
        KServiceSynchronizer        serviceSync;
        status = overlayManager->StartServiceClose(NULL,       // ParentAsync
                                                  serviceSync.CloseCompletionCallback());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)0);
        }

        status = serviceSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)0);
        }
    } else {
        status = STATUS_NOT_FOUND;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)&Allocator.GetKtlSystem(), (ULONGLONG)0);
    }
    
    return(status);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS> FileObjectTable::StopAndUnregisterOverlayManagerAsync(
    __in KAllocator& Allocator,
    __in_opt OverlayManager* OM
)
{
    //
    // Create and register the overlay manager
    //
    NTSTATUS status = STATUS_SUCCESS;
    OverlayManager::SPtr overlayManager;

    if (OM == NULL)
    {
        LookupOverlayManager(Allocator, overlayManager);
    } else {
        overlayManager = OM;
    }

    if (overlayManager)
    {
#if defined(UPASSTHROUGH)
        LONG count = 0;
        
        K_LOCK_BLOCK(gs_CreateStopOverlayManagerLock.Lock())
        {
            count = overlayManager->DecrementUsageCount();
            
            //
            // Only last caller should do the actual cleanup
            //
            if (count == 0)
            {
                //
                // Unregister the overlay manager
                //
                Allocator.GetKtlSystem().UnregisterGlobalObject(OVERLAY_MANAGER);
            } else {
                co_return STATUS_SUCCESS;
            }
        }
#else
        //
        // Unregister the overlay manager
        //
        Allocator.GetKtlSystem().UnregisterGlobalObject(OVERLAY_MANAGER);
#endif
        //
        // KAsyncService will not allow the service to be stopped
        // multiple times and so it is safe for multiple threads to
        // race and stop the service.
        //
        ktl::kservice::CloseAwaiter::SPtr awaiter;
        status = ktl::kservice::CloseAwaiter::Create(
            Allocator,
            KTL_TAG_TEST,
            *overlayManager, 
            awaiter,
            nullptr,
            nullptr);

        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, NULL, 0, 0);
            co_return status;
        }

        status =  co_await *awaiter;

        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)overlayManager.RawPtr(), (ULONGLONG)0);
        }
            
    } else {
        status = STATUS_NOT_FOUND;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)&Allocator.GetKtlSystem(), (ULONGLONG)0);
    }
    
    co_return status;
}
#endif

VOID FileObjectTable::LookupOverlayManager(
    __in KAllocator& Allocator,
    __out OverlayManager::SPtr& GlobalOverlayManager
    )
{
    KSharedBase::SPtr overlayManager;   
    WCHAR overlayManagerText[sizeof(OVERLAY_MANAGER) / sizeof(WCHAR)];

    KMemCpySafe(overlayManagerText, sizeof(overlayManagerText), OVERLAY_MANAGER, sizeof(OVERLAY_MANAGER));
    
    overlayManager = Allocator.GetKtlSystem().GetGlobalObjectSharedBase(overlayManagerText);
    GlobalOverlayManager = down_cast<OverlayManager>(overlayManager);
}



#pragma prefast(pop)
