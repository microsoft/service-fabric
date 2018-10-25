/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    KBlockDevice.cpp

Abstract:

    This file contains several implementations of the KBlockDevice abstraction.

    A block device supports block-level reads and writes.
    It is normally backed by a disk file. But it can also be backed by RAM.
    From the user's perspective, it issues I/Os against a block-addressable device
    with a linear address space.

Author:

    Peng Li (pengli)    24-Jan-2011

Environment:

    Kernel mode and User mode

Notes:

--*/

#include "ktl.h"
#include "ktrace.h"

//
// A block device implemented on top of a KBlockFile object.
//


class KDiskBlockDevice : public KBlockDevice
{
    K_FORCE_SHARED(KDiskBlockDevice);

public:
    FAILABLE
    KDiskBlockDevice(
        __in const KGuid& DeviceId,
        __in ULONGLONG DeviceSize,
        __in BOOLEAN IsWriteThrough,
        __in const KWString& FullyQualifiedDiskFileName,
        __in_opt KReadCache* const ReadCache,
        __in BOOLEAN NoCachingOnWrite,
        __in BOOLEAN UseSparseFile,
        __in_opt ULONG ReadAheadSize,
        __in KBlockFile::CreateOptions Options,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        );

    NTSTATUS
    Mount(
        __in BOOLEAN CreateNew,
        __in_opt KReadCache* ReadCache,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr
        );

    VOID
    QueryDeviceAttributes(
        __out DeviceAttributes& Attributes
        ) override;

    VOID
    QueryDeviceHandle(
        __out HANDLE& Handle
        ) override;

