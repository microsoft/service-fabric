/*++

Module Name:

    kreadcache.cpp

Abstract:

    This file contains the implementations of the KReadCache code.

Author:

    Norbert P. Kusters (norbertk) 9-Dec-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>


KReadCache::~KReadCache(
    )
{
    CacheItem* item;

    for (;;) {

        item = _RangeSorted.First();

        if (!item) {
            break;
        }

        item->Remove();

        _delete(item);
    }
}

KReadCache::KReadCache(
    __in ULONGLONG TotalCacheSize,
    __in ULONGLONG TargetBurstCacheSize,
    __in ULONG CorrelatedReferenceTimeoutInSeconds,
    __in ULONG AllocationTag
    ) :
    _RangeCompare(&CacheItem::RangeCompare),
    _LruKCompare(&CacheItem::LruKCompare),
    _RangeSorted(FIELD_OFFSET(CacheItem, RangeTableEntry), _RangeCompare),
    _LruList(FIELD_OFFSET(CacheItem, LruListEntry)),
    _LruKSorted(FIELD_OFFSET(CacheItem, LruKTableEntry), _LruKCompare)
{
    Initialize(TotalCacheSize, TargetBurstCacheSize, CorrelatedReferenceTimeoutInSeconds, AllocationTag);
}

NTSTATUS
KReadCache::Create(
    __in ULONGLONG TotalCacheSize,
    __in ULONGLONG TargetBurstCacheSize,
    __out KReadCache::SPtr& ReadCache,
    __in ULONG CorrelatedReferenceTimeoutInSeconds,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new read cache and returns a reference to it.

Arguments:

    TotalCacheSize                      - Supplies the total cache size.

    TargetBurstCacheSize                - Supplies the target burst cache size.

    ReadCache                           - Returns the read cache.

    CorrelatedReferenceTimeoutInSeconds - For the LRU-k algorithm, a touch doesn't count if it is within this timeout of the
                                            last recorded touch.

    AllocationTag                       - Supplies the allocation tag.

    Allocator                           - Supplies, optionally, the allocator.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    KReadCache* readCache;
    NTSTATUS status;

    readCache = _new(AllocationTag, Allocator) KReadCache(TotalCacheSize, TargetBurstCacheSize,
            CorrelatedReferenceTimeoutInSeconds, AllocationTag);

    if (!readCache) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = readCache->Status();

    if (!NT_SUCCESS(status)) {
        _delete(readCache);
        return status;
    }

    ReadCache = readCache;

    return STATUS_SUCCESS;
}

VOID
KReadCache::SetCacheSize(
    __in ULONGLONG TotalCacheSize,
    __in ULONGLONG TargetBurstCacheSize
    )

/*++

Routine Description:

    This routine will set the total and target burst cache sizes to the given values.  If the values are smaller than the
    current values then all of the memory is released synchronously with this call.

Arguments:

    TotalCacheSize          - Supplies the total cache size.

    TargetBurstCacheSize    - Supplies the target burst cache size.

Return Value:

    None.

--*/

{
    CacheItemList deleteList(FIELD_OFFSET(CacheItem, LruListEntry));

    _SpinLock.Acquire();

    _TotalCacheSize = TotalCacheSize;
    _TargetBurstCacheSize = TargetBurstCacheSize;

    AdjustCacheUsage(deleteList);

    _SpinLock.Release();

    DeleteCacheItems(deleteList);
}

VOID
KReadCache::QueryCacheSize(
    __out ULONGLONG& TotalCacheSize,
    __out ULONGLONG& TargetBurstCacheSize
    )

/*++

Routine Description:

    This routine will return the current maximum cache values for the burst and long term caches.

Arguments:

    TotalCacheSize          - Returns the total cache size.

    TargetBurstCacheSize    - Returns the target burst cache size.

Return Value:

    None.

--*/

{
    _SpinLock.Acquire();

    TotalCacheSize = _TotalCacheSize;
    TargetBurstCacheSize = _TargetBurstCacheSize;

    _SpinLock.Release();
}

