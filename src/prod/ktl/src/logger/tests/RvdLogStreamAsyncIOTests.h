/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLogFileTests.h

Abstract:

    Contains Log file test routine declarations.

    --*/

#pragma once
#include "RvdLoggerTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <ktl.h>
#include <ktrace.h>
#include <InternalKtlLogger.h>
#include "RvdLogTestHelpers.h"

class LogStreamIoContext : public KObject<LogStreamIoContext>, public KShared<LogStreamIoContext>
{
    K_FORCE_SHARED_WITH_INHERITANCE(LogStreamIoContext);

public:
    KListEntry          _IoListEntry;
    RvdLogAsn           _RecordAsn;
    ULONGLONG           _Version;
    ULONGLONG           _ReadVersion;

protected:
    KBuffer::SPtr       _Metadata;
    KIoBuffer::SPtr     _IoBuffer;

private:
    ULONG               _MetadataSize;
    ULONG               _IoBufferSize;

public:
    static LogStreamIoContext::SPtr
    Create(
        __in RvdLogAsn const& RecordAsn = 0,
        __in ULONGLONG const& Version = 0);

    static BOOLEAN
    ValidateRecordContents(
        RvdLogAsn Asn,
        ULONGLONG Version,
        void* Metadata,
        ULONG MetadataSize,
        void* IoBuffer,
        ULONG IoBufferSize);

    BOOLEAN CompareTo(
        __in LogStreamIoContext& Comparand) const;

    VOID
    Reset();

    VOID
    SetMetadataSize(__in const ULONG MetadataSize = 0);

    VOID
    SetIoBufferSize(__in const ULONG IoBufferSize = 0);

    NTSTATUS
    AllocateBuffers();

    BOOLEAN
    ValidateBuffers();

    __inline KBuffer::SPtr&
    GetMetadataRef()
    {
        return _Metadata;
    }

    __inline KIoBuffer::SPtr&
    GetIoBufferRef()
    {
        return _IoBuffer;
    }

    KBuffer::SPtr __inline
    GetMetadata()
    {
        return _Metadata;
    }

    KIoBuffer::SPtr __inline
    GetIoBuffer()
    {
        return _IoBuffer;
    }

    ULONG __inline
    GetMetadataSize()
    {
        return _MetadataSize;
    }

    ULONG __inline
    GetIoBufferSize()
    {
        return _IoBufferSize;
    }

    RvdLogStreamImp::AsyncWriteStream::SPtr     _DebugLastWriteOp;

private:
    VOID __inline static
    RepStoreULongLong(UCHAR* DestPtr, ULONGLONG Value, ULONG SizeInBytes)
    {
        // ULONG           ULongLongCount = SizeInBytes / sizeof(ULONGLONG);
        static_assert(sizeof(ULONGLONG) == 8, "ULONGLONG size error");
        ULONG               ULongLongCount = SizeInBytes >> 3;
        ULONGLONG           blockOffset = 0;

        //SizeInBytes -= ULongLongCount * sizeof(ULONGLONG);
        SizeInBytes -=  ULongLongCount << 3;

        while (ULongLongCount > 0)
        {
            ULONGLONG       value = Value | (blockOffset << 32);
            blockOffset++;
            ULONG todo = __min((4096 / sizeof(ULONGLONG)), ULongLongCount);
#ifdef _WIN64
            __stosq((ULONGLONG*)DestPtr, value, todo);
#else
            UNREFERENCED_PARAMETER(value);
#endif
            ULongLongCount -= todo;
            DestPtr += (todo * sizeof(ULONGLONG));
        }

        if (SizeInBytes > 0)
        {
            ULONGLONG       value = Value | (blockOffset << 32);
            __movsb(DestPtr, ((UCHAR*)&value), SizeInBytes);
        }
    }

