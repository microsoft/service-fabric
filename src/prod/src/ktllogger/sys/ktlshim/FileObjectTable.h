// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if defined(UDRIVER) || defined(KDRIVER)
class FileObjectEntry : public KObject<FileObjectEntry>, public KShared<FileObjectEntry>   
{
    K_FORCE_SHARED(FileObjectEntry);

    friend class FileObjectTable;
    
    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out FileObjectEntry::SPtr& FOE);
                                    
        static LONG
        Comparator(FileObjectEntry& Left, FileObjectEntry& Right) {
            // -1 if Left < Right
            // +1 if Left > Right
            //  0 if Left == Right
            if (Left._ObjectHolder.GetObjectId() < Right._ObjectHolder.GetObjectId())
            {
                return(-1);
            } else if (Left._ObjectHolder.GetObjectId() > Right._ObjectHolder.GetObjectId()) {
                return(+1);
            }
            return(0);
        };

        static ULONG
        GetLinksOffset() { return FIELD_OFFSET(FileObjectEntry, _TableEntry); };
        
    private:
        KTableEntry _TableEntry;
        KtlLogObjectHolder _ObjectHolder;
};
#endif

class RequestMarshallerKernel;

class FileObjectTable : public KObject<FileObjectTable>, public KShared<FileObjectTable>
{
    K_FORCE_SHARED(FileObjectTable);

    public:
        static NTSTATUS CreateAndRegisterOverlayManager(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        static NTSTATUS StopAndUnregisterOverlayManager(
            __in KAllocator& Allocator,
            __in_opt OverlayManager* OM = NULL          
        );

#if defined(K_UseResumable)
        static ktl::Awaitable<NTSTATUS> CreateAndRegisterOverlayManagerAsync(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );
        
        static ktl::Awaitable<NTSTATUS> StopAndUnregisterOverlayManagerAsync(
            __in KAllocator& Allocator,
            __in_opt OverlayManager* OM = NULL          
        );
#endif
        static NTSTATUS CreateAndRegisterOverlayManagerInternal(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out OverlayManager::SPtr &OM
        );
        
        static VOID FileObjectTable::LookupOverlayManager(
            __in KAllocator& Allocator,
            __out OverlayManager::SPtr& GlobalOverlayManager
        );

        static KGlobalSpinLockStorage      gs_CreateStopOverlayManagerLock;     
        
#if defined(UDRIVER) || defined(KDRIVER)
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out FileObjectTable::SPtr& FOT
        );

        VOID AddObject(
            __in KSharedBase& ObjectPtr,
            __in_opt KSharedBase* const CloseContext,
            __in FileObjectEntry& FileObjectEntry,
            __in RequestMarshaller::OBJECT_TYPE ObjectType,
            __in ULONGLONG ObjectId
            );

        NTSTATUS RemoveObject(
            __in ULONGLONG ObjectId
        );

        NTSTATUS LookupObject(
            __in ULONGLONG ObjectId,
            __out KtlLogObjectHolder& ObjectHolder
        );

        PVOID LookupObjectPointer(
            __in ULONGLONG ObjectId
        );

        inline KtlLogManagerKernel::SPtr GetLogManager()
        {
            return(_LogManager);
        }

        inline VOID ResetLogManager(
            )
        {
            _LogManager = nullptr;
        }

        NTSTATUS AddRequest(
            __in RequestMarshallerKernel* Request
        );

        VOID RemoveRequest(
            __in RequestMarshallerKernel* Request
        );

        VOID CancelAllRequests(
        );

        VOID FindRequest(
            __in ULONGLONG RequestId,
            __out KSharedPtr<RequestMarshallerKernel>& Request
            );

        NTSTATUS AddIoBufferElement(
            __in KIoBufferElementKernel& Element
        );

        VOID RemoveIoBufferElement(
            __in KIoBufferElementKernel& Element
        );
        VOID WaitForAllIoBufferElements(
        );
                        
        VOID ClearObjects(
        );

        ULONGLONG GetNextObjectIdIndex()
        {
            ULONGLONG objectIdIndex = 0;
            
            objectIdIndex = (ULONGLONG)InterlockedIncrement64((LONG64*)&_ObjectIdIndex);
            if (objectIdIndex == 0)
            {
                objectIdIndex = (ULONGLONG)InterlockedIncrement64((LONG64*)&_ObjectIdIndex);
                KInvariant(objectIdIndex != 0);
            }
            
            return(objectIdIndex);
        }

        inline OverlayManager::SPtr& GetOverlayManager()
        {
            KInvariant(_OverlayManager);
            return(_OverlayManager);
        }

    private:
        KSpinLock _Lock;
        FileObjectEntry::SPtr _LookupKey;
        KNodeTable<FileObjectEntry> _Table;

        OverlayManager::SPtr _OverlayManager;
        
        ULONGLONG _ObjectIdIndex;
        KtlLogManagerKernel::SPtr _LogManager;

        ActiveList<RequestMarshallerKernel, ULONGLONG> _RequestList;
        KSpinLock _RequestLock;

        LONGLONG _IoBufferElementsSize;
        ActiveList<KIoBufferElementKernel, ULONGLONG> _IoBufferElementList;
        KSpinLock _IoBufferElementLock;

    public:
#if DBG
        inline VOID IncrementPinMemoryFailureCount()
        {
            _OverlayManager->IncrementPinMemoryFailureCount();
        }
#endif
        inline LONGLONG IncrementPinnedIoBufferUsage(
            __in ULONG IoBufferElementSize
            )
        {
            return(_OverlayManager->IncrementPinnedIoBufferUsage(IoBufferElementSize));
        }

        inline LONGLONG DecrementPinnedIoBufferUsage(
            __in ULONG IoBufferElementSize
            )
        {
            return(_OverlayManager->DecrementPinnedIoBufferUsage(IoBufferElementSize));
        }

        inline LONGLONG GetPinnedIoBufferLimit()
        {
            return(_OverlayManager->GetPinnedIoBufferLimit());
        }

        inline LONGLONG GetPinnedIoBufferUsage()
        {
            return(_OverlayManager->GetPinnedIoBufferUsage());
        }

        inline BOOLEAN IsPinnedIoBufferUsageOverLimit()
        {
            return(_OverlayManager->IsPinnedIoBufferUsageOverLimit());
        }
#endif
};


