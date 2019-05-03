/*++

Module Name:

    kblockfile.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KBlockFile object.

Author:

    Norbert P. Kusters (norbertk) 28-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>

#if defined(PLATFORM_UNIX)
#include <ktlpal.h>
#include <unistd.h>
#include <sys/types.h>

#include <KXMApi.h>
#endif

#include <ktrace.h>
#include <kinstrumentop.h>

#if !defined(PLATFORM_UNIX)
#include <ntdddisk.h>
#endif

#if KTL_USER_MODE
#else
#include <mountmgr.h>
#endif

#include <HLPalFunctions.h>


#if KTL_USER_MODE
#else
extern "C" NTSTATUS KtlMmProbeAndLockSelectedPagesNoException (
  __inout  PMDLX MemoryDescriptorList,
  __in     PFILE_SEGMENT_ELEMENT SegmentArray,
  __in     KPROCESSOR_MODE AccessMode,
  __in     LOCK_OPERATION Operation
    );
#endif

#if defined(K_UseResumable)
using namespace ktl;
#endif

LONGLONG g_BlockFileNumBytesReceived = 0;
LONGLONG g_BlockFileNumBytesSent = 0;
BOOLEAN KBlockFile::NoScatterGather = FALSE;
LONG KBlockFile::DisableDiskIo = FALSE;

class KBlockFileStandard : public KBlockFile {

    K_FORCE_SHARED(KBlockFileStandard);
    
    FAILABLE
    KBlockFileStandard(
        __in KWString& FileName,
        __in CreateDisposition Disposition,
        __in CreateOptions Options,
        __in BOOLEAN IsWriteThrough,
        __in BOOLEAN CreateSparseFile,
        __in ULONG QueueDepth,                     
        __in ULONG AllocationTag
        );

    public:

        class CreateContext : public KAsyncContextBase, public KThreadPool::WorkItem {

            K_FORCE_SHARED(CreateContext);

            CreateContext(
                __in KWString& FileName,
                __in BOOLEAN IsWriteThrough,
                __in BOOLEAN CreateSparseFile,
                __in CreateDisposition Disposition,
                __in CreateOptions Options,
                __in ULONG QueueDepth,
                __out KBlockFile::SPtr& File,
                __in ULONG AllocationTag
                );

            private:

                struct CreateState {
                    CreateContext* Create;
                    NTSTATUS Status;
                };

                VOID
                Zero(
                    );

                VOID
                Cleanup(
                    );

                NTSTATUS
                Initialize(
                    __in KWString& FileName,
                    __in BOOLEAN IsWriteThrough,
                    __in BOOLEAN CreateSparseFile,
                    __in CreateDisposition Disposition,
                    __in CreateOptions Options,
                    __in ULONG QueueDepth,
                    __out KBlockFile::SPtr& File,
                    __in ULONG AllocationTag
                    );

                VOID
                OnStart(
                    );

                VOID
                Execute(
                    );

                static
                VOID
                Worker(
                    __inout VOID* State
                    );

                friend KBlockFile;
                friend KBlockFileStandard;

                KWString _FileName;
                BOOLEAN _IsWriteThrough;
                BOOLEAN _CreateSparseFile;
                CreateDisposition _Disposition;
                CreateOptions _Options;
                ULONG _QueueDepth;
                ULONG _AllocationTag;
                KBlockFile::SPtr* _File;
        };

        class SetFileSizeContext : public KAsyncContextBase, public KThreadPool::WorkItem {

            K_FORCE_SHARED(SetFileSizeContext);

            SetFileSizeContext(
                __inout KBlockFileStandard& File,
                __in ULONGLONG FileSize
                );

            private:

                VOID
                Zero(
                    );

                VOID
                Cleanup(
                    );

                NTSTATUS
                Initialize(
                    __inout KBlockFileStandard& File,
                    __in ULONGLONG FileSize
                    );

                VOID
                OnStart(
                    );

                VOID
                Execute(
                    );

                friend KBlockFileStandard;

                KBlockFileStandard::SPtr _File;
                ULONGLONG _FileSize;

        };

        class SetSystemIoPriorityHintStandard : public SetSystemIoPriorityHintContext, public KThreadPool::WorkItem {

            K_FORCE_SHARED(SetSystemIoPriorityHintStandard);

            SetSystemIoPriorityHintStandard(
                __inout KBlockFileStandard& File
                );

        private:

            VOID
                Zero(
                );

            VOID
                Cleanup(
                );

            NTSTATUS
                Initialize(
                __inout KBlockFileStandard& File
                );

            VOID
                OnStart(
                );

            VOID
                Execute(
                );

            friend KBlockFileStandard;

            KBlockFileStandard::SPtr _File;
            SystemIoPriorityHint _IoPriorityHint;
        };
        
        class EndOfFileStandard : public EndOfFileContext, public KThreadPool::WorkItem {

            K_FORCE_SHARED(EndOfFileStandard);

            EndOfFileStandard(
                __inout KBlockFileStandard& File
                );

        private:

            VOID
                Zero(
                );

            VOID
                Cleanup(
                );

            NTSTATUS
                Initialize(
                __inout KBlockFileStandard& File
                );

            VOID
                OnStart(
                );

            VOID
                Execute(
                );

            friend KBlockFileStandard;

            KBlockFileStandard::SPtr _File;
            enum { SetEndOfFileOperation, GetEndOfFileOperation } _Operation;
            LONGLONG _SetEndOfFile;
            LONGLONG* _GetEndOfFile;
        };

        class TransferStandard : public TransferContext {

            K_FORCE_SHARED(TransferStandard);

            public:

                VOID
                InitializeForTransfer(
                    __inout KBlockFileStandard& File,
                    __in IoPriority Priority,
                    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
                    __in TransferType Type,
                    __in ULONGLONG Offset,
                    __in_bcount(Length) VOID* Buffer,
                    __in ULONG Length
                    );

                VOID
                InitializeForTransfer(
                    __inout KBlockFileStandard& File,
                    __in IoPriority Priority,
                    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
                    __in TransferType Type,
                    __in ULONGLONG Offset,
                    __in KIoBuffer& IoBuffer
                    );

                VOID
                InitializeForCopy(
                    __inout KBlockFileStandard& File,
                    __in IoPriority Priority,
                    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
                    __in ULONGLONG SourceOffset,
                    __in ULONGLONG TargetOffset,
                    __in_bcount(Length) VOID* Buffer,
                    __in ULONG Length
                    );

                VOID
                InitializeForCopy(
                    __inout KBlockFileStandard& File,
                    __in IoPriority Priority,
                    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
                    __in ULONGLONG SourceOffset,
                    __in ULONGLONG TargetOffset,
                    __in KIoBuffer& IoBuffer
                    );


            private:

                VOID
                Zero(
                    );

                VOID
                Cleanup(
                    );

                NTSTATUS
                Initialize(
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

                NTSTATUS
                CheckAlignment(
                    );

                friend class KBlockFileStandard;

                KListEntry ListEntry;
                KListEntry ListEntryCompletion;
                                
                KBlockFileStandard::SPtr _File;
                IoPriority _Priority;
                SystemIoPriorityHint _IoPriorityHint;
                TransferType _TransferType;
                BOOLEAN _IsCopy;
                BOOLEAN _InQueue;
                ULONGLONG _Offset;
                ULONGLONG _TargetOffset;
                VOID* _Buffer;
                ULONG _Length;
                KIoBuffer::SPtr _IoBuffer;
                LONG _RefCount;
                NTSTATUS _Status;

                KInstrumentedOperation _InstrumentedOperation;
        };

        class FlushStandard : public FlushContext, public KThreadPool::WorkItem {

            K_FORCE_SHARED(FlushStandard);

            FlushStandard(
                __in KBlockFileStandard& File
                );

            public:

                VOID
                InitializeForFlush(
                    __inout KBlockFileStandard& File
                    );

            private:

                VOID
                Zero(
                    );

                VOID
                Cleanup(
                    );

                NTSTATUS
                Initialize(
                    __in KBlockFileStandard& File
                    );

                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

                VOID
                Execute(
                    );

#if KTL_USER_MODE
#else
                static
                NTSTATUS
                FlushCompletionRoutine(
                    __in PDEVICE_OBJECT DeviceObject,
                    __in PIRP Irp,
                    __in_xcount_opt("varies") PVOID Context
                    );
#endif

                friend class KBlockFileStandard;

                KBlockFileStandard::SPtr _File;
#if KTL_USER_MODE
#else
                PIRP _Irp;
#endif

        };

        class TrimStandard : public TrimContext, public KThreadPool::WorkItem {

            K_FORCE_SHARED(TrimStandard);

            TrimStandard(
                __in KBlockFileStandard& File
                );

            public:

                VOID
                InitializeForTrim(
                    __inout KBlockFileStandard& File,
                    __in ULONGLONG TrimFrom,
                    __in ULONGLONG TrimToPlusOne
                    );

            private:

                VOID
                Zero(
                    );

                VOID
                Cleanup(
                    );

                NTSTATUS
                Initialize(
                    __in KBlockFileStandard& File
                    );

                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

                VOID
                Execute(
                    );

                friend class KBlockFileStandard;

                KBlockFileStandard::SPtr _File;
                ULONGLONG _TrimFrom;
                ULONGLONG _TrimToPlusOne;
        };

        class QueryAllocationsStandard : public QueryAllocationsContext, public KThreadPool::WorkItem {

            K_FORCE_SHARED(QueryAllocationsStandard);

            QueryAllocationsStandard(
                __in KBlockFileStandard& File
                );

            public:

                VOID
                InitializeForQueryAllocations(
                    __inout KBlockFileStandard& File,
                    __in ULONGLONG QueryFrom,
                    __in ULONGLONG Length,
                    __inout KArray<AllocatedRange>& Results
                    );

            private:

                VOID
                Zero(
                    );

                VOID
                Cleanup(
                    );

                NTSTATUS
                Initialize(
                    __in KBlockFileStandard& File
                    );

                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

                VOID
                Execute(
                    );

                friend class KBlockFileStandard;

                KBlockFileStandard::SPtr _File;
                ULONGLONG _QueryFrom;
                ULONGLONG _Length;
                KArray<AllocatedRange>* _Results;
        };

        const WCHAR*
        GetFileName(
            ) override;

        ULONGLONG
        QueryFileSize(
            ) override;

        void
        QueryFileHandle(
            __out HANDLE& Handle
            ) override;

        BOOLEAN
        IsWriteThrough(
            ) override;

        BOOLEAN
        IsSparseFile(
            ) override;

        NTSTATUS
        SetSparseFile(
            __in BOOLEAN IsSparseFile
            ) override;
        
        static NTSTATUS AllocateCreateContext(
            __out CreateContext*& createContext,
            __in KWString& FileName,
            __in BOOLEAN IsWriteThrough,
            __in BOOLEAN IsSparseFile,
            __in KBlockFile::CreateDisposition Disposition,
            __in CreateOptions Options,
            __in ULONG QueueDepth,
            __out KBlockFile::SPtr& File,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
            );

        KBlockFile::SystemIoPriorityHint
        GetSystemIoPriorityHint(
            ) override;
        
        NTSTATUS
        AllocateSetSystemIoPriorityHint(
            __out SetSystemIoPriorityHintContext::SPtr& Async,
            __in ULONG AllocationTag
            ) override;

        NTSTATUS
        SetSystemIoPriorityHint(
            __in SystemIoPriorityHint IoPriorityHint,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt SetSystemIoPriorityHintContext* Async = NULL
            ) override;

        NTSTATUS
        AllocateEndOfFileContext(
            __out EndOfFileContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_FILE
            ) override ;

        NTSTATUS
        SetEndOfFile(
            __in LONGLONG EndOfFile,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) override ;

#if defined(K_UseResumable)
        Awaitable<NTSTATUS>
        SetEndOfFileAsync(
            __in LONGLONG EndOfFile,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) override;
#endif

        NTSTATUS
        GetEndOfFile(
            __out LONGLONG& EndOfFile,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) override ;

#if defined(K_UseResumable)
        Awaitable<NTSTATUS>
        GetEndOfFileAsync(
            __out LONGLONG& EndOfFile,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt EndOfFileContext* Async = NULL
            ) override;
#endif

        NTSTATUS
        SetFileSize(
            __in ULONGLONG FileSize,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync
            ) override;

#if defined(K_UseResumable)
        Awaitable<NTSTATUS>
        SetFileSizeAsync(
            __in ULONGLONG FileSize,
            __in_opt KAsyncContextBase* const ParentAsync
            ) override;
#endif

        NTSTATUS
        AllocateTransfer(
            __out TransferContext::SPtr& Async,
            __in ULONG AllocationTag
            ) override;

        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            ) override;

#if defined(K_UseResumable)
        Awaitable<NTSTATUS>
        TransferAsync(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
        ) override;
#endif

        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            ) override;

#if defined(K_UseResumable)
        Awaitable<NTSTATUS>
        TransferAsync(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
        ) override;
#endif
        
        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            ) override;

        NTSTATUS
        Transfer(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in TransferType Type,
            __in ULONGLONG Offset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            ) override;

        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            );

        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in_bcount(Length) VOID* Buffer,
            __in ULONG Length,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            );

        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            ) override;

        NTSTATUS
        Copy(
            __in IoPriority Priority,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __in ULONGLONG SourceOffset,
            __in ULONGLONG TargetOffset,
            __in KIoBuffer& IoBuffer,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TransferContext* Async
            ) override;

        NTSTATUS
        AllocateFlush(
            __out FlushContext::SPtr& Async,
            __in ULONG AllocationTag
            ) override;

        NTSTATUS
        Flush(
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt FlushContext* Async
            ) override;

#if defined(K_UseResumable)
        ktl::Awaitable<NTSTATUS>
        FlushAsync(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt FlushContext* Async
            ) override;
#endif

        NTSTATUS
        AllocateTrim(
            __out TrimContext::SPtr& Async,
            __in ULONG AllocationTag
            ) override;

        NTSTATUS
        Trim(
            __in ULONGLONG TrimFrom,
            __in ULONGLONG TrimToPlusOne,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt TrimContext* Async
            ) override;

#if defined(K_UseResumable)
        Awaitable<NTSTATUS>
        TrimAsync(
            __in ULONGLONG TrimFrom,
            __in ULONGLONG TrimToPlusOne,
            __in_opt KAsyncContextBase* const ParentAsync = NULL,
            __in_opt TrimContext* Async = NULL
            ) override;
#endif

        NTSTATUS
        AllocateQueryAllocations(
            __out QueryAllocationsContext::SPtr& Async,
            __in ULONG AllocationTag
            ) override;

        NTSTATUS
        QueryAllocations(
            __in ULONGLONG QueryFrom,
            __in ULONGLONG Length,
            __inout KArray<AllocatedRange>& Results,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt QueryAllocationsContext* Async
            ) override;

#if defined(K_UseResumable)
        Awaitable<NTSTATUS>
        QueryAllocationsAsync(
            __in ULONGLONG QueryFrom,
            __in ULONGLONG Length,
            __inout KArray<AllocatedRange>& Results,
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt QueryAllocationsContext* Async
            ) override;
#endif
        
        VOID
        CancelAll(
            ) override;

        VOID
        Close(
            ) override;

        VOID
        SetBackgroundQueueLength(
            __in ULONG BackgroundQueueLength
            ) override;

    private:

        //
        // KBlockFile is implemented to provide high performance IO to
        // a file. Files may be sparse or non sparse. User and kernel
        // mode accesses are implemented differently. Where possible,
        // scatter/gather IO is performed.

        //
        // Each request from the users of the KBlockFile represent their
        // data in a KIoBuffer which may contain multiple discontiguous
        // memory blocks. Each request is represented by a
        // TransferStandard object. The different memory blocks in the
        // TransferStandard may be written by different IO operations
        // and thus is refcounted. Only when all IO operations for a
        // TransferStandard have completed will the TransferStandard
        // itself be completed.

        //
        // When an IO is performed and a TransferStandard is received
        // the implementation will place it on a RequestQueue. There is
        // a foreground and a background request queue. Only when all
        // foreground requests have been completed will
        // TransferStandards be removed from the background request
        // queue.

        //
        // The implementation will process the request queues in
        // DispatchIoPackets(). This routine will loop processing the
        // different memory blocks to read/write in the
        // TransferStandard and placing them into an IoPacket in a way
        // that aggregates contiguous (on disk) IO and sets up for
        // scatter/gather IO if possible. The IoPacket describes the
        // all the buffers for the IO and the TransferStandard to which
        // they belong.
        //
        //
        // The IoPacket struct is a base struct and has two derivations
        // depending upon the type of file and code execution mode. For
        // user and kernel mode pinned files, and also user mode sparse
        // files, scatter/gather IO is available and the SGIoPacket
        // struct is used. For kernel mode sparse files, scatter/gather
        // IO is not available and so the NoSGIoPacket is used.

        //
        // Once as many TransferStandard have been processed to fill as
        // many available IoPackets are done then the IoPackets are
        // passed to SendIoPacket(). SendIoPacket will perform the IO
        // as appropriate for the type of file and code execution modes.

        //
        // Different file types and code execution modes
        // affect the implementation of FillIoPacket() and
        // SendIoPacket().
        //
        // NonSparse files in kernel mode - FillIoPacket() will walk
        // the list of KIoBufferElements in the TransferStandard and
        // fill the AlignedBuffers struct that describe the
        // scatter/gather IO. SendIoPacket will then create an IRP and
        // forward it to the top of the volume stack for the disk and
        // bypassing the file system.
        //
        // Sparse and NonSparse files in user mode - FillIoPacket has
        // same implementation as NonSparse in kernel mode.
        // SendIoPacket uses scatter/gather usermode IO apis.
        //
        // Sparse files in kernel mode - There are no scatter/gather
        // apis for this and so the FillIoPacket implementation will
        // walk each KIoBufferElement in the TransferStandard and for
        // KIoBufferElement, fill a new IoPacket. In this way there
        // will be parallel writes (up to the QueueDepth) for the
        // sparse file IO in kernel.
        //
        //
        // Note that the IoPackets are different types for
        // scatter/gather and non/scatter gather implementations since
        // the memory usage for scatter/gather is much higher and thus
        // would be wasted for non scatter/gather.
        //

        static const ULONG MaxIoSize = 0x100000; // 1 MB

        //
        // Note that KBlockFile::MaxBlocksPerIo must equal
        // KXM_MAX_FILE_SG and MAX_FILE_SEGMENT_ELEMENT
        //      
        static const ULONG MaxBlocksPerIo = MaxIoSize/BlockSize;

        struct IoPacket {
            KListEntry ListEntry;
            KBlockFileStandard* File;
            IoPriority Priority;
            TransferType Type;
            BOOLEAN IsCopy;
            ULONGLONG Offset;
            ULONGLONG TargetOffset;
            ULONG Length;
            ULONG RemainingLength;
            VOID* Buffer;
        };

#if KTL_USER_MODE
#else
        struct NoSGIoPacket;
        class NoSGFileIoOperation : public KAsyncContextBase, public KThreadPool::WorkItem
        {
            K_FORCE_SHARED(NoSGFileIoOperation);

            public:
                NoSGFileIoOperation(__in NoSGIoPacket* NoSGIoPacket);

                VOID
                StartNoSGOperation(
                    __in HANDLE Handle,
                    __in PVOID Buffer,
                    __in ULONG LengthToUse,
                    __in LARGE_INTEGER FileOffset,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

                NoSGIoPacket* GetNoSGIoPacket()
                {
                    return(_NoSGPacket);
                }

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

                VOID
                Execute(
                    );

            private:
                static VOID
                KBlockFileStandard::NoSGFileIoOperation::NoSGOperationApcCompletion(
                    __in PVOID Context,
                    __in PIO_STATUS_BLOCK IoStatusBlock,
                    __in ULONG Reserved
                    );

                NoSGIoPacket* _NoSGPacket;
                HANDLE _Handle;
                PVOID _Buffer;
                ULONG _LengthToUse;
                LARGE_INTEGER _FileOffset;
        };

        VOID NoSGIoOperationCompleted(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        struct NoSGIoPacket : IoPacket
        {
            IO_STATUS_BLOCK IoStatus;
            NoSGFileIoOperation::SPtr NoSGOperation;
            TransferStandard* TransferPacket;
        };
#endif

        struct SGIoPacket : IoPacket
        {
#if KTL_USER_MODE
            OVERLAPPED Overlapped;
            ULONG NumAlignedBuffers;
            HLPalFunctions::ALIGNED_BUFFER AlignedBuffers[MaxBlocksPerIo+1];
#else
            IRP* Irp;
            MDL* Mdl;
            MDL* PartialMdl;
            IO_STATUS_BLOCK IoStatus;
            KBlockFile::SystemIoPriorityHint IoPriorityHint;
            ULONGLONG StartTicks;
            ULONG NumFileSegmentElements;
            FILE_SEGMENT_ELEMENT FileSegmentElements[MaxBlocksPerIo + 1];
#endif
            ULONG NumTransferPackets;
            
            TransferStandard* TransferPackets[MaxBlocksPerIo];
        };

        struct RequestQueue {

            RequestQueue(
                );

            BOOLEAN
            IsEmpty(
                );

            KNodeList<TransferStandard> Queue;
            TransferStandard* CurrentRequest;
            ULONG CurrentRequestOffset;

        };

#if KTL_USER_MODE
#define IsScatterGatherAvailable() (TRUE)
#else
        __inline BOOLEAN
        IsScatterGatherAvailable(
            )
        {
            return( (! _IsSparseFile) && (! _IsReadOnlyFile) );
        }
#endif


        __inline IO_PRIORITY_HINT
        GetIoPriorityHint(
            __in SystemIoPriorityHint IoPriorityHint
            )
        {
            KAssert(IoPriorityHint != eNotDefined);

            static_assert(eCritical == IoPriorityCritical, "eCritical == IoPriorityCritical");
            static_assert(eHigh == IoPriorityHigh, "eHigh == IoPriorityHigh");
            static_assert(eNormal == IoPriorityNormal, "eNormal == IoPriorityNormal");
            static_assert(eLow == IoPriorityLow, "eLow == IoPriorityLow");
            static_assert(eVeryLow == IoPriorityVeryLow, "eVeryLow == IoPriorityVeryLow");

            //
            // Use the compiler to enforce equivalence in constant values
            //
           return((IO_PRIORITY_HINT)IoPriorityHint);
        }

        __inline void
        SetSystemIoPriorityHintInternal(
            __in SystemIoPriorityHint IoPriorityHint
            )
        {
            _IoPriorityHint = IoPriorityHint;
        }

#if KTL_USER_MODE
#else
        struct FileExtentKey {
            ULONGLONG FileOffset;
            ULONGLONG Length;
        };

        struct FileExtentValue {
            ULONGLONG VolumeOffset;
        };

        typedef KAvlTree<FileExtentValue, FileExtentKey> ExtentTable;
#endif

        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        Initialize(
            __in KWString& FileName,
            __in CreateDisposition Disposition,
            __in CreateOptions Options,
            __in BOOLEAN IsWriteThrough,
            __in BOOLEAN CreateSparseFile,
            __in ULONG AllocationTag
            );

        static
        NTSTATUS
        CreateSecurityDescriptor(
            __out SECURITY_DESCRIPTOR& SecurityDescriptor,
            __out ACL*& Acl,
            __in KAllocator& Allocator
            );

        VOID
        DispatchIoPackets(
            );

        BOOLEAN
        FillIoPacket(
            __inout IoPacket* Packet
            );

        VOID
        AddToIoPacket(
            __inout IoPacket* Packet,
            __in TransferType Type,
            __in BOOLEAN IsCopy,
            __in ULONGLONG Offset,
            __in ULONGLONG TargetOffset,
            __in const VOID* Buffer,
            __in ULONG Length,
            __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
            __out ULONG& LengthNotAdded
            );

        VOID
        SendIoPacket(
            __inout IoPacket* Packet
            );

#if KTL_USER_MODE
        static
        BOOL
        ManageVolumePrivilege(
            );

        static
        VOID
        IoCompletionRoutineUserMode(
            __in VOID* File,
            __inout OVERLAPPED* Overlapped,
            __in DWORD Win32ErrorCode,
            __in ULONG BytesTransferred
            );
#else
        static
        NTSTATUS IoCompletionRoutineKernelMode(
            __in IoPacket* IoPacketContext,
            __in NTSTATUS Status
        );

        static
        NTSTATUS
        IoCompletionRoutineForBlockFile(
            __in_opt PDEVICE_OBJECT DeviceObject,
            __in PIRP Irp,
            __in_xcount_opt("varies") PVOID Context
            );

        VOID
        SendIrp(
            __inout IoPacket* Packet,
            __in ULONGLONG FileOffset,
            __in ULONG Length
            );

        static
        LONG
        FileExtentComparison(
            __in const FileExtentKey& Left,
            __in const FileExtentKey& Right
            );

        NTSTATUS
        InitializeKernelMode(
            __in ULONG AllocationTag
            );

        NTSTATUS
        PinFile(
            );

        NTSTATUS
        IsNtfs(
            __out BOOLEAN* IsNtfs
            );

        NTSTATUS
        AdjustExtentTable(
            __in ULONGLONG OldFileSize,
            __in ULONGLONG NewFileSize
            );

        NTSTATUS
        ShrinkExtentTable(
            __in ULONGLONG NewFileSize
            );


#endif

#if KTL_USER_MODE
        friend class FlushWorkItem;
#endif

        KSpinLock _SpinLock;
        HANDLE _Handle;
        ULONG _QueueDepth;
        KWString _FileName;
        BOOLEAN _IsWriteThrough;
        BOOLEAN _IsSparseFile;
        BOOLEAN _IsReadOnlyFile;
        ULONGLONG _FileSize;
        ULONG _PageSize;
        RequestQueue _Foreground;
        RequestQueue _Background;
        KNodeList<IoPacket> _IoPacketList;
        ULONG _NumForegroundIoPacketsOutstanding;
        ULONG _NumBackgroundIoPacketsOutstanding;
        ULONG _MaxBackground;
        volatile LONG _NumOSIoPacketsOutstanding;
        SystemIoPriorityHint _IoPriorityHint;
#if KTL_USER_MODE
        VOID* _RegistrationContext;
#else
        ExtentTable::KeyComparisonFunc _ExtentTableComparisonFunction;
        DEVICE_OBJECT* _VolumeObject;
        FILE_OBJECT* _FileObject;
        ExtentTable _ExtentTable;
        ULONGLONG _NumberDelayedWrites;
        ULONGLONG _LastSlowWriteTicks;
#endif

        KInstrumentedComponent::SPtr _InstrumentedComponent;

};


KBlockFile::~KBlockFile(
    )
{
}

KBlockFile::KBlockFile(
    )
{
}

NTSTATUS KBlockFileStandard::AllocateCreateContext(
    __out KBlockFileStandard::CreateContext*& createContext,
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN IsSparseFile,
    __in KBlockFile::CreateDisposition Disposition,
    __in CreateOptions Options,
    __in ULONG QueueDepth,
    __out KBlockFile::SPtr& File,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KBlockFileStandard::CreateContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_FILE, Allocator) CreateContext(
        FileName,
        IsWriteThrough,
        IsSparseFile,
        Disposition,
        Options,
        QueueDepth,
        File,
        AllocationTag);

    if (!context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status))
    {
        _delete(context);
        return status;
    }

    createContext = context;
    return STATUS_SUCCESS;
}

NTSTATUS
KBlockFile::Create(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __out KBlockFile::SPtr& File,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new KBlockFile object.

Arguments:

    FileName        - Supplies the full path file name to create/open for this KBlockFile.

    IsWriteThrough  - Supplies whether or not all IO should be write through for this KBlockFile object.

    Disposition     - Supplies the create disposition.

    File            - Returns the KBlockFile object created by this method.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, a parent async to use for synchronizing the completion routine.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    NTSTATUS status;

    status = Create(
        FileName,
        IsWriteThrough,
        Disposition,
        static_cast<CreateOptions>(0),
        File,
        Completion,
        ParentAsync,
        Allocator,
        AllocationTag);

    return status;
}

NTSTATUS
KBlockFile::Create(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __out KBlockFile::SPtr& File,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KBlockFileStandard::CreateContext* context;

    NTSTATUS status = KBlockFileStandard::AllocateCreateContext(
        context,
        FileName,
        IsWriteThrough,
        FALSE,
        Disposition,
        Options,
        KBlockFile::DefaultQueueDepth,
        File,
        Allocator,
        AllocationTag);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    context->Start(ParentAsync, Completion);
    return STATUS_PENDING;
}

NTSTATUS
KBlockFile::Create(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in ULONG QueueDepth,                 
    __out KBlockFile::SPtr& File,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KBlockFileStandard::CreateContext* context;

    NTSTATUS status = KBlockFileStandard::AllocateCreateContext(
        context,
        FileName,
        IsWriteThrough,
        FALSE,
        Disposition,
        Options,
        QueueDepth,
        File,
        Allocator,
        AllocationTag);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    context->Start(ParentAsync, Completion);
    return STATUS_PENDING;
}


#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFile::CreateAsync(
    __in LPCWSTR FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KWString s(Allocator);
    s = FileName;
    if (! NT_SUCCESS(s.Status()))
    {
        co_return s.Status();
    }

    co_return co_await KBlockFile::CreateAsync(s, IsWriteThrough, Disposition, File, ParentAsync, Allocator, AllocationTag);    
}

Awaitable<NTSTATUS>
KBlockFile::CreateAsync(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KBlockFileStandard::CreateContext::SPtr context;
    NTSTATUS status;

    {
        KBlockFileStandard::CreateContext* tContext = nullptr;

        status = KBlockFileStandard::AllocateCreateContext(
            tContext,
            FileName,
            IsWriteThrough,
            FALSE,
            Disposition,
            static_cast<CreateOptions>(0),
            KBlockFile::DefaultQueueDepth,          
            File,
            Allocator,
            AllocationTag);

        if (!NT_SUCCESS(status))
        {
            co_return status;
        }

        context = tContext;
    }

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        Allocator,
        AllocationTag,
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;

    //co_return co_await CreateAsync(
    //    FileName,
    //    IsWriteThrough,
    //    Disposition,
    //    static_cast<CreateOptions>(0),
    //    File,
    //    ParentAsync,
    //    Allocator,
    //    AllocationTag);
}

Awaitable<NTSTATUS>
KBlockFile::CreateAsync(
   __in LPCWSTR FileName,
   __in BOOLEAN IsWriteThrough,
   __in CreateDisposition Disposition,
   __in CreateOptions Options,
   __out KBlockFile::SPtr& File,
   __in_opt KAsyncContextBase* const ParentAsync,
   __in KAllocator& Allocator,
   __in ULONG AllocationTag
    )
{
    KWString s(Allocator);
    s = FileName;
    if (! NT_SUCCESS(s.Status()))
    {
        co_return s.Status();
    }

    co_return co_await KBlockFile::CreateAsync(s, IsWriteThrough, Disposition, Options, File, ParentAsync, Allocator, AllocationTag);   
}

Awaitable<NTSTATUS>
KBlockFile::CreateAsync(
   __in KWString& FileName,
   __in BOOLEAN IsWriteThrough,
   __in CreateDisposition Disposition,
   __in CreateOptions Options,
   __out KBlockFile::SPtr& File,
   __in_opt KAsyncContextBase* const ParentAsync,
   __in KAllocator& Allocator,
   __in ULONG AllocationTag
   )
{
   KBlockFileStandard::CreateContext::SPtr context;
   NTSTATUS status;

   {
       // temp variables to work around compiler OOM
       KWString& fileNameTemp = FileName;
       BOOLEAN& isWriteThroughTemp = IsWriteThrough;
       CreateDisposition& dispositionTemp = Disposition;
       CreateOptions& optionsTemp = Options;

       KBlockFileStandard::CreateContext* tContext = nullptr;

       status = KBlockFileStandard::AllocateCreateContext(
           tContext,
           fileNameTemp,
           isWriteThroughTemp,
           FALSE,
           dispositionTemp,
           optionsTemp,
           KBlockFile::DefaultQueueDepth,          
           File,
           Allocator,
           AllocationTag);

       if (!NT_SUCCESS(status))
       {
           co_return status;
       }

       context = tContext;
   }

   ktl::kasync::StartAwaiter::SPtr awaiter;

   status = ktl::kasync::StartAwaiter::Create(
       Allocator,
       AllocationTag,
       *context,
       awaiter,
       ParentAsync,
       nullptr);

   if (!NT_SUCCESS(status))
   {
       co_return status;
   }

   status = co_await *awaiter;
   co_return status;
}

Awaitable<NTSTATUS>
KBlockFile::CreateAsync(
   __in LPCWSTR FileName,
   __in BOOLEAN IsWriteThrough,
   __in CreateDisposition Disposition,
   __in CreateOptions Options,
   __in ULONG QueueDepth,
   __out KBlockFile::SPtr& File,
   __in_opt KAsyncContextBase* const ParentAsync,
   __in KAllocator& Allocator,
   __in ULONG AllocationTag
    )
{
    KWString s(Allocator);
    s = FileName;
    if (! NT_SUCCESS(s.Status()))
    {
        co_return s.Status();
    }

    co_return co_await KBlockFile::CreateAsync(s, IsWriteThrough, Disposition, Options, QueueDepth, File, ParentAsync, Allocator, AllocationTag);   
}

Awaitable<NTSTATUS>
KBlockFile::CreateAsync(
   __in KWString& FileName,
   __in BOOLEAN IsWriteThrough,
   __in CreateDisposition Disposition,
   __in CreateOptions Options,
   __in ULONG QueueDepth,
   __out KBlockFile::SPtr& File,
   __in_opt KAsyncContextBase* const ParentAsync,
   __in KAllocator& Allocator,
   __in ULONG AllocationTag
   )
{
   KBlockFileStandard::CreateContext::SPtr context;
   NTSTATUS status;

   {
       // temp variables to work around compiler OOM
       KWString& fileNameTemp = FileName;
       BOOLEAN& isWriteThroughTemp = IsWriteThrough;
       CreateDisposition& dispositionTemp = Disposition;
       CreateOptions& optionsTemp = Options;

       KBlockFileStandard::CreateContext* tContext = nullptr;

       status = KBlockFileStandard::AllocateCreateContext(
           tContext,
           fileNameTemp,
           isWriteThroughTemp,
           FALSE,
           dispositionTemp,
           optionsTemp,
           QueueDepth,          
           File,
           Allocator,
           AllocationTag);

       if (!NT_SUCCESS(status))
       {
           co_return status;
       }

       context = tContext;
   }

   ktl::kasync::StartAwaiter::SPtr awaiter;

   status = ktl::kasync::StartAwaiter::Create(
       Allocator,
       AllocationTag,
       *context,
       awaiter,
       ParentAsync,
       nullptr);

   if (!NT_SUCCESS(status))
   {
       co_return status;
   }

   status = co_await *awaiter;
   co_return status;
}

#endif

NTSTATUS
KBlockFile::CreateSparseFile(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __out KBlockFile::SPtr& File,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new KSparseFile object.

Arguments:

    FileName        - Supplies the full path file name to create/open for this KSparseFile.

    IsWriteThrough  - Supplies whether or not all IO should be write through for this KSparseFile object.

    Disposition     - Supplies the create disposition.

    File            - Returns the KSparseFile object created by this method.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, a parent async to use for synchronizing the completion routine.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    NTSTATUS status;

    status = CreateSparseFile(FileName,
                              IsWriteThrough,
                              Disposition,
                              eNoIntermediateBuffering,
                              File,
                              Completion,
                              ParentAsync,
                              Allocator,
                              AllocationTag);
    return(status);
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFile::CreateSparseFileAsync(
    __in LPCWSTR FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    KWString s(Allocator);
    s = FileName;
    if (! NT_SUCCESS(s.Status()))
    {
        co_return s.Status();
    }

    co_return co_await KBlockFile::CreateSparseFileAsync(s, IsWriteThrough, Disposition, File, ParentAsync, Allocator, AllocationTag);  
}


Awaitable<NTSTATUS>
KBlockFile::CreateSparseFileAsync(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    KBlockFileStandard::CreateContext::SPtr context;
    NTSTATUS status;

    {
        KBlockFileStandard::CreateContext*  tContext = nullptr;
        status = KBlockFileStandard::AllocateCreateContext(
            tContext,
            FileName,
            IsWriteThrough,
            TRUE,
            Disposition,
            eNoIntermediateBuffering,
            KBlockFile::DefaultQueueDepth,
            File,
            Allocator,
            AllocationTag);

        if (!NT_SUCCESS(status))
        {
            co_return status;
        }

        context = tContext;
    }

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        Allocator,
        AllocationTag,
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}

Awaitable<NTSTATUS>
KBlockFile::CreateSparseFileAsync(
    __in LPCWSTR FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    KWString s(Allocator);
    s = FileName;
    if (! NT_SUCCESS(s.Status()))
    {
        co_return s.Status();
    }

    co_return co_await KBlockFile::CreateSparseFileAsync(s, IsWriteThrough, Disposition, Options, File, ParentAsync, Allocator, AllocationTag); 
}
#endif


NTSTATUS
KBlockFile::CreateSparseFile(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __out KBlockFile::SPtr& File,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new KSparseFile object.

Arguments:

    FileName        - Supplies the full path file name to create/open for this KSparseFile.

    IsWriteThrough  - Supplies whether or not all IO should be write through for this KSparseFile object.

    Disposition     - Supplies the create disposition.

    Options   - Supplies the options to use when creating the file

    File            - Returns the KSparseFile object created by this method.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, a parent async to use for synchronizing the completion routine.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KBlockFileStandard::CreateContext* context;

    NTSTATUS status = KBlockFileStandard::AllocateCreateContext(
        context,
        FileName,
        IsWriteThrough,
        TRUE,
        Disposition,
        Options,
        KBlockFile::DefaultQueueDepth,
        File,
        Allocator,
        AllocationTag);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    context->Start(ParentAsync, Completion);
    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFile::CreateSparseFileAsync(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    KBlockFileStandard::CreateContext::SPtr context;
    NTSTATUS status;
    
    // workaround for compiler OOM bug
    auto& tempFileName = FileName;
    auto& tempIsWriteThrough = IsWriteThrough;
    auto& tempDisposition = Disposition;
    auto& tempOptions = Options;
    auto& tempFile = File;
    auto& tempParentAsync = ParentAsync;
    auto& tempAllocator = Allocator;
    auto& tempAllocationTag = AllocationTag;

    {
        KBlockFileStandard::CreateContext*  tContext = nullptr;
        status = KBlockFileStandard::AllocateCreateContext(
            tContext,
            tempFileName,
            tempIsWriteThrough,
            TRUE,
            tempDisposition,
            tempOptions,
            KBlockFile::DefaultQueueDepth,
            tempFile,
            tempAllocator,
            tempAllocationTag);

        if (!NT_SUCCESS(status))
        {
            co_return status;
        }

        context = tContext;
    }

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        tempAllocator,
        tempAllocationTag,
        *context,
        awaiter,
        tempParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif

NTSTATUS
KBlockFile::CreateSparseFile(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in ULONG QueueDepth,
    __out KBlockFile::SPtr& File,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine creates a new KSparseFile object.

Arguments:

    FileName        - Supplies the full path file name to create/open for this KSparseFile.

    IsWriteThrough  - Supplies whether or not all IO should be write through for this KSparseFile object.

    Disposition     - Supplies the create disposition.

    Options   - Supplies the options to use when creating the file

    File            - Returns the KSparseFile object created by this method.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, a parent async to use for synchronizing the completion routine.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    KBlockFileStandard::CreateContext* context;

    NTSTATUS status = KBlockFileStandard::AllocateCreateContext(
        context,
        FileName,
        IsWriteThrough,
        TRUE,
        Disposition,
        Options,
        QueueDepth,
        File,
        Allocator,
        AllocationTag);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    context->Start(ParentAsync, Completion);
    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFile::CreateSparseFileAsync(
    __in LPCWSTR FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in ULONG QueueDepth,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    KWString s(Allocator);
    s = FileName;
    if (! NT_SUCCESS(s.Status()))
    {
        co_return s.Status();
    }

    co_return co_await KBlockFile::CreateSparseFileAsync(s, IsWriteThrough, Disposition, Options, QueueDepth, File, ParentAsync, Allocator, AllocationTag); 
}


Awaitable<NTSTATUS>
KBlockFile::CreateSparseFileAsync(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in ULONG QueueDepth,
    __out KBlockFile::SPtr& File,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    KBlockFileStandard::CreateContext::SPtr context;
    NTSTATUS status;
    
    // workaround for compiler OOM bug
    auto& tempFileName = FileName;
    auto& tempIsWriteThrough = IsWriteThrough;
    auto& tempDisposition = Disposition;
    auto& tempOptions = Options;
    auto& tempFile = File;
    auto& tempParentAsync = ParentAsync;
    auto& tempAllocator = Allocator;
    auto& tempAllocationTag = AllocationTag;
    auto& tempQueueDepth = QueueDepth;

    {
        KBlockFileStandard::CreateContext*  tContext = nullptr;
        status = KBlockFileStandard::AllocateCreateContext(
            tContext,
            tempFileName,
            tempIsWriteThrough,
            TRUE,
            tempDisposition,
            tempOptions,
            tempQueueDepth,
            tempFile,
            tempAllocator,
            tempAllocationTag);

        if (!NT_SUCCESS(status))
        {
            co_return status;
        }

        context = tContext;
    }

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        tempAllocator,
        tempAllocationTag,
        *context,
        awaiter,
        tempParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif


VOID
KBlockFile::SetBackgroundQueueLength(
    __in ULONG
    )
{
}

KBlockFile::SetSystemIoPriorityHintContext::~SetSystemIoPriorityHintContext(
    )
{
}

KBlockFile::SetSystemIoPriorityHintContext::SetSystemIoPriorityHintContext(
    )
{
}

KBlockFile::EndOfFileContext::~EndOfFileContext(
    )
{
}

KBlockFile::EndOfFileContext::EndOfFileContext(
    )
{
}

KBlockFile::TrimContext::~TrimContext(
    )
{
}

KBlockFile::TrimContext::TrimContext(
    )
{
}

KBlockFile::QueryAllocationsContext::~QueryAllocationsContext(
    )
{
}

KBlockFile::QueryAllocationsContext::QueryAllocationsContext(
    )
{
}

KBlockFile::TransferContext::~TransferContext(
    )
{
}

KBlockFile::TransferContext::TransferContext(
    )
{
}

KBlockFile::FlushContext::~FlushContext(
    )
{
}

KBlockFile::FlushContext::FlushContext(
    )
{
}

KBlockFileStandard::~KBlockFileStandard(
    )
{
    Cleanup();
}

KBlockFileStandard::KBlockFileStandard(
    __in KWString& FileName,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN CreateSparseFile,
    __in ULONG QueueDepth,
    __in ULONG AllocationTag
    ) : _IoPacketList(FIELD_OFFSET(IoPacket, ListEntry)),
        _FileName(GetThisAllocator())
      , _QueueDepth(QueueDepth)
#if KTL_USER_MODE
#else
    , _ExtentTableComparisonFunction(&KBlockFileStandard::FileExtentComparison)
    , _ExtentTable(_ExtentTableComparisonFunction, GetThisAllocator(), KTL_TAG_FILE)
#endif
{
    Zero();
    SetConstructorStatus(Initialize(FileName, Disposition, Options, IsWriteThrough, CreateSparseFile, AllocationTag));
}

const WCHAR*
KBlockFileStandard::GetFileName(
    )
{
    return _FileName;
}

ULONGLONG
KBlockFileStandard::QueryFileSize(
    )
{
    return _FileSize;
}

void
KBlockFileStandard::QueryFileHandle(
    __out HANDLE& Handle
    )
{
    Handle = _Handle;
}


BOOLEAN
KBlockFileStandard::IsWriteThrough(
    )

/*++

Routine Description:

    This routine returns whether or not the file has been opened write-through.  The user of this class will need to call
    'Flush' to guarantee that data made it past the disk's internal cache if 'IsWriteThrough' is not set properly.

Arguments:

    None.

Return Value:

    FALSE   - The file is not opened in write-through mode.

    TRUE    - The file is opened in write-through mode.

--*/

