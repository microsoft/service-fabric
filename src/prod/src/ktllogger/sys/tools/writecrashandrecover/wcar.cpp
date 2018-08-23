// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>

#include "../../inc/ktllogger.h"
#include "InternalKtlLogger.h"
#include "KtlLogShimKernel.h"

#include <windows.h>

#if defined(PLATFORM_UNIX)
#include <stdlib.h>
#endif

#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

static const ULONG fourKB = 4 * 1024;
 
KAllocator* g_Allocator;

typedef struct
{
    PCHAR ResultName;
    PWCHAR SharedLogPath;
    PWCHAR DedicatedLogPath;
    BOOLEAN UseSparse;
    BOOLEAN UseSharedLog;
    ULONG StreamCount;
    ULONG WriteSizeInBytes;
    ULONG WritesOutstanding;
    ULONG DurationInSeconds;
    ULONG FileSizeIn4KB;
} TestCaseInformation;


//
// Oracle file consists of a 4K block for each stream
//
typedef struct
{
    KGuid StreamGuid;
    RvdLogAsn NextAsn;
    ULONGLONG LastVersion;
} OracleInformation;

static const ULONG OracleInfoOnDiskSize = fourKB;

#if !defined(PLATFORM_UNIX)
static const UCHAR DriveLetter = 'F';
#endif

TestCaseInformation TestInfo = 
    { "WCAR",
      L"f",
      L"f",
      TRUE,           // UseSparse
      TRUE,           // UseSharedLog
      1,              // StreamCount
      256 * 1024,       // Maximum WriteSizeInBytes
      1,             // WritesOutstanding
      30,             // DurationInSeconds
      256 * 1024      // FileSizeIn4KB (1GB)
    };

#if !defined(PLATFORM_UNIX)
#define OracleFileName L"KtlLogOracle.dat"
#define ContainerPathName L"KtlLogContainer.log"
#else
#define OracleFileName L"/tmp/KtlLogOracle.dat"
#define ContainerPathName L"/tmp/KtlLogContainer.log"
#endif

NTSTATUS BuildPathName(
    __out KWString& FileName,
#if !defined(PLATFORM_UNIX)
    __in KGuid DiskId,
#endif
    __in PWCHAR PathName
    )
{
#if !defined(PLATFORM_UNIX)
    KString::SPtr guidString;
    BOOLEAN b;
    static const ULONG GUID_STRING_LENGTH = 40;

    //
    // Caller us using the default file name as only specified the disk
    //
    // Filename expected to be of the form:
    //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\Testfile.dat"


    guidString = KString::Create(KtlSystem::GlobalNonPagedAllocator(),
                                 GUID_STRING_LENGTH);
    if (! guidString)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    b = guidString->FromGUID(DiskId);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   

    FileName = L"\\GLOBAL??\\Volume";
    FileName += static_cast<WCHAR*>(*guidString);
    FileName += KVolumeNamespace::PathSeparator;
    FileName += PathName;
#else
    FileName = PathName;
#endif

    return(FileName.Status());
}

//
// Common code
//
class WorkerAsync : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(WorkerAsync);

    
    public:     
        virtual VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

    protected:
        virtual VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
            )
        {
            UNREFERENCED_PARAMETER(Status);
            UNREFERENCED_PARAMETER(Async);
            
            Complete(STATUS_SUCCESS);
        }

        virtual VOID OnReuse() = 0;

        virtual VOID OnCompleted()
        {
        }
        
    private:
        VOID OnStart()
        {
            _Completion.Bind(this, &WorkerAsync::OperationCompletion);
            FSMContinue(STATUS_SUCCESS, *this);
        }

        VOID OnCancel()
        {
            FSMContinue(STATUS_CANCELLED, *this);
        }

        VOID OperationCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        )
        {
            UNREFERENCED_PARAMETER(ParentAsync);
            FSMContinue(Async.Status(), Async);
        }

    protected:
        KAsyncContextBase::CompletionCallback _Completion;
        
};

WorkerAsync::WorkerAsync()
{
}

WorkerAsync::~WorkerAsync()
{
}


class StreamCloseSynchronizer : public KObject<StreamCloseSynchronizer>
{
    K_DENY_COPY(StreamCloseSynchronizer);

public:

    FAILABLE
        StreamCloseSynchronizer(__in BOOLEAN IsManualReset = FALSE)
        : _Event(IsManualReset, FALSE),
        _CompletionStatus(STATUS_SUCCESS)
    {
            _Callback.Bind(this, &StreamCloseSynchronizer::AsyncCompletion);
            SetConstructorStatus(_Event.Status());
        }

    ~StreamCloseSynchronizer()
    {
    }

    KtlLogStream::CloseCompletionCallback CloseCompletionCallback()
    {
        return(_Callback);
    }

    NTSTATUS
        WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr)
    {
            KInvariant(!KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());

            BOOLEAN b = _Event.WaitUntilSet(TimeoutInMilliseconds);
            if (!b)
            {
                if (IsCompleted)
                {
                    *IsCompleted = FALSE;
                }

                return STATUS_IO_TIMEOUT;
            }
            else
            {
                if (IsCompleted)
                {
                    *IsCompleted = TRUE;
                }

                return _CompletionStatus;
            }
        }


    VOID
        Reset()
    {
            _Event.ResetEvent();
            _CompletionStatus = STATUS_SUCCESS;
        }


protected:
    VOID
        AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogStream& LogStream,
        __in NTSTATUS Status)
    {
            UNREFERENCED_PARAMETER(Parent);
            UNREFERENCED_PARAMETER(LogStream);

            _CompletionStatus = Status;

            _Event.SetEvent();
        }

