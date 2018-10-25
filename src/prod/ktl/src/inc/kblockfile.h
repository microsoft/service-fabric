/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    kblockfile.h

Abstract:

    This file defines a block file class, overalapped unbuffered IO.  The file is created with a strong, admin-only ACL.

    The file is always opened exclusively and always sets valid-data-length to the file size.  In this respect, this
    'KBlockFile' class has the same behavior as a raw volume and could be implemented as a raw volume.

Author:

    Norbert P. Kusters (norbertk) 9-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KBlockFile : public KObject<KBlockFile>, public KShared<KBlockFile> {

    K_FORCE_SHARED_WITH_INHERITANCE(KBlockFile);

    public:

        //
        // Global tweaks to KBlockFile behavior.  These can be ignore by any implementation.
        //

        static BOOLEAN NoScatterGather;
        static LONG DisableDiskIo;

        //
        // KBlockFile constants and types.
        //

        static const ULONG BlockSize = 0x1000; // 4 KB

        static const ULONG DefaultQueueDepth = 256;
        
        enum CreateDisposition { eCreateNew, eCreateAlways, eOpenExisting, eOpenAlways, eTruncateExisting };

        //
        // These are bit flags corresponding to the equivalent flags
        // used for the CreateFile api
        //
        // eNoIntermediateBuffering - FILE_NO_INTERMEDIATE_BUFFERING
        // eSequentialAccess - FILE_SEQUENTIAL_ONLY
        // eRandomAccess - FILE_RANDOM_ACCESS
        //
        enum CreateOptions
        { 
            eNoIntermediateBuffering    = 1,    // Only supported for sparse files
            eSequentialAccess           = 2,    // Only supported for sparse files
            eRandomAccess               = 4,    // Only supported for sparse files
            eShareRead                  = 8,
            eShareWrite                 = 16,
            eShareDelete                = 32,
            eInheritFileSecurity        = 64,   // Security descriptor is inherited from parent
            eAlwaysUseFileSystem        = 128,  // IO is only done via file system apis
            eAvoidUseFileSystem         = 256,  // File may not be readable via file system apis
            eReadOnly                   = 512   // File will only be read
        };

        enum IoPriority { eForeground, eBackground };

        enum TransferType { eRead, eWrite };

		//
		// Default frequency to report file operations statistics
		//
		static const ULONG DefaultReportFrequency = 60 * 1000;   // Every minute
		
        //
        // NOTE: These values must match that of IO_PRIORITY_HINT
        //
        enum SystemIoPriorityHint { eNotDefined = -1, eCritical = 4, eHigh = 3, eNormal = 2, eLow = 1, eVeryLow = 0 };

        class TransferContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(TransferContext);
        };

        class FlushContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(FlushContext);
        };

        class TrimContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(TrimContext);
        };

        class QueryAllocationsContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(QueryAllocationsContext);
        };

        class SetSystemIoPriorityHintContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(SetSystemIoPriorityHintContext);
        };

        class EndOfFileContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(EndOfFileContext);
        };

        static
        NTSTATUS
        Create(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __out KBlockFile::SPtr& File,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );

#if defined(K_UseResumable)
        static
        ktl::Awaitable<NTSTATUS>
        CreateAsync(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );

        static
        ktl::Awaitable<NTSTATUS>
        CreateAsync(
            __in LPCWSTR FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );		
#endif

        static
        NTSTATUS
        Create(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __out KBlockFile::SPtr& File,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
        );

#if defined(K_UseResumable)
        static
        ktl::Awaitable<NTSTATUS>
        CreateAsync(
           __in KWString& FileName,
           __in BOOLEAN IsWriteThrough,
           __in CreateDisposition Disposition,
           __in CreateOptions Options,
           __out KBlockFile::SPtr& File,
           __in_opt KAsyncContextBase* const ParentAsync,
           __in KAllocator& Allocator,
           __in ULONG AllocationTag = KTL_TAG_FILE
        );

        static
        ktl::Awaitable<NTSTATUS>
        CreateAsync(
           __in LPCWSTR FileName,
           __in BOOLEAN IsWriteThrough,
           __in CreateDisposition Disposition,
           __in CreateOptions Options,
           __out KBlockFile::SPtr& File,
           __in_opt KAsyncContextBase* const ParentAsync,
           __in KAllocator& Allocator,
           __in ULONG AllocationTag = KTL_TAG_FILE
        );
		
#endif

        static
        NTSTATUS
        Create(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __in ULONG QueueDepth,
            __out KBlockFile::SPtr& File,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
        );

#if defined(K_UseResumable)
        static
        ktl::Awaitable<NTSTATUS>
        CreateAsync(
           __in KWString& FileName,
           __in BOOLEAN IsWriteThrough,
           __in CreateDisposition Disposition,
           __in CreateOptions Options,
           __in ULONG QueueDepth,
           __out KBlockFile::SPtr& File,
           __in_opt KAsyncContextBase* const ParentAsync,
           __in KAllocator& Allocator,
           __in ULONG AllocationTag = KTL_TAG_FILE
        );

        static
        ktl::Awaitable<NTSTATUS>
        CreateAsync(
           __in LPCWSTR FileName,
           __in BOOLEAN IsWriteThrough,
           __in CreateDisposition Disposition,
           __in CreateOptions Options,
           __in ULONG QueueDepth,
           __out KBlockFile::SPtr& File,
           __in_opt KAsyncContextBase* const ParentAsync,
           __in KAllocator& Allocator,
           __in ULONG AllocationTag = KTL_TAG_FILE
        );		