{
    return _IsWriteThrough;
}

BOOLEAN
KBlockFileStandard::IsSparseFile(
    )

/*++

Routine Description:

    This routine returns whether or not the file is a sparse file or not

Arguments:

    None.

Return Value:

    FALSE   - The file is not a sparse file

    TRUE    - The file is a sparse file

--*/

{
    return _IsSparseFile;
}

NTSTATUS
KBlockFileStandard::SetSparseFile(
    __in BOOLEAN IsSparseFile
    )

/*++

Routine Description:

    This routine returns whether or not the file is a sparse file or not

Arguments:

    None.

Return Value:

    FALSE   - The file is not a sparse file

    TRUE    - The file is a sparse file

--*/

{
    NTSTATUS status;

    if (! IsSparseFile)
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, NULL, (ULONGLONG)this, 0);
        return(STATUS_INVALID_PARAMETER);
    }
    
    if (_IsSparseFile)
    {
        return(STATUS_SUCCESS);
    }
    
    status = HLPalFunctions::SetUseFileSystemOnlyFlag(_Handle, IsSparseFile ? TRUE : FALSE);
    if (NT_SUCCESS(status))
    {
        _IsSparseFile = IsSparseFile;
    }
    
    return(status);
}

KBlockFile::SystemIoPriorityHint
KBlockFileStandard::GetSystemIoPriorityHint(
    )