    NTSTATUS
    AllocateReadContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) override;

    NTSTATUS
    AllocateNonContiguousReadContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) override;

    NTSTATUS
    AllocateWriteContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) override;

    NTSTATUS
    AllocateFlushContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) override;

    NTSTATUS
    AllocateTrimContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) override;

    NTSTATUS
    AllocateSetSystemIoPriorityHintContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) override;

    NTSTATUS
    AllocateQueryAllocationsContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) override;

    NTSTATUS
    Read(
        __in ULONGLONG Offset,
        __in ULONG Length,
        __in BOOLEAN ContiguousIoBuffer,
        __out KIoBuffer::SPtr& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override;

    NTSTATUS
    ReadNonContiguous(
        __in ULONGLONG Offset,
        __in ULONG ReadLength,
        __in ULONG BufferLength,
        __out KIoBuffer::SPtr& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr,
        __in_opt KArray<KAsyncContextBase::SPtr>* const BlockDeviceReadOps = nullptr
        ) override;

    NTSTATUS
    Write(
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override;

    NTSTATUS
    Write(
        __in KBlockFile::IoPriority IoPriority,
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override;

    NTSTATUS
    Write(
        __in KBlockFile::IoPriority IoPriority,
        __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override;

    NTSTATUS
    Flush(
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override;

    NTSTATUS
    Trim(
        __in ULONGLONG TrimFrom,
        __in ULONGLONG TrimToPlusOne,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override;

    NTSTATUS
    QueryAllocations(
        __in ULONGLONG QueryFrom,
        __in ULONGLONG Length,
        __inout KArray<KBlockFile::AllocatedRange>& Results,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override;

    NTSTATUS
    SetSystemIoPriorityHint(
        __in KBlockFile::SystemIoPriorityHint Hint,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) override ;

    NTSTATUS
    SetSparseFile(
        __in BOOLEAN IsSparseFile
        ) override ;
    
    VOID
    Close(
        ) override;

private:
    class KReadAheadContextImp;
    class KReadAheadContextOp;
    class KWriteAheadContextOp;

    NTSTATUS CreateKReadAheadContextImp(
        __in ULONG ReadAheadSize,
        __out KSharedPtr<KDiskBlockDevice::KReadAheadContextImp>& Context
        );

    NTSTATUS
    CreateKWriteAheadContextOp(
        __out KSharedPtr<KDiskBlockDevice::KWriteAheadContextOp>& Context
    );

    NTSTATUS
    CreateKReadAheadContextOp(
        __out KSharedPtr<KDiskBlockDevice::KReadAheadContextOp>& Context
    );

    __inline BOOLEAN
    IsReadAheadEnabled()
    {
        return(_ReadAheadContext);
    }

    __inline BOOLEAN
    IsReadAheadEligible(
        __in ULONGLONG Offset,
        __in ULONG Length
    )
    {
        //
        // Reads are eligible when the length of the read is smaller
        // than our read ahead size and the read is done at an offset
        // where there is enough space left to read ahead the full read
        // ahead buffer
        //
        return( (Length < _ReadAheadContext->_ReadAheadSize) &&
                (Offset < (_File->QueryFileSize() - _ReadAheadContext->_ReadAheadSize)) );
    };

    class KReadAheadContextImp : public KObject<KReadAheadContextImp>, public KShared<KReadAheadContextImp>
    {
        friend KBlockDevice;
        friend KDiskBlockDevice;
        friend KReadAheadContextOp;
        friend KWriteAheadContextOp;

        K_FORCE_SHARED(KReadAheadContextImp);

        KReadAheadContextImp(
            __in KDiskBlockDevice& BlockDevice,
            __in ULONG ReadAheadSize);

    private:
        VOID Invalidate(
            __in ULONGLONG Offset
            );

        VOID SetReadAheadBuffer(
            __in_opt KIoBuffer* ReadAheadBuffer,
            __in ULONGLONG BaseOffset,
            __in ULONGLONG TopReadAheadOffset
       );

        VOID UpdateReadAheadBuffer(
            __in KIoBuffer& ReadAheadBuffer,
            __in ULONGLONG BaseOffset,
            __in ULONGLONG TopReadAheadOffset
       );

        NTSTATUS TryGetFromReadAheadBuffer(
            __in ULONGLONG Offset,
            __in ULONG Length,
            __out KIoBuffer::SPtr& IoBuffer
        );

        BOOLEAN
        InvalidateReadAheadIfNeeded(
            __in ULONGLONG Offset,
            __in ULONG Length
        );


        //
        // General members
        //
        KSpinLock _Lock;
        KDiskBlockDevice* _BlockDevice;
        ULONG _ReadAheadSize;

        ULONGLONG _BaseOffset;
        ULONGLONG _TopReadAheadOffset;
        KIoBuffer::SPtr _ReadAheadBuffer;
    };

    class KReadAheadContextOp : public KAsyncContextBase
    {

        K_FORCE_SHARED(KReadAheadContextOp);

        friend KDiskBlockDevice;

        KReadAheadContextOp(
            __in KDiskBlockDevice& BlockDevice
            );

        VOID
        StartRead(
            __in ULONGLONG Offset,
            __in ULONG Length,
            __out KIoBuffer::SPtr& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync
            );

    private:
        VOID ReadCompleted(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

    protected:
        VOID
            OnStart(
            ) override;

        VOID
            OnReuse(
            ) override;

        VOID
            OnCompleted(
            ) override;

        VOID
            OnCancel(
            ) override;

        //
        // General members
        //
        KDiskBlockDevice::SPtr _BlockDevice;
        KBlockFile::TransferContext::SPtr _TransferContext;

        //
        // Parameters to api
        //
        ULONGLONG _Offset;
        ULONG _Length;
        KIoBuffer::SPtr* _IoBuffer;

        //
        // Members needed for functionality
        //
        KIoBuffer::SPtr _ReadAheadBufferInProgress;
    };

    class KWriteAheadContextOp : public KAsyncContextBase
    {

        K_FORCE_SHARED(KWriteAheadContextOp);

        friend KDiskBlockDevice;

        KWriteAheadContextOp(
            __in KDiskBlockDevice& _BlockDevice
            );

        VOID
        StartWrite(
            __in KBlockFile::IoPriority IoPriority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync
            );

    private:
        VOID WriteCompleted(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

    protected:
        VOID
            OnStart(
            ) override;

        VOID
            OnReuse(
            ) override;

        VOID
            OnCompleted(
            ) override;

        VOID
            OnCancel(
            ) override;

        //
        // General members
        //
        KDiskBlockDevice::SPtr _BlockDevice;
        KBlockFile::TransferContext::SPtr _TransferContext;

        //
        // Parameters to api
        //
        KBlockFile::IoPriority _IoPriority;
        KBlockFile::SystemIoPriorityHint _IoPriorityHint;
        ULONGLONG _Offset;
        KIoBuffer::SPtr _IoBuffer;

        //
        // Members needed for functionality
        //
        ULONG _Length;
    };

private:
    class MountContext;
    class CachedReadContext;
    class NonContiguousReadContext;

    DeviceAttributes _Attributes;
    KBlockFile::SPtr _File;
    KWString _FileName;
    BOOLEAN _UseSparseFile;
    KBlockFile::CreateOptions _Options;

    const ULONG _AllocationTag;

    KCachedBlockFile::SPtr _CachedFile;

    KReadAheadContextImp::SPtr _ReadAheadContext;
};

//
// This is the async context for the KDiskBlockDevice::Mount() API.
//

class KDiskBlockDevice::MountContext : public KAsyncContextBase
{
    K_FORCE_SHARED(MountContext);
public:
    NOFAIL
    MountContext(
        __in KDiskBlockDevice& BlockDevice
        );

    VOID
    StartMount(
        __in BOOLEAN CreateNew,
        __in_opt KReadCache* ReadCache,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr
        );

private:
    VOID
    OnStart();

    VOID
    OnReuse();

    VOID
    CreateFileCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    SetFileSizeCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    CauseFileDeletion(NTSTATUS StatusToCompleteWith);

    VOID
    DeleteFileCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    KDiskBlockDevice::SPtr _BlockDevice;
    KBlockFile::SPtr _File;
    BOOLEAN _CreateNew;
    NTSTATUS _CreationFailureStatus;

    KReadCache::SPtr _ReadCache;

};

//
// This is the async context for the KDiskBlockDevice::Read() API, whenever the KDiskBlockDevice is in cached mode.
//

class KDiskBlockDevice::CachedReadContext : public KAsyncContextBase {

    K_FORCE_SHARED(CachedReadContext);

    FAILABLE
    CachedReadContext(
        __in KDiskBlockDevice& BlockDevice
        );

    public:

    private:

        NTSTATUS
        Initialize(
            __in KDiskBlockDevice& BlockDevice
            );

        VOID
        InitializeForRead(
            __inout KDiskBlockDevice& BlockDevice,
            __in ULONGLONG Offset,
            __in ULONG Length,
            __in BOOLEAN ContiguousIoBuffer,
            __out KIoBuffer::SPtr& IoBuffer
            );

        VOID
        OnStart(
            );

        VOID
        OnCancel(
            );

        VOID
        OnReuse(
            );

        VOID
        ContinueRoutine(
            __in NTSTATUS LastStatus
            );

        VOID
        AsyncCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        friend class KDiskBlockDevice;

        KCachedBlockFile::ReadContext::SPtr _ReadContext;

        KDiskBlockDevice::SPtr _BlockDevice;
        ULONGLONG _Offset;
        ULONG _Length;
        BOOLEAN _ContiguousIoBuffer;
        KIoBuffer::SPtr* _IoBuffer;

        ULONG _Phase;

};

//
// This is the async context for the KDiskBlockDevice::ReadNonContiguous() API
//
class KDiskBlockDevice::NonContiguousReadContext : public KAsyncContextBase {

    K_FORCE_SHARED(NonContiguousReadContext);

    private:

        VOID
        Initialize(
            );

        NTSTATUS
        InitializeForRead(
            __inout KDiskBlockDevice& BlockDevice,
            __in ULONGLONG Offset,
            __in ULONG ReadLength,
            __in ULONG BufferLength,
            __in_opt KArray<KAsyncContextBase::SPtr>* BlockDeviceReadOps,
            __out KIoBuffer::SPtr& IoBuffer
            );

        VOID
        OnStart(
            ) override ;

        VOID
        OnCancel(
            ) override ;

        VOID
        OnReuse(
            ) override ;

        VOID
        OnCompleted(
            ) override ;

        VOID
        ContinueRoutine(
            __in NTSTATUS LastStatus
            );

        enum { UninitializedForRead  = -1, Initial = 0, Reading = 1, AggregateReads = 2 } _State;

        VOID
        AsyncCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        friend class KDiskBlockDevice;

        KDiskBlockDevice::SPtr _BlockDevice;
        ULONGLONG _Offset;
        ULONG _ReadLength;
        ULONG _BufferLength;
        KArray<KAsyncContextBase::SPtr> _BlockDeviceReadOps;
        KIoBuffer::SPtr* _IoBuffer;

        KIoBuffer::SPtr _AggregateIoBuffer;
        KArray<KIoBuffer::SPtr> _IoBuffers;
        ULONG _OutstandingReads;
        NTSTATUS _FinalStatus;
};


KDiskBlockDevice::KDiskBlockDevice(
    __in const KGuid& DeviceId,
    __in ULONGLONG DeviceSize,
    __in BOOLEAN IsWriteThrough,
    __in const KWString& FullyQualifiedDiskFileName,
    __in_opt KReadCache* const ReadCache,
    __in BOOLEAN NoCachingOnWrite,
    __in BOOLEAN UseSparseFile,
    __in_opt ULONG ReadAheadSize,
    __in KBlockFile::CreateOptions Options,
    __in ULONG AllocationTag
    ) :
    _AllocationTag(AllocationTag),
    _FileName(GetThisAllocator())

/*++

Routine Description:

    This routine is the ctor of KDiskBlockDevice.

Arguments:

    DeviceId - Supplies the logic ID of this block device.

    DeviceSize - Supplies the size of this device in bytes. It must be multiples of KBlockDevice::_BlockSize.

    IsWriteThrough - Indicates if writes to this device are all write-through.
        A 'TRUE' flag may be neccessary for correctness in certain scenarios, such as logging,
        but may have degraded write performance.

    FullyQualifiedDiskFileName - Supplies the fully qualified name of the file that provides storage for this device.
        For example, L"\\Device\\HarddiskVolume1\\myfile.dat" and L"\\??\\C:\\myfile.dat" are both fully qualified file names.
        See NtCreateFile() for more information of fully qualified file names.

    AllocationTag - Supplies the default tag for internal allocations.

    Allocator - Supplies the default allocator.

Return Value:

    None

--*/

{
    NTSTATUS status;

    KAssert(DeviceSize % KBlockDevice::_BlockSize == 0);

    KDiskBlockDevice::KReadAheadContextImp::SPtr readAheadContext = nullptr;

    if (ReadAheadSize > 0)
    {
        status = CreateKReadAheadContextImp(ReadAheadSize, readAheadContext);
        if (!NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return;
        }
    }

    _ReadAheadContext = readAheadContext;

    _Attributes.DeviceId = DeviceId;
    _Attributes.DeviceSize = DeviceSize;
    _Attributes.IsPersistent = TRUE;
    _Attributes.IsWriteThrough = IsWriteThrough;
    _Attributes.IsReadCacheEnabled = ReadCache ? TRUE : FALSE;
    _Attributes.NoCachingOnWrite = NoCachingOnWrite;

    _UseSparseFile = UseSparseFile;
    _Options = Options;

    _File = nullptr;

    _FileName = FullyQualifiedDiskFileName;
    status = _FileName.Status();
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _CachedFile = nullptr;
}

KDiskBlockDevice::~KDiskBlockDevice()

/*++

Routine Description:

    This routine is the dtor of KDiskBlockDevice.

Arguments:

    None

Return Value:

    None

--*/

{
}

NTSTATUS
KDiskBlockDevice::Mount(
    __in BOOLEAN CreateNew,
    __in_opt KReadCache* ReadCache,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine 'mounts' the block device. If CreateNew is 'TRUE', it creates a file and sets the file size to the device size.
    Otherwise, it opens the existing file and sets the file size.

Arguments:

    CreateNew - Indicates if we want to create a new device, or open an existing one.

    ReadCache - Supplies, optionally, a read cache to use for caching file data.

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    MountContext::SPtr async = _new(_AllocationTag, GetThisAllocator()) KDiskBlockDevice::MountContext(*this);
    if (!async)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = async->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    async->StartMount(CreateNew, ReadCache, Completion, ParentAsync);
    return STATUS_PENDING;
}

//
// Implementation of KDiskBlockDevice::MountContext
//

KDiskBlockDevice::MountContext::MountContext(
    __in KDiskBlockDevice& BlockDevice
    ) :
    _BlockDevice(&BlockDevice),
    _File(nullptr)

/*++

Routine Description:

    This routine is the ctor of MountContext.

Arguments:

    BlockDevice - Supplies the block device object.

Return Value:

    None

--*/

{
}

KDiskBlockDevice::MountContext::~MountContext()

/*++

Routine Description:

    This routine is the dtor of MountContext.

Arguments:

    None

Return Value:

    None

--*/

{
}

VOID
KDiskBlockDevice::MountContext::OnReuse()

/*++

Routine Description:

    This routine is invoked when Reuse() is called on this object.

Arguments:

    None

Return Value:

    None

--*/

{
    _BlockDevice = nullptr;
    _File = nullptr;
    _ReadCache = nullptr;
}

VOID
KDiskBlockDevice::MountContext::StartMount(
    __in BOOLEAN CreateNew,
    __in_opt KReadCache* ReadCache,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine starts mounting the block device.

Arguments:

    CreateNew - Indicates if we want to create a new device, or open an existing one.

    ReadCache - Supplies, optionally, a read cache to use with the file.

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

Return Value:

    None

--*/

{
    _CreateNew = CreateNew;
    _ReadCache = ReadCache;
    Start(ParentAsync, Completion);
}

VOID
KDiskBlockDevice::MountContext::OnStart()

/*++

Routine Description:

    This routine mounts the block device. It first creates the disk file, then sets the file size to the device size.

Arguments:

    None

Return Value:

    None

--*/

{
    KBlockFile::CreateDisposition disposition;

    if (_CreateNew)
    {
        disposition = KBlockFile::eCreateNew;
    }
    else
    {
        disposition = KBlockFile::eOpenExisting;
    }

    KAsyncContextBase::CompletionCallback callback(this, &KDiskBlockDevice::MountContext::CreateFileCompletion);
    NTSTATUS status;

    if (_BlockDevice->_UseSparseFile)
    {
        status = KBlockFile::CreateSparseFile(
            _BlockDevice->_FileName,                    // FileName
            _BlockDevice->_Attributes.IsWriteThrough,   // IsWriteThrough
            disposition,                                // Disposition
            _BlockDevice->_Options,                     // Create Options
            _File,                                      // File
            callback,                                   // Completion
            this,                                       // ParentAsync
            GetThisAllocator(),                         // Allocator
            _BlockDevice->_AllocationTag                // AllocationTag
            );
    }
    else {
        status = KBlockFile::Create(
            _BlockDevice->_FileName,                    // FileName
            _BlockDevice->_Attributes.IsWriteThrough,   // IsWriteThrough
            disposition,                                // Disposition
            _BlockDevice->_Options,                     // Create Options
            _File,                                      // File
            callback,                                   // Completion
            this,                                       // ParentAsync
            GetThisAllocator(),                         // Allocator
            _BlockDevice->_AllocationTag                // AllocationTag
            );
    }

    if (!K_ASYNC_SUCCESS(status))
    {
        Complete(status);
        return;
    }
}

VOID
KDiskBlockDevice::MountContext::CreateFileCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )

/*++

Routine Description:

    This is the async completion routine for KBlockFile::Create().

Arguments:

    Parent - Supplies the parent async

    CompletingOperation - Supplies the async op that is completing.

Return Value:

    None

--*/

{
    KAssert(Parent == static_cast<KAsyncContextBase*>(this));
#if !DBG
    UNREFERENCED_PARAMETER(Parent);
#endif

    NTSTATUS status = CompletingOperation.Status();
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    KAssert(_File);

    if (!_CreateNew)
    {
        // KBlockFile::eOpenExisting path; capture the file size

        if (_ReadCache)
        {
            status = KCachedBlockFile::Create(_BlockDevice->_Attributes.DeviceId, *_File, *_ReadCache, _BlockDevice->_CachedFile,
                    GetThisAllocator(), _BlockDevice->_AllocationTag, FALSE);
            _BlockDevice->_File = nullptr;
        }
        else
        {
            _BlockDevice->_File = _File;
            _BlockDevice->_CachedFile = nullptr;
        }

        _BlockDevice->_Attributes.DeviceSize = _File->QueryFileSize();

        Complete(STATUS_SUCCESS);
        return;
    }

    //
    // Set the file size to the device size.
    //
    // NOTE: From this point on any failures must cause the file just created to be deleted
    KAsyncContextBase::CompletionCallback callback(this, &KDiskBlockDevice::MountContext::SetFileSizeCompletion);
    status = _File->SetFileSize(
        _BlockDevice->_Attributes.DeviceSize,   // FileSize
        callback,                               // Completion
        this                                    // ParentAsync
        );
    if (!K_ASYNC_SUCCESS(status))
    {
        CauseFileDeletion(status);
        return;
    }
}

VOID
KDiskBlockDevice::MountContext::SetFileSizeCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )

/*++

Routine Description:

    This is the async completion routine for KBlockFile::Create().

Arguments:

    Parent - Supplies the parent async

    CompletingOperation - Supplies the async op that is completing.

Return Value:

    None

--*/

{
    KAssert(Parent == static_cast<KAsyncContextBase*>(this));
#if !DBG
    UNREFERENCED_PARAMETER(Parent);
#endif

    NTSTATUS status = CompletingOperation.Status();
    if (NT_SUCCESS(status))
    {
        if (_ReadCache)
        {
            status = KCachedBlockFile::Create(_BlockDevice->_Attributes.DeviceId, *_File, *_ReadCache, _BlockDevice->_CachedFile,
                    GetThisAllocator(), _BlockDevice->_AllocationTag, _BlockDevice->_Attributes.NoCachingOnWrite);
            _BlockDevice->_File = nullptr;
        }
        else
        {
            _BlockDevice->_File = _File;
            _BlockDevice->_CachedFile = nullptr;
        }
    }

    if (!NT_SUCCESS(status))
    {
        // SetSize (or cached file creation) failed!
        KAssert(_CreateNew);
        CauseFileDeletion(status);
        return;
    }

    Complete(status);
}

VOID
KDiskBlockDevice::MountContext::CauseFileDeletion(NTSTATUS StatusToCompleteWith)
//  Continued from SetFileSizeCompletion() OR CreateFileCompletion()
{
    KAssert(!NT_SUCCESS(StatusToCompleteWith));
    KAssert(_CreateNew);
    _CreationFailureStatus = StatusToCompleteWith;

    _File->Close();
    _File.Reset();

    KAsyncContextBase::CompletionCallback callback(this, &KDiskBlockDevice::MountContext::DeleteFileCompletion);
    NTSTATUS status = KVolumeNamespace::DeleteFileOrDirectory(_BlockDevice->_FileName, GetThisAllocator(), callback, this);
    if (!K_ASYNC_SUCCESS(status))
    {
        // BUG, pengli, xxxxx, Add trace output of StatusToCompleteWith
        Complete(status);
        return;
    }

    // Continued @ DeleteFileCompletion()
}

VOID
KDiskBlockDevice::MountContext::DeleteFileCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
//  Continued from CauseFileDeletion()
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletingOperation.Status();
    if (!NT_SUCCESS(status))
    {
        // BUG, pengli, xxxxx, Add trace output of _CreationFailureStatus
        Complete(status);
        return;
    }

    Complete(_CreationFailureStatus);
}

KDiskBlockDevice::CachedReadContext::~CachedReadContext(
    )
{
    // Nothing.
}

KDiskBlockDevice::CachedReadContext::CachedReadContext(
    __in KDiskBlockDevice& BlockDevice
    )
{
    SetConstructorStatus(Initialize(BlockDevice));
}

NTSTATUS
KDiskBlockDevice::CachedReadContext::Initialize(
    __in KDiskBlockDevice& BlockDevice
    )
{
    NTSTATUS status;

    status = BlockDevice._CachedFile->AllocateRead(_ReadContext, BlockDevice._AllocationTag);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    _IoBuffer = NULL;
    _Phase = 0;

    return STATUS_SUCCESS;
}

VOID
KDiskBlockDevice::CachedReadContext::InitializeForRead(
    __inout KDiskBlockDevice& BlockDevice,
    __in ULONGLONG Offset,
    __in ULONG Length,
    __in BOOLEAN ContiguousIoBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    )
{
    _BlockDevice = &BlockDevice;
    _Offset = Offset;
    _Length = Length;
    _ContiguousIoBuffer = ContiguousIoBuffer;
    _IoBuffer = &IoBuffer;
    _Phase = 1;
}

VOID
KDiskBlockDevice::CachedReadContext::OnStart(
    )
{
    ContinueRoutine(STATUS_SUCCESS);
}

VOID
KDiskBlockDevice::CachedReadContext::OnCancel(
    )
{
    _ReadContext->Cancel();
}

VOID
KDiskBlockDevice::CachedReadContext::OnReuse(
    )
{
    _ReadContext->Reuse();
    _BlockDevice = NULL;
}

VOID
KDiskBlockDevice::CachedReadContext::ContinueRoutine(
    __in NTSTATUS LastStatus
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &CachedReadContext::AsyncCompletion);
    NTSTATUS status = LastStatus;
    KIoBuffer::SPtr ioBuffer;
    VOID* buffer;
    ULONG offset;
    KIoBufferElement* ioBufferElement;
    ULONG size;

    switch (_Phase) {
        case 1:
            KFatal(NT_SUCCESS(status));

            _Phase = 2;

            //
            // Send the read down to the cached file.
            //

            status = _BlockDevice->_CachedFile->Read(KBlockFile::eForeground, _Offset, _Length, *_IoBuffer, completion, this,
                    _ReadContext.RawPtr());

            KFatal(status == STATUS_PENDING);

            break;

        case 2:
            if (!NT_SUCCESS(status)) {
                KTraceFailedAsyncRequest(status, this, 0, 0);
                Complete(status);
                break;
            }

            _Phase = 3;

            //
            // If a contigous request was made then verify that the result is contiguous, otherwise allocate and copy.
            //

            if (!_ContiguousIoBuffer || (*_IoBuffer)->QueryNumberOfIoBufferElements() == 1) {
                Complete(STATUS_SUCCESS);
                break;
            }

            status = KIoBuffer::CreateSimple(_Length, ioBuffer, buffer, GetThisAllocator(), _BlockDevice->_AllocationTag);

            if (!NT_SUCCESS(status)) {
                KTraceFailedAsyncRequest(status, this, 0, 0);
                Complete(status);
                break;
            }

            offset = 0;

            for (ioBufferElement = (*_IoBuffer)->First(); ioBufferElement;
                    ioBufferElement = (*_IoBuffer)->Next(*ioBufferElement)) {

                size = ioBufferElement->QuerySize();

                KMemCpySafe((UCHAR*)buffer + offset, _Length - offset, ioBufferElement->GetBuffer(), size);

                offset += size;
            }

            KInvariant(offset == _Length);

            *_IoBuffer = ioBuffer;

            Complete(STATUS_SUCCESS);

            break;

        default:
            KFatal(FALSE);
            break;

    }
}

VOID
KDiskBlockDevice::CachedReadContext::AsyncCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    ContinueRoutine(Async.Status());
}


KDiskBlockDevice::NonContiguousReadContext::~NonContiguousReadContext(
    )
{
    // Nothing.
}

KDiskBlockDevice::NonContiguousReadContext::NonContiguousReadContext(
    ) :
   _BlockDeviceReadOps(GetThisAllocator()),
   _IoBuffers(GetThisAllocator())
{
    NTSTATUS status;

    status = _BlockDeviceReadOps.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = _IoBuffers.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    Initialize();
}

VOID
KDiskBlockDevice::NonContiguousReadContext::Initialize(
    )
{
    _IoBuffer = nullptr;
    _AggregateIoBuffer = nullptr;
    _State = UninitializedForRead;
}

NTSTATUS
KDiskBlockDevice::NonContiguousReadContext::InitializeForRead(
    __inout KDiskBlockDevice& BlockDevice,
    __in ULONGLONG Offset,
    __in ULONG ReadLength,
    __in ULONG BufferLength,
    __in_opt KArray<KAsyncContextBase::SPtr>* BlockDeviceReadOps,
    __out KIoBuffer::SPtr& IoBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN b;
    ULONG readCount;

    _BlockDevice = &BlockDevice;
    _Offset = Offset;
    _ReadLength = ReadLength;
    _BufferLength = BufferLength;
    _IoBuffer = &IoBuffer;
    _State = Initial;

    KFinally([&] {
        if (! NT_SUCCESS(status))
        {
            _AggregateIoBuffer = nullptr;
            _BlockDevice = nullptr;
            _BlockDeviceReadOps.Clear();
            _IoBuffers.Clear();
        }
    });

    status = KIoBuffer::CreateEmpty(_AggregateIoBuffer, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }

    readCount = _ReadLength / _BufferLength;
    if ((readCount * _BufferLength) < _ReadLength)
    {
        readCount++;
    }

    if (BlockDeviceReadOps)
    {
        if (BlockDeviceReadOps->Count() != readCount)
        {
            //
            // Number of Ops must be consistent with ReadLength and BufferLength
            //
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, BlockDeviceReadOps->Count(), readCount);
            return status;
        }

        status = _BlockDeviceReadOps.Reserve(readCount);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            return status;
        }
        b = _BlockDeviceReadOps.SetCount(readCount);
        KInvariant(b);

        for (ULONG i = 0; i < readCount; i++)
        {
            _BlockDeviceReadOps[i] = (*BlockDeviceReadOps)[i];
        }
    } else {
        status = _BlockDeviceReadOps.Reserve(readCount);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            return status;
        }
        b = _BlockDeviceReadOps.SetCount(readCount);
        KInvariant(b);

        KAsyncContextBase::SPtr async;
        for (ULONG i = 0; i < readCount; i++)
        {
            status = _BlockDevice->AllocateReadContext(async, GetThisAllocationTag());
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, 0, 0);
                return status;
            }
            _BlockDeviceReadOps[i] = async;
        }
    }

    status = _IoBuffers.Reserve(readCount);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }
    b = _IoBuffers.SetCount(readCount);
    KInvariant(b);

    return STATUS_SUCCESS;
}

