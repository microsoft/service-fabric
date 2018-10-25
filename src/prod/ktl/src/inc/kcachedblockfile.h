/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kcachedblockfile.h

Abstract:

    This file defines and implements the a cached block file object.

Author:

    Norbert P. Kusters (norbertk) 17-Dec-2010

Environment:

Notes:

Revision History:

--*/

class KCachedBlockFile : public KObject<KCachedBlockFile>, public KShared<KCachedBlockFile> {

    K_FORCE_SHARED(KCachedBlockFile);

    FAILABLE
    KCachedBlockFile(
        __in const GUID& FileId,
        __inout KBlockFile& File,
        __inout KReadCache& ReadCache,
        __in BOOLEAN NoCachingOnWrite,
        __in ULONG AllocationTag
        );

    public:

        class SetFileSizeContext : public KAsyncContextBase {

            K_FORCE_SHARED(SetFileSizeContext);

            FAILABLE
            SetFileSizeContext(
                __in KCachedBlockFile& CachedFile,
                __in ULONG AllocationTag
                );

            public:

            private:

                NTSTATUS
                Initialize(
                    __in KCachedBlockFile& CachedFile,
                    __in ULONG AllocationTag
                    );

                VOID
                InitializeForSetFileSize(
                    __in KCachedBlockFile& CachedFile,
                    __in ULONGLONG FileSize
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
                RangeLockCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                SetFileSizeCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                friend class KCachedBlockFile;

                KRangeLock::AcquireContext::SPtr _RangeLockContext;

                KCachedBlockFile::SPtr _CachedFile;
                ULONGLONG _FileSize;

        };

        class ReadContext : public KAsyncContextBase {

            K_FORCE_SHARED(ReadContext);

            FAILABLE
            ReadContext(
                __in KCachedBlockFile& CachedFile,
                __in ULONG AllocationTag
                );

            public:

            private:

                NTSTATUS
                Initialize(
                    __in KCachedBlockFile& CachedFile,
                    __in ULONG AllocationTag
                    );

                VOID
                InitializeForRead(
                    __in KCachedBlockFile& CachedFile,
                    __in KBlockFile::IoPriority Priority,
                    __in ULONGLONG Offset,
                    __in ULONG Length,
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
                RangeLockCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                ReadCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                friend class KCachedBlockFile;

                ULONG _AllocationTag;
                KRangeLock::AcquireContext::SPtr _RangeLockContext;

                KCachedBlockFile::SPtr _CachedFile;
                KBlockFile::IoPriority _IoPriority;
                ULONGLONG _Offset;
                ULONG _Length;
                KIoBuffer::SPtr* _IoBuffer;
                ULONG _RefCount;
                NTSTATUS _Status;
                KArray<KBlockFile::TransferContext::SPtr> _TransferContextArray;

        };

        class TransferContext : public KAsyncContextBase {

            K_FORCE_SHARED(TransferContext);

            FAILABLE
            TransferContext(
                __in KCachedBlockFile& CachedFile,
                __in ULONG AllocationTag
                );

            public:

            private:

                NTSTATUS
                Initialize(
                    __in KCachedBlockFile& CachedFile,
                    __in ULONG AllocationTag
                    );

                VOID
                InitializeForReadSingle(
                    __inout KCachedBlockFile& CachedFile,
                    __in KBlockFile::IoPriority Priority,
                    __in ULONGLONG Offset,
                    __inout KIoBufferElement::SPtr& IoBufferElement
                    );

                VOID
                InitializeForWrite(
                    __inout KCachedBlockFile& CachedFile,
                    __in KBlockFile::IoPriority Priority,
                    __in ULONGLONG Offset,
                    __in KIoBufferElement& IoBufferElement
                    );

                VOID
                InitializeForWriteIoBuffer(
                    __inout KCachedBlockFile& CachedFile,
                    __in KBlockFile::IoPriority Priority,
                    __in ULONGLONG Offset,
                    __in KIoBuffer& IoBuffer
                    );