/*++

Routine Description:

    This routine returns the system io priority hint being set

Arguments:

    None.

Return Value:

    System IO priority hint

--*/

{
    return _IoPriorityHint;
}



NTSTATUS
KBlockFileStandard::SetFileSize(
    __in ULONGLONG FileSize,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This routine will trigger an asynchronous set file size action on the file.

Arguments:

    FileSize    - Supplies the file size.

    Completion  - Supplies the completion.

    ParentAsync - Supplies the parent async.

Return Value:

    NTSTATUS

--*/

{
    KBlockFileStandard::SetFileSizeContext* context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        return STATUS_ACCESS_DENIED;
    }
        
    context = _new(KTL_TAG_FILE, GetThisAllocator()) KBlockFileStandard::SetFileSizeContext(*this, FileSize);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFileStandard::SetFileSizeAsync(
    __in ULONGLONG FileSize,
    __in_opt KAsyncContextBase* const ParentAsync)
{
    KBlockFileStandard::SetFileSizeContext::SPtr context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        co_return STATUS_ACCESS_DENIED;
    }   
    
    context = _new(KTL_TAG_FILE, GetThisAllocator()) KBlockFileStandard::SetFileSizeContext(*this, FileSize);

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif

NTSTATUS
KBlockFileStandard::AllocateSetSystemIoPriorityHint(
    __out SetSystemIoPriorityHintContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new QueryAllocations context.

Arguments:

    Async           - Returns the newly allocated SetSystemIoPriorityHint context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    SetSystemIoPriorityHintStandard* setSystemIoPriorityHint;
    NTSTATUS status;

    setSystemIoPriorityHint = _new(AllocationTag, GetThisAllocator()) SetSystemIoPriorityHintStandard(*this);

    if (! setSystemIoPriorityHint) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = setSystemIoPriorityHint->Status();

    if (! NT_SUCCESS(status)) {
        _delete(setSystemIoPriorityHint);
        return status;
    }

    Async = setSystemIoPriorityHint;

    return STATUS_SUCCESS;
}

NTSTATUS
KBlockFileStandard::SetSystemIoPriorityHint(
    __in SystemIoPriorityHint IoPriorityHint,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt SetSystemIoPriorityHintContext* Async
)

/*++

Routine Description:

This routine will trigger an asynchronous set file size action on the file.

Arguments:

    IoPriorityHint    - Supplies the priority hint for IO associated with the KBlockFile

    Completion  - Supplies the completion.

    ParentAsync - Supplies the parent async.

Return Value:

NTSTATUS

--*/

{
    KBlockFileStandard::SetSystemIoPriorityHintStandard* context;
    NTSTATUS status;

    if (Async) {
        context = (SetSystemIoPriorityHintStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) KBlockFileStandard::SetSystemIoPriorityHintStandard(*this);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->_IoPriorityHint = IoPriorityHint;
    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}



NTSTATUS
KBlockFileStandard::AllocateEndOfFileContext(
    __out EndOfFileContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new EndOfFile context used for setting and
    getting the end of file value.

Arguments:

    Async           - Returns the newly allocated QueryAllocations context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    EndOfFileStandard* endOfFile;
    NTSTATUS status;

    endOfFile = _new(AllocationTag, GetThisAllocator()) EndOfFileStandard(*this);

    if (! endOfFile) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = endOfFile->Status();

    if (! NT_SUCCESS(status)) {
        _delete(endOfFile);
        return status;
    }

    Async = endOfFile;

    return STATUS_SUCCESS;
}

NTSTATUS
KBlockFileStandard::SetEndOfFile(
    __in LONGLONG EndOfFile,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt EndOfFileContext* Async
)
/*++

Routine Description:

    This routine will set the end of file metadata.

Arguments:

    EndOfFile   - Supplies the new end of file value

    Completion  - Supplies the completion.

    ParentAsync - Supplies the parent async.

Return Value:

NTSTATUS

--*/

{
    KBlockFileStandard::EndOfFileStandard* context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        return STATUS_ACCESS_DENIED;
    }
        
    if (Async) {
        context = (EndOfFileStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) KBlockFileStandard::EndOfFileStandard(*this);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->_SetEndOfFile = EndOfFile;
    context->_GetEndOfFile = NULL;
    context->_Operation = KBlockFileStandard::EndOfFileStandard::SetEndOfFileOperation;
    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFileStandard::SetEndOfFileAsync(
    __in LONGLONG EndOfFile,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt EndOfFileContext* Async
)
{
    KBlockFileStandard::EndOfFileStandard::SPtr context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        co_return STATUS_ACCESS_DENIED;
    }
        
    if (Async) {
        context = (EndOfFileStandard*)Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    }
    else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) KBlockFileStandard::EndOfFileStandard(*this);
    }

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    context->_SetEndOfFile = EndOfFile;
    context->_GetEndOfFile = NULL;
    context->_Operation = KBlockFileStandard::EndOfFileStandard::SetEndOfFileOperation;

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return{ status };
    }

    status = co_await *awaiter;
    co_return status;
}
#endif

NTSTATUS
KBlockFileStandard::GetEndOfFile(
    __in LONGLONG& EndOfFile,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt EndOfFileContext* Async
)
/*++

Routine Description:

    This routine will get the end of file metadata.

Arguments:

    EndOfFile   - Returns the current end of file value

    Completion  - Supplies the completion.

    ParentAsync - Supplies the parent async.

Return Value:

NTSTATUS

--*/