private:
    KEvent                                      _Event;
    NTSTATUS                                    _CompletionStatus;
    KtlLogStream::CloseCompletionCallback       _Callback;
};


class ContainerCloseSynchronizer : public KObject<ContainerCloseSynchronizer>
{
    K_DENY_COPY(ContainerCloseSynchronizer);

public:

    FAILABLE
        ContainerCloseSynchronizer(__in BOOLEAN IsManualReset = FALSE)
        : _Event(IsManualReset, FALSE),
        _CompletionStatus(STATUS_SUCCESS)
    {
            _Callback.Bind(this, &ContainerCloseSynchronizer::AsyncCompletion);
            SetConstructorStatus(_Event.Status());
        }

    ~ContainerCloseSynchronizer()
    {
    }

    KtlLogContainer::CloseCompletionCallback CloseCompletionCallback()
    {
        return(_Callback);
    }

    NTSTATUS
        WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr)
    {
            KInvariant(!KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());

            BOOLEAN b = _Event.WaitUntilSet(TimeoutInMilliseconds);
            if (!b)
            {
                if (IsCompleted)
                {
                    *IsCompleted = FALSE;
                }

                return STATUS_IO_TIMEOUT;
            }
            else
            {
                if (IsCompleted)
                {
                    *IsCompleted = TRUE;
                }

                return _CompletionStatus;
            }
        }


    VOID
        Reset()
    {
            _Event.ResetEvent();
            _CompletionStatus = STATUS_SUCCESS;
        }


protected:
    VOID
        AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogContainer& LogContainer,
        __in NTSTATUS Status)
    {
            UNREFERENCED_PARAMETER(Parent);
            UNREFERENCED_PARAMETER(LogContainer);

            _CompletionStatus = Status;

            _Event.SetEvent();
        }

private:
    KEvent                                      _Event;
    NTSTATUS                                    _CompletionStatus;
    KtlLogContainer::CloseCompletionCallback       _Callback;
};


//
// CoreLogger Test code
//
NTSTATUS
VerifyRawRecordCallback(
    __in_bcount(MetaDataBufferSize) UCHAR const *const MetaDataBuffer,
    __in ULONG MetaDataBufferSize,
    __in const KIoBuffer::SPtr& IoBuffer
)
{
    UNREFERENCED_PARAMETER(MetaDataBuffer);
    UNREFERENCED_PARAMETER(MetaDataBufferSize);
    UNREFERENCED_PARAMETER(IoBuffer);

    // TODO: add validation
    
    return(STATUS_SUCCESS);
}

NTSTATUS SetupRecord(
    KtlLogStream::SPtr logStream,
    KIoBuffer::SPtr& MetadataIoBuffer,
    KIoBuffer::SPtr& DataIoBuffer,
    ULONGLONG version,
    ULONGLONG asn,
    PUCHAR data,
    ULONG dataSize,
    ULONG offsetToStreamBlockHeader,
    BOOLEAN IsBarrierRecord,
    ULONGLONG*
    )
{
    NTSTATUS status;
    KtlLogStreamId logStreamId;
    PVOID metadataBuffer;
    PVOID dataBuffer = NULL;
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
    PUCHAR dataPtr;
    ULONG firstBlockSize;
    ULONG secondBlockSize;
    ULONG offsetToDataLocation = offsetToStreamBlockHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);
    KIoBuffer::SPtr dataIoBufferRef;

    KIoBuffer::SPtr newMetadataBuffer;
    KIoBuffer::SPtr newDataIoBuffer;

    if (((DataIoBuffer == nullptr) || (DataIoBuffer->QuerySize() == 0)) &&
        (offsetToStreamBlockHeader >= KLogicalLogInformation::FixedMetadataSize))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Copy data into exclusive buffers
    //
    PVOID src, dest;

    status = KIoBuffer::CreateSimple(MetadataIoBuffer->QuerySize(), newMetadataBuffer, metadataBuffer, *g_Allocator);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    dest = (PVOID)newMetadataBuffer->First()->GetBuffer();
    src = (PVOID)MetadataIoBuffer->First()->GetBuffer();

    KMemCpySafe(src, MetadataIoBuffer->QuerySize(), dest, MetadataIoBuffer->QuerySize());
    MetadataIoBuffer = newMetadataBuffer;


    if ((DataIoBuffer->QuerySize() != 0) && (DataIoBuffer->QueryNumberOfIoBufferElements() == 1))
    {
        status = KIoBuffer::CreateSimple(DataIoBuffer->QuerySize(), newDataIoBuffer, dataBuffer, *g_Allocator);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        dest = (PVOID)newDataIoBuffer->First()->GetBuffer();
        src = (PVOID)DataIoBuffer->First()->GetBuffer();
        KMemCpySafe(dest, DataIoBuffer->QuerySize(), src, DataIoBuffer->QuerySize());
        DataIoBuffer = newDataIoBuffer;
    }
    else
    {
        if (DataIoBuffer->QuerySize() == 0)
        {
            dataBuffer = NULL;
        }
        else
        {
            dataBuffer = (PVOID)DataIoBuffer->First()->GetBuffer();
        }
    }

    logStream->QueryLogStreamId(logStreamId);

    metadataBlockHeader = (KLogicalLogInformation::MetadataBlockHeader*)((PUCHAR)metadataBuffer +
                                                                         logStream->QueryReservedMetadataSize());

    RtlZeroMemory(metadataBuffer, KLogicalLogInformation::FixedMetadataSize);
    metadataBlockHeader->OffsetToStreamHeader = offsetToStreamBlockHeader;
    if (IsBarrierRecord)
    {
        metadataBlockHeader->Flags = KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord;
    } else {
        metadataBlockHeader->Flags = 0;
    }

    //
    // Stream block header may not cross metadata/iodata boundary
    //
    KInvariant( (offsetToStreamBlockHeader <= (KLogicalLogInformation::FixedMetadataSize - sizeof(KLogicalLogInformation::StreamBlockHeader))) ||
                (offsetToStreamBlockHeader >= KLogicalLogInformation::FixedMetadataSize) );
    if (offsetToStreamBlockHeader >= KLogicalLogInformation::FixedMetadataSize)
    {
        if (dataBuffer == NULL)
        {
            return(STATUS_INVALID_PARAMETER);
        }

        streamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)((PUCHAR)dataBuffer +
                                                          (offsetToStreamBlockHeader - KLogicalLogInformation::FixedMetadataSize));
    } else {
        streamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)((PUCHAR)metadataBuffer + offsetToStreamBlockHeader);
    }

    RtlZeroMemory(streamBlockHeader, sizeof(KLogicalLogInformation::StreamBlockHeader));

    streamBlockHeader->Signature = KLogicalLogInformation::StreamBlockHeader::Sig;
    streamBlockHeader->StreamId = logStreamId;
    streamBlockHeader->StreamOffset = asn;
    streamBlockHeader->HighestOperationId = version;
    streamBlockHeader->DataSize = dataSize;

    if (offsetToDataLocation < KLogicalLogInformation::FixedMetadataSize)
    {
        firstBlockSize = KLogicalLogInformation::FixedMetadataSize - offsetToDataLocation;
        if (firstBlockSize > dataSize)
        {
            firstBlockSize = dataSize;
        }

        dataPtr = (PUCHAR)metadataBuffer + offsetToDataLocation;
        KMemCpySafe(dataPtr, MetadataIoBuffer->First()->QuerySize() - offsetToDataLocation, data, firstBlockSize);

        dataPtr = (PUCHAR)dataBuffer;
    } else {
        firstBlockSize = 0;
        dataPtr = (PUCHAR)dataBuffer + (offsetToDataLocation - KLogicalLogInformation::FixedMetadataSize);
    }
    secondBlockSize = dataSize - firstBlockSize;

    KMemCpySafe(dataPtr, dataSize - firstBlockSize, (data + firstBlockSize), secondBlockSize);

    status = KIoBuffer::CreateEmpty(dataIoBufferRef, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = dataIoBufferRef->AddIoBufferReference(*DataIoBuffer,
                                          0,
                                          (secondBlockSize + 0xfff) & ~0xfff);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    streamBlockHeader->DataCRC64 = KLogicalLogInformation::ComputeDataBlockCRC(streamBlockHeader,
                                                                               *DataIoBuffer,
                                                                               offsetToDataLocation);
    streamBlockHeader->HeaderCRC64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(streamBlockHeader);

    DataIoBuffer = dataIoBufferRef;
    return(status);
}