VOID
KDiskBlockDevice::NonContiguousReadContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    ContinueRoutine(STATUS_SUCCESS);
}

VOID
KDiskBlockDevice::NonContiguousReadContext::OnCancel(
    )
{
    for (ULONG i = 0; i < _BlockDeviceReadOps.Count(); i++)
    {
        _BlockDeviceReadOps[i]->Cancel();
    }
}

VOID
KDiskBlockDevice::NonContiguousReadContext::OnReuse(
    )
{
    for (ULONG i = 0; i < _BlockDeviceReadOps.Count(); i++)
    {
        _BlockDeviceReadOps[i]->Reuse();
    }

    _BlockDevice = nullptr;
    _IoBuffer = nullptr;
    _State = UninitializedForRead;
}

VOID
KDiskBlockDevice::NonContiguousReadContext::OnCompleted(
    )
{

    _BlockDeviceReadOps.Clear();
    _IoBuffers.Clear();
    _AggregateIoBuffer = nullptr;
    _BlockDevice = nullptr;
}


VOID
KDiskBlockDevice::NonContiguousReadContext::ContinueRoutine(
    __in NTSTATUS LastStatus
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &NonContiguousReadContext::AsyncCompletion);
    NTSTATUS status = LastStatus;

    switch(_State)
    {
        case Initial:
        {
            ULONG i;
            ULONG readCount;
            ULONG readSize;
            ULONG lastReadSize;

            _FinalStatus = STATUS_SUCCESS;

            readCount = _BlockDeviceReadOps.Count();
            readSize = _BufferLength;
            lastReadSize = _ReadLength - ( (readCount-1) * readSize );

            //
            // Advance state in case there is an error when starting
            // the read asyncs
            //
            _State = Reading;

            _OutstandingReads = 0;
            ULONGLONG offset = _Offset;
            for (i = 0; i < readCount-1; i++)
            {
                status = _BlockDevice->Read(offset, readSize, TRUE, _IoBuffers[i], completion, this, _BlockDeviceReadOps[i]);
                if (! NT_SUCCESS(status))
                {
                    _FinalStatus = status;
                    KTraceFailedAsyncRequest(_FinalStatus, this, _OutstandingReads, 0);

                    if (_OutstandingReads == 0)
                    {
                        Complete(_FinalStatus);
                    }

                    return;

                } else {
                    _OutstandingReads++;
                }

                offset += readSize;
            }

            status = _BlockDevice->Read(offset, lastReadSize, TRUE, _IoBuffers[i], completion, this, _BlockDeviceReadOps[i]);
            if (! NT_SUCCESS(status))
            {
                _FinalStatus = status;
                KTraceFailedAsyncRequest(_FinalStatus, this, _OutstandingReads, 0);

                if (_OutstandingReads == 0)
                {
                    Complete(_FinalStatus);
                    return;
                }
            } else {
                _OutstandingReads++;
            }

            offset += lastReadSize;

            break;
        }

        case Reading:
        {
            if (! NT_SUCCESS(status))
            {
                _FinalStatus = status;
                KTraceFailedAsyncRequest(_FinalStatus, this, _OutstandingReads, 0);
            }

            _OutstandingReads--;
            if (_OutstandingReads > 0)
            {
                break;
            }

            // Fall through
            _State = AggregateReads;
        }

        case AggregateReads:
        {
            if (! NT_SUCCESS(_FinalStatus))
            {
                KTraceFailedAsyncRequest(_FinalStatus, this, _OutstandingReads, 0);
                Complete(_FinalStatus);
                return;
            }

            for (ULONG i = 0; i < _BlockDeviceReadOps.Count(); i++)
            {
                _AggregateIoBuffer->AddIoBuffer(*(_IoBuffers[i]));
            }

            *_IoBuffer = Ktl::Move(_AggregateIoBuffer);

            Complete(STATUS_SUCCESS);

            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }
}