{
    KBlockFileStandard::EndOfFileStandard::SPtr context;
    NTSTATUS status;

    if (Async) {
        context = (EndOfFileStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) KBlockFileStandard::EndOfFileStandard(*this);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    context->_SetEndOfFile = 0;
    context->_GetEndOfFile = &EndOfFile;
    context->_Operation = KBlockFileStandard::EndOfFileStandard::GetEndOfFileOperation;
    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFileStandard::GetEndOfFileAsync(
    __in LONGLONG& EndOfFile,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt EndOfFileContext* Async
)
/*++

Routine Description:

This routine will get the end of file metadata.

Arguments:

EndOfFile   - Returns the current end of file value

ParentAsync - Supplies the parent async.

Return Value:

NTSTATUS

--*/

{
    KBlockFileStandard::EndOfFileStandard::SPtr context;
    NTSTATUS status;

    if (Async) {
        context = (EndOfFileStandard*)Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    }
    else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) KBlockFileStandard::EndOfFileStandard(*this);
    }

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    context->_SetEndOfFile = 0;
    context->_GetEndOfFile = &EndOfFile;
    context->_Operation = KBlockFileStandard::EndOfFileStandard::GetEndOfFileOperation;

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif

NTSTATUS
KBlockFileStandard::AllocateTransfer(
    __out TransferContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a transfer context.

Arguments:

    Async           - Returns the newly allocated transfer context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    TransferStandard* transfer;
    NTSTATUS status;

    transfer = _new(AllocationTag, GetThisAllocator()) TransferStandard;

    if (!transfer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = transfer->Status();

    if (!NT_SUCCESS(status)) {
        _delete(transfer);
        return status;
    }

    Async = transfer;

    return STATUS_SUCCESS;
}

NTSTATUS
KBlockFileStandard::Transfer(
    __in IoPriority Priority,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new transfer.

Arguments:

    Priority    - Supplies the IO priority.

    Type        - Supplies whether this is a write or a read.

    Offset      - Supplies the byte offset of the IO.

    Buffer      - Supplies the buffer.

    Length      - Supplies the length.

    Completion  - Supplies the completion routine.

    ParentAsync - Supplies, optionally, the parent async to use to synchronize the completion.

    Async       - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    NTSTATUS status;

    status = Transfer(Priority,
                      KBlockFile::SystemIoPriorityHint::eNotDefined,
                      Type,
                      Offset,
                      Buffer,
                      Length,
                      Completion,
                      ParentAsync,
                      Async);
    return(status);
}

NTSTATUS
KBlockFileStandard::Transfer(
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new transfer.

Arguments:

    Priority    - Supplies the IO priority.

    Type        - Supplies whether this is a write or a read.

    Offset      - Supplies the byte offset of the IO.

    Buffer      - Supplies the buffer.

    Length      - Supplies the length.

    Completion  - Supplies the completion routine.

    ParentAsync - Supplies, optionally, the parent async to use to synchronize the completion.

    Async       - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    TransferStandard* context;
    NTSTATUS status;

    if ((_IsReadOnlyFile) &&
        (Type == KBlockFile::TransferType::eWrite))
    {
        return(STATUS_ACCESS_DENIED);
    }
            
    if (Async) {
        context = (TransferStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TransferStandard;
    }

    if (!context) {
        KInvariant(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        
        KInvariant(status != STATUS_INSUFFICIENT_RESOURCES);
        
        return status;
    }

    InterlockedExchangeAdd64(&g_BlockFileNumBytesReceived, Length);

    context->InitializeForTransfer(*this, Priority, IoPriorityHint, Type, Offset, Buffer, Length);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFileStandard::TransferAsync(
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length,
    __in_opt KAsyncContextBase* const ParentAsync = NULL,
    __in_opt TransferContext* Async = NULL
)
{
    NTSTATUS status;
    TransferStandard::SPtr context;

    if (Async) {
        context = (TransferStandard*)Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    }
    else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TransferStandard;
    }

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    InterlockedExchangeAdd64(&g_BlockFileNumBytesReceived, Length);

    context->InitializeForTransfer(*this, Priority, IoPriorityHint, Type, Offset, Buffer, Length);

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return{ status };
    }

    status = co_await *awaiter;
    co_return status;
}
#endif

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFileStandard::TransferAsync(
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsync = NULL,
    __in_opt TransferContext* Async = NULL
)
{
    NTSTATUS status;
    TransferStandard::SPtr context;

    if ((_IsReadOnlyFile) &&
        (Type == KBlockFile::TransferType::eWrite))
    {
        co_return STATUS_ACCESS_DENIED;
    }
            
    if (Async) {
        context = (TransferStandard*)Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    }
    else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TransferStandard;
    }

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    InterlockedExchangeAdd64(&g_BlockFileNumBytesReceived, IoBuffer.QuerySize());

    context->InitializeForTransfer(*this, Priority, IoPriorityHint, Type, Offset, IoBuffer);

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return{ status };
    }

    status = co_await *awaiter;
    co_return status;
}
#endif

NTSTATUS
KBlockFileStandard::Transfer(
    __in IoPriority Priority,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new transfer.

Arguments:

    Priority    - Supplies the IO priority.

    Type        - Supplies whether this is a write or a read.

    Offset      - Supplies the byte offset of the IO.

    IoBuffer    - Supplies the IO buffer.

    Completion  - Supplies the completion routine.

    ParentAsync - Supplies, optionally, the parent async to use to synchronize the completion.

    Async       - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    NTSTATUS status;

    status = Transfer(Priority,
                      KBlockFile::SystemIoPriorityHint::eNotDefined,
                      Type,
                      Offset,
                      IoBuffer,
                      Completion,
                      ParentAsync,
                      Async);

    return(status);
}
NTSTATUS
KBlockFileStandard::Transfer(
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new transfer.

Arguments:

    Priority    - Supplies the IO priority.

    IoPriorityHint - Supplies the system IO priority hint

    Type        - Supplies whether this is a write or a read.

    Offset      - Supplies the byte offset of the IO.

    IoBuffer    - Supplies the IO buffer.

    Completion  - Supplies the completion routine.

    ParentAsync - Supplies, optionally, the parent async to use to synchronize the completion.

    Async       - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    TransferStandard* context;
    NTSTATUS status;

    if ((_IsReadOnlyFile) &&
        (Type == KBlockFile::TransferType::eWrite))
    {
        return(STATUS_ACCESS_DENIED);
    }
                
    if (Async) {
        context = (TransferStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TransferStandard;
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

    InterlockedExchangeAdd64(&g_BlockFileNumBytesReceived, IoBuffer.QuerySize());

    context->InitializeForTransfer(*this, Priority, IoPriorityHint, Type, Offset, IoBuffer);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KBlockFileStandard::Copy(
    __in IoPriority Priority,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new copy operation.

Arguments:

    Priority        - Supplies the IO priority.

    SourceOffset    - Supplies the source byte offset of the copy.

    TargetOffset    - Supplies the target byte offset of the copy.

    Buffer          - Supplies the buffer.

    Length          - Supplies the length, in bytes, of the copy.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to use to synchronize the completion.

    Async           - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    NTSTATUS status;

    status = Copy(Priority,
                  KBlockFile::SystemIoPriorityHint::eNotDefined,
                  SourceOffset,
                  TargetOffset,
                  Buffer,
                  Length,
                  Completion,
                  ParentAsync,
                  Async);

    return(status);
}
NTSTATUS
KBlockFileStandard::Copy(
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new copy operation.

Arguments:

    Priority        - Supplies the IO priority.

    IoPriorityHint  - Supplies the system IO priority hint

    SourceOffset    - Supplies the source byte offset of the copy.

    TargetOffset    - Supplies the target byte offset of the copy.

    Buffer          - Supplies the buffer.

    Length          - Supplies the length, in bytes, of the copy.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to use to synchronize the completion.

    Async           - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    TransferStandard* context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        return(STATUS_ACCESS_DENIED);
    }
                
    if (Async) {
        context = (TransferStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TransferStandard;
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

    InterlockedExchangeAdd64(&g_BlockFileNumBytesReceived, 2*Length);

    context->InitializeForCopy(*this, Priority, IoPriorityHint, SourceOffset, TargetOffset, Buffer, Length);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KBlockFileStandard::Copy(
    __in IoPriority Priority,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new copy operation.

Arguments:

    Priority        - Supplies the IO priority.

    SourceOffset    - Supplies the source byte offset of the copy.

    TargetOffset    - Supplies the target byte offset of the copy.

    IoBuffer        - Supplies the IO buffer.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to use to synchronize the completion.

    Async           - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    NTSTATUS status;

    status = Copy(Priority,
                  KBlockFile::SystemIoPriorityHint::eNotDefined,
                  SourceOffset,
                  TargetOffset,
                  IoBuffer,
                  Completion,
                  ParentAsync,
                  Async);

    return(status);
}

NTSTATUS
KBlockFileStandard::Copy(
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in KIoBuffer& IoBuffer,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TransferContext* Async
    )

/*++

Routine Description:

    This routine will start a new copy operation.

Arguments:

    Priority        - Supplies the IO priority.

    IoPriorityHint  - Supplies the system IO priority hint

    SourceOffset    - Supplies the source byte offset of the copy.

    TargetOffset    - Supplies the target byte offset of the copy.

    IoBuffer        - Supplies the IO buffer.

    Completion      - Supplies the completion routine.

    ParentAsync     - Supplies, optionally, the parent async to use to synchronize the completion.

    Async           - Supplies, optionally, a previously allocated async to use for this transfer request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    TransferStandard* context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        return(STATUS_ACCESS_DENIED);
    }
                
    if (Async) {
        context = (TransferStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TransferStandard;
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

    InterlockedExchangeAdd64(&g_BlockFileNumBytesReceived, 2*IoBuffer.QuerySize());

    context->InitializeForCopy(*this, Priority, IoPriorityHint, SourceOffset, TargetOffset, IoBuffer);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KBlockFileStandard::AllocateFlush(
    __out FlushContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new flush context.

Arguments:

    Async           - Returns the newly allocated flush context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    FlushStandard* flush;
    NTSTATUS status;

    flush = _new(AllocationTag, GetThisAllocator()) FlushStandard(*this);

    if (!flush) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = flush->Status();

    if (!NT_SUCCESS(status)) {
        _delete(flush);
        return status;
    }

    Async = flush;

    return STATUS_SUCCESS;
}


NTSTATUS
KBlockFileStandard::Flush(
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt FlushContext* Async
    )

/*++

Routine Description:

    This routine will start an asynchronous flush operation.

Arguments:

    Completion  - Supplies the completion to be called once the flush is complete.

    ParentAsync - Supplies, optionally, the parent async to use to synchronize the completion.

    Async       - Supplies, optionally, a previously allocated async to use for this flush request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    FlushStandard* context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        return(STATUS_ACCESS_DENIED);
    }
                
    if (Async) {
        context = (FlushStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) FlushStandard(*this);
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

    context->InitializeForFlush(*this);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KBlockFileStandard::FlushAsync(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt FlushContext* Async
    )
{
    FlushStandard* context;
    NTSTATUS status;

    if (_IsReadOnlyFile)
    {
        co_return STATUS_ACCESS_DENIED;
    }
                
    if (Async) {
        context = (FlushStandard*)Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    }
    else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) FlushStandard(*this);
    }

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        if (context != Async) {
            _delete(context);
        }
        co_return status;
    }

    context->InitializeForFlush(*this);

    ktl::kasync::StartAwaiter::SPtr awaiter;
    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif


NTSTATUS
KBlockFileStandard::AllocateTrim(
    __out TrimContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new Trim context.

Arguments:

    Async           - Returns the newly allocated Trim context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    if (! _IsSparseFile)
    {
        return STATUS_NOT_SUPPORTED;
    }

    TrimStandard* trim;
    NTSTATUS status;

    trim = _new(AllocationTag, GetThisAllocator()) TrimStandard(*this);

    if (!trim) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = trim->Status();

    if (!NT_SUCCESS(status)) {
        _delete(trim);
        return status;
    }

    Async = trim;

    return STATUS_SUCCESS;
}

NTSTATUS
KBlockFileStandard::Trim(
    __in ULONGLONG TrimFrom,
    __in ULONGLONG TrimToPlusOne,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TrimContext* Async
    )

/*++

Routine Description:

    This routine will start an asynchronous Trim operation.

Arguments:

    Completion  - Supplies the completion to be called once the Trim is complete.

    ParentAsync - Supplies, optionally, the parent async to use to synchronize the completion.

    Async       - Supplies, optionally, a previously allocated async to use for this Trim request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    if (! _IsSparseFile)
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (_IsReadOnlyFile)
    {
        return(STATUS_ACCESS_DENIED);
    }
                
    TrimStandard::SPtr context;
    NTSTATUS status;

    if (Async) {
        context = (TrimStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TrimStandard(*this);
    }

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    context->InitializeForTrim(*this, TrimFrom, TrimToPlusOne);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFileStandard::TrimAsync(
    __in ULONGLONG TrimFrom,
    __in ULONGLONG TrimToPlusOne,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt TrimContext* Async
    )
{
    if (!_IsSparseFile)
    {
        co_return STATUS_NOT_SUPPORTED;
    }

    if (_IsReadOnlyFile)
    {
        co_return STATUS_ACCESS_DENIED;
    }
                
    TrimStandard::SPtr context;
    NTSTATUS status;

    if (Async) {
        context = (TrimStandard*)Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    }
    else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) TrimStandard(*this);
    }

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    context->InitializeForTrim(*this, TrimFrom, TrimToPlusOne);

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}

#endif

NTSTATUS
KBlockFileStandard::AllocateQueryAllocations(
    __out QueryAllocationsContext::SPtr& Async,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine allocates a new QueryAllocations context.

Arguments:

    Async           - Returns the newly allocated QueryAllocations context.

    AllocationTag   - Supplies the allocation tag.

Return Value:

    NTSTATUS

--*/

{
    if (! _IsSparseFile)
    {
        return STATUS_NOT_SUPPORTED;
    }

    QueryAllocationsStandard* queryAllocations;
    NTSTATUS status;

    queryAllocations = _new(AllocationTag, GetThisAllocator()) QueryAllocationsStandard(*this);

    if (!queryAllocations) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = queryAllocations->Status();

    if (!NT_SUCCESS(status)) {
        _delete(queryAllocations);
        return status;
    }

    Async = queryAllocations;

    return STATUS_SUCCESS;
}

NTSTATUS
KBlockFileStandard::QueryAllocations(
    __in ULONGLONG QueryFrom,
    __in ULONGLONG Length,
    __inout KArray<AllocatedRange>& Results,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt QueryAllocationsContext* Async
    )

/*++

Routine Description:

    This routine will start an asynchronous QueryAllocations operation.

Arguments:

    Completion  - Supplies the completion to be called once the QueryAllocations is complete.

    ParentAsync - Supplies, optionally, the parent async to use to synchronize the completion.

    Async       - Supplies, optionally, a previously allocated async to use for this QueryAllocations request.

Return Value:

    STATUS_PENDING                  - The given completion routine will be called when the request is complete.

    STATUS_INSUFFICIENT_RESOURCES   - Insufficient memory.

--*/

{
    if (! _IsSparseFile)
    {
        return STATUS_NOT_SUPPORTED;
    }

    QueryAllocationsStandard* context;
    NTSTATUS status;

    if (Async) {
        context = (QueryAllocationsStandard*) Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    } else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) QueryAllocationsStandard(*this);
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

    context->InitializeForQueryAllocations(*this, QueryFrom, Length, Results);

    context->Start(ParentAsync, Completion);

    return STATUS_PENDING;
}

#if defined(K_UseResumable)
Awaitable<NTSTATUS>
KBlockFileStandard::QueryAllocationsAsync(
    __in ULONGLONG QueryFrom,
    __in ULONGLONG Length,
    __inout KArray<AllocatedRange>& Results,
    __in_opt KAsyncContextBase* const ParentAsync = NULL,
    __in_opt QueryAllocationsContext* Async = NULL
    )
{
    if (!_IsSparseFile)
    {
        co_return STATUS_NOT_SUPPORTED;
    }

    QueryAllocationsStandard::SPtr context;
    NTSTATUS status;

    if (Async) {
        context = (QueryAllocationsStandard*)Async;
        if (context->Status() != K_STATUS_NOT_STARTED) {
            context->Reuse();
        }
    }
    else {
        context = _new(KTL_TAG_FILE, GetThisAllocator()) QueryAllocationsStandard(*this);
    }

    if (!context) {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        co_return status;
    }

    context->InitializeForQueryAllocations(*this, QueryFrom, Length, Results);

    ktl::kasync::StartAwaiter::SPtr awaiter;

    status = ktl::kasync::StartAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *context,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif


VOID
KBlockFileStandard::CancelAll(
    )

/*++

Routine Description:

    This routine will cancel all operations.

Arguments:

    None.

Return Value:

    None.

--*/

{
    TransferStandard* transfer;

    for (;;) {

        _SpinLock.Acquire();
        transfer = _Foreground.Queue.RemoveHead();
        if (!transfer) {
            transfer = _Background.Queue.RemoveHead();
        }
        _SpinLock.Release();

        if (!transfer) {

            //
            // Nothing left to cancel.
            //

            break;
        }

        KInvariant(transfer->_InQueue);
        transfer->_InQueue = FALSE;
        KInvariant(!transfer->_RefCount);

        //
        // Complete the transfer packet as canceled.
        //

        transfer->Complete(STATUS_CANCELLED);
    }
}

VOID
KBlockFileStandard::Close(
    )
{
    HANDLE h;

    h = InterlockedExchangePointer(&_Handle, NULL);

    if (h) {
        KNt::Close(h);
    }
}

VOID
KBlockFileStandard::SetBackgroundQueueLength(
    __in ULONG BackgroundQueueLength
    )
{
    _MaxBackground = BackgroundQueueLength;
}

VOID
KBlockFileStandard::Zero(
    )
{
    _Handle = NULL;
    _IsWriteThrough = FALSE;
    _FileSize = 0;
    _PageSize = 0;
    _NumForegroundIoPacketsOutstanding = 0;
    _NumBackgroundIoPacketsOutstanding = 0;
    _NumOSIoPacketsOutstanding = 0;
    _MaxBackground = 2;
#if KTL_USER_MODE
    _RegistrationContext = NULL;
#else
    _VolumeObject = NULL;
    _FileObject = NULL;
#endif
}

VOID
KBlockFileStandard::Cleanup(
    )

/*++

Routine Description:

    Cleanup routine for a KBlockFile object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IoPacket* ioPacket;

    if (_Handle) {


#if KTL_USER_MODE
#else
        //
        // Verify that this is not being called at raised IRQL
        //
        KInvariant(KeGetCurrentIrql() <= APC_LEVEL);
#endif

        KNt::Close(_Handle);
    }

    while (_IoPacketList.Count()) {
        ioPacket = _IoPacketList.RemoveHead();
        if (IsScatterGatherAvailable())
        {
#if KTL_USER_MODE           
#else
            SGIoPacket* sgPacket = (SGIoPacket*)ioPacket;

            IoFreeMdl(sgPacket->PartialMdl);
            IoFreeMdl(sgPacket->Mdl);
            IoFreeIrp(sgPacket->Irp);
#endif
        } else {
#if KTL_USER_MODE
#else
            NoSGIoPacket* nosgPacket = (NoSGIoPacket*)ioPacket;
            nosgPacket->NoSGOperation = nullptr;
#endif
        }
        _delete(ioPacket);
    }

#if KTL_USER_MODE
    if (_RegistrationContext) {
        GetThisKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(_RegistrationContext);
    }
#else
    if (_VolumeObject) {
        ObDereferenceObject(_VolumeObject);
    }

    if (_FileObject) {
        ObDereferenceObject(_FileObject);
    }

    _ExtentTable.RemoveAll();
#endif
}

NTSTATUS
KBlockFileStandard::Initialize(
    __in KWString& FileName,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN CreateSparseFile,
    __in ULONG AllocationTag
    )

/*++

Routine Description:

    This routine initializes a KBlockFile object.

Arguments:

    FileName        - Supplies the file name.

    IsWriteThrough  - Supplies whether or not the file is to be opened as write through.

Return Value:

    None.

--*/

{
    ACL* acl = NULL;
    SECURITY_DESCRIPTOR* sd;
    NTSTATUS status;
    ULONG createDisposition;
    ULONG createOptionsFlags;
    ULONG shareAccessFlags;
    ULONG desiredAccess;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK ioStatus;
    FILE_STANDARD_INFORMATION standardInformation;
    ULONG i;
    GET_LENGTH_INFORMATION lengthInfo;
    ULONG createFlags = 0;
    KStringView fileName((PWCHAR)FileName);


    if ((_QueueDepth == 0) || (_QueueDepth > KBlockFile::DefaultQueueDepth))
    {
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        return status;
    }

    status = KInstrumentedComponent::Create(_InstrumentedComponent, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        return status;
    }
        
    //
    // Remember the file name.
    //

    _FileName = FileName;

    status = _FileName.Status();

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        return status;
    }

    //
    // IoPriorityHint defaults to not defined
    //
    _IoPriorityHint = eNotDefined;

    //
    // We are in a system worker thread.  Blocking operations can be
    // done here.
    //

    //
    // Translate the create disposition.
    //

     switch (Disposition) {

        case eCreateNew:
            createDisposition = FILE_CREATE;
            break;

        case eCreateAlways:
            createDisposition = FILE_OVERWRITE_IF;
            break;

        case eOpenExisting:
            createDisposition = FILE_OPEN;
            break;

        case eOpenAlways:
            createDisposition = FILE_OPEN_IF;
            break;

        case eTruncateExisting:
            createDisposition = FILE_OVERWRITE;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            goto Finish;

    }

    //
    // Figure out the create options;
    //

    createOptionsFlags = FILE_NON_DIRECTORY_FILE;

    if (! CreateSparseFile)
    {
        createOptionsFlags |= FILE_NO_COMPRESSION;
        createOptionsFlags |= FILE_NO_INTERMEDIATE_BUFFERING;
    } else {
#if KTL_USER_MODE
        UNREFERENCED_PARAMETER(Options);
        //
        // User mode uses ReadFileScatter and WriteFileGather apis
        // which require the file be opened FILE_NO_INTERMEDIATE_BUFFERING
        //
        createOptionsFlags |= FILE_NO_INTERMEDIATE_BUFFERING;
#else
        if (Options & eNoIntermediateBuffering)
        {
            createOptionsFlags |= FILE_NO_INTERMEDIATE_BUFFERING;
        }

        if (Options & eSequentialAccess)
        {
            createOptionsFlags |= FILE_SEQUENTIAL_ONLY;
        }

        if (Options & eRandomAccess)
        {
            createOptionsFlags |= FILE_RANDOM_ACCESS;
        }
#endif
    }

    if (IsWriteThrough) {
        createOptionsFlags |= FILE_WRITE_THROUGH;
    }

    //
    // Figure out the share options
    //

    shareAccessFlags = 0;

    if (Options & eShareRead)
    {
        shareAccessFlags |= FILE_SHARE_READ;
    }
    if (Options & eShareWrite)
    {
        shareAccessFlags |= FILE_SHARE_WRITE;
    }
    if (Options & eShareDelete)
    {
        shareAccessFlags |= FILE_SHARE_DELETE;
    }

    //
    // Desired access.
    //
    if (Options & eReadOnly)
    {
        desiredAccess = STANDARD_RIGHTS_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES;
        _IsReadOnlyFile = TRUE;
    } else {
        desiredAccess = STANDARD_RIGHTS_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES | STANDARD_RIGHTS_WRITE | FILE_WRITE_DATA |
                        FILE_WRITE_ATTRIBUTES;
        _IsReadOnlyFile = FALSE;
    }

    //
    // Security descriptor
    //
#if KTL_USER_MODE
    //
    // For user mode on windows we need to use the default security
    // descriptor
    //
    sd = NULL;
#else
    //
    // For kernel mode we always use the restrictive security
    // descriptor. For Linux we ignore any security descriptors
    //
    SECURITY_DESCRIPTOR securityDescriptor;

    //
    // Create the restrictive security descriptor for the case where we end up creating the file.
    //
    status = CreateSecurityDescriptor(securityDescriptor, acl, GetThisAllocator());

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }
    
    sd = &securityDescriptor;
#endif
    
    //
    // Create/Open the file.
    //

#if KTL_USER_MODE
    if (! CreateSparseFile)
    {
        BOOL b = ManageVolumePrivilege();

        if (!b) {

            //
            // The needed privilege for setting the valid data length
            // could not be acquired.  Warn but do not fail.
            //
            status = STATUS_PRIVILEGE_NOT_HELD;
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
            status = STATUS_SUCCESS;
        }
    }
#endif

    InitializeObjectAttributes(&oa, FileName, OBJ_CASE_INSENSITIVE, NULL, sd);

    if ((CreateSparseFile) || (Options & eAlwaysUseFileSystem))
    {
        createFlags |= NTCF_FLAG_USE_FILE_SYSTEM_ONLY;
    }

    if (Options & eAvoidUseFileSystem)
    {
        createFlags |= NTCF_FLAG_AVOID_FILE_SYSTEM;
    }
    
    status = HLPalFunctions::NtCreateFileForKBlockFile(&_Handle,
                                                       desiredAccess,
                                                       &oa,
                                                       &ioStatus,
                                                       NULL,
                                                       FILE_ATTRIBUTE_NORMAL,
                                                       shareAccessFlags,
                                                       createDisposition,
                                                       createOptionsFlags,
                                                       NULL,
                                                       0,
                                                       createFlags);

    if (!NT_SUCCESS(status)) {
        _Handle = NULL;
        goto Finish;
    }

    _IsWriteThrough = IsWriteThrough;

    if (CreateSparseFile)
    {
        //
        // Ensure file is marked as a sparse file
        //
        HANDLE eventHandle;
        status = KNt::CreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

        if (!NT_SUCCESS(status)) {
            eventHandle = NULL;
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
            goto Finish;
        }

        FILE_SET_SPARSE_BUFFER fileSetSparse;
        fileSetSparse.SetSparse = TRUE;
        status = KNt::FsControlFile(_Handle, eventHandle, NULL, NULL, &ioStatus,
            FSCTL_SET_SPARSE,
            &fileSetSparse, sizeof(fileSetSparse),
            NULL, 0);

        if (status == STATUS_PENDING) {
            KNt::WaitForSingleObject(eventHandle, FALSE, NULL);
            status = ioStatus.Status;
        }

        KNt::Close(eventHandle);

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
            goto Finish;
        }
    }

    //
    // Determine if the file is sparse or not. Since there is no
    // equivalent way to do this on Linux, we need to rely upon the
    // flags being passed in and the upper layer tracking the file type.
    //
#if !defined(PLATFORM_UNIX)
    FILE_BASIC_INFORMATION basicInformation;

    status = KNt::QueryInformationFile(_Handle, &ioStatus, &basicInformation, sizeof(basicInformation),
        FileBasicInformation);

    if (! NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    if ((basicInformation.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) == FILE_ATTRIBUTE_SPARSE_FILE)
    {
        _IsSparseFile =  TRUE;
    } else {
        _IsSparseFile =  FALSE;
    }
#else
    _IsSparseFile = CreateSparseFile;
#endif

    //
    // Figure out the page size.
    //
#if KTL_USER_MODE
    SYSTEM_INFO systemInfo;

    GetSystemInfo(&systemInfo);
    _PageSize = systemInfo.dwPageSize;
#else
    _PageSize = PAGE_SIZE;

#endif
    
    //
    // Query the file size. 
    //
    status = KNt::QueryInformationFile(_Handle, &ioStatus, &standardInformation, sizeof(standardInformation),
            FileStandardInformation);

    if (NT_SUCCESS(status)) {

        //
        // EOF may not be on a block boundary so round it up
        //
        _FileSize = (standardInformation.EndOfFile.QuadPart + (LONGLONG)(_PageSize -1)) & (~((LONGLONG)(_PageSize-1)));
        
        if ((! _IsSparseFile) && (! _IsReadOnlyFile))
        {
            //
            // Set the valid data length to be equal to the file size.
            //
            status = HLPalFunctions::SetValidDataLength(_Handle,
                                                        standardInformation.EndOfFile.QuadPart,
                                                        standardInformation.EndOfFile.QuadPart,
                                                        GetThisAllocator());

            

#if KTL_USER_MODE
            if (status == STATUS_PRIVILEGE_NOT_HELD || status == STATUS_NOT_SUPPORTED)
            {
                //
                // Allow underprivileged processes to move forward
                // without preallocation
                //
                status = STATUS_SUCCESS;
            }
#endif
            if (!NT_SUCCESS(status)) {
                goto Finish;
            }
        }
    } else {

        //
        // This is not a file, but a block device.  Continue by getting the size of the "file" via IOCTLs.
        // And then, there's no need to set the valid data length either.
        //

        status = KNt::DeviceIoControlFile(_Handle, NULL, NULL, NULL, &ioStatus, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &lengthInfo,
                sizeof(lengthInfo));

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
            goto Finish;
        }

        _FileSize = lengthInfo.Length.QuadPart;

    }

#if KTL_USER_MODE
    status = GetThisKtlSystem().DefaultThreadPool().RegisterIoCompletionCallback(
        _Handle,
        IoCompletionRoutineUserMode,
        this,
        &_RegistrationContext);

    if (!NT_SUCCESS(status)) {
        _RegistrationContext = NULL;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    //
    // Get the extent table.
    //

    status = HLPalFunctions::AdjustExtentTable(_Handle, 0, _FileSize, GetThisAllocator());

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

#else

    //
    // For kernel mode, initialize the extent table.
    //

    status = InitializeKernelMode(AllocationTag);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

#endif

    //
    // Allocate up to QueueDepth IoPackets
    //
    for (i = 0; i < _QueueDepth; i++) {
        IoPacket* ioPacket = NULL;

        if (IsScatterGatherAvailable())
        {
            SGIoPacket* sgPacket = _new(AllocationTag, GetThisAllocator()) SGIoPacket;

            if (! sgPacket) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, _QueueDepth);
                goto Finish;
            }

            RtlZeroMemory(sgPacket, sizeof(SGIoPacket));

            ioPacket = (IoPacket*)sgPacket;

            KFinally([&]() {
                if (! NT_SUCCESS(status)) {

#if KTL_USER_MODE
#else
                    if (sgPacket->Irp != NULL)
                    {
                        IoFreeIrp(sgPacket->Irp);
                        sgPacket->Irp = NULL;
                    }

                    if (sgPacket->Mdl != NULL)
                    {
                        IoFreeMdl(sgPacket->Mdl);
                        sgPacket->Mdl = NULL;
                    }

                    if (sgPacket->PartialMdl != NULL)
                    {
                        IoFreeMdl(sgPacket->PartialMdl);
                        sgPacket->PartialMdl = NULL;
                    }
#endif

                    _delete(sgPacket);
                };
            });

#if KTL_USER_MODE
#else
            sgPacket->Irp = IoAllocateIrp(_VolumeObject->StackSize, FALSE);

            if (! sgPacket->Irp) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, _QueueDepth);
                goto Finish;
            }

            sgPacket->Mdl = IoAllocateMdl(0, MaxIoSize, FALSE, FALSE, NULL);

            if (!sgPacket->Mdl) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, _QueueDepth);
                goto Finish;
            }

            sgPacket->PartialMdl = IoAllocateMdl(0, MaxIoSize, FALSE, FALSE, NULL);

            if (!sgPacket->PartialMdl) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, _QueueDepth);
                goto Finish;
            }
        } else {
            NoSGIoPacket* nosgPacket = _new(AllocationTag, GetThisAllocator()) NoSGIoPacket;

            if (! nosgPacket) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, _QueueDepth);
                goto Finish;
            }

            RtlZeroMemory(nosgPacket, sizeof(NoSGIoPacket));

            ioPacket = (IoPacket*)nosgPacket;

            KFinally([&]() {
                if (! NT_SUCCESS(status)) {

                    nosgPacket->NoSGOperation = nullptr;
                    _delete(nosgPacket);
                };
            });

            nosgPacket->NoSGOperation = _new(AllocationTag, GetThisAllocator()) NoSGFileIoOperation(nosgPacket);
            if (! nosgPacket->NoSGOperation)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, _QueueDepth);
                goto Finish;
            }

            status = nosgPacket->NoSGOperation->Status();
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, _QueueDepth);
                goto Finish;
            }

