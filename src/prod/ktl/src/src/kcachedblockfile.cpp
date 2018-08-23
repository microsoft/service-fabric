/*++

Module Name:

    kcachedblockfile.cpp

Abstract:

    This file contains the implementations of the KCachedBlockFile code.

Author:

    Norbert P. Kusters (norbertk) 9-Dec-2010

Environment:

    Kernel mode and User mode

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>


KCachedBlockFile::~KCachedBlockFile(
    )
{
    if (_PerfCounterSetInstance) {
        _PerfCounterSetInstance->Stop();
    }
}

KCachedBlockFile::KCachedBlockFile(
   __in const GUID& FileId,
    __inout KBlockFile& File,
    __inout KReadCache& ReadCache,
    __in BOOLEAN NoCachingOnWrite,
    __in ULONG AllocationTag
    )
{
    SetConstructorStatus(Initialize(FileId, File, ReadCache, NoCachingOnWrite, AllocationTag));
}

NTSTATUS
KCachedBlockFile::Create(
    __in const GUID& FileId,
    __inout KBlockFile& File,
    __inout KReadCache& ReadCache,
    __out KCachedBlockFile::SPtr& CachedFile,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in BOOLEAN NoCachingOnWrite
    )

/*++

Routine Description:

    This routine creates a new cached block file object.

Arguments:

    File                - Supplies the non-cached file object.

    ReadCache           - Supplies the read cache to integrate this file cache into.

    CachedFile          - Returns the cached file.

    AllocationTag       - Supplies the allocation tag.

    Allocator           - Supplies the allocator.

    NoCachingOnWrite    - Supplies whether or not writes should be cached.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    KCachedBlockFile* cachedFile;
    NTSTATUS status;

    cachedFile = _new(AllocationTag, Allocator) KCachedBlockFile(FileId, File, ReadCache, NoCachingOnWrite, AllocationTag);

    if (!cachedFile) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = cachedFile->Status();

    if (!NT_SUCCESS(status)) {
        _delete(cachedFile);
        return status;
    }

    CachedFile = cachedFile;

    return STATUS_SUCCESS;
}

const WCHAR*
KCachedBlockFile::GetFileName(
    )
{
    return _File->GetFileName();
}

ULONGLONG
KCachedBlockFile::QueryFileSize(
    )
{
    return _File->QueryFileSize();
}

BOOLEAN
KCachedBlockFile::IsWriteThrough(
    )
{
    return _File->IsWriteThrough();
}

VOID
KCachedBlockFile::QueryFileId(
    __out GUID& FileId
    )
{
    FileId = _FileId;
}

NTSTATUS
KCachedBlockFile::AllocateSetFileSize(
    __out SetFileSizeContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new set file size context.

Arguments:

    Async           - Returns the allocated set file size context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    SetFileSizeContext* context;
    NTSTATUS status;

    context = _new(AllocationTag, GetThisAllocator()) SetFileSizeContext(*this, AllocationTag);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    Async = context;

    return STATUS_SUCCESS;
}

NTSTATUS
KCachedBlockFile::SetFileSize(
    __in ULONGLONG FileSize,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt SetFileSizeContext* Async
    )

/*++

Routine Description:

    This routine sets the size of the file.  A range lock is taken in order to drain any outstanding IO to the part of the
    file that is being truncated.

Arguments:

    FileSize    - Supplies the new file size.

    Completion  - Supplies the completion routine.

    ParentAsync - Supplies, optionally, the parent async to synchronize the completion to.

    Async       - Supplies, optionally, the async to use for this request.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   - The async was not supplied and could not be allocated.

    STATUS_PENDING                  - The completion will be called.

--*/
{
    SetFileSizeContext* context;
    NTSTATUS status;

    if (Async) {
        context = Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_CACHED_BLOCK_FILE, GetThisAllocator()) SetFileSizeContext(*this, KTL_TAG_CACHED_BLOCK_FILE);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        return status;
    }

    context->InitializeForSetFileSize(*this, FileSize);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KCachedBlockFile::AllocateRead(
    __out ReadContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine will allocate a read async context.

Arguments:

    Async           - Returns the read async context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    ReadContext* context;
    NTSTATUS status;

    context = _new(AllocationTag, GetThisAllocator()) ReadContext(*this, AllocationTag);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    Async = context;

    return STATUS_SUCCESS;
}

NTSTATUS
KCachedBlockFile::Read(
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __in ULONG Length,
    __out KIoBuffer::SPtr& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt ReadContext* Async
    )

/*++

Routine Description:

    This routine performs a cached read on the file.

Arguments:

    Priority    - The priority to issue the read, should the read miss the cache.

    Offset      - The offset of the read.

    Length      - The length of the read.

    IoBuffer    - Returns the IO buffer of the read.

    Completion  - Supplies the completion routine.

    ParentAsync - Supplies, optionally, the parent async to synchronize the completion with.

    Async       - Supplies, optionally, the async context to use for this request.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   - The async was not provided and the allocation of the async failed.

    STATUS_PENDING

--*/