VOID
KDiskBlockDevice::NonContiguousReadContext::AsyncCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    ContinueRoutine(Async.Status());
}


VOID
KDiskBlockDevice::QueryDeviceAttributes(
    __out DeviceAttributes& Attributes
    )

/*++

Routine Description:

    This routine queries the fixed device attributes.

Arguments:

    Attributes - Returns the attributes of the device

Return Value:

    None

--*/

{
    Attributes = _Attributes;
}


VOID
KDiskBlockDevice::QueryDeviceHandle(
    __out HANDLE& Handle
    )

/*++

Routine Description:

    This routine queries the fixed device underlying handle.

Arguments:

    Handle - Returns the HANDLE of the device

Return Value:

    None

--*/

{
    _File->QueryFileHandle(Handle);
}


NTSTATUS
KDiskBlockDevice::AllocateFlushContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates an async context that can be used in KDiskBlockDevice::Flush().

Arguments:

    Async - Returns the allocated async context on success.

    AllocationTag - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    KBlockFile::FlushContext::SPtr flushContext = nullptr;

    if (_File)
    {
        status = _File->AllocateFlush(flushContext, AllocationTag);
    }
    else
    {
        status = _CachedFile->AllocateFlush(flushContext, AllocationTag);
    }

    Async = up_cast<KAsyncContextBase>(flushContext);
    return status;
}

NTSTATUS
KDiskBlockDevice::AllocateTrimContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
)