#endif
        }

        ioPacket->File = this;

        _IoPacketList.AppendTail(ioPacket);

        status = STATUS_SUCCESS;
    }

    //
    // File is all setup, enable instrumentation
    //
    _InstrumentedComponent->SetComponentName(fileName);
    _InstrumentedComponent->SetReportFrequency(DefaultReportFrequency);
    
Finish:


    if (acl != NULL)
    {
        _delete(acl);
    }

    return status;
}

NTSTATUS
KBlockFileStandard::CreateSecurityDescriptor(
    __out SECURITY_DESCRIPTOR& SecurityDescriptor,
    __out ACL*& Acl,
    __in KAllocator& Allocator
    )

/*++

Routine Description:

    This routine creates a strong security descriptor.

Arguments:

    SecurityDescriptor  - Returns the security descriptor.

    Acl                 - Return the ACL.

Return Value:

    NTSTATUS

--*/

{
    return HLPalFunctions::CreateSecurityDescriptor(SecurityDescriptor, Acl, Allocator);
}

VOID
KBlockFileStandard::DispatchIoPackets(
    )

/*++

Routine Description:

    This routine sends down as many IO packets as are free and available to send down.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IoPacket* ioPacket;
    BOOLEAN b;

    //
    // Engage as many IO packets as possible for the remaining IO.
    //

    while (_IoPacketList.Count()) {

        ioPacket = _IoPacketList.RemoveHead();

        b = FillIoPacket(ioPacket);

        if (!b) {

            //
            // No work remains.  Return the io packet to the pool.  We are done.
            //

            _IoPacketList.AppendTail(ioPacket);

            break;
        }

        //
        // Send down IO.
        //

        _SpinLock.Release();

        SendIoPacket(ioPacket);

        _SpinLock.Acquire();
    }
}

BOOLEAN
KBlockFileStandard::FillIoPacket(
    __inout IoPacket* Packet
    )

/*++

Routine Description:

    This routine will examine the current queue of requests and determine if there is anything to place in this IO packet.
    If the IO packet is filled then this routine return TRUE, other FALSE.

    In some cases this routine will return FALSE even if there are some requests outstanding.  This can happen for priority
    reasons.  If FALSE is returned, for any reason, this routine is re-called after every io completion to re-evaluate.

Arguments:

    Packet  - Supplies the IO packet to fill.

Return Value:

    FALSE   - The IO packet was not used.

    TRUE    - The IO packet was filled as much as possible with requests at the front of the queue.

--*/

{
    SGIoPacket* sgPacket = (SGIoPacket*)Packet;
#if KTL_USER_MODE
#else
    NoSGIoPacket* nosgPacket = (NoSGIoPacket*)Packet;
#endif

    //
    // Choose foreground queue unless it is empty.  If the foreground queue is empty and there are still outstanding
    // foreground IO requests, then do nothing.
    //

    RequestQueue* queue = &_Foreground;
    Packet->Priority = eForeground;

    if (queue->IsEmpty() && !_NumForegroundIoPacketsOutstanding) {

        //
        // At this point the foreground queue is empty, meaning there are no foreground requests waiting to be sent to the disk.
        // In addition, because '_NumForegroundIoPacketsOutstanding' is 0, there are no foreground requests pending to the
        // disk either.  In this case we can consider sending background requests.
        //

        if (_NumBackgroundIoPacketsOutstanding >= _MaxBackground) {

            //
            // The maximum number of background io requests has already been achieved, so we don't consider any more to
            // send down until then next IO completion.
            //

            return FALSE;
        }
        queue = &_Background;
        Packet->Priority = eBackground;
    }

    Packet->Type = eRead;
    Packet->IsCopy = FALSE;
    Packet->Offset = 0;
    Packet->TargetOffset = 0;
    Packet->Length = 0;
    Packet->RemainingLength = 0;
    Packet->Buffer = NULL;

    if (IsScatterGatherAvailable())
    {
#if KTL_USER_MODE
        sgPacket->NumAlignedBuffers = 0;
#else
        sgPacket->NumFileSegmentElements = 0;
#endif
        sgPacket->NumTransferPackets = 0;
    } else {
#if KTL_USER_MODE
#else
        nosgPacket->TransferPacket = NULL;
#endif
    }

    if (queue->CurrentRequest) {

        //
        // Try to add what remains of the current request.
        //

        if (queue->CurrentRequest->_Buffer) {

            ULONGLONG fileOffset = queue->CurrentRequest->_Offset + queue->CurrentRequestOffset;
            ULONGLONG targetOffset = queue->CurrentRequest->_TargetOffset + queue->CurrentRequestOffset;
            VOID* buffer = (VOID*) ((UCHAR*) queue->CurrentRequest->_Buffer + queue->CurrentRequestOffset);
            ULONG length = queue->CurrentRequest->_Length - queue->CurrentRequestOffset;
            ULONG lengthNotAdded;

            AddToIoPacket(Packet, queue->CurrentRequest->_TransferType, queue->CurrentRequest->_IsCopy, fileOffset, targetOffset,
                          buffer, length, queue->CurrentRequest->_IoPriorityHint, lengthNotAdded);

            if (IsScatterGatherAvailable())
            {
                sgPacket->TransferPackets[sgPacket->NumTransferPackets++] = queue->CurrentRequest;
            } else {
#if KTL_USER_MODE
#else
                nosgPacket->TransferPacket = queue->CurrentRequest;
#endif
            }

            if (lengthNotAdded) {

                //
                // We are leaving this request as the 'current' so we need to add a ref count for the io packet.
                //

                InterlockedIncrement(&queue->CurrentRequest->_RefCount);
                queue->CurrentRequestOffset += Packet->Length;

                KInvariant(queue->CurrentRequest->_Length > queue->CurrentRequestOffset);

                goto Finish;
            }

            //
            // Here we take the ref that was there to keep the request as 'current' and use it for the io packet.
            //

            queue->CurrentRequest = NULL;
            queue->CurrentRequestOffset = 0;

        } else {

            KIoBuffer* ioBuffer = queue->CurrentRequest->_IoBuffer.RawPtr();
            KIoBufferElement* element;
            ULONG offset = 0;

            for (element = ioBuffer->First(); element; element = ioBuffer->Next(*element)) {

                ULONG next = offset + element->QuerySize();

                if (next > queue->CurrentRequestOffset) {

                    //
                    // We have found the first element that has not yet been transfered.
                    //

                    ULONGLONG fileOffset = queue->CurrentRequest->_Offset + queue->CurrentRequestOffset;
                    ULONGLONG targetOffset = queue->CurrentRequest->_TargetOffset + queue->CurrentRequestOffset;
                    VOID* buffer = (VOID*) ((UCHAR*) element->GetBuffer() + (queue->CurrentRequestOffset - offset));
                    ULONG length = next - queue->CurrentRequestOffset;
                    ULONG lengthNotAdded;

                    AddToIoPacket(Packet, queue->CurrentRequest->_TransferType, queue->CurrentRequest->_IsCopy, fileOffset,
                                  targetOffset, buffer, length, queue->CurrentRequest->_IoPriorityHint, lengthNotAdded);

                    //
                    // Remember where we are on this current transfer packet.
                    //

                    queue->CurrentRequestOffset += (length - lengthNotAdded);

                    if (lengthNotAdded) {

                        KInvariant(queue->CurrentRequest->_Length > queue->CurrentRequestOffset);

                        //
                        // We cannot add any more to this io packet.  The transfer packet remains as 'current'.
                        // Add a new ref for the io packet.
                        //

                        if (IsScatterGatherAvailable())
                        {
                            sgPacket->TransferPackets[sgPacket->NumTransferPackets++] = queue->CurrentRequest;
                        } else {
#if KTL_USER_MODE
#else
                            nosgPacket->TransferPacket = queue->CurrentRequest;
#endif
                        }

                        InterlockedIncrement(&queue->CurrentRequest->_RefCount);
                        goto Finish;
                    }
                }

                offset = next;
            }

            //
            // Add this transfer packet to the IO packet.  The ref is just borrowed from the 'current' ref.
            //

            if (IsScatterGatherAvailable())
            {
                sgPacket->TransferPackets[sgPacket->NumTransferPackets++] = queue->CurrentRequest;
            } else {
#if KTL_USER_MODE
#else
                nosgPacket->TransferPacket = queue->CurrentRequest;
#endif
            }

            queue->CurrentRequest = NULL;
            queue->CurrentRequestOffset = 0;
        }
    }

    //
    // Now that the current request is exhausted follow up with the next one at the head of the queue.
    //

    while (queue->Queue.Count() && Packet->Length < MaxIoSize) {

        TransferStandard* transfer = queue->Queue.PeekHead();

        if (transfer->_Buffer) {

            ULONG lengthNotAdded;

            AddToIoPacket(Packet, transfer->_TransferType, transfer->_IsCopy, transfer->_Offset, transfer->_TargetOffset,
                    transfer->_Buffer, transfer->_Length, transfer->_IoPriorityHint, lengthNotAdded);

            if (lengthNotAdded == transfer->_Length) {

                //
                // Nothing from this transfer packet could be added to this IO packet.  We are done filling this IO packet.
                //

                goto Finish;
            }

            //
            // We are doing something with this transfer packet so pull it off the queue.
            //

            queue->Queue.RemoveHead();

            KInvariant(transfer->_InQueue);
            transfer->_InQueue = FALSE;

            if (lengthNotAdded) {

                //
                // Not all of the transfer packet was used up.  Set up a remembrance of what remains.  And then we are also
                // done with filling this IO packet.
                //

                queue->CurrentRequest = transfer;
                queue->CurrentRequestOffset = transfer->_Length - lengthNotAdded;

                KInvariant(queue->CurrentRequest->_Length > queue->CurrentRequestOffset);

                //
                // We take 2 references to this transfer packet.  The first one if for keeping it as 'current'.  The
                // second is because the of the IO packet that will refer to it.
                //

                if (IsScatterGatherAvailable())
                {
                    sgPacket->TransferPackets[sgPacket->NumTransferPackets++] = queue->CurrentRequest;
                } else {
#if KTL_USER_MODE
#else
                    nosgPacket->TransferPacket = queue->CurrentRequest;
#endif
                }

                KInvariant(!queue->CurrentRequest->_RefCount);
                queue->CurrentRequest->_RefCount = 2;

                goto Finish;
            }

        } else {

            //
            // Iterate through the pieces of this IO buffer.
            //

            KIoBuffer* ioBuffer = transfer->_IoBuffer.RawPtr();
            KIoBufferElement* element;
            ULONG offset = 0;
            BOOLEAN removed = FALSE;

            for (element = ioBuffer->First(); element; element = ioBuffer->Next(*element)) {

                ULONGLONG fileOffset = transfer->_Offset + offset;
                ULONGLONG targetOffset = transfer->_TargetOffset + offset;
                ULONG elementSize = element->QuerySize();
                ULONG lengthNotAdded;

                AddToIoPacket(Packet, transfer->_TransferType, transfer->_IsCopy, fileOffset, targetOffset, element->GetBuffer(),
                        elementSize, transfer->_IoPriorityHint, lengthNotAdded);

                if (lengthNotAdded == elementSize) {

                    //
                    // Nothing was added from this element.  We are done.  If we did anything with this transfer packet
                    // then remove it from the queue and remember where we were.
                    //

                    if (removed) {
                        if (IsScatterGatherAvailable())
                        {
                            sgPacket->TransferPackets[sgPacket->NumTransferPackets++] = transfer;
                        } else {
#if KTL_USER_MODE
#else
                            nosgPacket->TransferPacket = transfer;
#endif
                        }
                        KInvariant(!transfer->_RefCount);
                        transfer->_RefCount = 2;
                        queue->CurrentRequest = transfer;
                        queue->CurrentRequestOffset = offset;

                        KInvariant(queue->CurrentRequest->_Length > queue->CurrentRequestOffset);
                    }

                    goto Finish;
                }

                //
                // We took something from this transfer packet.  Has it been removed from the queue yet?
                //

                if (!removed) {
                    removed = TRUE;
                    queue->Queue.RemoveHead();
                    KInvariant(transfer->_InQueue);
                    transfer->_InQueue = FALSE;
                }

                if (lengthNotAdded) {

                    //
                    // We didn't completely consume this element, so we are done.  Save where we are for the next packet.
                    //

                    queue->CurrentRequest = transfer;
                    queue->CurrentRequestOffset = offset + elementSize - lengthNotAdded;

                    KInvariant(queue->CurrentRequest->_Length > queue->CurrentRequestOffset);

                    //
                    // Set the ref count to 2, for 'current' and for the io packet.
                    //

                    if (IsScatterGatherAvailable())
                    {
                        sgPacket->TransferPackets[sgPacket->NumTransferPackets++] = transfer;
                    } else {
#if KTL_USER_MODE
#else
                        nosgPacket->TransferPacket = transfer;
#endif
                    }

                    KInvariant(!transfer->_RefCount);
                    transfer->_RefCount = 2;

                    goto Finish;
                }

                offset += elementSize;
            }
        }

        //
        // The entire transfer packet was added to the IO packet.
        //
        if (IsScatterGatherAvailable())
        {
            sgPacket->TransferPackets[sgPacket->NumTransferPackets++] = transfer;
        } else {
#if KTL_USER_MODE
#else
            nosgPacket->TransferPacket = transfer;
#endif
        }

        KInvariant(!transfer->_RefCount);
        transfer->_RefCount = 1;
    }

Finish:

    if (! Packet->Length) {
        return FALSE;
    }

    if (Packet->Priority == eForeground) {
        _NumForegroundIoPacketsOutstanding++;
    } else {
        _NumBackgroundIoPacketsOutstanding++;
    }

    return TRUE;
}

VOID
KBlockFileStandard::AddToIoPacket(
    __inout IoPacket* Packet,
    __in TransferType Type,
    __in BOOLEAN IsCopy,
    __in ULONGLONG Offset,
    __in ULONGLONG TargetOffset,
    __in const VOID* Buffer,
    __in ULONG Length,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __out ULONG& LengthNotAdded
    )

/*++

Routine Description:

    This routine adds the given buffer and length to this IO packet and returns the length that was not added.

Arguments:

    Packet          - Supplies the IO packet.

    Type            - Supplies whether this is a read or a write.

    IsCopy          - Supplies whether or not this is a copy.  A copy is a read and then a write.

    Offset          - Supplies the offset for the read or write.

    TargetOffset    - Supplies the target offset of the copy if 'IsCopy' is set to true.

    Buffer          - Supplies the buffer.

    Length          - Supplies the length.

    IoPriorityHint  - Supplies the Io priority hint to use for the request

    LengthNotAdded  - Returns the length not added.

Return Value:

    None.

--*/

{
    LengthNotAdded = Length;

    if (Packet->Buffer) {

        //
        // This packet is already defined in a form that it cannot be added to.
        //

        return;
    }

    //
    // Determine if the new stuff is aligned ok.
    //

    ULONG_PTR alignment = ((ULONG_PTR) Buffer)%_PageSize;

    if (alignment || Length%_PageSize || NoScatterGather || (! IsScatterGatherAvailable())) {

        //
        // This transfer is not aligned, or scatter gather is not allowed.  Only add it if the current packet is empty.
        //

        if (Packet->Length) {
            return;
        }

        Packet->Type = Type;
        Packet->IsCopy = IsCopy;
        Packet->Offset = Offset;
        Packet->TargetOffset = TargetOffset;
        Packet->Length = Length;
        if (Length > MaxIoSize) {
            Packet->Length = MaxIoSize;
        }
        Packet->Buffer = (VOID*) Buffer;

        LengthNotAdded = Length - Packet->Length;

        return;
    }

    //
    // Initialize fields for a not used packet.
    //
    KInvariant(IsScatterGatherAvailable());
    SGIoPacket* sgPacket = (SGIoPacket*)Packet;

    if (!Packet->Length) {
        Packet->Type = Type;
        Packet->IsCopy = IsCopy;
        Packet->Offset = Offset;
        Packet->TargetOffset = TargetOffset;
#if KTL_USER_MODE
        UNREFERENCED_PARAMETER(IoPriorityHint);
#else
        sgPacket->IoPriorityHint = IoPriorityHint;
#endif
    }

    //
    // The new buffer is correctly aligned, and the current packet is correctly aligned.  Does this IO fall after the last one?
    //

    if (Packet->Offset + Packet->Length != Offset) {
        return;
    }

    //
    // Is this the same operation as the last one?
    //

    if (Packet->Type != Type) {
        return;
    }

    //
    // Are these both copies or both not copies?
    //

    if (Packet->IsCopy != IsCopy) {
        return;
    }

    //
    // If this is a copy, then does the target line up as well?
    //

    if (Packet->IsCopy && Packet->TargetOffset + Packet->Length != TargetOffset) {
        return;
    }

    //
    // The stars are aligned and so are these requests.
    //

    ULONG length = Length;

    if (length > MaxIoSize - Packet->Length) {
        length = MaxIoSize - Packet->Length;
    }

    if (!length) {
        return;
    }

    LengthNotAdded = Length - length;

#if KTL_USER_MODE
    sgPacket->AlignedBuffers[sgPacket->NumAlignedBuffers].buf = (void *)Buffer;
    sgPacket->AlignedBuffers[sgPacket->NumAlignedBuffers++].npages = length / _PageSize;
#else
    for (ULONG i = 0; i < length; i += _PageSize) {
        sgPacket->FileSegmentElements[sgPacket->NumFileSegmentElements++].Buffer = (VOID*) ((UCHAR*) Buffer + i);
    }
#endif

    Packet->Length += length;
}

VOID
KBlockFileStandard::SendIoPacket(
    __inout IoPacket* Packet
    )

/*++

Routine Description:

    This routine sends down this IO packet as a request to the file through either the file system API or via an IRP to the
    volume device object.

Arguments:

    Packet  - Supplies the IO packet to send.

Return Value:

    None.

--*/