class KtlLogRunner : public WorkerAsync
{
    K_FORCE_SHARED(KtlLogRunner);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out KtlLogRunner::SPtr& Context
        )
        {
            NTSTATUS status;
            KtlLogRunner::SPtr context;
            
            context = _new(AllocationTag, Allocator) KtlLogRunner();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (! NT_SUCCESS(status))
            {
                return(status);
            }
            
            context->_LogStream = nullptr;
            context->_IsCancelled = FALSE;
            context->_LogStream = nullptr;
            context->_LogSpaceAllowed = 0;
            
            context->_HighestAsn = 0;

            context->_WriteContext = nullptr;
            context->_EmptyIoBuffer = nullptr;
            context->_IoBuffer = nullptr;
            context->_Metadata = nullptr;
            context->_DataWritten = nullptr;            
            
            Context = context.RawPtr();

            return(STATUS_SUCCESS);     
        }
        
        VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
        {
            UNREFERENCED_PARAMETER(Parameters);

            _State = Initial;
            
            Start(ParentAsyncContext, CallbackPtr);
        }

        void SetStartParameters(
            __in KtlLogStream& LogStream,
            __in KBlockFile& OracleFile,
            __in ULONGLONG OracleFileOffset,
            __in KGuid& StreamGuid,
            __in TestCaseInformation* TestInformation
        )
        {
            _LogStream = &LogStream;
            _OracleFile = &OracleFile;
            _OracleFileOffset = OracleFileOffset;
            _TestInformation = TestInformation;
            _StreamGuid = StreamGuid;
            
            _WriteSize = _TestInformation->WriteSizeInBytes;
            _UseSharedLog = _TestInformation->UseSharedLog;

            _LogSpaceAllowed = 384 * 1024 * 1024;
        }
        
    private:
        enum  FSMState { Initial = 0, RecoverAsnAndVerson=1, WriteRecord=2, WriteOracle=3, CrashOrContinue=4, Completed =5 };
        FSMState _State;

        VOID FSMContinue(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
            ) override
        {
            UNREFERENCED_PARAMETER(Async);
            
            if (_IsCancelled)
            {
                Complete(STATUS_CANCELLED);
                return;
            }

            // TODO: Do I want this ?? Or retry ???
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            switch (_State)
            {
                case Initial:
                {
                    srand((ULONG)GetTickCount());
                    
                    _OffsetToStreamBlockHeaderM = _LogStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

                    _Asn = KtlLogAsn::Min().Get();
                    _Version = 0;

                    _LastTruncationAsn = KtlLogAsn::Min().Get();

                    Status = _LogStream->CreateAsyncWriteContext(_WriteContext);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    Status = KIoBuffer::CreateEmpty(_EmptyIoBuffer, *g_Allocator);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    ULONG maxIoBufferSize = (_WriteSize + 0xfff) &~ 0xfff;
                    Status = KBuffer::Create(maxIoBufferSize + KLogicalLogInformation::FixedMetadataSize,
                                             _DataWritten,
                                             *g_Allocator);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    PVOID p;
                    Status = KIoBuffer::CreateSimple(fourKB, _OracleIoBuffer, p, GetThisAllocator());
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }
                    _OracleInfo = (OracleInformation*)p;


                    Status = KBuffer::Create(sizeof(KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation),
                                             _TailAndVersionBuffer,
                                             GetThisAllocator());
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }
                    
                    //
                    // Recover _NextAsn, _Version
                    //
                    KtlLogStream::AsyncIoctlContext::SPtr ioctl;
    
                    Status = _LogStream->CreateAsyncIoctlContext(ioctl);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    _State = RecoverAsnAndVerson;
                    ioctl->StartIoctl(KLogicalLogInformation::QueryLogicalLogTailAsnAndHighestOperationControl,
                                      NULL,
                                      _Result,
                                      _TailAndVersionBuffer,
                                      NULL,
                                      _Completion);                 
                    break;
                }

                case RecoverAsnAndVerson:
                {
                    KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation* tailAndVersion;
                    tailAndVersion = (KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation*)_TailAndVersionBuffer->GetBuffer();

                    _Asn = tailAndVersion->TailAsn;
                    _Version = tailAndVersion->HighestOperationCount;
                    
                    _State = WriteRecord;
                    // Fall through
                }

                case WriteRecord:
                {
WriteRecords:                   
#if 0  // Enable this to enable retries
                    if (! NT_SUCCESS(Status)) 
                    {
                        if (Status != STATUS_INSUFFICIENT_RESOURCES)
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        } else {
                            //
                            // Retry and hope for success next time
                            //
                            _RetryCount++;
                            KTraceFailedAsyncRequest(Status, this, _RetryCount, _IoBuffer->QuerySize());

                            if (_RetryCount == 256)
                            {
                                Complete(Status);
                                return;                     
                            }

                            _WriteContext->Reuse();
                            _WriteContext->StartWrite(_WriteAsn,
                                                     _Version,
                                                     KLogicalLogInformation::FixedMetadataSize,
                                                     _Metadata,
                                                     _IoBuffer,
                                                     0,    // Reservation
                                                     this,    // ParentAsync
                                                     _Completion);
                            return;
                        }
                    } else {
#endif
                        _RetryCount = 0;
                        ULONGLONG nextTruncation = _LastTruncationAsn + _LogSpaceAllowed;
                        if (_Asn >= nextTruncation)
                        {
                            KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                            Status = _LogStream->CreateAsyncTruncateContext(truncateContext);
                            if (! NT_SUCCESS(Status))
                            {
                                KTraceFailedAsyncRequest(Status, this, _State, 0);
                                Complete(Status);
                                return;
                            }
                            
                            ULONGLONG truncationAsn = _Asn - (_LogSpaceAllowed / 2);

                            truncateContext->Truncate(truncationAsn, truncationAsn);
                            _LastTruncationAsn = truncationAsn;
                        }

                        ULONG dataSizeWritten;
                        ULONG maxIoBufferSize;

                        dataSizeWritten = (rand() % (_WriteSize -1)) + 1;
                        
                        maxIoBufferSize = (_WriteSize + 0xfff) &~ 0xfff;

                        PVOID p;

                        Status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                                         _Metadata,
                                                         p,
                                                         *g_Allocator);
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        }

                        Status = KIoBuffer::CreateSimple(maxIoBufferSize, _IoBuffer, p, *g_Allocator);
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        }
                        
                        if (dataSizeWritten <= (_OffsetToStreamBlockHeaderM + sizeof(KLogicalLogInformation::StreamBlockHeader)))
                        {
                            _IoBuffer = _EmptyIoBuffer;
                        }

                        
                        PUCHAR dataWrittenPtr;

                        dataWrittenPtr = (PUCHAR)_DataWritten->GetBuffer();

                        _Version = _Version + 1;
                        Status = SetupRecord(_LogStream,
                                                     _Metadata,
                                                     _IoBuffer,
                                                     _Version,
                                                     _Asn,
                                                     dataWrittenPtr,
                                                     dataSizeWritten,
                                                     _OffsetToStreamBlockHeaderM,
                                                     TRUE,
                                                     NULL);

                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);
                            Complete(Status);
                            return;
                        }

                        _WriteAsn = _Asn;                       
                        _WriteContext->Reuse();

                        _WriteContext->StartWrite(_WriteAsn,
                                                 _Version,
                                                 KLogicalLogInformation::FixedMetadataSize,
                                                 _Metadata,
                                                 _IoBuffer,
                                                 0,    // Reservation
                                                 this,    // ParentAsync
                                                 _Completion);

                        _Asn = _Asn + dataSizeWritten;
                        _State = WriteOracle;