{
    ReadContext* context;
    NTSTATUS status;

    if (Async) {
        context = Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_CACHED_BLOCK_FILE, GetThisAllocator()) ReadContext(*this, KTL_TAG_CACHED_BLOCK_FILE);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        return status;
    }

    context->InitializeForRead(*this, Priority, Offset, Length, IoBuffer);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KCachedBlockFile::AllocateTransfer(
    __out TransferContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new transfer context.

Arguments:

    Async           - Returns the new transfer context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    TransferContext* context;
    NTSTATUS status;

    context = _new(AllocationTag, GetThisAllocator()) TransferContext(*this, AllocationTag);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    Async = context;

    return STATUS_SUCCESS;
}

NTSTATUS
KCachedBlockFile::ReadSingle(
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __inout KIoBufferElement::SPtr& IoBufferElement,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine performs a cached read on the file.  The result is guaranteed to be virtually contiguous, i.e. it is
    guaranteed to be a single IO buffer element.

Arguments:

    Priority        - The priority to issue the read, should the read miss the cache.

    Offset          - The offset of the read.

    IoBufferElement - Supplies the IO buffer element to read into.  If a cache hit is possible then this field will return a
                        a different IO buffer element.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to synchronize the completion with.

    Async           - Supplies, optionally, the async context to use for this request.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   - The async was not provided and the allocation of the async failed.

    STATUS_PENDING

--*/

{
    TransferContext* context;
    NTSTATUS status;

    if (Async) {
        context = Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_CACHED_BLOCK_FILE, GetThisAllocator()) TransferContext(*this, KTL_TAG_CACHED_BLOCK_FILE);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        return status;
    }

    context->InitializeForReadSingle(*this, Priority, Offset, IoBufferElement);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KCachedBlockFile::Write(
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __in KIoBufferElement& IoBufferElement,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine performs a write-through cached write on the file.

Arguments:

    Priority        - The priority to issue the read, should the read miss the cache.

    Offset          - The offset of the write.

    IoBufferElement - The IO buffer element to write.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to synchronize the completion with.

    Async           - Supplies, optionally, the async context to use for this request.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   - The async was not provided and the allocation of the async failed.

    STATUS_PENDING

--*/

{
    TransferContext* context;
    NTSTATUS status;

    if (Async) {
        context = Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_CACHED_BLOCK_FILE, GetThisAllocator()) TransferContext(*this, KTL_TAG_CACHED_BLOCK_FILE);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        return status;
    }

    context->InitializeForWrite(*this, Priority, Offset, IoBufferElement);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KCachedBlockFile::Write(
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine performs a write-through cached write on the file.

Arguments:

    Priority        - The priority to issue the read, should the read miss the cache.

    Offset          - The offset of the write.

    IoBufferElement - The IO buffer element to write.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to synchronize the completion with.

    Async           - Supplies, optionally, the async context to use for this request.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   - The async was not provided and the allocation of the async failed.

    STATUS_PENDING

--*/

{
    TransferContext* context;
    NTSTATUS status;

    if (Async) {
        context = Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_CACHED_BLOCK_FILE, GetThisAllocator()) TransferContext(*this, KTL_TAG_CACHED_BLOCK_FILE);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        return status;
    }

    context->InitializeForWriteIoBuffer(*this, Priority, Offset, IoBuffer);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KCachedBlockFile::Copy(
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in KIoBufferElement& IoBufferElement,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine performs a cached copy on the file.

Arguments:

    Priority        - The priority to issue the read, should the read miss the cache.

    SourceOffset    - Supplies the offset to copy from.

    TargetOffset    - Supplies the offset to copy to.

    IoBufferElement - The IO buffer element to use for the copy.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to synchronize the completion with.

    Async           - Supplies, optionally, the async context to use for this request.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   - The async was not provided and the allocation of the async failed.

    STATUS_PENDING

--*/

{
    ULONG length;
    TransferContext* context;
    NTSTATUS status;

    //
    // It is a programming error to issue overlapping ranges.
    //

    length = IoBufferElement.QuerySize();

    KFatal((SourceOffset + length <= TargetOffset) || (TargetOffset + length <= SourceOffset));

    //
    // The ranges do not overlap, so continue.
    //

    if (Async) {
        context = Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_CACHED_BLOCK_FILE, GetThisAllocator()) TransferContext(*this, KTL_TAG_CACHED_BLOCK_FILE);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        return status;
    }

    context->InitializeForCopy(*this, Priority, SourceOffset, TargetOffset, IoBufferElement);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KCachedBlockFile::AllocateFlush(
    __out KBlockFile::FlushContext::SPtr& Async,
    __in ULONG AllocationTag
    )
{
    return _File->AllocateFlush(Async, AllocationTag);
}

NTSTATUS
KCachedBlockFile::Flush(
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KBlockFile::FlushContext* Async
    )
{
    return _File->Flush(Completion, ParentAsync, Async);
}

VOID
KCachedBlockFile::CancelAll(
    )
{
    return _File->CancelAll();
}

VOID
KCachedBlockFile::Close(
    )
{
    _File->Close();
}

VOID
KCachedBlockFile::SetBackgroundQueueLength(
    __in ULONG BackgroundQueueLength
    )
{
    _File->SetBackgroundQueueLength(BackgroundQueueLength);
}

VOID
KCachedBlockFile::PurgeCache(
    ULONGLONG Offset,
    ULONGLONG Length
    )

/*++

Routine Description:

    This routine removes the given range from the read cache.

Arguments:

    Offset  - Supplies the file offset.

    Length  - Supplies the length.

Return Value:

    None.

--*/

{
    _ReadCache->RemoveRange(_FileId, Offset, Length);
}

NTSTATUS
KCachedBlockFile::AllocateRegisterForEviction(
    __out KReadCache::RegisterForEvictionContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a register for eviction context.

Arguments:

    Async           - Returns the register for eviction context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES

    STATUS_SUCCESS

--*/

{
    return _ReadCache->AllocateRegisterForEviction(Async, AllocationTag);
}

VOID
KCachedBlockFile::RegisterForEviction(
    __inout KReadCache::RegisterForEvictionContext& Async,
    __in ULONGLONG FileOffset,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine registers the given async to be completed upon the eviction of the given cache item.

Arguments:

    Async       - Supplies the register for eviction context.

    FileOffset  - Supplies the file offset of the cache item to register with.

    Completion  - Supplies the completion callback.

    ParentAsync - Supplies, optionally, the parent async to synchronize the completion with.

Return Value:

    None.

--*/

{
    _ReadCache->RegisterForEviction(Async, _FileId, FileOffset, Completion, ParentAsync);
}

VOID
KCachedBlockFile::Touch(
    __in KReadCache::RegisterForEvictionContext& Async
    )

/*++

Routine Description:

    This routine will register a touch on the cache item that is currently registered for eviction.

Arguments:

    Async   - Supplies the register for eviction context.

Return Value:

    None.

--*/

{
    _ReadCache->Touch(Async);
}

NTSTATUS
KCachedBlockFile::Initialize(
    __in const GUID& FileId,
    __inout KBlockFile& File,
    __inout KReadCache& ReadCache,
    __in BOOLEAN NoCachingOnWrite,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    KPerfCounterSet::SPtr counterSet;

    _FileId = FileId;
    _File = &File;
    _ReadCache = &ReadCache;
    _NoCachingOnWrite = NoCachingOnWrite;

    status = KRangeLock::Create(_RangeLock, GetThisAllocator(), AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Set up the perf counters.
    //

    _ReadCount = 0;
    _BytesRead = 0;
    _CachedReadCount = 0;
    _CachedBytesRead = 0;
    _WriteCount = 0;
    _BytesWritten = 0;
    _CopyCount = 0;
    _BytesCopied = 0;
    _CachedCopyCount = 0;
    _CachedBytesCopied = 0;

    counterSet = GetThisKtlSystem().GetGlobalObject<KPerfCounterSet>(KtlPerInstanceCachedBlockFileCountersIdString);

    if (!counterSet) {

        //
        // This instance of KTL does not have any performance counters.
        //

        return STATUS_SUCCESS;
    }

    status = counterSet->SpawnInstance(_File->GetFileName(), _PerfCounterSetInstance);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(1, &_ReadCount);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(2, &_BytesRead);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(3, &_CachedReadCount);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(4, &_CachedBytesRead);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(5, &_WriteCount);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(6, &_BytesWritten);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(7, &_CopyCount);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(8, &_BytesCopied);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(9, &_CachedCopyCount);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->SetCounterAddress(10, &_CachedBytesCopied);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _PerfCounterSetInstance->Start();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

KCachedBlockFile::SetFileSizeContext::~SetFileSizeContext(
    )
{
    // Nothing.
}

KCachedBlockFile::SetFileSizeContext::SetFileSizeContext(
    __in KCachedBlockFile& CachedFile,
    __in ULONG AllocationTag
    )
{
    SetConstructorStatus(Initialize(CachedFile, AllocationTag));
}

NTSTATUS
KCachedBlockFile::SetFileSizeContext::Initialize(
    __in KCachedBlockFile& CachedFile,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    status = CachedFile._RangeLock->AllocateAcquire(_RangeLockContext, AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    _FileSize = 0;

    return STATUS_SUCCESS;
}

VOID
KCachedBlockFile::SetFileSizeContext::InitializeForSetFileSize(
    __in KCachedBlockFile& CachedFile,
    __in ULONGLONG FileSize
    )
{
    _CachedFile = &CachedFile;
    _FileSize = FileSize;
}

VOID
KCachedBlockFile::SetFileSizeContext::OnStart(
    )

/*++

Routine Description:

    This routine is called to start a set file size action.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KAsyncContextBase::CompletionCallback completion(this, &SetFileSizeContext::RangeLockCompletion);

    //
    // As a first step, acquire the range beyond the new end of the file.
    //

    _CachedFile->_RangeLock->AcquireRange(*_RangeLockContext, _FileSize, MAXULONGLONG - _FileSize, completion, this);
}

VOID
KCachedBlockFile::SetFileSizeContext::OnCancel(
    )
{
    _RangeLockContext->Cancel();
}

VOID
KCachedBlockFile::SetFileSizeContext::OnReuse(
    )
{
    _RangeLockContext->Reuse();
    _CachedFile = NULL;
}

VOID
KCachedBlockFile::SetFileSizeContext::RangeLockCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )

/*++

Routine Description:

    This routine is called when the range lock acquire is completed.

Arguments:

    ParentAsync - Supplies the parent async.

    Async       - Supplies the completing operation async.

Return Value:

    None.

--*/

{
    NTSTATUS status = Async.Status();
    KAsyncContextBase::CompletionCallback completion(this, &SetFileSizeContext::SetFileSizeCompletion);

    UNREFERENCED_PARAMETER(ParentAsync);            

    //
    // If the range lock acquire was not successful (cancelled, in other words), just fail here.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Proceed now with the set-file-size.
    //

    status = _CachedFile->_File->SetFileSize(_FileSize, completion, this);

    if (!NT_SUCCESS(status)) {

        //
        // Release the range lock.
        //

        _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);

        KTraceFailedAsyncRequest(status, this, 0, 0);

        Complete(status);
        return;
    }

    KFatal(status == STATUS_PENDING);
}

VOID
KCachedBlockFile::SetFileSizeContext::SetFileSizeCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )

/*++

Routine Description:

    This routine is called when the underling block file's set file size operation completes.

Arguments:

    ParentAsync - Supplies the parent async.

    Async       - Supplies the completing operation async.

Return Value:

    None.

--*/

{
    NTSTATUS status = Async.Status();

    UNREFERENCED_PARAMETER(ParentAsync);    

    //
    // The set-file-size action is complete.  Release the range lock.
    //

    _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);

    //
    // Complete the status of this request as the status of the underlying set-file-size action.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

KCachedBlockFile::ReadContext::~ReadContext(
    )
{
    // Nothing.
}

KCachedBlockFile::ReadContext::ReadContext(
    __in KCachedBlockFile& CachedFile,
    __in ULONG AllocationTag
    )
    : _TransferContextArray(GetThisAllocator())
{
    SetConstructorStatus(Initialize(CachedFile, AllocationTag));
}

NTSTATUS
KCachedBlockFile::ReadContext::Initialize(
    __in KCachedBlockFile& CachedFile,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    _AllocationTag = AllocationTag;

    status = CachedFile._RangeLock->AllocateAcquire(_RangeLockContext, AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    _IoPriority = KBlockFile::eBackground;
    _Offset = 0;
    _Length = 0;
    _IoBuffer = NULL;
    _RefCount = 0;
    _Status = STATUS_SUCCESS;

    return _TransferContextArray.Status();
}

VOID
KCachedBlockFile::ReadContext::InitializeForRead(
    __in KCachedBlockFile& CachedFile,
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __in ULONG Length,
    __out KIoBuffer::SPtr& IoBuffer
    )
{
    _CachedFile = &CachedFile;
    _IoPriority = Priority;
    _Offset = Offset;
    _Length = Length;
    _IoBuffer = &IoBuffer;
    KFatal(!_RefCount);
    _Status = STATUS_SUCCESS;
    _TransferContextArray.Clear();
}

VOID
KCachedBlockFile::ReadContext::OnStart(
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &ReadContext::RangeLockCompletion);

    //
    // As a first step, acquire the range.
    //

    _CachedFile->_RangeLock->AcquireRange(*_RangeLockContext, _Offset, _Length, completion, this);
}

VOID
KCachedBlockFile::ReadContext::OnCancel(
    )
{
    ULONG i;

    _RangeLockContext->Cancel();

    for (i = 0; i < _TransferContextArray.Count(); i++) {
        _TransferContextArray[i]->Cancel();
    }
}

VOID
KCachedBlockFile::ReadContext::OnReuse(
    )
{
    _RangeLockContext->Reuse();
    _CachedFile = NULL;
    _TransferContextArray.Clear();
}

VOID
KCachedBlockFile::ReadContext::RangeLockCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )

/*++

Routine Description:

    This routine is called when the range lock acquire is completed.

Arguments:

    ParentAsync - Supplies the parent async.

    Async       - Supplies the completing operation async.

Return Value:

    None.

--*/

{
    NTSTATUS status = Async.Status();
    KAsyncContextBase::CompletionCallback completion(this, &ReadContext::ReadCompletion);
    ULONGLONG offset = _Offset;
    ULONGLONG end = offset + _Length;
    BOOLEAN b;
    ULONGLONG actualOffset;
    KIoBufferElement::SPtr ioBufferElement;
    ULONG remaining;
    ULONG length;
    ULONG delta;
    VOID* buffer;
    KBlockFile::TransferContext::SPtr transferContext;

    UNREFERENCED_PARAMETER(ParentAsync);    

    //
    // If the range lock was not acquired, i.e. cancelled, then complete the request.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Start off with an empty IO buffer.
    //

    status = KIoBuffer::CreateEmpty(*_IoBuffer, GetThisAllocator(), _AllocationTag);

    if (!NT_SUCCESS(status)) {
        _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    _RefCount++;

    while (offset < end) {

        remaining = (ULONG) (end - offset);

        //
        // Check the cache.
        //

        b = _CachedFile->_ReadCache->Query(_CachedFile->_FileId, offset, actualOffset, ioBufferElement);

        if (b) {

            //
            // This is a cache hit.
            //

            KFatal(offset >= actualOffset);

            delta = (ULONG) (offset - actualOffset);
            length = remaining;

            if (length > ioBufferElement->QuerySize() - delta) {
                length = ioBufferElement->QuerySize() - delta;
            }

            status = (*_IoBuffer)->AddIoBufferElementReference(*ioBufferElement, delta, length, _AllocationTag);

            if (!NT_SUCCESS(status)) {
                _Status = status;
                break;
            }

            offset += length;
            continue;
        }

        //
        // This is a cache miss, where does the next cache hit occur?
        //

        b = _CachedFile->_ReadCache->QueryEqualOrNext(_CachedFile->_FileId, offset, actualOffset);

        if (!b || actualOffset >= end) {
            actualOffset = end;
        }

        length = (ULONG) (actualOffset - offset);

        status = KIoBufferElement::CreateNew(length, ioBufferElement, buffer, GetThisAllocator(), _AllocationTag);

        if (!NT_SUCCESS(status)) {
            _Status = status;
            break;
        }

        (*_IoBuffer)->AddIoBufferElement(*ioBufferElement);

        status = _CachedFile->_File->AllocateTransfer(transferContext, _AllocationTag);

        if (!NT_SUCCESS(status)) {
            _Status = status;
            break;
        }

        status = _TransferContextArray.Append(transferContext);

        if (!NT_SUCCESS(status)) {
            _Status = status;
            break;
        }

        _RefCount++;

        status = _CachedFile->_File->Transfer(_IoPriority, KBlockFile::eRead, offset, buffer, length, completion, this,
                transferContext.RawPtr());

        KFatal(status == STATUS_PENDING);

        offset += length;
    }

    _RefCount--;

    if (!_RefCount) {
        _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);

        if (NT_SUCCESS(_Status)) {
            KFatal((*_IoBuffer)->QuerySize() == _Length);
        } else {
            KTraceFailedAsyncRequest(_Status, this, 0, 0);
        }

        //
        // In this case all of the read was from the cache.  Increment the performance counters accordingly.
        //

        _CachedFile->_ReadCount++;
        _CachedFile->_BytesRead += _Length;
        _CachedFile->_CachedReadCount++;
        _CachedFile->_CachedBytesRead += _Length;

        Complete(_Status);
    }
}

VOID
KCachedBlockFile::ReadContext::ReadCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )

/*++

Routine Description:

    This routine is called when a read completes from the block file object.

Arguments:

    ParentAsync - Supplies the parent async.

    Async       - Supplies the completing request.

Return Value:

    None.

--*/

{
    NTSTATUS status = Async.Status();
    ULONGLONG offset;
    KIoBufferElement* elem;
    BOOLEAN b;
    ULONGLONG actualOffset;

    UNREFERENCED_PARAMETER(ParentAsync);            

    if (!NT_SUCCESS(status)) {
        _Status = status;
    }

    _RefCount--;

    if (_RefCount) {
        return;
    }

    //
    // The request is complete.  Add whatever is not already in the cache, to the cache.
    //

    offset = _Offset;

    KIoBuffer* ioBuffer = _IoBuffer->RawPtr();

    for (elem = ioBuffer->First(); elem; elem = ioBuffer->Next(*elem)) {

        b = _CachedFile->_ReadCache->QueryEqualOrNext(_CachedFile->_FileId, offset, actualOffset);

        if (!b || actualOffset > offset) {

            //
            // This buffer element is not already in the cache, so, add it.
            //

            status = _CachedFile->_ReadCache->Add(_CachedFile->_FileId, offset, *elem);
            KAssert(NT_SUCCESS(status));
        }

        offset += elem->QuerySize();
    }

    _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);

    if (NT_SUCCESS(_Status)) {
        KFatal((*_IoBuffer)->QuerySize() == _Length);
    } else {
        KTraceFailedAsyncRequest(_Status, this, 0, 0);
    }

    //
    // This read was completed with at least some of the data read in from the disk.  Adjust the performance counters
    // accordingly.
    //

    _CachedFile->_ReadCount++;
    _CachedFile->_BytesRead += _Length;


    Complete(_Status);
}

KCachedBlockFile::TransferContext::~TransferContext(
    )
{
    // Nothing.
}

KCachedBlockFile::TransferContext::TransferContext(
    __in KCachedBlockFile& CachedFile,
    __in ULONG AllocationTag
    )
{
    SetConstructorStatus(Initialize(CachedFile, AllocationTag));
}

NTSTATUS
KCachedBlockFile::TransferContext::Initialize(
    __in KCachedBlockFile& CachedFile,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    status = CachedFile._File->AllocateTransfer(_LowerTransferContext, AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KRangeLock::AllocateAcquireStatic(_RangeLockContext, GetThisAllocator(), AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KRangeLock::AllocateAcquireStatic(_SecondRangeLockContext, GetThisAllocator(), AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    _IsCopy = FALSE;
    _IsWrite = FALSE;
    _WasCopy = FALSE;
    _IoPriority = KBlockFile::eBackground;
    _Offset = 0;
    _Length = 0;
    _ReadIoBufferElement = NULL;
    _TargetOffset = 0;
    _HasCopyCacheHit = FALSE;

    return STATUS_SUCCESS;
}

VOID
KCachedBlockFile::TransferContext::InitializeForReadSingle(
    __inout KCachedBlockFile& CachedFile,
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __inout KIoBufferElement::SPtr& IoBufferElement
    )

/*++

Routine Description:

    This routine initializes this transfer context for a read operation.

Arguments:

    CachedFile  - Supplies the cached file object.

    Priority    - Supplies the IO priority.

    Offset      - Supplies the offset.

    Length      - Supplies the length.

    IoBuffer    - Returns the IO buffer of the read data.

Return Value:

    None.

--*/

{
    _IsCopy = FALSE;
    _IsWrite = FALSE;
    _WasCopy = FALSE;
    _CachedFile = &CachedFile;
    _IoPriority = Priority;
    _Offset = Offset;
    _Length = IoBufferElement->QuerySize();
    _IoBufferElement = IoBufferElement;
    _IoBuffer = NULL;
    _ReadIoBufferElement = &IoBufferElement;
    _TargetOffset = 0;
    _HasCopyCacheHit = FALSE;
}

VOID
KCachedBlockFile::TransferContext::InitializeForWrite(
    __inout KCachedBlockFile& CachedFile,
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __in KIoBufferElement& IoBufferElement
    )

/*++

Routine Description:

    This routine initializes this transfer context for a write operation.

Arguments:

    CachedFile      - Supplies the cached file object.

    Priority        - Supplies the IO priority.

    Offset          - Supplies the offset.

    IoBufferElement - Supplies the IO buffer element to write.

Return Value:

    None.

--*/

{
    _IsCopy = FALSE;
    _IsWrite = TRUE;
    _WasCopy = FALSE;
    _CachedFile = &CachedFile;
    _IoPriority = Priority;
    _Offset = Offset;
    _Length = IoBufferElement.QuerySize();
    _IoBufferElement = &IoBufferElement;
    _IoBuffer = NULL;
    _ReadIoBufferElement = NULL;
    _TargetOffset = 0;
    _HasCopyCacheHit = FALSE;
}

VOID
KCachedBlockFile::TransferContext::InitializeForWriteIoBuffer(
    __inout KCachedBlockFile& CachedFile,
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer
    )
{
    _IsCopy = FALSE;
    _IsWrite = TRUE;
    _WasCopy = FALSE;
    _CachedFile = &CachedFile;
    _IoPriority = Priority;
    _Offset = Offset;
    _Length = IoBuffer.QuerySize();
    _IoBufferElement = NULL;
    _IoBuffer = &IoBuffer;
    _ReadIoBufferElement = NULL;
    _TargetOffset = 0;
    _HasCopyCacheHit = FALSE;
}

VOID
KCachedBlockFile::TransferContext::InitializeForCopy(
    __inout KCachedBlockFile& CachedFile,
    __in KBlockFile::IoPriority Priority,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in KIoBufferElement& IoBufferElement
    )

/*++

Routine Description:

    This routine initializes this transfer context for a copy operation.

Arguments:

    CachedFile      - Supplies the cached file object.

    Priority        - Supplies the IO priority.

    SourceOffset    - Supplies the source offset.

    TargetOffset    - Supplies the target offset.

    IoBufferElement - Supplies the IO buffer element to use for the copy.

Return Value:

    None.

--*/

{
    _IsCopy = TRUE;
    _IsWrite = FALSE;
    _WasCopy = TRUE;
    _CachedFile = &CachedFile;
    _IoPriority = Priority;
    _Offset = SourceOffset;
    _Length = IoBufferElement.QuerySize();
    _IoBufferElement = &IoBufferElement;
    _IoBuffer = NULL;
    _ReadIoBufferElement = NULL;
    _TargetOffset = TargetOffset;
    _HasCopyCacheHit = FALSE;
}

VOID
KCachedBlockFile::TransferContext::OnStart(
    )

/*++

Routine Description:

    This routine is the start routine for the transfer, either read-single, write, or copy.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KAsyncContextBase::CompletionCallback completion(this, &TransferContext::RangeLockCompletion);
    ULONGLONG offset = _Offset;

    //
    // As a first step, acquire the range.  In the case of a copy, acquire the leftmost range first to avoid a deadlock.
    //

    if (_WasCopy && _TargetOffset < _Offset) {
        offset = _TargetOffset;
    }

    _CachedFile->_RangeLock->AcquireRange(*_RangeLockContext, offset, _Length, completion, this);
}

VOID
KCachedBlockFile::TransferContext::OnCancel(
    )
{
    _LowerTransferContext->Cancel();
    _RangeLockContext->Cancel();
    _SecondRangeLockContext->Cancel();
}

VOID
KCachedBlockFile::TransferContext::OnReuse(
    )
{
    _LowerTransferContext->Reuse();
    _RangeLockContext->Reuse();
    _SecondRangeLockContext->Reuse();
    _CachedFile = NULL;
    _IoBufferElement = NULL;
    _IoBuffer = NULL;
}

VOID
KCachedBlockFile::TransferContext::RangeLockCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )

/*++

Routine Description:

    This routine is called when the range lock acquire is completed.

Arguments:

    ParentAsync - Supplies the parent async.

    Async       - Supplies the completing operation async.

Return Value:

    None.

--*/

{
    NTSTATUS status = Async.Status();
    KAsyncContextBase::CompletionCallback completion(this, &TransferContext::SecondRangeLockCompletion);
    ULONGLONG offset;

    UNREFERENCED_PARAMETER(ParentAsync);    

    //
    // If this lock acquire failed, i.e. was cancelled, then just complete the request.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // If this is a copy then acquire the other range.
    //

    if (_WasCopy) {

        offset = _Offset;

        if (offset < _TargetOffset) {
            offset = _TargetOffset;
        }

        _CachedFile->_RangeLock->AcquireRange(*_SecondRangeLockContext, offset, _Length, completion, this);

        return;
    }

    //
    // This is a not a copy, just invoke the remaining logic directly.
    //

    SecondRangeLockCompletion(ParentAsync, Async);
}

VOID
KCachedBlockFile::TransferContext::SecondRangeLockCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )

/*++

Routine Description:

    This routine is called when the second range lock acquire is completed.

Arguments:

    ParentAsync - Supplies the parent async.

    Async       - Supplies the completing operation async.

Return Value:

    None.

--*/

{
    NTSTATUS status = Async.Status();
    KAsyncContextBase::CompletionCallback completion(this, &TransferContext::TransferCompletion);
    BOOLEAN b;
    ULONGLONG fileOffset;
    KIoBufferElement::SPtr ioBufferElement;
    KIoBufferElement::SPtr ioBufferElementReference;
    ULONG length;
    ULONG delta;

    UNREFERENCED_PARAMETER(ParentAsync);                

    //
    // If this range lock failed, i.e. was cancelled, then complete the request.
    //

    if (!NT_SUCCESS(status)) {
        _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    if (_IsCopy || !_IsWrite) {

        //
        // This is a copy or a read-single.  Look into the cache and see if there is a cache hit here.
        //

        b = _CachedFile->_ReadCache->Query(_CachedFile->_FileId, _Offset, fileOffset, ioBufferElement);

        if (b) {

            //
            // Check the offset and buffer element returned to make sure that the range we want is really there.
            //

            KFatal(fileOffset <= _Offset);

            delta = (ULONG) (_Offset - fileOffset);

            length = ioBufferElement->QuerySize();

            KFatal(length > delta);

            length -= delta;

            if (length < _Length) {

                //
                // We didn't get everything that we wanted.  So, just proceed with the full IO.
                //

                b = FALSE;

            } else {

                //
                // Put together an exact IO buffer element.
                //

                if (!delta && length == _Length) {

                    //
                    // We can just take this io buffer element as is.
                    //

                    _IoBufferElement = ioBufferElement;

                } else {

                    status = KIoBufferElement::CreateReference(*ioBufferElement, delta, _Length, ioBufferElementReference,
                            GetThisAllocator(), KTL_TAG_CACHED_BLOCK_FILE);

                    if (NT_SUCCESS(status)) {

                        //
                        // Take this new IO buffer element reference as our IO buffer.
                        //

                        _IoBufferElement = ioBufferElementReference;

                    } else {

                        //
                        // The allocation failed.  Do the buffer copy as a last resort.
                        //

                        KMemCpySafe(
                            (VOID*)_IoBufferElement->GetBuffer(), 
                            _IoBufferElement->QuerySize(), 
                            (UCHAR*)ioBufferElement->GetBuffer() + delta,
                            _Length);
                    }
                }
            }

            if (b) {

                //
                // We have a good cache hit.  Complete if a read, convert to write if a copy.
                //

                if (!_IsCopy) {

                    //
                    // This is a read.  Complete it.
                    //

                    *_ReadIoBufferElement = _IoBufferElement;

                    _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);

                    //
                    // This read was completed from cache.  Adjust the performance counters accordingly.
                    //

                    _CachedFile->_ReadCount++;
                    _CachedFile->_BytesRead += _Length;
                    _CachedFile->_CachedReadCount++;
                    _CachedFile->_CachedBytesRead += _Length;

                    Complete(STATUS_SUCCESS);
                    return;
                }

                //
                // Convert this copy into a write.
                //

                _IsCopy = FALSE;
                _IsWrite = TRUE;
                _Offset = _TargetOffset;

                //
                // Remember that we have a cache hit on the copy.
                //

                _HasCopyCacheHit = TRUE;
            }
        }

        if (!b) {

            //
            // In this case, there is no cache hit.  Send down either the copy or the read.
            //

            if (_IsCopy) {
                status = _CachedFile->_File->Copy(_IoPriority, _Offset, _TargetOffset, (VOID*) _IoBufferElement->GetBuffer(),
                        _IoBufferElement->QuerySize(), completion, this, _LowerTransferContext.RawPtr());
            } else {
                status = _CachedFile->_File->Transfer(_IoPriority, KBlockFile::eRead, _Offset,
                        (VOID*) _IoBufferElement->GetBuffer(), _IoBufferElement->QuerySize(), completion, this,
                        _LowerTransferContext.RawPtr());
            }

            KFatal(status == STATUS_PENDING);

            return;
        }
    }

    //
    // This is a write or a copy that converted to a write.  Send it down.
    //

    if (_IoBufferElement) {
        status = _CachedFile->_File->Transfer(_IoPriority, KBlockFile::eWrite, _Offset, (VOID*) _IoBufferElement->GetBuffer(),
                _IoBufferElement->QuerySize(), completion, this, _LowerTransferContext.RawPtr());
    } else {
        status = _CachedFile->_File->Transfer(_IoPriority, KBlockFile::eWrite, _Offset, *_IoBuffer, completion, this,
                _LowerTransferContext.RawPtr());
    }

    KFatal(status == STATUS_PENDING);
}

VOID
KCachedBlockFile::TransferContext::TransferCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )

/*++

Routine Description:

    This routine is the transfer completion of a copy, read-single, or write request.

Arguments:

    ParentAsync - Supplies the parent async.

    Async       - Supplies the completing request.

Return Value:

    None.

--*/

{
    NTSTATUS status = Async.Status();
    KIoBufferElement* ioBufferElement;
    ULONGLONG offset;

    UNREFERENCED_PARAMETER(ParentAsync);    

    if (NT_SUCCESS(status) && !_WasCopy) {

        //
        // This is a successful request, so, update the cache.
        //

        if (_IsWrite && _CachedFile->_NoCachingOnWrite) {
            _CachedFile->_ReadCache->RemoveRange(_CachedFile->_FileId, _Offset, _Length);
        } else {
            if (_IoBufferElement) {
                status = _CachedFile->_ReadCache->Add(_CachedFile->_FileId, _Offset, *_IoBufferElement);
                KAssert(NT_SUCCESS(status));
            } else {

                offset = _Offset;

                for (ioBufferElement = _IoBuffer->First(); ioBufferElement;
                        ioBufferElement = _IoBuffer->Next(*ioBufferElement)) {

                    status = _CachedFile->_ReadCache->Add(_CachedFile->_FileId, offset, *ioBufferElement);
                    KAssert(NT_SUCCESS(status));

                    offset += ioBufferElement->QuerySize();
                }
            }
        }
    }

    _CachedFile->_RangeLock->ReleaseRange(*_RangeLockContext);

    if (_WasCopy) {
        _CachedFile->_RangeLock->ReleaseRange(*_SecondRangeLockContext);
    }

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    //
    // Update the performance counters.
    //

    if (_WasCopy) {
        _CachedFile->_CopyCount++;
        _CachedFile->_BytesCopied += _Length;
        if (_HasCopyCacheHit) {
            _CachedFile->_CachedCopyCount++;
            _CachedFile->_CachedBytesCopied += _Length;
        }
    } else if (_IsWrite) {
        _CachedFile->_WriteCount++;
        _CachedFile->_BytesWritten += _Length;
    } else {
        _CachedFile->_ReadCount++;
        _CachedFile->_BytesWritten++;
    }

    Complete(status);
}