/*++

Routine Description:

This routine allocates an async context that can be used in KDiskBlockDevice::Trim().

Arguments:

Async - Returns the allocated async context on success.

AllocationTag - Supplies the allocation tag.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS status;
    KBlockFile::TrimContext::SPtr trimContext = nullptr;

    if (_File)
    {
        status = _File->AllocateTrim(trimContext, AllocationTag);
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
    }

    Async = up_cast<KAsyncContextBase>(trimContext);

    return status;
}

NTSTATUS
KDiskBlockDevice::AllocateSetSystemIoPriorityHintContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
)

/*++

Routine Description:

This routine allocates an async context that can be used in KDiskBlockDevice::Trim().

Arguments:

Async - Returns the allocated async context on success.

AllocationTag - Supplies the allocation tag.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS status;
    KBlockFile::SetSystemIoPriorityHintContext::SPtr setSystemIoPriorityHintContext = nullptr;

    if (_File)
    {
        status = _File->AllocateSetSystemIoPriorityHint(setSystemIoPriorityHintContext, AllocationTag);
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
    }

    Async = up_cast<KAsyncContextBase>(setSystemIoPriorityHintContext);

    return status;
}

NTSTATUS
KDiskBlockDevice::AllocateQueryAllocationsContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
)

/*++

Routine Description:

This routine allocates an async context that can be used in KDiskBlockDevice::QueryAllocations().

Arguments:

Async - Returns the allocated async context on success.

AllocationTag - Supplies the allocation tag.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS status;
    KBlockFile::QueryAllocationsContext::SPtr queryAlocationsContext = nullptr;

    if (_File)
    {
        status = _File->AllocateQueryAllocations(queryAlocationsContext, AllocationTag);
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
    }

    Async = up_cast<KAsyncContextBase>(queryAlocationsContext);

    return status;
}

NTSTATUS
KDiskBlockDevice::Flush(
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
    )

/*++

Routine Description:

    This routine flushes the entire block device. It is analagous to the disk FLUSH command.

Arguments:

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    Async - Supplies an optional async context that is allocated through KDiskBlockDevice::AllocateFlushContext(),
        or reused from a previous context.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    NTSTATUS status;
    KBlockFile::FlushContext* flushContext = (KBlockFile::FlushContext*)Async.RawPtr();

    if (_File)
    {
        status = _File->Flush(Completion, ParentAsync, flushContext);
    }
    else
    {
        status = _CachedFile->Flush(Completion, ParentAsync, flushContext);
    }

    return status;
}

NTSTATUS
KDiskBlockDevice::Trim(
    __in ULONGLONG TrimFrom,
    __in ULONGLONG TrimToPlusOne,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
)

/*++

Routine Description:

    This routine trims allocations from a sparse file

Arguments:

    TrimFrom is the file offset to trim from

    TrimToPlusOne is the file offset that starts the block after the trim

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    Async - Supplies an optional async context that is allocated through KDiskBlockDevice::AllocateFlushContext(),
    or reused from a previous context.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    NTSTATUS status;
    KBlockFile::TrimContext* trimContext = (KBlockFile::TrimContext*)Async.RawPtr();

    if (_File)
    {
        status = _File->Trim(TrimFrom, TrimToPlusOne, Completion, ParentAsync, trimContext);
    }
    else
    {
        return(STATUS_NOT_SUPPORTED);
    }

    return status;
}

NTSTATUS
KDiskBlockDevice::SetSystemIoPriorityHint(
    __in KBlockFile::SystemIoPriorityHint Hint,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
)

/*++

Routine Description:

    This routine trims allocations from a sparse file

Arguments:

    Hint is the Io priority hint to set for the file handle

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    Async - Supplies an optional async context that is allocated through KDiskBlockDevice::AllocateFlushContext(),
    or reused from a previous context.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    NTSTATUS status;
    KBlockFile::SetSystemIoPriorityHintContext* setSystemIoPriorityHintContext = (KBlockFile::SetSystemIoPriorityHintContext*)Async.RawPtr();

    if (_File)
    {
        status = _File->SetSystemIoPriorityHint(Hint, Completion, ParentAsync, setSystemIoPriorityHintContext);
    }
    else
    {
        return(STATUS_NOT_SUPPORTED);
    }

    return status;
}


NTSTATUS
KDiskBlockDevice::QueryAllocations(
    __in ULONGLONG QueryFrom,
    __in ULONGLONG Length,
    __inout KArray<KBlockFile::AllocatedRange>& Results,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
)

/*++

Routine Description:

This routine trims allocations from a sparse file

Arguments:

    QueryFrom is the file offset to begin returning allocations

    Length is the size of the file region for which to return allocations

    Results returns with an array of allocation information

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    Async - Supplies an optional async context that is allocated through KDiskBlockDevice::AllocateFlushContext(),
    or reused from a previous context.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    NTSTATUS status;
    KBlockFile::QueryAllocationsContext* queryAllocationsContext = (KBlockFile::QueryAllocationsContext*)(Async.RawPtr());

    if (_File)
    {
        status = _File->QueryAllocations(QueryFrom, Length, Results, Completion, ParentAsync, queryAllocationsContext);
    }
    else
    {
        return(STATUS_NOT_SUPPORTED);
    }

    return status;
}

NTSTATUS
KDiskBlockDevice::SetSparseFile(
    __in BOOLEAN IsSparseFile
    )
{
    return(_File->SetSparseFile(IsSparseFile));
}

VOID
KDiskBlockDevice::Close()
{
    _File->Close();
}

NTSTATUS
KBlockDevice::CreateDiskBlockDevice(
    __in const KGuid& DeviceId,
    __in ULONGLONG DeviceSize,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN CreateNew,
    __in const KWString& FullyQualifiedDiskFileName,
    __out KBlockDevice::SPtr& DeviceObject,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in_opt KReadCache* ReadCache,
    __in BOOLEAN NoCachingOnWrite,
    __in BOOLEAN UseSparseFile,
    __in ULONG ReadAheadSize,
    __in KBlockFile::CreateOptions Options
    )

/*++

Routine Description:

    This routine creates a KDiskBlockDevice object.

Arguments:

    DeviceId - Supplies the logic ID of this block device.

    DeviceSize - Supplies the size of this device in bytes. It must be multiples of KBlockDevice::_BlockSize.

    IsWriteThrough - Indicates if writes to this device are all write-through.
        A 'TRUE' flag may be neccessary for correctness in certain scenarios, such as logging,
        but may have degraded write performance.

    CreateNew - Indicates if we want to create a new device, or open an existing one.

    FullyQualifiedDiskFileName - Supplies the fully qualified name of the file that provides storage for this device.
        For example, L"\\Device\\HarddiskVolume1\\myfile.dat" and L"\\??\\C:\\myfile.dat" are both fully qualified file names.
        See NtCreateFile() for more information of fully qualified file names.

    DeviceObject - Returns the created object on success.

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    AllocationTag - Supplies the default tag for internal allocations.

    Allocator - Supplies the default allocator.

    ReadCache - Supplies, optionally, a read cache to use with this block device.

    NoCachingOnWrite - Supplies whether or not to cache writes, if caching is enabled.

    UseSParseFile - if TRUE then underlying file is marked sparse

    ReadAheadSize - if 0 then no read ahead caching is performed. If > 0 then that number of bytes is used for the
                    read ahead bufffer. Reads will first try to be accomodated from the read ahead buffer but if not
                    within the read ahead buffer then a full read of the ReadAheadSize will be performed and then
                    the read satisfied from the read ahead buffer. The caller is responsible to ensure consistency of the
                    read ahead buffer if any writes are performed.

    CreateOptions - flags that affect how KBlockFile opens the underlying file

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    DeviceObject = nullptr;

    KBlockDevice::SPtr deviceObject = _new(AllocationTag, Allocator) KDiskBlockDevice(
        DeviceId,
        DeviceSize,
        IsWriteThrough,
        FullyQualifiedDiskFileName,
        ReadCache,
        NoCachingOnWrite,
        UseSparseFile,
        ReadAheadSize,
        Options,
        AllocationTag
        );
    if (!deviceObject)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = deviceObject->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    DeviceObject = Ktl::Move(deviceObject);

    KDiskBlockDevice* deviceObjectPtr = DeviceObject.DownCast<KDiskBlockDevice>();
    status = deviceObjectPtr->Mount(CreateNew, ReadCache, Completion, ParentAsync);
    return status;
}

NTSTATUS KDiskBlockDevice::CreateKReadAheadContextImp(
    __in ULONG ReadAheadSize,
    __out KSharedPtr<KDiskBlockDevice::KReadAheadContextImp>& Context
    )
{
    NTSTATUS status;

    Context = nullptr;

    KDiskBlockDevice::KReadAheadContextImp::SPtr newReadAheadContext;
    newReadAheadContext = _new(GetThisAllocationTag(), GetThisAllocator()) KDiskBlockDevice::KReadAheadContextImp(*this, ReadAheadSize);

    if (!newReadAheadContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }

    status = newReadAheadContext->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(newReadAheadContext);

    return(STATUS_SUCCESS);
}

//
// KReadAheadContextImp
//
KDiskBlockDevice::KReadAheadContextImp::KReadAheadContextImp(
    __in KDiskBlockDevice& BlockDevice,
    __in ULONG ReadAheadSize
    ) : _BlockDevice(&BlockDevice),
        _ReadAheadSize(ReadAheadSize)
{
    Invalidate((ULONGLONG)-1);
}

KDiskBlockDevice::KReadAheadContextImp::~KReadAheadContextImp()
{
    _ReadAheadBuffer = nullptr;
    _BlockDevice = nullptr;
}

VOID
KDiskBlockDevice::KReadAheadContextImp::Invalidate(
    ULONGLONG Offset
)
{
    //
    // Offset may be useful in the future when multiple read ahead buffers are cached
    //
    UNREFERENCED_PARAMETER(Offset);

    SetReadAheadBuffer(nullptr, ULONGLONG_MAX, 0);
}

VOID
KDiskBlockDevice::KReadAheadContextImp::SetReadAheadBuffer(
    __in_opt KIoBuffer* ReadAheadBuffer,
    __in ULONGLONG BaseOffset,
    __in ULONGLONG TopReadAheadOffset
    )
{
    K_LOCK_BLOCK(_Lock)
    {
        _ReadAheadBuffer = ReadAheadBuffer;
        _BaseOffset = BaseOffset;
        _TopReadAheadOffset = TopReadAheadOffset;
    }
}

VOID
KDiskBlockDevice::KReadAheadContextImp::UpdateReadAheadBuffer(
    __in KIoBuffer& ReadAheadBuffer,
    __in ULONGLONG BaseOffset,
    __in ULONGLONG TopReadAheadOffset
    )
{
    K_LOCK_BLOCK(_Lock)
    {
        if (_BaseOffset == BaseOffset)
        {
            _ReadAheadBuffer = &ReadAheadBuffer;
            _BaseOffset = BaseOffset;
            _TopReadAheadOffset = TopReadAheadOffset;
        }
    }
}

NTSTATUS
KDiskBlockDevice::KReadAheadContextImp::TryGetFromReadAheadBuffer(
    __in ULONGLONG Offset,
    __in ULONG Length,
    __out KIoBuffer::SPtr& IoBuffer
)
{
    NTSTATUS status = STATUS_NOT_FOUND;
    ULONGLONG topReadOffset = Offset + Length;
    KIoBuffer::SPtr ioBuffer;

    K_LOCK_BLOCK(_Lock)
    {
        if ((_ReadAheadBuffer) &&
            (Offset >= _BaseOffset) &&
            (topReadOffset <= _TopReadAheadOffset))
        {
            status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());
            if (NT_SUCCESS(status))
            {
                ULONG startingOffset = (ULONG)(Offset - _BaseOffset);
                status = ioBuffer->AddIoBufferReference(*_ReadAheadBuffer, startingOffset, Length, GetThisAllocationTag());
                if (NT_SUCCESS(status))
                {
                    IoBuffer = ioBuffer;
                }
                else {
                    KTraceFailedAsyncRequest(status, (KAsyncContextBase*)this, Offset, Length);
                }
            }
            else {
                KTraceFailedAsyncRequest(status, (KAsyncContextBase*)this, Offset, Length);
            }

            KInvariant(status != STATUS_NOT_FOUND);
        }
    }

    return(status);
}

BOOLEAN
KDiskBlockDevice::KReadAheadContextImp::InvalidateReadAheadIfNeeded(
    __in ULONGLONG Offset,
    __in ULONG Length
)
{
    ULONGLONG topReadOffset = Offset + Length;
    BOOLEAN isInBuffer = FALSE;

    K_LOCK_BLOCK(_Lock)
    {
        //
        // Only invalidate when write collides with current read ahead
        //
        isInBuffer = ((Offset >= _BaseOffset) && (Offset < _TopReadAheadOffset) ||
                      ((topReadOffset > _BaseOffset) && ((topReadOffset <= _TopReadAheadOffset))));
        if (isInBuffer)
        {
            _ReadAheadBuffer = nullptr;
            _BaseOffset = ULONGLONG_MAX;
            _TopReadAheadOffset = 0;
        }
    }

    return(isInBuffer);
}

//
// KReadAheadContextOp
//
NTSTATUS
KDiskBlockDevice::AllocateReadContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates an async context that can be used in KDiskBlockDevice::Read().

Arguments:

    Async - Returns the allocated async context on success.

    AllocationTag - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;

    Async = nullptr;

    if (_File)
    {
        KDiskBlockDevice::KReadAheadContextOp::SPtr readAheadContextOp;

        status = CreateKReadAheadContextOp(readAheadContextOp);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        status = _File->AllocateTransfer(readAheadContextOp->_TransferContext, AllocationTag);

        if (NT_SUCCESS(status))
        {
            Async = up_cast<KAsyncContextBase>(readAheadContextOp);
        }
    }
    else
    {
        KDiskBlockDevice::CachedReadContext::SPtr readContext;

        readContext = _new(AllocationTag, GetThisAllocator()) KDiskBlockDevice::CachedReadContext(*this);

        if (readContext) {
            status = readContext->Status();
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(status))
        {
            Async = up_cast<KAsyncContextBase>(readContext);
        }
    }

    return status;
}


NTSTATUS
KDiskBlockDevice::AllocateNonContiguousReadContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates an async context that can be used in
    KDiskBlockDevice::ReadNonContiguous().

Arguments:

    Async - Returns the allocated async context on success.

    AllocationTag - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;

    Async = nullptr;

    KDiskBlockDevice::NonContiguousReadContext::SPtr readContext;

    readContext = _new(AllocationTag, GetThisAllocator()) KDiskBlockDevice::NonContiguousReadContext();

    if (readContext) {
        status = readContext->Status();
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status))
    {
        Async = up_cast<KAsyncContextBase>(readContext);
    }

    return status;
}

NTSTATUS
KDiskBlockDevice::CreateKReadAheadContextOp(
    __out KSharedPtr<KDiskBlockDevice::KReadAheadContextOp>& Context
)
{
    NTSTATUS status;

    Context = nullptr;

    KSharedPtr<KDiskBlockDevice::KReadAheadContextOp> context;
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KDiskBlockDevice::KReadAheadContextOp(*this);

    if (!context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);

    return(STATUS_SUCCESS);
}

NTSTATUS
KDiskBlockDevice::Read(
    __in ULONGLONG Offset,
    __in ULONG Length,
    __in BOOLEAN ContiguousIoBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
    )

/*++

Routine Description:

    This routine reads a region of the block device to an IO_BUFFER. The IO_BUFFER is created by KDiskBlockDevice.
    Once its ref count drops to zero, it will release all its memory.

    The caller can use the 'ContiguousIoBuffer' flag to control the cardinality
    of the output IO buffer. It is normally simpler to use one piece of
    virtually contiguous buffer. However, be aware that KBlockDevice abstraction may be
    implemented with internal caching; some part of the data may be read from
    the cache and others are read from external storage. If 'ContiguousIoBuffer' is TRUE,
    it may force memory allocation and copy to merge multiple buffers into one
    virtually contiguous buffer.

Arguments:

    Offset - Supplies the offset to read. It must be multiples of KBlockDevice::_BlockSize.

    Length - Supplies the length to read. It must be multiples of KBlockDevice::_BlockSize.

    ContiguousIoBuffer - If this flag is TRUE, the returned IoBuffer only
        contains a single IO buffer element. Otherwise, it may contain
        multiple IO buffer elements.

    IoBuffer - Returns the IO buffer that is read from the device.

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    Async - Supplies an optional async context that is allocated through KDiskBlockDevice::AllocateTransferContext(),
        or reused from a previous context.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    NTSTATUS status;
    PVOID buffer;

    KAssert(Offset % KBlockDevice::_BlockSize == 0);
    KAssert(Length % KBlockDevice::_BlockSize == 0);

    if (Offset + Length > _Attributes.DeviceSize)
    {
        status = STATUS_INVALID_PARAMETER;
        return status;
    }

    #ifdef TRACE_BLOCK_DEVICE_IO
    KDbgPrintf("Device READ: Offset = %x, Length = %x\n", Offset, Length);
    #endif

    if (_File)
    {
        KDiskBlockDevice::KReadAheadContextOp::SPtr readAheadContextOp;
        KBlockFile::TransferContext* transferContext;

        readAheadContextOp = down_cast<KDiskBlockDevice::KReadAheadContextOp, KAsyncContextBase>(Async);

        if (readAheadContextOp)
        {
            transferContext = readAheadContextOp->_TransferContext.RawPtr();
        }
        else
        {
            transferContext = nullptr;
        }

        //
        // Ignore the 'ContiguousIoBuffer' flag. For disk file based 'block device',
        // there is no caching at the block device layer for now. We always allocate a
        // single piece of buffer.
        //

        UNREFERENCED_PARAMETER(ContiguousIoBuffer);

        if ((IsReadAheadEnabled()) && (IsReadAheadEligible(Offset, Length)))
        {
            //
            // If we are doing read ahead then fork off to that
            // functionality
            //
            if (!readAheadContextOp)
            {
                status = CreateKReadAheadContextOp(readAheadContextOp);
                if (!NT_SUCCESS(status))
                {
                    return(status);
                }
            }

            readAheadContextOp->Reuse();

            readAheadContextOp->StartRead(
                Offset,                     // Offset
                Length,                     // Length
                IoBuffer,                   // Result data
                Completion,                 // Completion
                ParentAsync                // ParentAsync
                );
            status = STATUS_PENDING;
        }
        else
        {
            status = KIoBuffer::CreateSimple(Length, IoBuffer, buffer, GetThisAllocator());
            if (!NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, nullptr, 0, Length);
                IoBuffer = nullptr;
                return status;
            }

            status = _File->Transfer(
                KBlockFile::eForeground,    // Priority
                KBlockFile::eRead,          // Type
                Offset,                     // Offset
                buffer,                     // Buffer
                Length,                     // Length
                Completion,                 // Completion
                ParentAsync,                // ParentAsync
                transferContext             // Async
                );
        }
    }
    else
    {
        KDiskBlockDevice::CachedReadContext::SPtr readContext;

        if (Async) {

            readContext = Async.DownCast<KDiskBlockDevice::CachedReadContext>();

            if (readContext->Status() != K_STATUS_NOT_STARTED) {
                readContext->Reuse();
            }

        } else {

            readContext = _new(_AllocationTag, GetThisAllocator()) KDiskBlockDevice::CachedReadContext(*this);

            if (readContext)
            {
                status = readContext->Status();
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }

        readContext->InitializeForRead(*this, Offset, Length, ContiguousIoBuffer, IoBuffer);
        readContext->Start(ParentAsync, Completion);

        status = STATUS_PENDING;
    }

    return status;
}


NTSTATUS
KDiskBlockDevice::ReadNonContiguous(
    __in ULONGLONG Offset,
    __in ULONG ReadLength,
    __in ULONG BufferLength,
    __out KIoBuffer::SPtr& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async,
    __in_opt KArray<KAsyncContextBase::SPtr>* const BlockDeviceReadOps
    )
/*++

Routine Description:

    This routine reads a region of the block device to an non contiguous KIoBuffer.
    The KIoBuffer is created by KDiskBlockDevice.
    Once its ref count drops to zero, it will release all its memory.

    The purpose of this method is to read a large region of a disk
    block device into a non contiguous KIoBuffer as a way to mitigate
    issues when large contiguous memory blocks are not available due to
    pool fragmentation.

Arguments:

    Offset - Supplies the offset to read. It must be multiples of KBlockDevice::_BlockSize.

    ReadLength - Supplies the length to read. It must be multiples of KBlockDevice::_BlockSize.

    BufferLength - Supplies the length of each KIoBufferElement

    IoBuffer - Returns the IO buffer that is read from the device.

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    Async - Supplies an optional async context that is allocated through KDiskBlockDevice::AllocateNonContiguousReadContext(),
        or reused from a previous context.

    BlockDeviceReadOps - Supplies an array of read contexts allocated through KBlockDevice::AllocateTransferContext
        If present then the number of Ops in the array must be consistent with the ReadLength and BufferLength

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/
{
    NTSTATUS status;

    KAssert(Offset % KBlockDevice::_BlockSize == 0);
    KAssert(ReadLength % KBlockDevice::_BlockSize == 0);
    KAssert(BufferLength % KBlockDevice::_BlockSize == 0);

    if (Offset + ReadLength > _Attributes.DeviceSize)
    {
        status = STATUS_INVALID_PARAMETER;
        return status;
    }

    #ifdef TRACE_BLOCK_DEVICE_IO
    KDbgPrintf("Device READNonContiguous: Offset = %x, Length = %x\n", Offset, Length);
    #endif

    //
    // Ensure allocation of NonContiguousReadContext
    //
    KDiskBlockDevice::NonContiguousReadContext::SPtr readContext;
    readContext = down_cast<KDiskBlockDevice::NonContiguousReadContext, KAsyncContextBase>(Async);
    if (! readContext)
    {
        readContext = _new(GetThisAllocationTag(), GetThisAllocator()) KDiskBlockDevice::NonContiguousReadContext();

        if (readContext) {
            status = readContext->Status();
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (! NT_SUCCESS(status))
        {
            return status;
        }
    }

    //
    // Initialize and start NonContiguousReadContext
    readContext->Reuse();
    status = readContext->InitializeForRead(*this, Offset, ReadLength, BufferLength, BlockDeviceReadOps, IoBuffer);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    readContext->Start(ParentAsync, Completion);

    status = STATUS_PENDING;

    return status;
}


KDiskBlockDevice::KReadAheadContextOp::KReadAheadContextOp(
    __in KDiskBlockDevice& BlockDevice
    ) : _BlockDevice(&BlockDevice)
{
}

KDiskBlockDevice::KReadAheadContextOp::~KReadAheadContextOp()
{
}

VOID
KDiskBlockDevice::KReadAheadContextOp::OnReuse()
{
    _Length = 0;
    _Offset = 0;
    _IoBuffer = nullptr;
}

VOID
KDiskBlockDevice::KReadAheadContextOp::OnCancel()
{
}

VOID
KDiskBlockDevice::KReadAheadContextOp::OnCompleted()
{
}

VOID KDiskBlockDevice::KReadAheadContextOp::ReadCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);

    NTSTATUS status = Async.Status();
    KDiskBlockDevice::KReadAheadContextImp::SPtr readAheadContextImp = _BlockDevice->_ReadAheadContext;

    if (NT_SUCCESS(status))
    {
        //
        // Update this read ahead buffer into the ReadAheadContext if
        // it has not been invalidated by an intersecting write or a
        // parallel read
        //
        readAheadContextImp->UpdateReadAheadBuffer(*_ReadAheadBufferInProgress, _Offset, (_Offset + _ReadAheadBufferInProgress->QuerySize()));

        //
        // Return data requested by caller
        //
        KIoBuffer::SPtr ioBuffer;
        status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());
        if (NT_SUCCESS(status))
        {
            status = ioBuffer->AddIoBufferReference(*_ReadAheadBufferInProgress, 0, _Length, GetThisAllocationTag());
            if (NT_SUCCESS(status))
            {
                *_IoBuffer = ioBuffer;
                status = STATUS_SUCCESS;
            }
            else
            {
                KTraceFailedAsyncRequest(status, this, _Offset, _Length);
            }
        }
        else
        {
            KTraceFailedAsyncRequest(status, this, _Offset, _Length);
        }
        _ReadAheadBufferInProgress = nullptr;
    }
    else
    {
        if (_ReadAheadBufferInProgress->QuerySize() > _Length)
        {
            KTraceFailedAsyncRequest(status, this, _Offset, readAheadContextImp->_ReadAheadSize);

            //
            // If the read with readahead failed then we retry the read for exactly what the caller wanted. It is
            // possible that the file does not actually have the data that the read ahead is looking for
            //
            PVOID buffer;

            _ReadAheadBufferInProgress = nullptr;
            status = KIoBuffer::CreateSimple(_Length, _ReadAheadBufferInProgress, buffer, GetThisAllocator());
            if (NT_SUCCESS(status))
            {
                KAsyncContextBase::CompletionCallback completion(this, &KDiskBlockDevice::KReadAheadContextOp::ReadCompleted);

                status = _BlockDevice->_File->Transfer(
                    KBlockFile::eForeground,    // Priority
                    KBlockFile::eRead,          // Type
                    _Offset,                     // Offset
                    buffer,                     // Buffer
                    _Length,                     // Length
                    completion,                 // Completion
                    this,                // ParentAsync
                    _TransferContext.RawPtr()             // Async
                    );

                if (!NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, _Offset, _Length);
                    Complete(status);
                }
                return;
            }
            else
            {
                KTraceFailedAsyncRequest(status, this, _Offset,_Length);
            }
        }
        else
        {
            //
            // Even though we read just what the caller asked for, it still failed. Return
            // the bad news back
            //
            KTraceFailedAsyncRequest(status, this, _Offset, _Length);
        }
    }

    Complete(status);
}

VOID
KDiskBlockDevice::KReadAheadContextOp::OnStart()
{
    NTSTATUS status;
    KDiskBlockDevice::KReadAheadContextImp::SPtr readAheadContextImp = _BlockDevice->_ReadAheadContext;


    KInvariant((_Offset % KBlockFile::BlockSize) == 0);
    KInvariant((_Length % KBlockFile::BlockSize) == 0);
    KInvariant(_IoBuffer != nullptr);

    *_IoBuffer = nullptr;

    status = readAheadContextImp->TryGetFromReadAheadBuffer(_Offset, _Length, *_IoBuffer);

    if (status == STATUS_NOT_FOUND)
    {
        //
        // This was not in the read ahead buffer so read the buffer needed
        //
        PVOID buffer;

        status = KIoBuffer::CreateSimple(readAheadContextImp->_ReadAheadSize, _ReadAheadBufferInProgress, buffer, GetThisAllocator());
        if (NT_SUCCESS(status))
        {
            //
            // Set the readahead to the buffer we are reading now. Note
            // that the IoBuffer is null since we haven't finished
            // reading it yet. If no intervening write for this region
            // is encountered while the read is in flight then the
            // completion routine will save the readahead. If an
            // intervening write does occur then the readahead will not
            // be saved as we will not know if its data is stale or not.
            //
            // Note that an intervening read could also flow through
            // this codepath and potentially also set the
            // KReadAheadContext which would also invalidate this read.
            // A potential optimization would be to detect this case
            // and the second read not overwrite the KReadAheadContext
            // but instead just do the write as the user requested. Or
            // the KReadAheadContext could keep multiple readahead
            // buffers.
            //
            readAheadContextImp->SetReadAheadBuffer(nullptr, _Offset, (_Offset + _ReadAheadBufferInProgress->QuerySize()));

            KAsyncContextBase::CompletionCallback completion(this, &KDiskBlockDevice::KReadAheadContextOp::ReadCompleted);

            status = _BlockDevice->_File->Transfer(
                KBlockFile::eForeground,    // Priority
                KBlockFile::eRead,          // Type
                _Offset,                     // Offset
                buffer,                     // Buffer
                readAheadContextImp->_ReadAheadSize,                     // Length
                completion,                 // Completion
                this,
                _TransferContext.RawPtr()             // Async
                );

            if (!NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, _Offset, readAheadContextImp->_ReadAheadSize);
                Complete(status);
            }

            return;
        }
        else
        {
            KTraceFailedAsyncRequest(status, this, _Offset, readAheadContextImp->_ReadAheadSize);
        }
    }

    Complete(status);
}

VOID
KDiskBlockDevice::KReadAheadContextOp::StartRead(
    __in ULONGLONG Offset,
    __in ULONG Length,
    __out KIoBuffer::SPtr& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    _Offset = Offset;
    _Length = Length;
    _IoBuffer = &IoBuffer;

    Start(ParentAsync, Completion);
}


//
// WriteAheadContextOp
//
NTSTATUS
KDiskBlockDevice::AllocateWriteContext(
    __out KAsyncContextBase::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates an async context that can be used in KDiskBlockDevice::Write().

Arguments:

    Async - Returns the allocated async context on success.

    AllocationTag - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;

    Async = nullptr;

    if (_File)
    {
        KBlockFile::TransferContext::SPtr transferContext;

        status = _File->AllocateTransfer(transferContext, AllocationTag);

        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        if (IsReadAheadEnabled())
        {
            KDiskBlockDevice::KWriteAheadContextOp::SPtr writeAheadContextOp;

            status = CreateKWriteAheadContextOp(writeAheadContextOp);
            if (!NT_SUCCESS(status))
            {
                return(status);
            }

            writeAheadContextOp->_TransferContext = Ktl::Move(transferContext);

            Async = up_cast<KAsyncContextBase>(writeAheadContextOp);
        } else {
            Async = up_cast<KAsyncContextBase>(transferContext);
        }
    }
    else
    {
        KCachedBlockFile::TransferContext::SPtr transferContext;

        status = _CachedFile->AllocateTransfer(transferContext, AllocationTag);

        if (NT_SUCCESS(status))
        {
            Async = up_cast<KAsyncContextBase>(transferContext);
        }
    }

    return status;
}

NTSTATUS
KDiskBlockDevice::CreateKWriteAheadContextOp(
    __out KSharedPtr<KDiskBlockDevice::KWriteAheadContextOp>& Context
)
{
    NTSTATUS status;

    Context = nullptr;

    KSharedPtr<KDiskBlockDevice::KWriteAheadContextOp> context;
    context = _new(GetThisAllocationTag(), GetThisAllocator()) KDiskBlockDevice::KWriteAheadContextOp(*this);

    if (!context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);

    return(STATUS_SUCCESS);
}

NTSTATUS
KDiskBlockDevice::Write(
__in ULONGLONG Offset,
__in KIoBuffer& IoBuffer,
__in KAsyncContextBase::CompletionCallback Completion,
__in_opt KAsyncContextBase* const ParentAsync,
__in_opt KAsyncContextBase::SPtr Async
)
{
    NTSTATUS status;

    status = Write(
        KBlockFile::eForeground,    // Priority
        KBlockFile::SystemIoPriorityHint::eNotDefined,
        Offset,
        IoBuffer,
        Completion,
        ParentAsync,
        Async
        );

    return(status);
}

NTSTATUS
KDiskBlockDevice::Write(
    __in KBlockFile::IoPriority IoPriority,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
    )
{
    NTSTATUS status;

    status = Write(
        IoPriority,
        KBlockFile::SystemIoPriorityHint::eNotDefined,
        Offset,
        IoBuffer,
        Completion,
        ParentAsync,
        Async
        );

    return(status);
}

NTSTATUS
KDiskBlockDevice::Write(
    __in KBlockFile::IoPriority IoPriority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncContextBase::SPtr Async
    )

/*++

Routine Description:

    This routine writes an entire IO_BUFFER to a region of the block device.

Arguments:

    IoPriority - Specifies which queue the write should be placed -
                 foreground or background queue.

    IoPriorityHint - Specifies which Io priority hint to specify when
                     the write IRP is passed to the storage driver.

    Offset - Supplies the offset to write. It must be multiples of KBlockDevice::_BlockSize.

    IoBuffer - Supplies the IO buffer to write.

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    Async - Supplies an optional async context that is allocated through KDiskBlockDevice::AllocateTransferContext(),
        or reused from a previous context.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    NTSTATUS status;

    #if DBG

    ULONG length = IoBuffer.QuerySize();

    KAssert(Offset % KBlockDevice::_BlockSize == 0);
    KAssert(length % KBlockDevice::_BlockSize == 0);
    KAssert(Offset + length <= _Attributes.DeviceSize);

    #ifdef TRACE_BLOCK_DEVICE_IO
    KDbgPrintf("Device WRITE: Offset = %x, Length = %x\n", Offset, length);
    #endif

    #endif

    if (_File)
    {
        if (IsReadAheadEnabled())
        {
            KDiskBlockDevice::KWriteAheadContextOp::SPtr writeAheadContextOp;

            writeAheadContextOp = down_cast<KDiskBlockDevice::KWriteAheadContextOp, KAsyncContextBase>(Async);

            if (! writeAheadContextOp)
            {
                status = CreateKWriteAheadContextOp(writeAheadContextOp);
                if (! NT_SUCCESS(status))
                {
                    return(status);
                }
            }

            //
            // Read ahead is enabled so start the async that will perform the write.
            //
            writeAheadContextOp->Reuse();

            writeAheadContextOp->StartWrite(
                IoPriority,
                IoPriorityHint,             // System io priority hint
                Offset,
                IoBuffer,
                Completion,                 // Completion
                ParentAsync                // ParentAsync
                );
            status = STATUS_PENDING;
        }
        else
        {
            //
            // No read ahead so pass onward
            //
            KBlockFile::TransferContext* transferContext = down_cast<KBlockFile::TransferContext, KAsyncContextBase>(Async);

            status = _File->Transfer(
                IoPriority,                 // Priority
                IoPriorityHint,             // System io priority hint
                KBlockFile::eWrite,         // Type
                Offset,                     // Offset
                IoBuffer,                   // IoBuffer
                Completion,                 // Completion
                ParentAsync,                // ParentAsync
                transferContext             // Async
                );
        }
    }
    else
    {
        KCachedBlockFile::TransferContext* transferContext = Async.DownCast<KCachedBlockFile::TransferContext>();

        status = _CachedFile->Write(
            KBlockFile::eForeground,    // Priority
            Offset,                     // Offset
            IoBuffer,                   // IoBuffer
            Completion,                 // Completion
            ParentAsync,                // ParentAsync
            transferContext             // Async
            );
    }

    return status;
}

KDiskBlockDevice::KWriteAheadContextOp::KWriteAheadContextOp(
    __in KDiskBlockDevice& BlockDevice
    ) : _BlockDevice(&BlockDevice)
{
}

KDiskBlockDevice::KWriteAheadContextOp::~KWriteAheadContextOp()
{
}

VOID
KDiskBlockDevice::KWriteAheadContextOp::OnReuse()
{
    _Offset = 0;
    _IoBuffer = nullptr;
}

VOID
KDiskBlockDevice::KWriteAheadContextOp::OnCancel()
{
}

VOID
KDiskBlockDevice::KWriteAheadContextOp::OnCompleted()
{
}

VOID KDiskBlockDevice::KWriteAheadContextOp::WriteCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);

    NTSTATUS status = Async.Status();

    if (NT_SUCCESS(status))
    {
        //
        // See if we need to invalidate the current readahead context
        //
        KDiskBlockDevice::KReadAheadContextImp::SPtr readAheadContext = _BlockDevice->_ReadAheadContext;
        BOOLEAN invalidateReadAhead;

        invalidateReadAhead = readAheadContext->InvalidateReadAheadIfNeeded(_Offset, _Length);
    }
    else
    {
        KTraceFailedAsyncRequest(status, this, _Offset, _Length);
    }

    Complete(status);
}

VOID
KDiskBlockDevice::KWriteAheadContextOp::OnStart()
{
    NTSTATUS status;

    //
    // Let the write happen as any invalidation should occur on the
    // completion of the write.
    //
    KAsyncContextBase::CompletionCallback completion(this, &KDiskBlockDevice::KWriteAheadContextOp::WriteCompleted);

    status = _BlockDevice->_File->Transfer(
        _IoPriority,                 // Priority
        _IoPriorityHint,             // System Io priority hint
        KBlockFile::eWrite,         // Type
        _Offset,                     // Offset
        *_IoBuffer,                   // IoBuffer
        completion,                 // Completion
        this,                      // ParentAsync
        _TransferContext.RawPtr()             // Async
        );
}

VOID
KDiskBlockDevice::KWriteAheadContextOp::StartWrite(
    __in KBlockFile::IoPriority IoPriority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    _IoPriority = IoPriority;
    _Offset = Offset;
    _IoBuffer = &IoBuffer;
    _IoPriorityHint = IoPriorityHint;
    _Length = _IoBuffer ? _IoBuffer->QuerySize() : 0;

    Start(ParentAsync, Completion);
}


//
// RamBlockDevice
//
NTSTATUS
KBlockDevice::CreateRamBlockDevice(
    __in const KGuid& DeviceId,
    __in ULONGLONG DeviceSize,
    __out KBlockDevice::SPtr& DeviceObject,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a RAM-based KBlockDevice object.

Arguments:

    DeviceId - Supplies the logic ID of this block device.

    DeviceSize - Supplies the size of this device in bytes. It must be multiples of KBlockDevice::_BlockSize.

    DeviceObject - Returns the created object on success.

    Completion - Supplies the async completion callback delegate.

    ParentAsync - Supplies an optional parent async context.

    AllocationTag - Supplies the default tag for internal allocations.

    Allocator - Supplies the default allocator.

Return Value:

    Appropriate error code or STATUS_PENDING.

--*/

{
    UNREFERENCED_PARAMETER(DeviceId);
    UNREFERENCED_PARAMETER(DeviceSize);
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Completion);
    UNREFERENCED_PARAMETER(ParentAsync);
    UNREFERENCED_PARAMETER(Allocator);
    UNREFERENCED_PARAMETER(AllocationTag);

    return STATUS_NOT_IMPLEMENTED;
}