#if 0
                    }
#endif
                    break;                  
                }

                case WriteOracle:
                {
                    _OracleInfo->StreamGuid = _StreamGuid;
                    _OracleInfo->NextAsn = _Asn;
                    _OracleInfo->LastVersion = _Version;

                    _State = CrashOrContinue;
                    Status = _OracleFile->Transfer(KBlockFile::eForeground,
                                            KBlockFile::eWrite,
                                            _OracleFileOffset,
                                            *_OracleIoBuffer,
                                            _Completion);
                    break;
                }

                case CrashOrContinue:
                {
                    _State = WriteRecord;
                    goto WriteRecords;
                }

                default:
                {
                    KInvariant(FALSE);
                }
            }

            return;
        }

        
        VOID OnCancel() override
        {
            _IsCancelled = TRUE;
        }

        VOID OnReuse() override
        {
            _IsCancelled = FALSE;
        }

        VOID OnCompleted() override
        {
            _LogStream = nullptr;
            _TestInformation = nullptr;

            _WriteContext = nullptr;

            _EmptyIoBuffer = nullptr;
            _IoBuffer = nullptr;
            _Metadata = nullptr;
            _DataWritten = nullptr;
            
        }
        
    private:
        //
        // Parameters
        //
        KBlockFile::SPtr _OracleFile;
        ULONGLONG _OracleFileOffset;
        KtlLogStream::SPtr _LogStream;
        TestCaseInformation* _TestInformation;
        ULONG _WriteSize;
        BOOLEAN _UseSharedLog;
        KGuid _StreamGuid;

        //
        // Internal
        //
        KBuffer::SPtr _TailAndVersionBuffer;
        ULONG _Result;
        KtlLogStream::AsyncWriteContext::SPtr _WriteContext;
        KIoBuffer::SPtr _OracleIoBuffer;
        OracleInformation* _OracleInfo;
        KIoBuffer::SPtr _EmptyIoBuffer;
        KIoBuffer::SPtr _IoBuffer;
        KIoBuffer::SPtr _Metadata;
        KBuffer::SPtr _DataWritten;
        ULONG _OffsetToStreamBlockHeaderM;
        ULONG _RetryCount;
        ULONGLONG _LastTruncationAsn;
        ULONGLONG _Asn;
        ULONGLONG _WriteAsn;
        ULONGLONG _Version;
        ULONGLONG _LogSpaceAllowed;
        ULONGLONG _HighestAsn;

        BOOLEAN _IsCancelled;
};