                VOID
                InitializeForCopy(
                    __inout KCachedBlockFile& CachedFile,
                    __in KBlockFile::IoPriority Priority,
                    __in ULONGLONG SourceOffset,
                    __in ULONGLONG TargetOffset,
                    __in KIoBufferElement& IoBufferElement
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
                RangeLockCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                SecondRangeLockCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                VOID
                TransferCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                friend class KCachedBlockFile;

                KBlockFile::TransferContext::SPtr _LowerTransferContext;
                KRangeLock::AcquireContext::SPtr _RangeLockContext;
                KRangeLock::AcquireContext::SPtr _SecondRangeLockContext;

                BOOLEAN _IsCopy;
                BOOLEAN _IsWrite;
                BOOLEAN _WasCopy;
                BOOLEAN _HasCopyCacheHit;

                KCachedBlockFile::SPtr _CachedFile;
                KBlockFile::IoPriority _IoPriority;
                ULONGLONG _Offset;
                ULONG _Length;
                KIoBufferElement::SPtr _IoBufferElement;
                KIoBuffer::SPtr _IoBuffer;

                KIoBufferElement::SPtr* _ReadIoBufferElement;
                ULONGLONG _TargetOffset;

        };

        static
        NTSTATUS
        Create(
            __in const GUID& FileId,
            __inout KBlockFile& File,
            __inout KReadCache& ReadCache,
            __out KCachedBlockFile::SPtr& CachedFile,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_CACHED_BLOCK_FILE,
            __in BOOLEAN NoCachingOnWrite = FALSE
            );

        const WCHAR*
        GetFileName(
            );

        ULONGLONG
        QueryFileSize(
            );

        BOOLEAN
        IsWriteThrough(
            );

        VOID
        QueryFileId(
            __out GUID& FileId
            );

        NTSTATUS
        AllocateSetFileSize(
            __out SetFileSizeContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_CACHED_BLOCK_FILE
            );

        NTSTATUS
        SetFileSize(
            __in ULONGLONG FileSize,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt SetFileSizeContext* Async = NULL
            );

        NTSTATUS
        AllocateRead(
            __out ReadContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_CACHED_BLOCK_FILE
            );

        NTSTATUS
        Read(
            __in KBlockFile::IoPriority Priority,
            __in ULONGLONG Offset,
            __in ULONG Length,
            __out KIoBuffer::SPtr& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt ReadContext* Async = NULL
            );

        NTSTATUS
        AllocateTransfer(
            __out TransferContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_CACHED_BLOCK_FILE
            );

        NTSTATUS
        ReadSingle(
            __in KBlockFile::IoPriority Priority,
            __in ULONGLONG Offset,
            __inout KIoBufferElement::SPtr& IoBufferElement,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            );

        NTSTATUS
        Write(
            __in KBlockFile::IoPriority Priority,
            __in ULONGLONG Offset,
            __in KIoBufferElement& IoBufferElement,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            );

        NTSTATUS
        Write(
            __in KBlockFile::IoPriority Priority,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            );

        NTSTATUS
        Copy(
            __in KBlockFile::IoPriority Priority,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in KIoBufferElement& IoBufferElement,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            );

        NTSTATUS
        AllocateFlush(
            __out KBlockFile::FlushContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_CACHED_BLOCK_FILE
            );

        NTSTATUS
        Flush(
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt KBlockFile::FlushContext* Async = NULL
            );

        VOID
        CancelAll(
            );

        VOID
        Close(
            );

        VOID
        SetBackgroundQueueLength(
            __in ULONG BackgroundQueueLength
            );

        VOID
        PurgeCache(
            ULONGLONG Offset,
            ULONGLONG Length
            );

        NTSTATUS
        AllocateRegisterForEviction(
            __out KReadCache::RegisterForEvictionContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_CACHED_BLOCK_FILE
            );

        VOID
        RegisterForEviction(
            __inout KReadCache::RegisterForEvictionContext& Async,
            __in ULONGLONG FileOffset,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            );

        VOID
        Touch(
            __in KReadCache::RegisterForEvictionContext& Async
            );

    private:

        NTSTATUS
        Initialize(
            __in const GUID& FileId,
            __inout KBlockFile& File,
            __inout KReadCache& ReadCache,
            __in BOOLEAN NoCachingOnWrite,
            __in ULONG AllocationTag
            );

        GUID _FileId;
        KBlockFile::SPtr _File;
        KReadCache::SPtr _ReadCache;
        KRangeLock::SPtr _RangeLock;
        BOOLEAN _NoCachingOnWrite;

        //
        // Performance Counters.
        //

        ULONGLONG _ReadCount;
        ULONGLONG _BytesRead;
        ULONGLONG _CachedReadCount;
        ULONGLONG _CachedBytesRead;
        ULONGLONG _WriteCount;
        ULONGLONG _BytesWritten;
        ULONGLONG _CopyCount;
        ULONGLONG _BytesCopied;
        ULONGLONG _CachedCopyCount;
        ULONGLONG _CachedBytesCopied;

        KPerfCounterSetInstance::SPtr _PerfCounterSetInstance;

};