{
    InterlockedExchangeAdd64(&g_BlockFileNumBytesSent, Packet->Length);

    SGIoPacket* sgPacket = (SGIoPacket*)Packet;

#if KTL_USER_MODE
    LARGE_INTEGER offset;
    BOOL b;

    offset.QuadPart = Packet->Offset;

    sgPacket->Overlapped.Offset = offset.LowPart;
    sgPacket->Overlapped.OffsetHigh = offset.HighPart;

    if (Packet->Buffer) {
        if (Packet->Type == eWrite) {
            b = WriteFile(_Handle, Packet->Buffer, Packet->Length, NULL, &sgPacket->Overlapped);
        } else {
            b = ReadFile(_Handle, Packet->Buffer, Packet->Length, NULL, &sgPacket->Overlapped);
        }
    } else {
        if (Packet->Type == eWrite) {
            b = HLPalFunctions::WriteFileGatherPal(_Handle,
                                sgPacket->NumAlignedBuffers,
                                sgPacket->AlignedBuffers,
                                Packet->Length,
                                &sgPacket->Overlapped,
                                _PageSize);
        } else {
            b = HLPalFunctions::ReadFileScatterPal(_Handle,
                                sgPacket->NumAlignedBuffers,
                                sgPacket->AlignedBuffers,
                                Packet->Length,
                                &sgPacket->Overlapped,
                                _PageSize);
        }
    }

    DWORD error = GetLastError();
    if (!b && (error != ERROR_IO_PENDING)) {
        if (NT_SUCCESS((NTSTATUS)sgPacket->Overlapped.Internal)) {
            if (error == ERROR_INVALID_HANDLE)
            {
                sgPacket->Overlapped.Internal = (ULONG_PTR)STATUS_INVALID_HANDLE;
            } else {
                sgPacket->Overlapped.Internal = (ULONG_PTR)STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        KTraceFailedAsyncRequest(error, NULL, (ULONGLONG)this, this->_IsSparseFile);
        KTraceFailedAsyncRequest(error, NULL, Packet->Offset, Packet->Length);
        IoCompletionRoutineUserMode(this, &sgPacket->Overlapped, error, 0);
    }
#else  // KTL_USER_MODE
    if (IsScatterGatherAvailable())
    {
        NTSTATUS status;
        MDL* mdl = sgPacket->Mdl;

        if (Packet->Buffer) {
            MmInitializeMdl(mdl, Packet->Buffer, Packet->Length);
            MmBuildMdlForNonPagedPool(mdl);
        } else {
            MmInitializeMdl(mdl, Ptr64ToPtr(sgPacket->FileSegmentElements[0].Buffer), Packet->Length);
            status = KtlMmProbeAndLockSelectedPagesNoException(mdl, sgPacket->FileSegmentElements, KernelMode,
                                                               (Packet->Type == eWrite) ? IoReadAccess : IoWriteAccess);
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, (ULONGLONG)Packet);
                KTraceFailedAsyncRequest(status, NULL, Packet->Offset, Packet->Length);
                KInvariant(FALSE);
            }
        }
        sgPacket->Irp->MdlAddress = mdl;
    }

    SendIrp(Packet, Packet->Offset, Packet->Length);
#endif
}

#if KTL_USER_MODE

BOOL
KBlockFileStandard::ManageVolumePrivilege(
    )

/*++

Routine Description:

    This routine will turn on or turn off the manage volume privilege.

Arguments:

Return Value:

    FALSE   - The privilege could not be granted to the thread.

    TRUE    - Success.

--*/

{
    return HLPalFunctions::ManageVolumePrivilege();
}

#endif

#if KTL_USER_MODE

VOID
KBlockFileStandard::IoCompletionRoutineUserMode(
    __in VOID*,
    __inout OVERLAPPED* Overlapped,
    __in DWORD,
    __in ULONG
    )

/*++

Routine Description:

    This is the completion port linked completion routine for IO.

Arguments:

    File        - Supplies a pointer to the KBlockFileStandard object.

    Overlapped  - Supplies the overlapped packet.

Return Value:

    None.

--*/

{
    SGIoPacket* sgPacket = CONTAINING_RECORD(Overlapped, SGIoPacket, Overlapped);
    IoPacket* ioPacket = (IoPacket*)sgPacket;
    KBlockFileStandard* file = ioPacket->File;
    NTSTATUS status = (NTSTATUS) Overlapped->Internal;

#else    // KTL_USER_MODE

NTSTATUS
KBlockFileStandard::IoCompletionRoutineKernelMode(
    __in IoPacket* IoPacketContext,
    __in NTSTATUS Status
    )
{
    IoPacket* ioPacket = IoPacketContext;
    SGIoPacket* sgPacket = (SGIoPacket*)ioPacket;
    NoSGIoPacket* nosgPacket = (NoSGIoPacket*)ioPacket;

    KBlockFileStandard* file = ioPacket->File;
    NTSTATUS status = Status;

    InterlockedDecrement(&file->_NumOSIoPacketsOutstanding);

    if (ioPacket->RemainingLength && NT_SUCCESS(status)) {

        KInvariant(file->IsScatterGatherAvailable());

        ULONGLONG newOffset;
        ULONG newLength;
        VOID* newBuffer;

        //
        // In this case the file was not contiguous at this point and a re-issue of the IO needs to be done for up
        // to the remaining length.  For this we use the 'PartialMdl' with the main 'Mdl' as the source.
        //

        newOffset = ioPacket->Offset + ioPacket->Length - ioPacket->RemainingLength;
        newLength = ioPacket->RemainingLength;
        newBuffer = (VOID*) ((UCHAR*) MmGetMdlVirtualAddress(sgPacket->Mdl) + (ULONG) (newOffset - ioPacket->Offset));

        MmInitializeMdl(sgPacket->PartialMdl, newBuffer, newLength);
        IoBuildPartialMdl(sgPacket->Mdl, sgPacket->PartialMdl, newBuffer, newLength);

        sgPacket->Irp->MdlAddress = sgPacket->PartialMdl;

        file->SendIrp(ioPacket, newOffset, newLength);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (file->IsScatterGatherAvailable())
    {
        if (! ioPacket->Buffer) {
            MmUnlockPages(sgPacket->Mdl);
        }
    }
#endif

    KNodeList<TransferStandard> transferList(FIELD_OFFSET(TransferStandard, ListEntryCompletion));
    TransferStandard* transfer;
    LONG r;
    ULONG numTransferPackets;
    
    //
    // Common completion code for both user and kernel mode.
    //

    //
    // Update the common status on failure.
    //

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)file, 0);
        KTraceFailedAsyncRequest(status, NULL, ioPacket->Offset, ioPacket->Length);

#if KTL_USER_MODE
#else
        if (file->IsScatterGatherAvailable())
        {
#endif
            for (ULONG i = 0; i < sgPacket->NumTransferPackets; i++) {
                transfer = sgPacket->TransferPackets[i];
                InterlockedExchange(&transfer->_Status, status);
            }
#if KTL_USER_MODE
#else
        } else {
            transfer = nosgPacket->TransferPacket;
            InterlockedExchange(&transfer->_Status, status);
        }
#endif
    }

    //
    // If this is a copy, make sure that we did the write after the read.
    //

    if (ioPacket->IsCopy && ioPacket->Type == eRead) {
        ioPacket->Type = eWrite;
        ioPacket->Offset = ioPacket->TargetOffset;

        file->SendIoPacket(ioPacket);
        goto Finish;
    }

    //
    // Cache the list of transfer packets so the io packet can be
    // reused. The transfer packets need to be completed last as they
    // hold a reference on the KBlockFile which needs to stay in
    // existence until this routine finishes
    //

#if KTL_USER_MODE
#else
    if (file->IsScatterGatherAvailable())
    {
#endif
        numTransferPackets = sgPacket->NumTransferPackets;
        KInvariant(numTransferPackets <= MaxBlocksPerIo);

        for (ULONG i = 0; i < numTransferPackets; i++)
        {
            transfer = sgPacket->TransferPackets[i];
            r = InterlockedDecrement(&transfer->_RefCount);
            KInvariant(r >= 0);
            if (r == 0) {           
                transferList.AppendTail(transfer);
            }
        }

#if KTL_USER_MODE
#else
    } else {
        transfer = nosgPacket->TransferPacket;
        r = InterlockedDecrement(&transfer->_RefCount);
        KInvariant(r >= 0);
        if (r == 0) {           
            transferList.AppendTail(transfer);
        }
    }
#endif

    //
    // See if there is more IO to do
    //
    file->_SpinLock.Acquire();

    if (ioPacket->Priority == eForeground) {
        KInvariant(file->_NumForegroundIoPacketsOutstanding);
        file->_NumForegroundIoPacketsOutstanding--;
    } else {
        KInvariant(file->_NumBackgroundIoPacketsOutstanding);
        file->_NumBackgroundIoPacketsOutstanding--;
    }

    //
    // Return the IO packet to the free list. This must be done before
    // completing the transfer packets
    //

    file->_IoPacketList.AppendTail(ioPacket);

    file->DispatchIoPackets();

    if (DisableDiskIo) {

        //
        // Grab whatever is on the queues and fail those.
        //

        KNodeList<TransferStandard> dumpQueue(FIELD_OFFSET(TransferStandard, ListEntry));
        TransferStandard* transfer1;

        dumpQueue.AppendTail(file->_Foreground.Queue);
        dumpQueue.AppendTail(file->_Background.Queue);

        file->_SpinLock.Release();

        while (dumpQueue.Count()) {
            transfer1 = dumpQueue.RemoveHead();
            transfer1->Complete(STATUS_NO_SUCH_DEVICE);
        }

    } else {
        file->_SpinLock.Release();
    }

    //
    // Decrement the ref count on the transfer packets that are depending on this IO.
    //
    while (transferList.Count()) {
        BOOLEAN b;
        
        transfer = transferList.RemoveHead();
        if (!NT_SUCCESS(transfer->_Status)) {
            KTraceFailedAsyncRequest(transfer->_Status, transfer, transfer->_Offset, transfer->_Length);
            KTraceFailedAsyncRequest(transfer->_Status, transfer, transfer->_TargetOffset, transfer->_Length);
        }

        transfer->_InstrumentedOperation.EndOperation(static_cast<ULONGLONG>(transfer->_Length));
        b = transfer->Complete(transfer->_Status);
        KAssert(b);
    }

Finish:


#if KTL_USER_MODE
    ;
#else
    return STATUS_MORE_PROCESSING_REQUIRED;
#endif
}


#if KTL_USER_MODE
#else

NTSTATUS
KBlockFileStandard::IoCompletionRoutineForBlockFile(
    __in_opt PDEVICE_OBJECT,
    __in PIRP,
    __in_xcount_opt("varies") PVOID Context
    )

/*++

Routine Description:

    This is the IRP completion routine for an IRP_MJ_READ or IRP_MJ_WRITE to the volume.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IRP.

    Context         - Supplies the IoPacket context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    SGIoPacket* sgPacket = (SGIoPacket*) Context;
    IoPacket* ioPacket = (IoPacket*)sgPacket;
    KBlockFileStandard* file = ioPacket->File;
    NTSTATUS status = sgPacket->Irp->IoStatus.Status;
    
    // Record complete time and trace on too long
    ULONGLONG finalTicks, totalTicks;
    
    finalTicks = KNt::GetTickCount64();
    totalTicks = finalTicks - sgPacket->StartTicks;
    if (totalTicks > 500)
    {
        ULONGLONG deltaTicks = finalTicks - file->_LastSlowWriteTicks;
        if (deltaTicks > (60 * 1000))
        {
            KTraceFailedAsyncRequest(STATUS_IO_TIMEOUT, NULL, totalTicks, file->_NumberDelayedWrites);
            file->_NumberDelayedWrites = 0;
            file->_LastSlowWriteTicks = finalTicks;
        } else {
            file->_NumberDelayedWrites++;
        }
    }

    status = IoCompletionRoutineKernelMode(ioPacket, status);

    return(status);
}


VOID
KBlockFileStandard::NoSGIoOperationCompleted(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    //
    // This is the completion routine for the sparse file async.
    // Proceed with the KBlockFile state machine
    //
    KBlockFileStandard::NoSGFileIoOperation::SPtr nosgOperation = (KBlockFileStandard::NoSGFileIoOperation*)(&Async);
    NoSGIoPacket* nosgPacket = nosgOperation->GetNoSGIoPacket();
    IoPacket* ioPacket = (IoPacket*)nosgPacket;
    NTSTATUS status = nosgPacket->IoStatus.Status;

    status = IoCompletionRoutineKernelMode(ioPacket, status);
    KInvariant(status == STATUS_MORE_PROCESSING_REQUIRED);
}

VOID
KBlockFileStandard::NoSGFileIoOperation::Execute(
    )
{
    //
    // Complete the sparse operation back to the caller
    //
    NoSGIoPacket* nosgPacket = _NoSGPacket;
    NTSTATUS status = nosgPacket->IoStatus.Status;
    Complete(status);
}

VOID
KBlockFileStandard::NoSGFileIoOperation::NoSGOperationApcCompletion(
    __in PVOID Context,
    __in PIO_STATUS_BLOCK,
    __in ULONG
    )
{
    //
    // This is the APC completion routine for the sparse file
    // operation. Fire off a work item for this as we can't perform any
    // completions inside of an APC. When KAsyncContextBase is enhanced
    // to handle APC completions, we can skip the work item.
    //

    NoSGIoPacket* nosgPacket = (NoSGIoPacket*) Context;
    KBlockFileStandard::NoSGFileIoOperation::SPtr thisPtr = nosgPacket->NoSGOperation;

    thisPtr->GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*thisPtr);
}

VOID
KBlockFileStandard::NoSGFileIoOperation::OnStart(
    )
{
    NTSTATUS status;

    NoSGIoPacket* nosgPacket = _NoSGPacket;

    if (nosgPacket->Type == eWrite)
    {
        status = ZwWriteFile(_Handle, NULL, KBlockFileStandard::NoSGFileIoOperation::NoSGOperationApcCompletion, nosgPacket,
            &nosgPacket->IoStatus,
            _Buffer,
            _LengthToUse,
            &_FileOffset,
            NULL);
    }
    else
    {
        status = ZwReadFile(_Handle, NULL, KBlockFileStandard::NoSGFileIoOperation::NoSGOperationApcCompletion, nosgPacket,
            &nosgPacket->IoStatus,
            _Buffer,
            _LengthToUse,
            &_FileOffset,
            NULL);
    }

    if ((status != STATUS_PENDING) && (status != STATUS_SUCCESS))
    {
        //
        // If STATUS_PENDING or STATUS_SUCCESS is returned then completion will come
        // from the APC completion routine. Any
        // other status is a synchronous completion and we need to
        // complete ourselves.
        //
        KTraceFailedAsyncRequest(status, NULL, _FileOffset.QuadPart, _LengthToUse);
        nosgPacket->IoStatus.Status = status;
        Complete(status);
    }
}

VOID
KBlockFileStandard::NoSGFileIoOperation::StartNoSGOperation(
    __in HANDLE Handle,
    __in PVOID Buffer,
    __in ULONG LengthToUse,
    __in LARGE_INTEGER FileOffset,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _Handle = Handle;
    _Buffer = Buffer;
    _LengthToUse = LengthToUse;
    _FileOffset = FileOffset;

    Start(ParentAsyncContext, CallbackPtr);
}

KBlockFileStandard::NoSGFileIoOperation::~NoSGFileIoOperation()
{
}

KBlockFileStandard::NoSGFileIoOperation::NoSGFileIoOperation(__in KBlockFileStandard::NoSGIoPacket* NoSGPacket)
{
    _NoSGPacket = NoSGPacket;
}

VOID
KBlockFileStandard::NoSGFileIoOperation::OnReuse(
    )
{
}

VOID
KBlockFileStandard::NoSGFileIoOperation::OnCompleted(
    )
{
}

VOID
KBlockFileStandard::NoSGFileIoOperation::OnCancel(
    )
{
}

#endif


#if KTL_USER_MODE
#else
VOID
KBlockFileStandard::SendIrp(
    __inout IoPacket* Packet,
    __in ULONGLONG FileOffset,
    __in ULONG Length
    )

/*++

Routine Description:

    This routine sends down the IO packet's irp to the volume stack for the given file offset and length.  If the whole length
    is not available then this routine will set 'RemainingLength' to show how much is left for the IO.

Arguments:

    Packet      - Supplies the IO packet.

    FileOffset  - Supplies the file offset.

    Length      - Supplies the length;

Return Value:

    None.

--*/

{
    if (IsScatterGatherAvailable())
    {
        SGIoPacket* sgPacket = (SGIoPacket*)Packet;
        IRP* irp = sgPacket->Irp;
        FileExtentKey key;
        FileExtentValue value;
        IO_STACK_LOCATION* nextSp;
        ULONGLONG delta;
        ULONGLONG offset;
        ULONG length;

        key.FileOffset = FileOffset;
        key.Length = 1;

        _SpinLock.Acquire();

        BOOLEAN foundExtent;

        foundExtent = _ExtentTable.FindInRange(key, value);

        if (! foundExtent) {
            _SpinLock.Release();
            KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER, NULL, key.FileOffset, key.Length);
            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            IoCompletionRoutineForBlockFile(NULL, irp, Packet);
            return;
        }

        _SpinLock.Release();

        delta = FileOffset - key.FileOffset;
        offset = value.VolumeOffset + delta;
        length = Length;

        if (key.Length - delta < length) {
            Packet->RemainingLength = length - ((ULONG) (key.Length - delta));
            length = (ULONG) (key.Length - delta);
        } else {
            Packet->RemainingLength = 0;
        }

        irp->Cancel = FALSE;
        irp->Tail.Overlay.OriginalFileObject = _FileObject;
        nextSp = IoGetNextIrpStackLocation(irp);
        irp->Tail.Overlay.Thread = NULL;
        nextSp->Parameters.Read.ByteOffset.QuadPart = offset;
        nextSp->Parameters.Read.Length = length;
        nextSp->Parameters.Read.Key = 0;
        nextSp->MajorFunction = (Packet->Type == eWrite) ? IRP_MJ_WRITE : IRP_MJ_READ;
        nextSp->DeviceObject = _VolumeObject;
        nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

        if (_IsWriteThrough && nextSp->MajorFunction == IRP_MJ_WRITE) {
            nextSp->Flags |= SL_WRITE_THROUGH;
        }

        KBlockFile::SystemIoPriorityHint ioPriorityHint;
        ioPriorityHint = sgPacket->IoPriorityHint;

        if (ioPriorityHint != eNotDefined)
        {
            //
            // MSDN indicates that this api can fail however we can
            // silently ignore this and just allow the IO to happen at
            // whatever priority anyway.
            //
            IoSetIoPriorityHint(irp, GetIoPriorityHint(ioPriorityHint));
        }

        IoSetCompletionRoutine(irp, IoCompletionRoutineForBlockFile, Packet, TRUE, TRUE, TRUE);

        //
        // Record start time
        //
        sgPacket->StartTicks = KNt::GetTickCount64();
        
        IoCallDriver(nextSp->DeviceObject, irp);
        InterlockedIncrement(&_NumOSIoPacketsOutstanding);
    } else {
        NoSGIoPacket* nosgPacket = (NoSGIoPacket*)Packet;
        PVOID buffer;
        ULONG lengthToUse;
        LARGE_INTEGER fileOffset;

        fileOffset.QuadPart = FileOffset;

        //
        // Assume only one write operation per IoPacket is filled
        //
        buffer = Packet->Buffer;
        lengthToUse = Packet->Length;
        Packet->RemainingLength = 0;

        //
        // Kick off sparse operation as an async so the async can
        // perform synchronous and asynchronous completions
        //
        NoSGFileIoOperation::SPtr nosgOperation = nosgPacket->NoSGOperation;
        KAsyncContextBase::CompletionCallback completion(this,
                                                         &KBlockFileStandard::NoSGIoOperationCompleted);

        InterlockedIncrement(&_NumOSIoPacketsOutstanding);
        nosgOperation->Reuse();
        nosgOperation->StartNoSGOperation(_Handle,
                                              buffer,
                                              lengthToUse,
                                              fileOffset,
                                              NULL,         // Run in any apartment
                                              completion);
    }
}
#endif