KtlLogRunner::KtlLogRunner()
{
}

KtlLogRunner::~KtlLogRunner()
{
}

#if !defined(PLATFORM_UNIX)
NTSTATUS FindDiskIdForDriveLetter1(
    __in UCHAR DriveLetter1,
    __out GUID& DiskIdGuid
    )
{
    NTSTATUS status;
    KSynchronizer sync;

    KVolumeNamespace::VolumeInformationArray volInfo(KtlSystem::GlobalNonPagedAllocator());
    status = KVolumeNamespace::QueryVolumeListEx(volInfo, KtlSystem::GlobalNonPagedAllocator(), sync);
    if (!K_ASYNC_SUCCESS(status))
    {
        return status;
    }
    status = sync.WaitForCompletion();

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Find Drive volume GUID
    //
    ULONG i;
    for (i = 0; i < volInfo.Count(); i++)
    {
        if (volInfo[i].DriveLetter == DriveLetter1)
        {
            DiskIdGuid = volInfo[i].VolumeId;
            return(STATUS_SUCCESS);
        }
    }

    return(STATUS_UNSUCCESSFUL);
}
#endif

VOID HangForever()
{
    while(TRUE)
    {
        KNt::Sleep(60 * 60 * 1000);
    }
}

NTSTATUS VerifyTailAndVersion(
    KtlLogStream::SPtr LogStream,
    ULONGLONG ExpectedVersion,
    ULONGLONG ExpectedAsn
    )
{
    NTSTATUS status;
    KtlLogStream::AsyncIoctlContext::SPtr ioctl;
    KBuffer::SPtr tailAndVersionBuffer;
    KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation* tailAndVersion;
    KSynchronizer sync;
    ULONG result;

    status = KBuffer::Create(sizeof(KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation),
                             tailAndVersionBuffer,
                             *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = LogStream->CreateAsyncIoctlContext(ioctl);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    ioctl->StartIoctl(KLogicalLogInformation::QueryLogicalLogTailAsnAndHighestOperationControl,
                      NULL,
                      result,
                      tailAndVersionBuffer,
                      NULL,
                      sync);
    status =  sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    tailAndVersion = (KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation*)tailAndVersionBuffer->GetBuffer();
    if ((tailAndVersion->HighestOperationCount != ExpectedVersion) &&
        (tailAndVersion->HighestOperationCount != (ExpectedVersion+1)))
    {
        printf("tailAndVersion->HighestOperationCount (0x%llx) != ExpectedVersion (0x%llx)\n", tailAndVersion->HighestOperationCount, ExpectedVersion);
        HangForever();
    }
    
    if (tailAndVersion->TailAsn < ExpectedAsn)
    {
        printf("tailAndVersion->TailAsn (0x%llx) != ExpectedAsn (0x%llx)\n", tailAndVersion->TailAsn, ExpectedAsn);
        HangForever();
    }

    printf("Asn %lld\n", tailAndVersion->TailAsn);
    printf("Version %lld\n", tailAndVersion->HighestOperationCount);
    printf("\n");
    
    return(STATUS_SUCCESS);
}

NTSTATUS PerformKtlLogTestCase(
    __in TestCaseInformation* TestInfo1,
    __in KBlockFile& OracleFile,
    __in GUID DiskId
    )
{
    NTSTATUS status;
    NTSTATUS finalStatus = STATUS_SUCCESS;
    KSynchronizer sync;
    ULONG streamCount = TestInfo1->StreamCount;
    KtlLogManager::SPtr logManager;
    KArray<KGuid> streamGuids(*g_Allocator, streamCount, streamCount, 100);
    KArray<KtlLogStream::SPtr> streams(*g_Allocator, streamCount, streamCount, 100);
    KArray<KtlLogRunner::SPtr> klRunners(*g_Allocator, streamCount, streamCount, 100);
    KArray<KSynchronizer> klSyncs(*g_Allocator, streamCount, streamCount, 100);
    KWString pathName(*g_Allocator);
    KServiceSynchronizer        serviceSync;
    KBuffer::SPtr securityDescriptor = nullptr;
    KString::SPtr containerPath;
    KBlockFile::SPtr oracleFile = &OracleFile;
    KWString cPath(*g_Allocator);
    KtlLogManager::AsyncOpenLogContainer::SPtr openAsync;
    ULONG r;
    GUID nullGUID = { 0 };
    KtlLogContainerId nullLogContainerId = static_cast<KtlLogContainerId>(nullGUID);        

    srand((ULONG)GetTickCount());

#ifdef UPASSTHROUGH
    status = KtlLogManager::CreateInproc(KTL_TAG_TEST, *g_Allocator, logManager);
#else
    status = KtlLogManager::CreateDriver(ALLOCATION_TAG, *g_Allocator, logManager);
#endif
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = serviceSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    
    //
    // Configure logger with no throttling limits
    //
    KtlLogManager::MemoryThrottleLimits* memoryThrottleLimits;
    KtlLogManager::AsyncConfigureContext::SPtr configureContext;
    KBuffer::SPtr inBuffer;
    KBuffer::SPtr outBuffer;
    ULONG result;
    // {A2663496-BD65-4a87-AA9D-762B4F6FE1C8}
    KGuid logContainerGuid(0xa2663496, 0xbd65, 0x4a87, 0xaa, 0x9d, 0x76, 0x2b, 0x4f, 0x6f, 0xe1, 0xc8);
    KtlLogContainerId logContainerId;
    ContainerCloseSynchronizer closeContainerSync;
    KtlLogContainer::SPtr logContainer;
    KtlLogContainer::AsyncOpenLogStreamContext::SPtr openStreamAsync;   

    status = logManager->CreateAsyncConfigureContext(configureContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        goto done;
    }

    status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleLimits),
                             inBuffer,
                             *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        goto done;
    }

    //
    // Configure defaults
    //
    memoryThrottleLimits = (KtlLogManager::MemoryThrottleLimits*)inBuffer->GetBuffer();
    memoryThrottleLimits->WriteBufferMemoryPoolMax = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits->WriteBufferMemoryPoolMin = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits->WriteBufferMemoryPoolPerStream = KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStream;
    memoryThrottleLimits->PinnedMemoryLimit = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    memoryThrottleLimits->PeriodicFlushTimeInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec;
    memoryThrottleLimits->PeriodicTimerIntervalInSec = KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec;
    memoryThrottleLimits->AllocationTimeoutInMs = KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMs;        
    memoryThrottleLimits->MaximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_NoLimit;
    
    configureContext->Reuse();
    configureContext->StartConfigure(KtlLogManager::ConfigureMemoryThrottleLimits2,
                                     inBuffer.RawPtr(),
                                     result,
                                     outBuffer,
                                     NULL,
                                     sync);
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        goto done;
    }
    
    //
    // Open container and streams, verifying against oracle
    //
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = BuildPathName(cPath,
#if !defined(PLATFORM_UNIX)
                           DiskId,
#endif
                           ContainerPathName);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = KString::Create(containerPath,
                             *g_Allocator,
                             (PWCHAR)cPath,
                             TRUE);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = logManager->CreateAsyncOpenLogContainerContext(openAsync);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    openAsync->StartOpenLogContainer(*containerPath,
                                         nullLogContainerId,
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = logContainer->CreateAsyncOpenLogStreamContext(openStreamAsync);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    for (ULONG i = 0; i < streamCount; i++)
    {
        KIoBuffer::SPtr oracleIoBuffer;
        OracleInformation* oracleInfo;
        PVOID p;

        status = KIoBuffer::CreateSimple(OracleInfoOnDiskSize, oracleIoBuffer, p, *g_Allocator);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        
        status = oracleFile->Transfer(KBlockFile::eForeground,
                                KBlockFile::eRead,
                                i * OracleInfoOnDiskSize,
                                *oracleIoBuffer,
                                sync);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

        oracleInfo = (OracleInformation*)oracleIoBuffer->First()->GetBuffer();


        streamGuids[i] = oracleInfo->StreamGuid;

        RvdLogUtility::AsciiGuid g(streamGuids[i]);
        
        ULONG openedMetadataLength;
        openStreamAsync->Reuse();
        openStreamAsync->StartOpenLogStream(streamGuids[i],
                                                &openedMetadataLength,
                                                streams[i],
                                                NULL,    // ParentAsync
                                                sync);

        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }           

        //
        // Validate against oracle
        //              
        VerifyTailAndVersion(streams[i],
                             oracleInfo->LastVersion,
                             oracleInfo->NextAsn.Get());
        
        if (! TestInfo1->UseSharedLog)
        {
            KtlLogStream::AsyncIoctlContext::SPtr ioctl;
            status = streams[i]->CreateAsyncIoctlContext(ioctl);
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, nullptr, 0, 0);
                goto done;
            }

            KBuffer::SPtr inBuffer1, outBuffer1;
            ULONG result1;

            inBuffer1 = nullptr;

            ioctl->StartIoctl(KLogicalLogInformation::WriteOnlyToDedicatedLog, inBuffer1.RawPtr(), result1, outBuffer1, NULL, sync);
            status = sync.WaitForCompletion();
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, nullptr, 0, 0);
                goto done;
            }           
        }               
    }

    printf("All streams vaidated\n");
    //
    // Create the cl runners
    //
    for (ULONG i = 0; i < streamCount; i++)
    {
        KtlLogRunner::SPtr klRunner;
        
        status = KtlLogRunner::Create(*g_Allocator, KTL_TAG_TEST, klRunner);
        
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

        klRunner->SetStartParameters(*streams[i],
                                     *oracleFile,
                                     i * OracleInfoOnDiskSize,
                                     streamGuids[i],
                                     TestInfo1);

        klRunners[i] = klRunner;        
    }

    //
    // Start the cl runners
    //
    for (ULONG i = 0; i < streamCount; i++)
    {
        klRunners[i]->StartIt(nullptr,
                                nullptr,
                                klSyncs[i]);
    }


    //
    // Delay for the duration of the test
    //
    KNt::Sleep(TestInfo1->DurationInSeconds * 1000);

    //
    // Maybe kill the process somehow
    //
    r = rand() % 3;
    switch(r)
    {
        case 0:
        {
            printf("Exit by Crash\n");
            int* pp = nullptr;
            int p = *pp;
            UNREFERENCED_PARAMETER(p);
            KInvariant(FALSE);
        }

        case 1:
        {
            printf("Exit by ExitProcess\n");
#if !defined(PLATFORM_UNIX)
            ExitProcess(0);
#else
            exit(0);
#endif
        }
    }

    printf("Exit Gracefully\n");
    
    //
    // Cancel file runners and then wait for completion
    //
    for (ULONG i = 0; i < streamCount; i++)
    {
        klRunners[i]->Cancel();
    }
    
    for (ULONG i = 0; i < streamCount; i++)
    {
        status = klSyncs[i].WaitForCompletion();
        if (status != STATUS_CANCELLED)
        {
            finalStatus = status;
        }
    }