    BOOLEAN _inline static
    RepScanULongLong(UCHAR* DestPtr, ULONGLONG Value, ULONG SizeInBytes, ULONGLONG StartingBlock = 0)
    {
        // BUG, amith, xxxxx, try and find a way to use a repe scasq intrinsic
        ULONGLONG       blockCount = StartingBlock;
        while (SizeInBytes >= sizeof(ULONGLONG))
        {
            ULONGLONG       value = Value | (blockCount << 32);
            blockCount++;
            ULONG           todo = __min(SizeInBytes, 4096);

            SizeInBytes -= todo;
            while (todo >= sizeof(ULONGLONG))
            {
                if (*((ULONGLONG*)DestPtr) != value)
                {
                    return FALSE;
                }
                DestPtr += sizeof(ULONGLONG);
                todo -= sizeof(ULONGLONG);
            }

            SizeInBytes = todo;
        }

        while (SizeInBytes > 0)
        {
            ULONGLONG       value = Value | (blockCount << 32);
            SizeInBytes--;
            if (DestPtr[SizeInBytes] != ((UCHAR*)(&value))[SizeInBytes])
            {
                return FALSE;
            }
        }

        return TRUE;
    }

protected:
    LogStreamIoContext(
        __in RvdLogAsn const& RecordAsn,
        __in ULONGLONG const& Version);

};

enum IoType
{
    Undefined = 0,
    Write,
    Read,
    Truncate
};

class AsyncLogStreamIoContext : public LogStreamIoContext
{
    K_FORCE_SHARED(AsyncLogStreamIoContext);

public:
    IoType                                                          _AsyncIoType;
    KAsyncContextBase::SPtr                                         _AsyncIoContext;
    PVOID                                                           _OpaqueContext;
    KAsyncQueue<AsyncLogStreamIoContext>::DequeueOperation::SPtr    _DequeueOp;
    KListEntry                                                      _HistoryListEntry;
    KListEntry                                                      _TruncateHoldListEntry;

    //* IoType::Write specific state
    BOOLEAN                                 _DoReadAfterWriteVerify;

public:
    static AsyncLogStreamIoContext::SPtr
    Create(
        __in RvdLogAsn const& RecordAsn = 0,
        __in ULONGLONG const& Version = 0,
        __in const IoType AsyncIoType = IoType::Undefined);

    VOID
    Reset();

private:
    AsyncLogStreamIoContext(
            __in RvdLogAsn const& RecordAsn,
            __in ULONGLONG const& Version,
            __in IoType const AsyncIoType);

};

class IoRequestGenerator
{
public:
    IoRequestGenerator();

    ~IoRequestGenerator();

    VOID
    Initialize();

    NTSTATUS
    GenerateNewRequest(
        __in ULONG MaxRecordSize,
        __out AsyncLogStreamIoContext::SPtr& IoRequest);

    NTSTATUS
    GenerateTruncateRequest(
        __out AsyncLogStreamIoContext::SPtr& IoRequest,
        __in RvdLogAsn Asn);

    NTSTATUS
    GenerateWriteRequest(
        __out AsyncLogStreamIoContext::SPtr& IoRequest,
        __in RvdLogAsn Asn,
        __in ULONGLONG Version,
        __in ULONG MetadataSize,
        __in ULONG IoBufferSize,
        __in BOOLEAN DoReadVerify);

public:
    // Starting ASN for writes
    ULONGLONG _StartingAsn;

    // Starting version for writes
    ULONGLONG _StartingVersion;

    // Log Write may be issued out of order in ASN space and this will control the degree to which this is done. 0 = sequential order
    ULONG _LogWriteReorderWindow;

    // Log tail cannot be longer than these many records
    ULONG _LogTailMaxLength;

    // Probability that a version change happens in terms of number of log writes
    ULONG _VersionChangeProbability;

    // Whena version change does happen, this is the maximum number of log records that need to be re-written with the new version number
    ULONG _VersionChangeOverlapMaxSize;

    // Probability of a log read  in terms of number of log writes
    ULONG _LogReadProbability;

    // Limits on User Metadata size
    ULONG _MinUserMetadataSize;
    ULONG _MaxUserMetadataSize;