#if KTL_USER_MODE
#else

LONG
KBlockFileStandard::FileExtentComparison(
    __in const FileExtentKey& Left,
    __in const FileExtentKey& Right
    )

/*++

Routine Description:

    This routine compares two file extent keys.

Arguments:

    Left    - Supplies the left file extent key.

    Right   - Supplies the right file extent key.

Return Value:

    < 0 - The left is less than the right.
    0   - The left and the right are equal.
    > 0 - The left is greater than the right.

--*/

{
    //
    // Two overlapping ranges are 'equal'.  This is for search.
    //

    if (Left.FileOffset + Left.Length <= Right.FileOffset) {
        return -1;
    }

    if (Right.FileOffset + Right.Length <= Left.FileOffset) {
        return 1;
    }

    return 0;
}

#endif

#if KTL_USER_MODE
#else

NTSTATUS
KBlockFileStandard::InitializeKernelMode(
    __in ULONG
    )

/*++

Routine Description:

    This routine initializes the kernel mode parts of a KBlockFileStandard, including the extent map.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;

    //
    // Get the file object from the file handle.
    //

    status = ObReferenceObjectByHandle(_Handle, 0, *IoFileObjectType, KernelMode, (VOID**) &_FileObject, NULL);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        _FileObject = NULL;
        return status;
    }

    //
    // The volume object is the top of the stack of the "named" device object.
    //

    _VolumeObject = IoGetAttachedDeviceReference(_FileObject->DeviceObject);

    if ((! _IsSparseFile) && (! _IsReadOnlyFile))
    {
        //
        // Pin the file so that the location of the file cannot change.
        //

        status = PinFile();

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
            return status;
        }

        //
        // Initialize the extent table.
        //

        status = AdjustExtentTable(0, _FileSize);

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
            return status;
        }
    }

    _NumberDelayedWrites = 0;
    _LastSlowWriteTicks = 0;

    return STATUS_SUCCESS;
}

#endif

#if KTL_USER_MODE
#else

NTSTATUS
KBlockFileStandard::PinFile(
    )

/*++

Routine Description:

    This routine pins a file, so that it cannot be moved.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    MOUNTDEV_NAME* mountdevName = NULL;
    HANDLE h = NULL;
    ULONG mountdevNameSize;
    NTSTATUS status;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    IRP* irp;
    UNICODE_STRING volumeNameUnicodeString;
    KWString volumeName(GetThisAllocator());
    OBJECT_ATTRIBUTES oa;
    MARK_HANDLE_INFO markHandleInfo;
    BOOLEAN isNtfs;
    HANDLE eventHandle = NULL;

    //
    // Using the device object for the volume, get the name of the volume.
    //

    mountdevNameSize = 500;
    mountdevName = (PMOUNTDEV_NAME) _new(KTL_TAG_FILE, GetThisAllocator()) UCHAR[mountdevNameSize];

    if (!mountdevName) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, _VolumeObject, NULL, 0, mountdevName,
            mountdevNameSize, FALSE, &event, &ioStatus);

    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    status = IoCallDriver(_VolumeObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    //
    // Convert the return value to a KWString.
    //

    volumeNameUnicodeString.Length = volumeNameUnicodeString.MaximumLength = mountdevName->NameLength;
    volumeNameUnicodeString.Buffer = mountdevName->Name;

    volumeName = volumeNameUnicodeString;

    status = volumeName.Status();

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    //
    // Now, using the name of the volume, open a DASD handle to the volume, so that this handle can be passed in to PIN
    // the file.
    //

    InitializeObjectAttributes(&oa, volumeName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = KNt::OpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    //
    // Now, pin the file.
    //

    status = KNt::CreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

    if (!NT_SUCCESS(status)) {
        eventHandle = NULL;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    RtlZeroMemory(&markHandleInfo, sizeof(MARK_HANDLE_INFO));
    markHandleInfo.VolumeHandle = h;
    markHandleInfo.HandleInfo = MARK_HANDLE_PROTECT_CLUSTERS;

    status = KNt::FsControlFile(_Handle, eventHandle, NULL, NULL, &ioStatus, FSCTL_MARK_HANDLE, &markHandleInfo, sizeof(markHandleInfo),
            NULL, 0);

    if (status == STATUS_PENDING) {
        KNt::WaitForSingleObject(eventHandle, FALSE, NULL);
        status = ioStatus.Status;
    }


    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    //
    // Next, validate that this is an NTFS file since we can't do this
    // kind of direct IO on just any file system.  The file system needs to
    // give meaningful values for retrieval pointers and bytes per cluster.
    // Also, the file system should not re-direct IO.
    //

    status = IsNtfs(&isNtfs);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    if (!isNtfs) {
        status = STATUS_FILE_SYSTEM_LIMITATION;
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        goto Finish;
    }

    //
    // BUGBUG Need to guard this with a DISMOUNT notification registration.
    //

Finish:

    if (eventHandle) {
        KNt::Close(eventHandle);
    }

    _delete(mountdevName);

    if (h) {
        KNt::Close(h);
    }

    return status;
}

#endif

#if KTL_USER_MODE
#else

NTSTATUS
KBlockFileStandard::IsNtfs(
    __out BOOLEAN* IsNtfs
    )

/*++

Routine Description:

    This routine determines whether or not the file represented by this object is on an NTFS volume.

Arguments:

    IsNtfs  - Returns whether or not this file is on an NTFS volume.

Return Value:

    NTSTATUS

--*/

{
    FILE_FS_ATTRIBUTE_INFORMATION* fsAttributeInfo = NULL;
    ULONG size;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    size = FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName) + 4*sizeof(WCHAR);

    fsAttributeInfo = (FILE_FS_ATTRIBUTE_INFORMATION*) _new(KTL_TAG_FILE, GetThisAllocator()) UCHAR[size];

    if (!fsAttributeInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    status = KNt::QueryVolumeInformationFile(_Handle, &ioStatus, fsAttributeInfo, size, FileFsAttributeInformation);

    if (status == STATUS_BUFFER_OVERFLOW) {
        *IsNtfs = FALSE;
        status = STATUS_SUCCESS;
        goto Finish;
    }

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    if (fsAttributeInfo->FileSystemNameLength == 8 &&
        fsAttributeInfo->FileSystemName[0] == 'N' &&
        fsAttributeInfo->FileSystemName[1] == 'T' &&
        fsAttributeInfo->FileSystemName[2] == 'F' &&
        fsAttributeInfo->FileSystemName[3] == 'S') {

        *IsNtfs = TRUE;

    } else {
        *IsNtfs = FALSE;
    }

Finish:

    _delete(fsAttributeInfo);

    return status;
}

#endif

#if KTL_USER_MODE
#else

NTSTATUS
KBlockFileStandard::AdjustExtentTable(
    __in ULONGLONG OldFileSize,
    __in ULONGLONG NewFileSize
    )

/*++

Routine Description:

    This routine adjusts the extent table to account for a change in the size of the size of the file.

Arguments:

    OldFileSize - Supplies the old file size that corresponds to the current state of the extent table.

    NewFileSize - Supplies the new file size.

Arguments:

    NTSTATUS

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;
    FILE_FS_SIZE_INFORMATION fsSizeInfo;
    ULONG bytesPerNtfsCluster;
    STARTING_VCN_INPUT_BUFFER startingVcn;
    RETRIEVAL_POINTERS_BUFFER retrievalPointer;
    LONGLONG delta;
    FileExtentKey key;
    FileExtentValue value;

    if (OldFileSize == NewFileSize) {
        return STATUS_SUCCESS;
    }

    //
    // Figure out the bytes per NTFS cluster.
    //

    status = KNt::QueryVolumeInformationFile(_Handle, &ioStatus, &fsSizeInfo, sizeof(FILE_FS_SIZE_INFORMATION),
            FileFsSizeInformation);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, NewFileSize);
        return status;
    }

    bytesPerNtfsCluster = fsSizeInfo.SectorsPerAllocationUnit*fsSizeInfo.BytesPerSector;

    //
    // If this is a shrink.  Then invoke the shrink logic.
    //

    if (OldFileSize > NewFileSize) {
        status = ShrinkExtentTable(NewFileSize);
        return status;
    }

    HANDLE eventHandle;
    status = KNt::CreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, NULL, (ULONGLONG)this, 0);
        return status;
    }

    //
    // Go through all of the retrieval pointers and enter them in to the table.
    //

    startingVcn.StartingVcn.QuadPart = OldFileSize/bytesPerNtfsCluster;

    for (;;) {

        status = KNt::FsControlFile(_Handle, eventHandle, NULL, NULL, &ioStatus, FSCTL_GET_RETRIEVAL_POINTERS, &startingVcn,
                sizeof(startingVcn), &retrievalPointer, sizeof(retrievalPointer));

        if (status == STATUS_PENDING) {
            KNt::WaitForSingleObject(eventHandle, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
            if (status == STATUS_END_OF_FILE) {
                status = STATUS_SUCCESS;
            } else {
                KTraceFailedAsyncRequest(status, nullptr, NewFileSize, startingVcn.StartingVcn.QuadPart);
            }
            break;
        }

        if (!retrievalPointer.ExtentCount) {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, startingVcn.StartingVcn.QuadPart);

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (retrievalPointer.Extents[0].Lcn.QuadPart == -1) {
            if (status != STATUS_BUFFER_OVERFLOW) {
                KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, startingVcn.StartingVcn.QuadPart);
                break;
            }
            startingVcn.StartingVcn.QuadPart = retrievalPointer.Extents[0].NextVcn.QuadPart;
            continue;
        }

        delta = startingVcn.StartingVcn.QuadPart - retrievalPointer.StartingVcn.QuadPart;
        value.VolumeOffset = (retrievalPointer.Extents[0].Lcn.QuadPart + delta)*bytesPerNtfsCluster;
        key.FileOffset = startingVcn.StartingVcn.QuadPart*bytesPerNtfsCluster;
        key.Length = (retrievalPointer.Extents[0].NextVcn.QuadPart - startingVcn.StartingVcn.QuadPart)*bytesPerNtfsCluster;

        _SpinLock.Acquire();
        status = _ExtentTable.Insert(value, key);
        _SpinLock.Release();


        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, startingVcn.StartingVcn.QuadPart);
            break;
        }

        startingVcn.StartingVcn.QuadPart = retrievalPointer.Extents[0].NextVcn.QuadPart;
    }

    KNt::Close(eventHandle);

    return status;
}

#endif

#if KTL_USER_MODE
#else

NTSTATUS
KBlockFileStandard::ShrinkExtentTable(
    __in ULONGLONG NewFileSize
    )

/*++

Routine Description:

    This routine will shrink the extent table to the given size.

Arguments:

    NewFileSize - Supplies the new file size.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    FileExtentKey searchKey;
    FileExtentKey foundKey;
    FileExtentValue foundValue;

    searchKey.FileOffset = NewFileSize;
    searchKey.Length = MAXULONGLONG - NewFileSize;

    _SpinLock.Acquire();

    for (;;) {

        ExtentTable::Iterator iterator = _ExtentTable.Find(searchKey);

        if (!iterator.IsValid()) {
            break;
        }

        foundKey = iterator.GetKey();
        foundValue = iterator.Get();

        //
        // This entry contains at least some of what is being truncated, so, take it out of the extent list.
        //

        _ExtentTable.Remove(foundKey);

        //
        // Check to see if some of the entry needs to be put back.
        //

        if (foundKey.FileOffset >= NewFileSize) {
            continue;
        }

        //
        // Put back that part of the entry that is still part of the file.
        //

        foundKey.Length = NewFileSize - foundKey.FileOffset;

        status = _ExtentTable.Insert(foundValue, foundKey);

        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    _SpinLock.Release();

    return status;
}

#endif

KBlockFileStandard::CreateContext::~CreateContext(
    )
{
    Cleanup();
}

KBlockFileStandard::CreateContext::CreateContext(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN CreateSparseFile,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in ULONG QueueDepth,
    __out KBlockFile::SPtr& File,
    __in ULONG AllocationTag
    )   :   _FileName(GetThisAllocator())
{
    Zero();
    SetConstructorStatus(Initialize(FileName, IsWriteThrough, CreateSparseFile, Disposition, Options, QueueDepth, File, AllocationTag));
}

VOID
KBlockFileStandard::CreateContext::Zero(
    )
{
    _IsWriteThrough = FALSE;
    _CreateSparseFile = FALSE;
    _Disposition = eCreateNew;
    _Options = static_cast<CreateOptions>(0);
    _AllocationTag = 0;
    _QueueDepth = KBlockFile::DefaultQueueDepth;
    _File = NULL;
}

VOID
KBlockFileStandard::CreateContext::Cleanup(
    )
{
}

NTSTATUS
KBlockFileStandard::CreateContext::Initialize(
    __in KWString& FileName,
    __in BOOLEAN IsWriteThrough,
    __in BOOLEAN CreateSparseFile,
    __in CreateDisposition Disposition,
    __in CreateOptions Options,
    __in ULONG QueueDepth,
    __out KBlockFile::SPtr& File,
    __in ULONG AllocationTag
    )
{
    _FileName = FileName;
    _IsWriteThrough = IsWriteThrough;
    _CreateSparseFile = CreateSparseFile;
    _Disposition = Disposition;
    _Options = Options;
    _AllocationTag = AllocationTag;
    _QueueDepth = QueueDepth;
    _File = &File;

    return _FileName.Status();
}

VOID
KBlockFileStandard::CreateContext::OnStart(
    )
{
    //
    // Get off of the no-block worker thread pool.
    //

    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KBlockFileStandard::CreateContext::Execute(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CreateState createState;

    createState.Create = this;

#if KTL_USER_MODE
    //
    // For the user mode version of this we need to take the privilege.  Doing this is not allowed on the thread pool.  So,
    // create a thread just for the create file.
    //

    KThread::SPtr thread;

    status = KThread::Create(Worker, &createState, thread, GetThisAllocator());

    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    //
    // Wait for the thread to exit.
    //

    thread->Wait();
#else
    //
    // For the kernel mode version, there is no reason to add a privilege to the thread doing the create.  So, we can
    // just stay on this worker thread to do the create file.
    //

    Worker(&createState);
#endif

    status = createState.Status;

    goto Finish;

Finish:

    //
    // Complete the request.
    //

    Complete(status);
}

VOID
KBlockFileStandard::CreateContext::Worker(
    __inout VOID* State
    )

/*++

Routine Description:

    This routine will do the job of creating the KBlockFile object.

Arguments:

    CreateState - Supplies the 'CreateState' context.

Return Value:

    None.

--*/