#endif
        
        static
        NTSTATUS
        CreateSparseFile(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __out KBlockFile::SPtr& File,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );

#if defined(K_UseResumable)
        static
        ktl::Awaitable<NTSTATUS>
        CreateSparseFileAsync(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );

        static
        ktl::Awaitable<NTSTATUS>
        CreateSparseFileAsync(
            __in LPCWSTR FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );
		
#endif

        static
        NTSTATUS
        CreateSparseFile(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __out KBlockFile::SPtr& File,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );

#if defined(K_UseResumable)
        static
        ktl::Awaitable<NTSTATUS>
        CreateSparseFileAsync(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );

        static
        ktl::Awaitable<NTSTATUS>
        CreateSparseFileAsync(
            __in LPCWSTR FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );		
#endif

        static
        NTSTATUS
        CreateSparseFile(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __in ULONG QueueDepth,
            __out KBlockFile::SPtr& File,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );

#if defined(K_UseResumable)
        static
        ktl::Awaitable<NTSTATUS>
        CreateSparseFileAsync(
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __in ULONG QueueDepth,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );
		
        static
        ktl::Awaitable<NTSTATUS>
        CreateSparseFileAsync(
            __in LPCWSTR FileName,
            __in BOOLEAN IsWriteThrough,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __in ULONG QueueDepth,
            __out KBlockFile::SPtr& File,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_FILE
            );
#endif
        
        virtual
        const WCHAR*
        GetFileName(
            ) = 0;

        virtual
        ULONGLONG
        QueryFileSize(
            ) = 0;

        virtual
        void
        QueryFileHandle(
            __out HANDLE& Handle
            ) = 0;

        virtual
        BOOLEAN
        IsWriteThrough(
            ) = 0;

        virtual
        BOOLEAN
        IsSparseFile(
            ) = 0;

        virtual
        NTSTATUS
        SetSparseFile(
            __in BOOLEAN IsSparseFile
            ) = 0;

        virtual
        KBlockFile::SystemIoPriorityHint
        GetSystemIoPriorityHint(
            ) = 0;

        virtual
        NTSTATUS
        SetFileSize(
            __in ULONGLONG FileSize,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        SetFileSizeAsync(
            __in ULONGLONG FileSize,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
        ) = 0;
#endif

        virtual
        NTSTATUS
        AllocateTransfer(
            __out TransferContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_FILE
            ) = 0;

        virtual
        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        TransferAsync(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;
#endif

        virtual
        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        TransferAsync(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;
#endif

        virtual
        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

        virtual
        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

        virtual
        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

        virtual
        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

        virtual
        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

        virtual
        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TransferContext* Async = NULL
            ) = 0;

        virtual
        NTSTATUS
        AllocateFlush(
            __out FlushContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_FILE
            ) = 0;

        virtual
        NTSTATUS
        Flush(
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt FlushContext* Async = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        FlushAsync(
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt FlushContext* Async = NULL
            ) = 0;
#endif

        virtual
        NTSTATUS
        AllocateTrim(
            __out TrimContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_FILE
            ) = 0;

        virtual
        NTSTATUS
        Trim(
            __in ULONGLONG TrimFrom,
            __in ULONGLONG TrimToPlusOne,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TrimContext* Async = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        TrimAsync(
            __in ULONGLONG TrimFrom,
            __in ULONGLONG TrimToPlusOne,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TrimContext* Async = NULL
        ) = 0;
#endif

        //
        // This describes the range of space that contains allocations
        // within a sparse file
        //
        struct AllocatedRange
        {
            ULONGLONG Offset;
            ULONGLONG Length;
        };

        virtual
        NTSTATUS
        AllocateQueryAllocations(
            __out QueryAllocationsContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_FILE
            ) = 0;

        virtual
        NTSTATUS
        QueryAllocations(
            __in ULONGLONG QueryFrom,
            __in ULONGLONG Length,
            __inout KArray<AllocatedRange>& Results,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt QueryAllocationsContext* Async = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        QueryAllocationsAsync(
            __in ULONGLONG QueryFrom,
            __in ULONGLONG Length,
            __inout KArray<AllocatedRange>& Results,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt QueryAllocationsContext* Async = NULL
            ) = 0;
#endif
        
        virtual
        NTSTATUS
        AllocateSetSystemIoPriorityHint(
            __out SetSystemIoPriorityHintContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_FILE
            ) = 0;

        virtual
        NTSTATUS
        SetSystemIoPriorityHint(
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt SetSystemIoPriorityHintContext* Async = NULL
            ) = 0;

        virtual
        NTSTATUS
        AllocateEndOfFileContext(
            __out EndOfFileContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_FILE
            ) = 0;

        virtual
        NTSTATUS
        SetEndOfFile(
            __in LONGLONG EndOfFile,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        SetEndOfFileAsync(
            __in LONGLONG EndOfFile,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) = 0;
#endif

        virtual
        NTSTATUS
        GetEndOfFile(
            __out LONGLONG& EndOfFile,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) = 0;

#if defined(K_UseResumable)
        virtual
        ktl::Awaitable<NTSTATUS>
        GetEndOfFileAsync(
            __out LONGLONG& EndOfFile,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) = 0;
#endif

        virtual
        VOID
        CancelAll(
            ) = 0;

        virtual
        VOID
        Close(
            ) = 0;

        virtual
        VOID
        SetBackgroundQueueLength(
            __in ULONG BackgroundQueueLength
            );
};