    // Limits on User IoBuffers size
    ULONG _MinUserIoBufferSize;
    ULONG _MaxUserIoBufferSize;

private:
    KNodeListShared<AsyncLogStreamIoContext>    _LogWriteHistory;
    ULONGLONG                                   _HighestWrittenAsn;
    ULONGLONG                                   _LastTruncAsn;
    ULONGLONG                                   _WriteAsnLo;
    ULONGLONG                                   _WriteAsnHi;
    ULONGLONG                                   _CurrentVersion;
    ULONG                                       _LogTrimLength;
    AsyncLogStreamIoContext::SPtr               _LogWriteRepeat;

private:
    RvdLogAsn
    GetNextWriteAsn();
};

// This class is a test wrapper for RvdLogManager::AsyncCreateLog operation and is itself implemented as an async operation.
// It performs basic checks after the log operation completes. Any error condition encountered in the course of this test
// operation will cause it to complete with that error condition. If there is a possibility of multiple errors occurring,
// the first error condition that was encountered will cause completion. The other errors will be hidden but can be reported
// via trace statements.
class AsyncLogStreamIoEngine : public KAsyncContextBase
{
    K_FORCE_SHARED(AsyncLogStreamIoEngine);

public:
    static AsyncLogStreamIoEngine::SPtr
    Create();

    virtual VOID
    StartIoEngine(
        __in RvdLogStream::SPtr const& LogStream,
        __in KAsyncQueue<AsyncLogStreamIoContext>::SPtr IoRequestQueue,
        __in ULONG MaxOutstandingIoRequests,
        __in RvdLogAsn LowestAsnToWrite,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

private:
    VOID
    OnStart();

    VOID
    OnReuse();

    VOID
    IssueNextIo(__in_opt KAsyncContextBase* const Parent, __in KAsyncContextBase& CompletedContext);

    static VOID
    StaticStreamWriteCompleted(
        __in void* Context,
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletedContext);

    VOID
    StreamWriteCompleted(
        AsyncLogStreamIoContext::SPtr WriteReq,
        RvdLogStream::AsyncWriteContext&  CompletedWriteOp);

    static VOID
    StaticStreamReadAfterWriteCompleted(
        __in void* Context,
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletedContext);

    VOID
    StreamReadAfterWriteCompleted(
        AsyncLogStreamIoContext::SPtr WriteReq,
        RvdLogStream::AsyncReadContext&  CompletedReadOp);

    static VOID
    StaticStreamReReadDelayCompleted(
        __in void* WriteReqContext,
        __in_opt KAsyncContextBase* const IoEngineParent,
        __in KAsyncContextBase& CompletedContext);

    VOID
    StreamReReadDelayCompleted(AsyncLogStreamIoContext::SPtr WriteReq);

    VOID
    CommonCompleteOperation(NTSTATUS Status, AsyncLogStreamIoContext& Request);

    VOID
    CommonCompleteWriteOperation(AsyncLogStreamIoContext& WriteReq);

    NTSTATUS
    DoTruncate(RvdLogAsn TruncationPoint);

private:
    static ULONG const                          _MaxAsn = (1024 * 1024) - 1;
    RvdLogStream::SPtr                          _LogStream;
    KAsyncQueue<AsyncLogStreamIoContext>::SPtr  _IoRequestQueue;
    ULONG                                       _IoQueueDepth;
    LONG volatile                               _ActivityCount;
    NTSTATUS volatile                           _CompletionStatus;
    KAsyncContextBase::CompletionCallback       _IssueNextOperationCallback;
    ULONG                                       _ReadVerifyTries;
    KSpinLock                                   _TruncationLock;
        KBitmap                                 _CompletedAsnBitmap;
        RvdLogAsn                               _NextExpectedWrittenAsn;
        RvdLogAsn                               _HighestWrittenAsn;
        RvdLogAsn                               _HighestRequestedTruncationPoint;
        RvdLogAsn                               _HighestTruncationPoint;
        KNodeList<AsyncLogStreamIoContext>      _TruncateHoldList;

    KBuffer::SPtr                               _AsnBitmapBuffer;
 };

