/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    KBlockDevice.h

Abstract:

    This file defines a block device abstraction. 
    
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

#pragma once

const ULONG KTL_TAG_BLOCK_DEVICE = 'klBK';      // 'KBlk' Default TAG for KBlockDevice allocations.

class KBlockDevice : public KObject<KBlockDevice>, public KShared<KBlockDevice>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KBlockDevice);
public:
    static const ULONG _BlockSize = 0x1000;     // 4 KB

    static
    NTSTATUS
    CreateDiskBlockDevice(
        __in const KGuid& DeviceId,
        __in ULONGLONG DeviceSize,
        __in BOOLEAN IsWriteThrough,
        __in BOOLEAN CreateNew,
        __in const KWString& FullyQualifiedDiskFileName,
        __out KBlockDevice::SPtr& DeviceObject,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE,
        __in_opt KReadCache* ReadCache = NULL,
        __in BOOLEAN NoCachingOnWrite = FALSE,
        __in BOOLEAN UseSparseFile = FALSE,
        __in ULONG ReadAheadSize = 0,
        __in KBlockFile::CreateOptions = KBlockFile::CreateOptions::eNoIntermediateBuffering
        );


    static
    NTSTATUS
    CreateRamBlockDevice(
        __in const KGuid& DeviceId,
        __in ULONGLONG DeviceSize,
        __out KBlockDevice::SPtr& DeviceObject,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        );

    struct DeviceAttributes
    {
        //
        // Unique ID of this block device.
        // It is a logic ID and can be used to differentiate different KBlockDevice objects.
        // This ID may or may not be related to any physical volume and disk ID.
        //
        KGuid DeviceId;

        //
        // Size of the block device in bytes. It must be multiples of KBlockDevice::_BlockSize.
        //
        ULONGLONG DeviceSize;        

        //
        // Boolean flag to indicate if writes to this device are write-through.
        //
        BOOLEAN IsWriteThrough;

        //
        // Boolean flag to indicate if this device has a persistent storage.
        // If this flag is FALSE, data written to this device will be lost after crash or reboot.
        //
        BOOLEAN IsPersistent;

        //
        // Boolean flag to indicate if this device has a read cache enabled.
        //
        BOOLEAN IsReadCacheEnabled;

        //
        // Boolean flag to indicate if this device will cache writes into the read cache.
        //
        BOOLEAN NoCachingOnWrite;
    };

    virtual
    VOID
    QueryDeviceAttributes(
        __out DeviceAttributes& Attributes
        ) = 0;

    virtual
    VOID
    QueryDeviceHandle(
        __out HANDLE& Handle
        ) = 0;

    virtual
    NTSTATUS
    AllocateReadContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) = 0;

    virtual
    NTSTATUS
    AllocateNonContiguousReadContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) = 0;

    virtual
    NTSTATUS
    AllocateWriteContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) = 0;

    virtual
    NTSTATUS
    AllocateFlushContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) = 0;

    virtual
    NTSTATUS
    AllocateTrimContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) = 0;

    virtual
    NTSTATUS
    AllocateQueryAllocationsContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) = 0;

    virtual
    NTSTATUS
    AllocateSetSystemIoPriorityHintContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        ) = 0;	
	
    virtual
    NTSTATUS
    Read(
        __in ULONGLONG Offset,
        __in ULONG Length,
        __in BOOLEAN ContiguousIoBuffer,
        __out KIoBuffer::SPtr& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) = 0;

    virtual
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
        ) = 0;

    virtual
    NTSTATUS
    Write(
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) = 0;

    virtual
    NTSTATUS
    Write(
        __in KBlockFile::IoPriority IoPriority,
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) = 0;

    virtual
    NTSTATUS
    Write(
        __in KBlockFile::IoPriority IoPriority,
		__in KBlockFile::SystemIoPriorityHint IoPriorityHint,
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) = 0;

    virtual
    NTSTATUS
    Flush(
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr        
        ) = 0;

    virtual
    NTSTATUS
    Trim(
        __in ULONGLONG TrimFrom,
        __in ULONGLONG TrimToPlusOne,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) = 0;

    virtual
    NTSTATUS
    SetSystemIoPriorityHint(
        __in KBlockFile::SystemIoPriorityHint Hint,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) = 0;

    virtual
    NTSTATUS
    QueryAllocations(
        __in ULONGLONG QueryFrom,
        __in ULONGLONG Length,
        __inout KArray<KBlockFile::AllocatedRange>& Results,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        ) = 0;

	virtual
	NTSTATUS
	SetSparseFile(
		__in BOOLEAN IsSparseFile
		) = 0;

    virtual
    VOID
    Close(
        ) = 0;
};

//
// Implementation of the base class ctor and dtor.
//

inline
KBlockDevice::KBlockDevice()
{
}

inline
KBlockDevice::~KBlockDevice()
{
}