{
    CreateState* state = (CreateState*) State;
    CreateContext* create = state->Create;
    KBlockFileStandard* file;

    //
    // Check to see that the file name was copied successfully.
    //

    if (!NT_SUCCESS(create->_FileName.Status())) {
        state->Status = create->_FileName.Status();
        return;
    }

    //
    // This is executing in a system worker thread.  Invoke blocking constructor here.
    //

    file = _new(create->_AllocationTag, create->GetThisAllocator()) KBlockFileStandard(create->_FileName,
            create->_Disposition, create->_Options,
            create->_IsWriteThrough, create->_CreateSparseFile, create->_QueueDepth, create->_AllocationTag);

    if (!file) {
        state->Status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

    state->Status = file->Status();

    if (!NT_SUCCESS(state->Status)) {
        _delete(file);
        return;
    }

    *(create->_File) = file;
}

KBlockFileStandard::SetFileSizeContext::~SetFileSizeContext(
    )
{
    Cleanup();
}

KBlockFileStandard::SetFileSizeContext::SetFileSizeContext(
    __inout KBlockFileStandard& File,
    __in ULONGLONG FileSize
    )
{
    Zero();
    SetConstructorStatus(Initialize(File, FileSize));
}

VOID
KBlockFileStandard::SetFileSizeContext::Zero(
    )
{
    _FileSize = 0;
}

VOID
KBlockFileStandard::SetFileSizeContext::Cleanup(
    )
{
}

NTSTATUS
KBlockFileStandard::SetFileSizeContext::Initialize(
    __inout KBlockFileStandard& File,
    __in ULONGLONG FileSize
    )
{
    _File = &File;
    _FileSize = FileSize;
    return STATUS_SUCCESS;
}

VOID
KBlockFileStandard::SetFileSizeContext::OnStart(
    )
{
    //
    // Get off of the no-block worker thread pool.
    //

    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KBlockFileStandard::SetFileSizeContext::Execute(
    )

/*++

Routine Description:

    This routine executes the set file size work item on a system worker thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;

    status = HLPalFunctions::SetFileSize(_File->_Handle,
                                         _File->_IsSparseFile,
                                         BlockSize,
                                         _File->_FileSize,
                                         _FileSize,
                                         GetThisAllocator());
    if (NT_SUCCESS(status))
    {
#if KTL_USER_MODE
#else
        //
        // Adjust the extent table.
        //
        status = _File->AdjustExtentTable(_File->_FileSize, _FileSize);

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, _FileSize);
            goto Finish;
        }

#endif      
        //
        // Remember the new file size.
        //
        _File->_FileSize = _FileSize;       
    }

#if KTL_USER_MODE
#else
Finish:
#endif      
    //
    // Complete this async operation.
    //
    Complete(status);
}


KBlockFileStandard::SetSystemIoPriorityHintStandard::~SetSystemIoPriorityHintStandard(
    )
{
    Cleanup();
}

KBlockFileStandard::SetSystemIoPriorityHintStandard::SetSystemIoPriorityHintStandard(
    __inout KBlockFileStandard& File
    )
{
    Zero();
    SetConstructorStatus(Initialize(File));
}

VOID
KBlockFileStandard::SetSystemIoPriorityHintStandard::Zero(
)
{
    _IoPriorityHint = eNotDefined;
}

VOID
KBlockFileStandard::SetSystemIoPriorityHintStandard::Cleanup(
)
{
}

NTSTATUS
KBlockFileStandard::SetSystemIoPriorityHintStandard::Initialize(
    __inout KBlockFileStandard& File
)
{
    _File = &File;
    return STATUS_SUCCESS;
}

VOID
KBlockFileStandard::SetSystemIoPriorityHintStandard::OnStart(
)
{
#if KTL_USER_MODE
    //
    // For user mode always get off of the no-block worker thread pool.
    //
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
#else
    //
    // For kernel mode, only defer to worker thread if we need to set IoPriority on
    // the handle. Otherwise it can be done inline here.
    //
    if (_File->IsScatterGatherAvailable())
    {
        _File->SetSystemIoPriorityHintInternal(_IoPriorityHint);
        Complete(STATUS_SUCCESS);
    } else {
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
    }
#endif
}

VOID
KBlockFileStandard::SetSystemIoPriorityHintStandard::Execute(
)
/*++

Routine Description:

    This routine executes the SetIoPriorityHint work item on a system worker thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    
    //
    // Create this as a union to enforce an 8 byte alignment. For some
    // reason FileIoPriorityHintInformation requires an 8 byte
    // alignment
    //
    union
    {
        LONGLONG dummy;
        FILE_IO_PRIORITY_HINT_INFORMATION priorityHint;
    } u;
    IO_STATUS_BLOCK ioStatus;

    u.priorityHint.PriorityHint = _File->GetIoPriorityHint(_IoPriorityHint);

    status = KNt::SetInformationFile(_File->_Handle,
                                     &ioStatus,
                                     &u.priorityHint,
                                     sizeof(FILE_IO_PRIORITY_HINT_INFORMATION),
                                     FileIoPriorityHintInformation);

    if (NT_SUCCESS(status)) {
        _File->SetSystemIoPriorityHintInternal(_IoPriorityHint);
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}


KBlockFileStandard::EndOfFileStandard::~EndOfFileStandard(
    )
{
    Cleanup();
}

KBlockFileStandard::EndOfFileStandard::EndOfFileStandard(
    __inout KBlockFileStandard& File
    )
{
    Zero();
    SetConstructorStatus(Initialize(File));
}

VOID
KBlockFileStandard::EndOfFileStandard::Zero(
)
{
    _SetEndOfFile = 0;
    _GetEndOfFile = NULL;
}

VOID
KBlockFileStandard::EndOfFileStandard::Cleanup(
)
{
}

NTSTATUS
KBlockFileStandard::EndOfFileStandard::Initialize(
    __inout KBlockFileStandard& File
)
{
    _File = &File;
    return STATUS_SUCCESS;
}

VOID
KBlockFileStandard::EndOfFileStandard::OnStart(
)
{
    //
    // Always pass off of the no-block worker thread pool.
    //
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KBlockFileStandard::EndOfFileStandard::Execute(
)
/*++

Routine Description:

    This routine executes the EndOfFile work item on a system worker thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    switch(_Operation)
    {
        case GetEndOfFileOperation:
        {
            FILE_STANDARD_INFORMATION standardInformation;

            status = KNt::QueryInformationFile(_File->_Handle, &ioStatus, &standardInformation, sizeof(standardInformation),
                                               FileStandardInformation);


            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, 0);
            } else {
                *_GetEndOfFile = standardInformation.EndOfFile.QuadPart;
            }

            Complete(status);
            break;
        }

        case SetEndOfFileOperation:
        {
            FILE_END_OF_FILE_INFORMATION endOfFile;

            endOfFile.EndOfFile.QuadPart = _SetEndOfFile;

            status = KNt::SetInformationFile(_File->_Handle, &ioStatus, &endOfFile, sizeof(endOfFile), FileEndOfFileInformation);

            if (NT_SUCCESS(status))
            {
#if KTL_USER_MODE
                // TODO: is _FileSize correct ??? Does it matter ??
                status = HLPalFunctions::AdjustExtentTable(_File->_Handle, _File->_FileSize, _File->_FileSize, GetThisAllocator());
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, endOfFile.EndOfFile.HighPart);
                }
#endif              
            } else {
                KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, endOfFile.EndOfFile.HighPart);
            }
            
            Complete(status);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }
}


KBlockFileStandard::TransferStandard::~TransferStandard(
    )
{
    Cleanup();
}


KBlockFileStandard::TransferStandard::TransferStandard(
    ) 
{
    Zero();
    SetConstructorStatus(Initialize());
}

VOID
KBlockFileStandard::TransferStandard::InitializeForTransfer(
    __inout KBlockFileStandard& File,
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length
    )
{
    _File = &File;
    _Priority = Priority;

    if (IoPriorityHint == KBlockFile::SystemIoPriorityHint::eNotDefined)
    {
        _IoPriorityHint = _File->_IoPriorityHint;
    } else {
        _IoPriorityHint = IoPriorityHint;
    }

    _TransferType = Type;
    _IsCopy = FALSE;
    _InQueue = FALSE;
    _Offset = Offset;
    _TargetOffset = 0;
    _Buffer = Buffer;
    _Length = Length;
    _IoBuffer = NULL;
    _RefCount = 0;
    _Status = STATUS_SUCCESS;

    _InstrumentedOperation.SetInstrumentedComponent(*(_File->_InstrumentedComponent));
}

VOID
KBlockFileStandard::TransferStandard::InitializeForTransfer(
    __inout KBlockFileStandard& File,
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in TransferType Type,
    __in ULONGLONG Offset,
    __in KIoBuffer& IoBuffer
    )
{
    _File = &File;
    _Priority = Priority;

    if (IoPriorityHint == KBlockFile::SystemIoPriorityHint::eNotDefined)
    {
        _IoPriorityHint = _File->_IoPriorityHint;
    } else {
        _IoPriorityHint = IoPriorityHint;
    }

    _TransferType = Type;
    _IsCopy = FALSE;
    _InQueue = FALSE;
    _Offset = Offset;
    _TargetOffset = 0;
    _Buffer = NULL;
    _Length = IoBuffer.QuerySize();
    _IoBuffer = &IoBuffer;
    _RefCount = 0;
    _Status = STATUS_SUCCESS;
    
    _InstrumentedOperation.SetInstrumentedComponent(*(_File->_InstrumentedComponent));
}

VOID
KBlockFileStandard::TransferStandard::InitializeForCopy(
    __inout KBlockFileStandard& File,
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in_bcount(Length) VOID* Buffer,
    __in ULONG Length
    )
{
    _File = &File;
    _Priority = Priority;

    if (IoPriorityHint == KBlockFile::SystemIoPriorityHint::eNotDefined)
    {
        _IoPriorityHint = _File->_IoPriorityHint;
    } else {
        _IoPriorityHint = IoPriorityHint;
    }

    _TransferType = eRead;
    _IsCopy = TRUE;
    _InQueue = FALSE;
    _Offset = SourceOffset;
    _TargetOffset = TargetOffset;
    _Buffer = Buffer;
    _Length = Length;
    _IoBuffer = NULL;
    _RefCount = 0;
    _Status = STATUS_SUCCESS;
    
    _InstrumentedOperation.SetInstrumentedComponent(*(_File->_InstrumentedComponent));
}

VOID
KBlockFileStandard::TransferStandard::InitializeForCopy(
    __inout KBlockFileStandard& File,
    __in IoPriority Priority,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
    __in ULONGLONG SourceOffset,
    __in ULONGLONG TargetOffset,
    __in KIoBuffer& IoBuffer
    )
{
    _File = &File;
    _Priority = Priority;

    if (IoPriorityHint == KBlockFile::SystemIoPriorityHint::eNotDefined)
    {
        _IoPriorityHint = _File->_IoPriorityHint;
    } else {
        _IoPriorityHint = IoPriorityHint;
    }

    _TransferType = eRead;
    _IsCopy = TRUE;
    _InQueue = FALSE;
    _Offset = SourceOffset;
    _TargetOffset = TargetOffset;
    _Buffer = NULL;
    _Length = IoBuffer.QuerySize();
    _IoBuffer = &IoBuffer;
    _RefCount = 0;
    _Status = STATUS_SUCCESS;
    
    _InstrumentedOperation.SetInstrumentedComponent(*(_File->_InstrumentedComponent));
}

VOID
KBlockFileStandard::TransferStandard::Zero(
    )
{
    _Priority = eBackground;
    _TransferType = eRead;
    _IsCopy = FALSE;
    _InQueue = FALSE;
    _Offset = 0;
    _TargetOffset = 0;
    _Buffer = NULL;
    _Length = 0;
    _RefCount = 0;
    _Status = STATUS_SUCCESS;
}

VOID
KBlockFileStandard::TransferStandard::Cleanup(
    )
{
}

NTSTATUS
KBlockFileStandard::TransferStandard::Initialize(
    )
{
    return STATUS_SUCCESS;
}

VOID
KBlockFileStandard::TransferStandard::OnStart(
    )

/*++

Routine Description:

    This routine starts off a transfer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;

    _InstrumentedOperation.BeginOperation();    
    
    if (DisableDiskIo) {
        Complete(STATUS_NO_SUCH_DEVICE);
        return;
    }

    //
    // Validate that the request is properly aligned.
    //

    status = CheckAlignment();

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, 0);
        Complete(status);
        return;
    }

    _File->_SpinLock.Acquire();

    //
    // Take this request and put it at the end of the line.
    //

    if (_Priority == eBackground) {
        _File->_Background.Queue.AppendTail(this);
    } else {
        _File->_Foreground.Queue.AppendTail(this);
    }

    KInvariant(!this->_InQueue);
    this->_InQueue = TRUE;

    _File->DispatchIoPackets();

    _File->_SpinLock.Release();
}

VOID
KBlockFileStandard::TransferStandard::OnCancel(
    )

/*++

Routine Description:

    Basic cancel support for transfer packets that are still in the queue.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _File->_SpinLock.Acquire();

    if (!_InQueue) {

        //
        // The request is already being processed by the file system.  Don't do anything with cancel but let the request
        // complete normally instead.
        //

        _File->_SpinLock.Release();

        KTraceCancelCalled(this, FALSE, FALSE, 0);

        return;
    }

    //
    // This transfer packet is in the queue, waiting to be processed.  Just cancel it now.
    //

    if (_Priority == eBackground) {
        _File->_Background.Queue.Remove(this);
    } else {
        _File->_Foreground.Queue.Remove(this);
    }

    KInvariant(_InQueue);
    _InQueue = FALSE;

    _File->_SpinLock.Release();

    //
    // Complete this request with STATUS_CANCELLED.
    //

    KTraceCancelCalled(this, TRUE, TRUE, 0);

    Complete(STATUS_CANCELLED);
}

VOID
KBlockFileStandard::TransferStandard::OnReuse(
    )

/*++

Routine Description:

    This routine will free up resources held by this transfer packet.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Release references.
    //

    _File = NULL;
    _IoBuffer = NULL;
}

NTSTATUS
KBlockFileStandard::TransferStandard::CheckAlignment(
    )

/*++

Routine Description:

    This routine verifies that the offsets, length, and memory are all 'BlockSize' aligned.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    ULONG_PTR memPtr;

    if (_Offset%BlockSize) {
        KAssert(FALSE);
        return K_STATUS_MISALIGNED_IO;
    }

    if (_IsCopy && _TargetOffset%BlockSize) {
        KAssert(FALSE);
        return K_STATUS_MISALIGNED_IO;
    }

    if (_Length%BlockSize) {
        KAssert(FALSE);
        return K_STATUS_MISALIGNED_IO;
    }

    if (_Buffer) {
        memPtr = (ULONG_PTR) _Buffer;

        if (memPtr%BlockSize) {
            KAssert(FALSE);
            return K_STATUS_MISALIGNED_IO;
        }

    } else {

        KIoBufferElement* elem;

        for (elem = _IoBuffer->First(); elem; elem = _IoBuffer->Next(*elem)) {

            memPtr = (ULONG_PTR) elem->GetBuffer();

            if (memPtr%BlockSize) {
                KAssert(FALSE);
                return K_STATUS_MISALIGNED_IO;
            }

            if (elem->QuerySize()%BlockSize) {
                KAssert(FALSE);
                return K_STATUS_MISALIGNED_IO;
            }
        }
    }

    return STATUS_SUCCESS;
}

KBlockFileStandard::FlushStandard::~FlushStandard(
    )
{
    Cleanup();
}

KBlockFileStandard::FlushStandard::FlushStandard(
    __in KBlockFileStandard& File
    )
{
    Zero();
    SetConstructorStatus(Initialize(File));
}

VOID
KBlockFileStandard::FlushStandard::InitializeForFlush(
    __inout KBlockFileStandard& File
    )
{
    _File = &File;
}

VOID
KBlockFileStandard::FlushStandard::Zero(
    )
{
#if KTL_USER_MODE
#else
    _Irp = NULL;
#endif
}

VOID
KBlockFileStandard::FlushStandard::Cleanup(
    )
{
#if KTL_USER_MODE
#else
    if (_Irp) {
        IoFreeIrp(_Irp);
    }
#endif
}

NTSTATUS
KBlockFileStandard::FlushStandard::Initialize(
    __in KBlockFileStandard& File
    )
{
    NTSTATUS status = STATUS_SUCCESS;

#if KTL_USER_MODE
    UNREFERENCED_PARAMETER(File);
#else
    _Irp = IoAllocateIrp(File._VolumeObject->StackSize, FALSE);

    if (!_Irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

#endif

    return status;
}

VOID
KBlockFileStandard::FlushStandard::OnStart(
    )

/*++

Routine Description:

    This routine starts off a flush.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE

    //
    // The flush is officially started.  Since this is a blocking operation, we need to queue this to a system worker thread.
    //

    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);

#else

    IO_STACK_LOCATION* nextSp;

    //
    // We have the irp in the flush packet.  Just initialize it and send it.
    //

    _Irp->Cancel = FALSE;
    _Irp->Tail.Overlay.OriginalFileObject = _File->_FileObject;
    _Irp->Tail.Overlay.Thread = NULL;
    nextSp = IoGetNextIrpStackLocation(_Irp);
    nextSp->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    nextSp->DeviceObject = _File->_VolumeObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(_Irp, FlushCompletionRoutine, this, TRUE, TRUE, TRUE);

    IoCallDriver(nextSp->DeviceObject, _Irp);

#endif
}

VOID
KBlockFileStandard::FlushStandard::OnReuse(
    )
{
    _File = NULL;
}

VOID
KBlockFileStandard::FlushStandard::Execute(
    )

/*++

Routine Description:

    This routine executes a synchronous flush command on a system worker thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if KTL_USER_MODE

    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    //
    // We're on a thread that can block.  Call FlushFileBuffers.
    //

    status = KNt::FlushBuffersFile(_File->_Handle, &ioStatus);

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, 0);
    }

    Complete(status);

#endif
}

#if KTL_USER_MODE
#else

NTSTATUS
KBlockFileStandard::FlushStandard::FlushCompletionRoutine(
    __in PDEVICE_OBJECT,
    __in PIRP Irp,
    __in_xcount_opt("varies") PVOID Context
    )

/*++

Routine Description:

    This routine is the completion for the flush IRP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IRP.

    Context         - Supplies the IoPacket context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    FlushStandard* context = (FlushStandard*) Context;
    NTSTATUS status = Irp->IoStatus.Status;

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, context, (ULONGLONG)context->_File, 0);
    }

    context->Complete(status);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

#endif

KBlockFileStandard::TrimStandard::~TrimStandard(
    )
{
    Cleanup();
}

KBlockFileStandard::TrimStandard::TrimStandard(
    __in KBlockFileStandard& File
    )
{
    Zero();
    SetConstructorStatus(Initialize(File));
}

VOID
KBlockFileStandard::TrimStandard::InitializeForTrim(
    __inout KBlockFileStandard& File,
    __in ULONGLONG TrimFrom,
    __in ULONGLONG TrimToPlusOne
    )
{
    _File = &File;
    _TrimFrom = TrimFrom;
    _TrimToPlusOne = TrimToPlusOne;
}

VOID
KBlockFileStandard::TrimStandard::Zero(
    )
{
}

VOID
KBlockFileStandard::TrimStandard::Cleanup(
    )
{
}

NTSTATUS
KBlockFileStandard::TrimStandard::Initialize(
    __in KBlockFileStandard&
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

VOID
KBlockFileStandard::TrimStandard::OnStart(
    )

/*++

Routine Description:

    This routine starts off a Trim.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // The Trim is officially started.  Since this is a blocking operation, we need to queue this to a system worker thread.
    //

    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KBlockFileStandard::TrimStandard::OnReuse(
    )
{
    _File = NULL;
}

VOID
KBlockFileStandard::TrimStandard::Execute(
    )

/*++

Routine Description:

    This routine executes a synchronous Trim command on a system worker thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    HANDLE eventHandle = NULL;
    IO_STATUS_BLOCK ioStatus;

    status = KNt::CreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

    if (!NT_SUCCESS(status)) {
        eventHandle = NULL;
        goto Finish;
    }

    FILE_ZERO_DATA_INFORMATION zeroDataInfo;
    zeroDataInfo.FileOffset.QuadPart = _TrimFrom;
    zeroDataInfo.BeyondFinalZero.QuadPart = _TrimToPlusOne;

    status = KNt::FsControlFile(_File->_Handle, eventHandle, NULL, NULL, &ioStatus,
        FSCTL_SET_ZERO_DATA,
        &zeroDataInfo, sizeof(zeroDataInfo),
        NULL, 0);

    if (status == STATUS_PENDING) {
        KNt::WaitForSingleObject(eventHandle, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, 0);
        KTraceFailedAsyncRequest(status, this, _TrimFrom, _TrimToPlusOne);
        goto Finish;
    }

Finish:
    if (eventHandle) {
        KNt::Close(eventHandle);
    }

    Complete(status);
}


KBlockFileStandard::QueryAllocationsStandard::~QueryAllocationsStandard(
    )
{
    Cleanup();
}

KBlockFileStandard::QueryAllocationsStandard::QueryAllocationsStandard(
    __in KBlockFileStandard& File
    )
{
    Zero();
    SetConstructorStatus(Initialize(File));
}

VOID
KBlockFileStandard::QueryAllocationsStandard::InitializeForQueryAllocations(
    __inout KBlockFileStandard& File,
    __in ULONGLONG QueryFrom,
    __in ULONGLONG Length,
    __inout KArray<AllocatedRange>& Results
    )
{
    _File = &File;
    _QueryFrom = QueryFrom;
    _Length = Length;
    _Results = &Results;
}

VOID
KBlockFileStandard::QueryAllocationsStandard::Zero(
    )
{
}

VOID
KBlockFileStandard::QueryAllocationsStandard::Cleanup(
    )
{
}

NTSTATUS
KBlockFileStandard::QueryAllocationsStandard::Initialize(
    __in KBlockFileStandard&
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

VOID
KBlockFileStandard::QueryAllocationsStandard::OnStart(
    )

/*++

Routine Description:

    This routine starts off a QueryAllocations.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // The QueryAllocations is officially started.  Since this is a blocking operation, we need to queue this to a system worker thread.
    //

    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

VOID
KBlockFileStandard::QueryAllocationsStandard::OnReuse(
    )
{
    _File = NULL;
}

VOID
KBlockFileStandard::QueryAllocationsStandard::Execute(
    )

/*++

Routine Description:

    This routine executes a synchronous QueryAllocations command on a system worker thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    HANDLE eventHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    ULONG bytesReturned;
    ULONGLONG queryFrom;
    ULONGLONG length;
    ULONGLONG topOffset;
    ULONG i;

    ULONG lastAllocatedRangeIndex = (ULONG)-1;
    queryFrom = _QueryFrom;
    length = _Length;
    topOffset = _QueryFrom + _Length;
    do
    {
        status = KNt::CreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

        if (!NT_SUCCESS(status)) {
            KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, 0);
            eventHandle = NULL;
            goto Finish;
        }

        FILE_ALLOCATED_RANGE_BUFFER in;
        FILE_ALLOCATED_RANGE_BUFFER out[2];
        in.FileOffset.QuadPart = queryFrom;
        in.Length.QuadPart = length;

        status = KNt::FsControlFile(_File->_Handle, eventHandle, NULL, NULL, &ioStatus,
            FSCTL_QUERY_ALLOCATED_RANGES ,
            &in, sizeof(in),
            out, 2*sizeof(FILE_ALLOCATED_RANGE_BUFFER));

        if (status == STATUS_PENDING) {
            KNt::WaitForSingleObject(eventHandle, FALSE, NULL);
            status = ioStatus.Status;
        }
        bytesReturned = (ULONG)ioStatus.Information;

        if ((NT_SUCCESS(status)) || (status == STATUS_BUFFER_OVERFLOW))
        {
            ULONG entriesReturned = bytesReturned / sizeof(FILE_ALLOCATED_RANGE_BUFFER);
            for (i = 0; i < entriesReturned; i++)
            {
                struct AllocatedRange allocatedRange;
                allocatedRange.Offset = out[i].FileOffset.QuadPart;
                allocatedRange.Length = out[i].Length.QuadPart;
                
                if (lastAllocatedRangeIndex != (ULONG)-1)
                {
                    if (allocatedRange.Offset == ((*_Results)[lastAllocatedRangeIndex].Offset + (*_Results)[lastAllocatedRangeIndex].Length))
                    {
                        //
                        // If this entry can be aggregated with
                        // previous entry then do so
                        //
                        (*_Results)[lastAllocatedRangeIndex].Length += allocatedRange.Length;
                        continue;
                    }
                }
                _Results->Append(allocatedRange);
                lastAllocatedRangeIndex = _Results->Count() - 1;
            }
        } else {
            KTraceFailedAsyncRequest(status, this, (ULONGLONG)_File, 0);
            goto Finish;
        }

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            i--;
            queryFrom = out[i].FileOffset.QuadPart + out[i].Length.QuadPart;
            if (queryFrom >= topOffset)
            {
                status = STATUS_SUCCESS;
                break;
            }
            length = topOffset - queryFrom;
        }

        KNt::Close(eventHandle);
        eventHandle = NULL;
    } while (status == STATUS_BUFFER_OVERFLOW);


Finish:
    if (eventHandle) {
        KNt::Close(eventHandle);
    }

    Complete(status);
}




KBlockFileStandard::RequestQueue::RequestQueue(
    ) : Queue(FIELD_OFFSET(TransferStandard, ListEntry))
{
    CurrentRequest = NULL;
    CurrentRequestOffset = 0;
}

BOOLEAN
KBlockFileStandard::RequestQueue::IsEmpty(
    )
{
    if (Queue.Count()) {
        return FALSE;
    }

    if (CurrentRequest) {
        return FALSE;
    }

    return TRUE;
}