VOID
KReadCache::QueryCacheUsage(
    __out ULONGLONG& TotalCacheUsage,
    __out ULONGLONG& BurstCacheUsage
    )

/*++

Routine Description:

    This routine will return the current amounts in used for the burst and long term caches.

Arguments:

    TotalCacheUsage - Returns the total cache usage.

    BurstCacheUsage - Returns the burst cache usage.

Return Value:

    None.

--*/

{
    _SpinLock.Acquire();

    TotalCacheUsage = _CurrentCacheSize;
    BurstCacheUsage = _CurrentBurstCacheSize;

    _SpinLock.Release();
}

NTSTATUS
KReadCache::Add(
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __inout KIoBufferElement& IoBufferElement
    )

/*++

Routine Description:

    This routine will add a new element to the cache.  New element always enter the cache in the burst part of the cache.
    After this the cache is adjusted so that the burst and long term parts stay below the required threshold.

Arguments:

    FileId          - Supplies the file id of the memory about to be cached.

    FileOffset      - Supplies the file offset of the memory about to be cached.

    IoBufferElement - Supplies the memory to be cached.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    CacheItem* newCacheItem = NULL;
    CacheItemList deleteList(FIELD_OFFSET(CacheItem, LruListEntry));

    //
    // Create a new node to add to the table.
    //

    newCacheItem = _new(_AllocationTag, GetThisAllocator()) CacheItem(*this, FileId, FileOffset, IoBufferElement);

    if (!newCacheItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SpinLock.Acquire();

    //
    // Remove any entries currently in the range.
    //

    RemoveRangeHelper(FileId, FileOffset, IoBufferElement.QuerySize(), deleteList);

    //
    // Insert the item into the cache.
    //

    newCacheItem->Insert();

    //
    // Check limits and purge.
    //

    AdjustCacheUsage(deleteList);

    _SpinLock.Release();

    DeleteCacheItems(deleteList);

    return STATUS_SUCCESS;
}

BOOLEAN
KReadCache::Query(
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __out ULONGLONG& ActualFileOffset,
    __out KIoBufferElement::SPtr& IoBufferElement
    )

/*++

Routine Description:

    This routine searches the cache for a cache item that includes the given offset for the given file.

Arguments:

    FileId              - Supplies the file id.

    FileOffset          - Supplies the file offset.

    ActualFileOffset    - Returns the file offset for the item returned.

    IoBufferElement     - Returns the IO buffer element.

Return Value:

    FALSE   - The requested item did not exist in the cache.

    TRUE    - The requested item is returned.

--*/

{
    CacheItem searchKey(FileId, FileOffset, 1);
    CacheItem* item;

    _SpinLock.Acquire();

    //
    // Check the table to see if the requested item is present.
    //

    item = _RangeSorted.Lookup(searchKey);

    if (!item) {

        //
        // The requested item is not in the cache.
        //

        _SpinLock.Release();

        return FALSE;
    }

    //
    // Record a new touch on the cached item.
    //

    item->Touch();

    //
    // Return the found cache item.
    //

    ActualFileOffset = item->_FileOffset;

    IoBufferElement = item->_IoBufferElement;

    _SpinLock.Release();

    return TRUE;
}

BOOLEAN
KReadCache::QueryEqualOrNext(
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __out ULONGLONG& CacheItemFileOffset
    )

/*++

Routine Description:

    This routine will return the start of the cache entry that contains the given file offset, or else the offset of the
    cache entry that follows the given file offset.

Arguments:

    FileId                  - Supplies the file id.

    FileOffset              - Supplies the file offset

    CacheItemFileOffset    - Returns the cache item file offset.

Return Value:

    FALSE   - No cache entry was found.

    TRUE    - A cache entry was found.

--*/

{
    CacheItem searchKey(FileId, FileOffset, 1);
    CacheItem* item;

    _SpinLock.Acquire();

    item = _RangeSorted.LookupEqualOrNext(searchKey);

    if (item) {
        if (item->_FileId == FileId) {
            CacheItemFileOffset = item->_FileOffset;
        } else {
            item = NULL;
        }
    }

    _SpinLock.Release();

    return item ? TRUE : FALSE;
}