done:
    for (ULONG i = 0; i < streamCount; i++)
    {
        klRunners[i] = nullptr;
    }

    for (ULONG i = 0; i < streamCount; i++)
    {
        StreamCloseSynchronizer closeStreamSync;
        
        streams[i]->StartClose(NULL,
                               closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
    }
    
    logContainer->StartClose(NULL,
                             closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    openAsync = nullptr;
    configureContext = nullptr;
    openStreamAsync = nullptr;
    logContainer = nullptr;
    configureContext = nullptr;
    for (ULONG i = 0; i < streamCount; i++)
    {
        streams[i] = nullptr;
    }
    
    status = logManager->StartCloseLogManager(NULL, // ParentAsync
                                              serviceSync.CloseCompletionCallback());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    logManager = nullptr;
    status = serviceSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    
    return(finalStatus);
}

NTSTATUS CreateContainersAndLogs(
    __in KWString& OraclePathName1,
    __in ULONG StreamCount,
    __in KGuid DiskId,
    __in TestCaseInformation* TestInfo1
    )
{
    NTSTATUS status;
    KBlockFile::SPtr oracleFile;    
    ULONG oracleFileSize = StreamCount * OracleInfoOnDiskSize;
    KIoBuffer::SPtr oracleIoBuffer;
    PVOID p;
    OracleInformation* oracleInfo;
    KSynchronizer sync;
    ContainerCloseSynchronizer closeContainerSync;  

    status = KIoBuffer::CreateSimple(OracleInfoOnDiskSize, oracleIoBuffer, p, *g_Allocator);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    oracleInfo = (OracleInformation*)p;
    
    //
    // Create oracle and logs
    //
    status = KBlockFile::Create(OraclePathName1,
                            TRUE,
                            KBlockFile::eCreateAlways,
                            KBlockFile::eInheritFileSecurity,
                            oracleFile,
                            sync,
                            NULL,        // Parent
                            *g_Allocator);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = sync.WaitForCompletion();      
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = oracleFile->SetFileSize((ULONGLONG)oracleFileSize,
                                      sync);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    KtlLogManager::SPtr logManager;
    KServiceSynchronizer        serviceSync;
    KBuffer::SPtr securityDescriptor = nullptr;
    KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
    KtlLogManager::AsyncCreateLogContainer::SPtr createContainerAsync;
    LONGLONG logSize = 0x200000000;  // 8GB
    KString::SPtr containerPath;
    KtlLogContainer::SPtr logContainer;
    
    status = KtlLogManager::CreateInproc(KTL_TAG_TEST, *g_Allocator, logManager);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = logManager->StartOpenLogManager(NULL, // ParentAsync
                                             serviceSync.OpenCompletionCallback());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    status = serviceSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    KWString cPath(*g_Allocator);
    // {A2663496-BD65-4a87-AA9D-762B4F6FE1C8}
    KGuid logContainerGuid(0xa2663496, 0xbd65, 0x4a87, 0xaa, 0x9d, 0x76, 0x2b, 0x4f, 0x6f, 0xe1, 0xc8);
    KtlLogContainerId logContainerId;
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);


    status = BuildPathName(cPath,
#if !defined(PLATFORM_UNIX)
                           DiskId,
#endif
                           ContainerPathName);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = KString::Create(containerPath,
                             *g_Allocator,
                             (PWCHAR)cPath,
                             TRUE);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
        
    status = logManager->CreateAsyncCreateLogContainerContext(createContainerAsync);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    createContainerAsync->StartCreateLogContainer(*containerPath,
                                                  logContainerId,
                                                  logSize,
                                                  0,            // Max Number Streams
                                                  0,            // Max Record Size
                                                  logContainer,
                                                  NULL,         // ParentAsync
                                                  sync);
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
    
    status = logContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
        
    for (ULONG i = 0; i < StreamCount; i++)
    {
        KString::SPtr streamPath;
        KWString s(*g_Allocator);
        KWString guidName(*g_Allocator);
        KGuid logStreamGuid;
        KtlLogStreamId logStreamId;
        ULONG metadataLength = 0x10000;
        const LONGLONG StreamSize = 512 * 1024 * 1024;
        KtlLogStream::SPtr stream;
            
        logStreamGuid.CreateNew();
        logStreamId = static_cast<KtlLogStreamId>(logStreamGuid);

#if !defined(PLATFORM_UNIX)
        guidName = L"";
#else
        guidName = L"/tmp/";
#endif
        guidName += logStreamGuid;
        
        status = BuildPathName(s,
#if !defined(PLATFORM_UNIX)
                               DiskId,
#endif
                               (PWCHAR)guidName);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

                
        status = KString::Create(streamPath,
                                 *g_Allocator,
                                 (PWCHAR)s,
                                 TRUE);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        
        createStreamAsync->Reuse();
        createStreamAsync->StartCreateLogStream(logStreamId,
                                                KLogicalLogInformation::GetLogicalLogStreamType(),
                                                nullptr,           // Alias
                                                KString::CSPtr(streamPath.RawPtr()),
                                                securityDescriptor,                                             
                                                metadataLength,
                                                StreamSize,
                                                1024*1024,  // 1MB
                                                TestInfo1->UseSparse ? KtlLogManager::FlagSparseFile : 0,
                                                stream,
                                                NULL,    // ParentAsync
                                                sync);
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

        
        StreamCloseSynchronizer closeStreamSync;
        
        stream->StartClose(NULL,
                               closeStreamSync.CloseCompletionCallback());

        status = closeStreamSync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        
        oracleInfo->StreamGuid = logStreamGuid;
        oracleInfo->NextAsn = KtlLogAsn::Min();
        oracleInfo->LastVersion = 0;
        status = oracleFile->Transfer(KBlockFile::eForeground,
                                KBlockFile::eWrite,
                                i * OracleInfoOnDiskSize,
                                *oracleIoBuffer,
                                sync);

        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }
        status = sync.WaitForCompletion();
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }

    }

    oracleFile = nullptr;

    logContainer->StartClose(NULL,
                             closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS TestSequence()
{
    NTSTATUS status;
    KBlockFile::SPtr oracleFile;
    KWString oraclePathName(*g_Allocator);
    KSynchronizer sync;
    GUID volumeGuid;

#if !defined(PLATFORM_UNIX)
    status = FindDiskIdForDriveLetter1(DriveLetter,
                                       volumeGuid);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }
#endif

    //
    // Build the filename appropriately
    //  
    status = BuildPathName(oraclePathName,
#if !defined(PLATFORM_UNIX)
                           volumeGuid,
#endif
                           OracleFileName);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    //
    // See if the oracle exists and if so then verify results for the
    // streams.
    //
    status = KBlockFile::Create(oraclePathName,
                            TRUE,
                            KBlockFile::eOpenExisting,
                            KBlockFile::eInheritFileSecurity,
                            oracleFile,
                            sync,
                            NULL,        // Parent
                            *g_Allocator);    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {

        status = CreateContainersAndLogs(oraclePathName,
                                         TestInfo.StreamCount,
                                         volumeGuid,
                                         &TestInfo);
        
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);
            return(status);
        }       
    } else if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);     
    } else {
        oracleFile = nullptr;
    }

    //
    // Recover oracle information 
    //  
    status = KBlockFile::Create(oraclePathName,
                            TRUE,
                            KBlockFile::eOpenAlways,
                            KBlockFile::eInheritFileSecurity,
                            oracleFile,
                            sync,
                            NULL,        // Parent
                            *g_Allocator);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, nullptr, 0, 0);
        return(status);
    }

    status = PerformKtlLogTestCase(&TestInfo, *oracleFile, volumeGuid);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    oracleFile->Close();

    //
    // Sleep to give time for asyncs to be cleaned up before allocation
    // check. It requires a context switch and a little time on UP
    // machines.
    //
    KNt::Sleep(500);

    return STATUS_SUCCESS;
}


NTSTATUS
TheMain(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);


    EventRegisterMicrosoft_Windows_KTL();

    KtlSystem* system = nullptr;
    NTSTATUS Result = KtlSystem::Initialize(FALSE, &system);
    KInvariant(NT_SUCCESS(Result));


    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

#ifdef UDRIVER
    //
    // For UDRIVER, need to perform work done in PNP Device Add
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(*g_Allocator, KTL_TAG_TEST);
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        return(status);
    }
#endif
    
    Result = TestSequence();

#ifdef UDRIVER
    NTSTATUS status;
    status = FileObjectTable::StopAndUnregisterOverlayManager(*g_Allocator);
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        return(status);
    }
#endif
    
    EventUnregisterMicrosoft_Windows_KTL();
    
    KtlSystem::Shutdown();

    return Result;
}

#define CONVERT_TO_ARGS(argc, cargs) \
    std::vector<WCHAR*> args_vec(argc);\
    WCHAR** args = (WCHAR**)args_vec.data();\
    std::vector<std::wstring> wargs(argc);\
    for (int iter = 0; iter < argc; iter++)\
    {\
        wargs[iter] = Utf8To16(cargs[iter]);\
        args[iter] = (WCHAR*)(wargs[iter].data());\
    }\

#if !defined(PLATFORM_UNIX)
int
wmain(int argc, WCHAR* args[])
{
    return TheMain(argc, args);
}
#else
#include <vector>
int main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs);
    return TheMain(argc, args); 
}
#endif