VOID
KReadCache::RemoveRange(
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __in ULONGLONG Length
    )

/*++

Routine Description:

    This routine removes all cache items that overlap with the given range.

Arguments:

    FileId      - Supplies the file id.

    FileOffset  - Supplies the file offset.

    Length      - Supplies the length of the range.

Return Value:

    None.

--*/

{
    CacheItemList deleteList(FIELD_OFFSET(CacheItem, LruListEntry));

    _SpinLock.Acquire();

    RemoveRangeHelper(FileId, FileOffset, Length, deleteList);

    _SpinLock.Release();

    DeleteCacheItems(deleteList);
}

NTSTATUS
KReadCache::AllocateRegisterForEviction(
    __out RegisterForEvictionContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new register for eviction context.

Arguments:

    Async           - Returns the new register for eviction context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    RegisterForEvictionContext* context;

    context = _new(AllocationTag, GetThisAllocator()) RegisterForEvictionContext;

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Async = context;

    return STATUS_SUCCESS;
}

VOID
KReadCache::RegisterForEviction(
    __inout RegisterForEvictionContext& Async,
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine registers an eviction context that will be completed when the given item is taken out of the cache.

Arguments:

    Async       - Supplies the async context to use to register for eviction.

    FileId      - Supplies the file id.

    FileOffset  - Supplies the file offset.

    Completion  - Supplies the completion.

    ParentAsync - Supplies, optionally, the parent async to synchronize the completion with.

Return Value:

    FALSE   - The given cache item was not found.  The completion will not be called.

    TRUE    - The context was successfully registered.

--*/

{
    //
    // Start the eviction operation, so that it can be completed or cancelled later.
    //

    if (Async.Status() != K_STATUS_NOT_STARTED) {
        Async.Reuse();
    }

    //
    // Initialize the context.
    //

    Async.InitializeForRegister(*this, FileId, FileOffset);

    Async.Start(ParentAsync, Completion);
}

VOID
KReadCache::Touch(
    __in RegisterForEvictionContext& Async
    )

/*++

Routine Description:

    This routine will register a touch for the item that is registered for eviction.

Arguments:

    Async   - Supplies the eviction registration.

Return Value:

    None.

--*/

{
    _SpinLock.Acquire();

    if (Async._Item) {
        Async._Item->Touch();
    }

    _SpinLock.Release();
}

VOID
KReadCache::Initialize(
    __in ULONGLONG TotalCacheSize,
    __in ULONGLONG TargetBurstCacheSize,
    __in ULONG CorrelatedReferenceTimeoutInSeconds,
    __in ULONG AllocationTag
    )
{
    _TotalCacheSize = TotalCacheSize;
    _TargetBurstCacheSize = TargetBurstCacheSize;
    _CorrelatedReferenceTimeoutInTicks = 1000*CorrelatedReferenceTimeoutInSeconds;
    _AllocationTag = AllocationTag;
    _CurrentCacheSize = 0;
    _CurrentBurstCacheSize = 0;
    _CurrentSequenceNumber = 0;
}

VOID
KReadCache::AdjustCacheUsage(
    __inout CacheItemList& DeleteList
    )

/*++

Routine Description:

    This routine ensures that the cache usage is at or below the set values.

Arguments:

    DeleteList  - Supplies a list to put the deleted items on.

Return Value:

    None.

Notes:

    This function is called while holding the lock.

--*/

{
    CacheItem* item;
    BOOLEAN b;

    //
    // Get rid of stuff until we are under the limit.
    //

    while (_CurrentCacheSize > _TotalCacheSize) {

        //
        // Start with the burst cache if it is above the limit.
        //

        if (_CurrentBurstCacheSize > _TargetBurstCacheSize) {

            //
            // Take a peek at the least recently used item in the burst part of the cache.
            //

            item = _LruList.PeekTail();
            KFatal(item);

            //
            // If this item is not a candidate for the LRU-k table then simply remove it.
            //

            if (!item->_TouchHistory[item->_NextTouchIndex]) {
                item->Remove();
                DeleteList.AppendTail(item);
                continue;
            }

            //
            // Since this item is a candidate for the LRU-k table, insert it there.  Transfer the size to the LRU-k cache.
            //

            _LruList.RemoveTail();
            item->_InLru = FALSE;

            b = _LruKSorted.Insert(*item);
            KFatal(b);

            //
            // This item is no longer part of the burst cache.
            //

            _CurrentBurstCacheSize -= item->_Length;

            continue;
        }

        //
        // The burst cache is under its limit.  Take out of the LRU-k part of the cache.
        //

        item = _LruKSorted.First();
        KFatal(item);

        //
        // Remove this item from the cache.
        //

        item->Remove();

        DeleteList.AppendTail(item);
    }
}

VOID
KReadCache::DeleteCacheItems(
    __inout CacheItemList& DeleteList
    )

/*++

Routine Description:

    This routine will delete all of the items in the list.

Arguments:

    DeleteList  - Supplies the list of items to delete.

Return Value:

    None.

--*/

{
    CacheItem* item;

    while (DeleteList.Count()) {
        item = DeleteList.RemoveHead();
        _delete(item);
    }
}

VOID
KReadCache::RemoveRangeHelper(
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __in ULONGLONG Length,
    __inout CacheItemList& DeleteList
    )

/*++

Routine Description:

    This is the remove range logic without acquiring the lock.

Arguments:

    FileId      - Supplies the file id.

    FileOffset  - Supplies the file offset.

    Length      - Supplies the length.

    DeleteList  - Supplies the list to add the deleted cache items to.

Return Value:

    None.

--*/

{
    ULONG i = 0;
    CacheItem searchKey(FileId, FileOffset, Length);
    CacheItem* item;

    //
    // Continue to remove items from the cache until none match this range.
    //

    for (;;) {

        item = _RangeSorted.Lookup(searchKey);

        if (!item) {
            break;
        }

        item->Remove();

        DeleteList.AppendTail(item);

        i++;

        if (i == 16) {

            //
            // Allow the spin lock to be taken by someone else every 16 iterations.
            //

            i = 0;
            _SpinLock.Release();

            //
            // Also allow the processor to be taken by another thread.
            //

            KNt::Sleep(0);

            _SpinLock.Acquire();
        }
    }
}

KReadCache::RegisterForEvictionContext::~RegisterForEvictionContext(
    )
{
    // Nothing.
}

KReadCache::RegisterForEvictionContext::RegisterForEvictionContext(
    )
{
    Initialize();
}

VOID
KReadCache::RegisterForEvictionContext::Initialize(
    )
{
    _ReadCache = NULL;
    RtlZeroMemory(&_FileId, sizeof(_FileId));
    _FileOffset = 0;
    _Item = NULL;
}

VOID
KReadCache::RegisterForEvictionContext::InitializeForRegister(
    __inout KReadCache& ReadCache,
    __in const GUID& FileId,
    __in ULONGLONG FileOffset
    )
{
    _ReadCache = &ReadCache;
    _FileId = FileId;
    _FileOffset = FileOffset;
    _Item = NULL;
}

VOID
KReadCache::RegisterForEvictionContext::OnStart(
    )
{
    CacheItem searchKey(_FileId, _FileOffset, 1);
    CacheItem* item;
    ULONG i;

    _ReadCache->_SpinLock.Acquire();

    item = _ReadCache->_RangeSorted.Lookup(searchKey);

    if (!item) {

        //
        // The item to register for eviction with was not found.  Simply return that the item is actually already evicted.
        //

        _ReadCache->_SpinLock.Release();

        Complete(STATUS_SUCCESS);

        return;
    }

    //
    // Insert the eviction context into the item.
    //

    _Item = item;

    item->_EvictionNotificationList.AppendTail(this);
    AddRef();

    //
    // Additionally, anything that is 'registered' is automatically to be considered 'non-burst'.  Therefore update the
    // touch table so that it has this timestamp over the entire K range.
    //

    for (i = 0; i < LruKValue; i++) {
       item->UpdateHistory(TRUE);
    }

    _ReadCache->_SpinLock.Release();
}

VOID
KReadCache::RegisterForEvictionContext::OnCancel(
    )
{
    BOOLEAN b;

    _ReadCache->_SpinLock.Acquire();

    if (_Item) {
        _Item->_EvictionNotificationList.Remove(this);
        Release();
    }

    _ReadCache->_SpinLock.Release();

    b = Complete(STATUS_CANCELLED);

    KTraceCancelCalled(this, TRUE, b, 0);
}

VOID
KReadCache::RegisterForEvictionContext::OnReuse(
    )
{
    _ReadCache = NULL;
}

KReadCache::CacheItem::~CacheItem(
    )
{
    RegisterForEvictionContext* context;

    //
    // Complete all those that are registered for the eviction of this item.
    //

    while (_EvictionNotificationList.Count()) {
        context = _EvictionNotificationList.RemoveHead();
        KFatal(!context->_Item);
        context->Complete(STATUS_SUCCESS);
        context->Release();
    }
}

KReadCache::CacheItem::CacheItem(
    __in KReadCache& ReadCache,
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __in KIoBufferElement& IoBufferElement
    ) : _EvictionNotificationList(FIELD_OFFSET(RegisterForEvictionContext, RegisterForEvictionListEntry))
{
    Initialize(ReadCache, FileId, FileOffset, IoBufferElement);
}

KReadCache::CacheItem::CacheItem(
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __in ULONGLONG Length
    ) : _EvictionNotificationList(FIELD_OFFSET(RegisterForEvictionContext, RegisterForEvictionListEntry))
{
    Initialize(FileId, FileOffset, Length);
}

VOID
KReadCache::CacheItem::UpdateHistory(
    __in BOOLEAN BypassCorrelatedReferenceCheck
    )

/*++

Routine Description:

    This routine updates the history on this cache item with a new touch time.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONGLONG tickCount;
    ULONGLONG s;

    tickCount = KNt::GetTickCount64();

    if (!BypassCorrelatedReferenceCheck && tickCount - _LastTouchTickCount < _ReadCache->_CorrelatedReferenceTimeoutInTicks) {

        //
        // This is a correlated reference.  No touch counted for this.
        //

        return;
    }

    //
    // Count this as a new touch.
    //

    _LastTouchTickCount = tickCount;
    s = ++(_ReadCache->_CurrentSequenceNumber);
    _TouchHistory[_NextTouchIndex] = s;
    _NextTouchIndex = (_NextTouchIndex  + 1)%LruKValue;
}

VOID
KReadCache::CacheItem::Insert(
    )

/*++

Routine Description:

    This routine will insert this item into the cache.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOLEAN b;

    //
    // Mark a touch, remembering the current sequence number for this access.
    //

    UpdateHistory();

    //
    // Record the change to the used amount of memory.
    //

    _ReadCache->_CurrentBurstCacheSize += _Length;
    _ReadCache->_CurrentCacheSize += _Length;

    //
    // Add this cache item to the burst LRU and also to the Range Sorted Table.
    //

    _InLru = TRUE;
    _ReadCache->_LruList.InsertHead(this);

    b = _ReadCache->_RangeSorted.Insert(*this);
    KFatal(b);
}

VOID
KReadCache::CacheItem::Remove(
    )

/*++

Routine Description:

    This routine takes out this cache item from the range table and the LRU list or the LRU-k table.

Arguments:

    None.

Return Value:

    None.

--*/

{
    RegisterForEvictionContext* context;

    //
    // Take the item out of the burst or long term cache, whereever it is.
    //

    if (_InLru) {

        _ReadCache->_LruList.Remove(this);

        KFatal(_Length <= _ReadCache->_CurrentBurstCacheSize);
        KFatal(_Length <= _ReadCache->_CurrentCacheSize);

        _ReadCache->_CurrentBurstCacheSize -= _Length;
        _ReadCache->_CurrentCacheSize -= _Length;

    } else {

        _ReadCache->_LruKSorted.Remove(*this);

        KFatal(_Length <= _ReadCache->_CurrentCacheSize);

        _ReadCache->_CurrentCacheSize -= _Length;
    }

    //
    // Take the item out of the search table.
    //

    _ReadCache->_RangeSorted.Remove(*this);

    //
    // Make 'touch' impossible at this point.
    //

    for (context = _EvictionNotificationList.PeekHead(); context; context = _EvictionNotificationList.Successor(context)) {
        context->_Item = NULL;
    }

}

VOID
KReadCache::CacheItem::Touch(
    )

/*++

Routine Description:

    This routine records a touch on the given cache item.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Record this touch in the history.
    //

    UpdateHistory();

    //
    // Take this out of its current cache location.
    //

    if (_InLru) {

        //
        // Just put this item at the MRU end of the LRU list.
        //

        _ReadCache->_LruList.Remove(this);
        _ReadCache->_LruList.InsertHead(this);

        return;
    }

    //
    // Take this out of the LRU-k part of the cache and place it in the LRU part of the cache.
    //

    _ReadCache->_LruKSorted.Remove(*this);

    _InLru = TRUE;
    _ReadCache->_LruList.InsertHead(this);

    //
    // Adjust the amount of burst cache being used accordingly.
    //

    _ReadCache->_CurrentBurstCacheSize += _Length;
}

LONG
KReadCache::CacheItem::RangeCompare(
    __in CacheItem& First,
    __in CacheItem& Second
    )

/*++

Routine Description:

    This routine compares the ranges of 2 cache items, returning that they are equal if they overlap, or otherwise returning
    which one is in front of the other.

Arguments:

    First   - Supplies the first cache item.

    Second  - Supplies the second cache item.

Return Value:

    <0  - The first cache item's range comes before the second cache item's range.

    0   - The 2 cache item ranges overlap.

    >0  - The first cache item's ranges comes after the second cache item's range.

--*/

{
    LONG r;

    //
    // Compare the GUIDs first.
    //

    r = memcmp(&First._FileId, &Second._FileId, sizeof(GUID));

    if (r) {
        return r;
    }

    //
    // Compare the ranges.
    //

    if (First._FileOffset + First._Length <= Second._FileOffset) {
        return -1;
    }

    if (Second._FileOffset + Second._Length <= First._FileOffset) {
        return 1;
    }

    return 0;
}

LONG
KReadCache::CacheItem::LruKCompare(
    __in CacheItem& First,
    __in CacheItem& Second
    )

/*++

Routine Description:

    This routine compares the k-th penultimate touches of the given cache items.

Arguments:

    First   - Supplies the first cache item.

    Second  - Supplies the second cache item.

Return Value:

    <0  - The first cache item is less important than the second cache item.

    0   - The 2 cache items are the same.

    >0  - The first cache item is more important than the second cache item.

--*/

{
    ULONGLONG f = First._TouchHistory[First._NextTouchIndex];
    ULONGLONG s = Second._TouchHistory[Second._NextTouchIndex];

    if (f < s) {
        return -1;
    }

    if (f > s) {
        return 1;
    }

    return 0;
}

VOID
KReadCache::CacheItem::Initialize(
    __in KReadCache& ReadCache,
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __in KIoBufferElement& IoBufferElement
    )
{
    ULONG i;

    _ReadCache = &ReadCache;
    _FileId = FileId;
    _FileOffset = FileOffset;
    _Length = IoBufferElement.QuerySize();
    _IoBufferElement = &IoBufferElement;
    _LastTouchTickCount = 0;

    for (i = 0; i < LruKValue; i++) {
        _TouchHistory[i] = 0;
    }

    _NextTouchIndex = 0;
    _InLru = FALSE;
}

VOID
KReadCache::CacheItem::Initialize(
    __in const GUID& FileId,
    __in ULONGLONG FileOffset,
    __in ULONGLONG Length
    )
{
    ULONG i;

    _ReadCache = NULL;
    _FileId = FileId;
    _FileOffset = FileOffset;
    _Length = Length;
    _LastTouchTickCount = 0;

    for (i = 0; i < LruKValue; i++) {
        _TouchHistory[i] = 0;
    }

    _NextTouchIndex = 0;
    _InLru = FALSE;
}
