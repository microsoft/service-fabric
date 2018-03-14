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

#include <ktllogger.h>

#include "CloseSync.h"

#define ALLOCATION_TAG 'abba'

class AsyncRWTStressContext : public KAsyncContextBase
{
    K_FORCE_SHARED(AsyncRWTStressContext);

    public:
        static NTSTATUS
        Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out AsyncRWTStressContext::SPtr& Context
        );

        VOID StartStress(
            __in KtlLogContainer::SPtr& LogContainer,
            __in ULONG RepeatCount,
            __in ULONG RecordCountPerIteration,
            __in ULONG RecordSize,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        );
        
    private:
        VOID RWTStressCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        );

        VOID
        CloseCompletion(
            __in KAsyncContextBase* const ParentAsync,
            __in KtlLogStream& LogStream,
            __in NTSTATUS Status
            );
        
        //
        // Initial -> CreateStream             # Creates a new stream for
        // CreateStream -> WriteRecords        # Write records in parallel
        // WriteRecords -> WaitForWriteRecords # Wait until all records written
        // WaitForWriteRecords -> ReadRecords  # Read records in parallel
        // ReadRecords -> WaitForReadRecords   # Wait until all records written
        // WaitForReadRecords -> Truncate      # Truncate stream completely
        // Truncate -> AdvanceASN              # Advance ASN to new value
        // AdvanceASN -> WriteRecords or Completed # Repeat if RepeatCount > 0
        // Completed -> CloseStream            # Close the stream
        //      
        enum FSMState { Initial, CreateStream,
                        WriteRecords, WaitForWritenRecords,
                        ReadRecords, WaitForReadRecords,
                        Truncate, AdvanceASN, CloseStream, Completed };
        
        FSMState _State;

        VOID NextState();
        
        VOID DoComplete(
            __in NTSTATUS Status
            );

        VOID RWTStressFSM(
            __in NTSTATUS Status,
            __in KAsyncContextBase& Async
        );
        
        VOID PickStreamId(
            );

        NTSTATUS PrepareRecordForWrite(
            __in KtlLogStream::SPtr& logStream,
            __in ULONGLONG version,
            __in KtlLogAsn recordAsn,
            __in ULONG myMetadataSize,
            __in ULONG dataBufferBlocks,
            __in ULONG dataBufferBlockSize,
            __out KIoBuffer::SPtr& dataIoBuffer,
            __out KIoBuffer::SPtr& metadataIoBuffer 
            );
        
    protected:
        VOID OnStart();
        
        VOID OnReuse();

    private:
        //
        // Parameters
        //
        KtlLogContainer::SPtr _LogContainer;
        ULONG _RepeatCount;
        ULONG _RecordCountPerIteration;
        ULONG _RecordSize;

        //
        // Internal
        //
        KtlLogStreamId _LogStreamId;
        KtlLogStream::SPtr _LogStream;
        KtlLogAsn _LastAsn;

        class PerRecord : public KObject<PerRecord>, public KShared<PerRecord>
        {
            friend AsyncRWTStressContext;
            
            K_FORCE_SHARED(PerRecord);

            public:
                KtlLogStream::AsyncWriteContext::SPtr _WriteContext;
                KtlLogStream::AsyncReadContext::SPtr _ReadContext;
                KIoBuffer::SPtr _DataIoBuffer;
                KIoBuffer::SPtr _MetadataIoBuffer;
                KIoBuffer::SPtr _ReadDataIoBuffer;
                KIoBuffer::SPtr _ReadMetadataIoBuffer;
                ULONGLONG _Version;
                ULONG _MetadataSize;
                ULONG _RetryCount;
                RvdLogAsn _RecordAsn;
        };

        KArray<PerRecord::SPtr> _PR;

        ULONG _RecordsWritten;
        ULONG _RecordsRead;
        NTSTATUS _LastStatus;
};

AsyncRWTStressContext::PerRecord::PerRecord()
{
}

AsyncRWTStressContext::PerRecord::~PerRecord()
{
}

NTSTATUS AsyncRWTStressContext::PrepareRecordForWrite(
    __in KtlLogStream::SPtr& logStream,
    __in ULONGLONG version,
    __in KtlLogAsn recordAsn,
    __in ULONG myMetadataSize, // = 0x100;
    __in ULONG dataBufferBlocks, // 8
    __in ULONG dataBufferBlockSize, // 16* 0x1000
    __out KIoBuffer::SPtr& dataIoBuffer,
    __out KIoBuffer::SPtr& metadataIoBuffer 
    )
{
    UNREFERENCED_PARAMETER(version);
    UNREFERENCED_PARAMETER(recordAsn);
    
    NTSTATUS status;
    
    //
    // Write records
    //
    {
        //
        // Build a data buffer (In different elements)
        //
        ULONG dataBufferSize = dataBufferBlockSize;

        status = KIoBuffer::CreateEmpty(dataIoBuffer,
                                        GetThisAllocator());
        KInvariant(NT_SUCCESS(status));

        for (ULONG i = 0; i < dataBufferBlocks; i++)
        {
            KIoBufferElement::SPtr dataElement;
            VOID* dataBuffer = NULL;

            status = KIoBufferElement::CreateNew(dataBufferSize,        // Size
                                                      dataElement,
                                                      dataBuffer,
                                                      GetThisAllocator());
            KInvariant(NT_SUCCESS(status));

            if (dataBuffer != NULL)  // Make compiler happy
            {
                for (int j = 0; j < 0x1000; j++)
                {
                    ((PUCHAR)dataBuffer)[j] = (UCHAR)(i*j);
                }
            }

            dataIoBuffer->AddIoBufferElement(*dataElement);
        }


        //
        // Build metadata
        //
        ULONG metadataBufferSize = ((logStream->QueryReservedMetadataSize() + myMetadataSize) + 0xFFF) & ~(0xFFF);        
        PVOID metadataBuffer;
        PUCHAR myMetadataBuffer;

        status = KIoBuffer::CreateSimple(metadataBufferSize,
                                         metadataIoBuffer,
                                         metadataBuffer,
                                         GetThisAllocator());
        KInvariant(NT_SUCCESS(status));

        myMetadataBuffer = ((PUCHAR)metadataBuffer)+ logStream->QueryReservedMetadataSize();
        for (ULONG i = 0; i < myMetadataSize; i++)
        {
            myMetadataBuffer[i] = (UCHAR)i;
        }

    }
    
    return(status);
}

VOID
AsyncRWTStressContext::CloseCompletion(
    __in KAsyncContextBase* const ParentAsync,
    __in KtlLogStream& LogStream,
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    UNREFERENCED_PARAMETER(LogStream);

    RWTStressFSM(Status, *this);
}


VOID AsyncRWTStressContext::RWTStressCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
)
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    RWTStressFSM(Async.Status(), Async);
}

VOID AsyncRWTStressContext::NextState(
    )
{
    if (_State == Initial)
    {
        _State = CreateStream;
        return;
    }

    if (_State == CreateStream)
    {
        _State = WriteRecords;
        return;
    }

    if (_State == WriteRecords)
    {
        _State = WaitForWritenRecords;
        return;
    }

    if (_State == WaitForWritenRecords)
    {
        if (NT_SUCCESS(_LastStatus))
        {
            _State = ReadRecords;
        } else {
            _State = Completed;
        }
        return;
    }

    if (_State == ReadRecords)
    {
        _State = WaitForReadRecords;
        return;
    }

    if (_State == WaitForReadRecords)
    {
        if (NT_SUCCESS(_LastStatus))
        {
            _State = Truncate;
        } else {
            _State = Completed;
        }
        return;
    }

    if (_State == Truncate)
    {
        _State = AdvanceASN;
        return;
    }

    if (_State == AdvanceASN)
    {
        if (_RepeatCount-- == 0)
        {
            _State = Completed;
        } else {
            _State = WriteRecords;
        }
        return;
    }
}

VOID AsyncRWTStressContext::PickStreamId(
    )
{
    KGuid logStreamGuid;
    KtlLogStreamId logStreamId;
    
    logStreamGuid.CreateNew();
    _LogStreamId = static_cast<KtlLogStreamId>(logStreamGuid);
}

VOID AsyncRWTStressContext::DoComplete(
    __in NTSTATUS Status
    )
{
    _LogContainer = nullptr;
    _LogStream = nullptr;

    for (ULONG i = 0; i < _RecordCountPerIteration; i++)
    {
        _PR[i] = nullptr;
    }

    Complete(Status);
}

VOID AsyncRWTStressContext::RWTStressFSM(
    __in NTSTATUS Status,
    __in KAsyncContextBase& Async
)
{
    KAsyncContextBase::CompletionCallback completion(this, &AsyncRWTStressContext::RWTStressCompletion);

    // TODO: Close Stream and container on error
    if (! NT_SUCCESS(Status) && (_State != WaitForWritenRecords) && (_State != WaitForReadRecords))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _State = Completed;
    }

    do
    {
        switch (_State)
        {
            case Initial:
            {
                BOOLEAN b;

                _LastStatus = STATUS_SUCCESS;
                
                Status = _PR.Reserve(_RecordCountPerIteration);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                }
                
                b = _PR.SetCount(_RecordCountPerIteration);
                if (! b)
                {                   
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(STATUS_INSUFFICIENT_RESOURCES);
                }

                for (ULONG i = 0; i < _RecordCountPerIteration; i++)
                {
                    _PR[i] = _new(ALLOCATION_TAG, GetThisAllocator()) PerRecord();
                    if (_PR[i] == nullptr)
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        DoComplete(STATUS_INSUFFICIENT_RESOURCES);
                    }

                }
                
                NextState();
                break;
            }

            case CreateStream:
            {
                KtlLogContainer::AsyncCreateLogStreamContext::SPtr createStreamAsync;
                ULONG metadataLength = 0x10000;

                PickStreamId();
                
                Status = _LogContainer->CreateAsyncCreateLogStreamContext(createStreamAsync);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                }

                KAssert(_LogStream == nullptr);
                KBuffer::SPtr securityDescriptor = nullptr;
                createStreamAsync->StartCreateLogStream(_LogStreamId,
                                                        nullptr,
                                                        nullptr,
                                                        securityDescriptor,
                                                        metadataLength,
                                                        DEFAULT_STREAM_SIZE,
                                                        DEFAULT_MAX_RECORD_SIZE,
                                                        _LogStream,
                                                        this,
                                                        completion);

                NextState();
                return;
            }

            case WriteRecords:
            {
                ULONGLONG version = 1;
                KtlLogAsn recordAsn;
                ULONG myMetadataSize = 0x100;
                ULONG blockSize;

                recordAsn = _LastAsn;
                _RecordsWritten = 0;
                for (ULONG i = 0; i < _RecordCountPerIteration; i++)
                {
                    Status = _LogStream->CreateAsyncWriteContext(_PR[i]->_WriteContext);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        DoComplete(Status);
                    }

                    //
                    // Split record into 8 KIoBufferElements for fun.
                    // So make sure record is at least 32K in size
                    //
                    KAssert(_RecordSize > (8 * 0x1000));
                    blockSize = _RecordSize / 8;
                    
                    recordAsn = recordAsn.Get() + 1;
                    Status = PrepareRecordForWrite(_LogStream,
                                                   version,
                                                   recordAsn,
                                                   myMetadataSize,// myMetadataSize
                                                   8,             // dataBufferBlocks
                                                   blockSize,   // dataBufferBlockSize
                                                   _PR[i]->_DataIoBuffer,
                                                   _PR[i]->_MetadataIoBuffer);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        DoComplete(Status);
                    }
                    
                    _PR[i]->_WriteContext->StartWrite(recordAsn,
                                                  version,
                                                  _LogStream->QueryReservedMetadataSize() + myMetadataSize,
                                                   _PR[i]->_MetadataIoBuffer,
                                                   _PR[i]->_DataIoBuffer,
                                                  NULL,    // Reservation
                                                  this,    // ParentAsync
                                                  completion);
                }
                
                NextState();                
                return;
            }

            case WaitForWritenRecords:
            {
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _LastStatus = Status;
                    KInvariant(FALSE);
                }
                
                _RecordsWritten++;
                
                if (_RecordsWritten == _RecordCountPerIteration)
                {               
                    // Advance to ReadRecords state
                    NextState();
                    break;
                }
                
                return;
            }

            case ReadRecords:
            {
                KtlLogAsn recordAsn;
                
                recordAsn = _LastAsn;
                _RecordsRead = 0;
                for (ULONG i = 0; i < _RecordCountPerIteration; i++)
                {
                    recordAsn = recordAsn.Get() + 1;
                    
                    Status = _LogStream->CreateAsyncReadContext(_PR[i]->_ReadContext);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        DoComplete(Status);
                    }

                    _PR[i]->_RetryCount = 0;
                    _PR[i]->_RecordAsn = recordAsn;
                    _PR[i]->_ReadContext->StartRead(_PR[i]->_RecordAsn,
                                             &_PR[i]->_Version,
                                             _PR[i]->_MetadataSize,
                                             _PR[i]->_ReadMetadataIoBuffer,
                                             _PR[i]->_ReadDataIoBuffer,
                                             this,    // ParentAsync
                                             completion);
                }

                
                NextState();                
                return;
            }

            case WaitForReadRecords:
            {
                // TODO: Verify dataIoBuffer and metadataIoBuffer

                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);

                    if (Status == STATUS_NOT_FOUND)
                    {
                        //
                        // Retry read once.
                        // There is a race condition in the core logger where for a specific record asn
                        // QueryRecords will return DispostionPersisted but the write has not actually
                        // completed yet. And so the condition where a write occurs and the shared write
                        // completes and thus the write is completed back to the application but the dedicated
                        // write is still in progress and then a dedicated read occurs and thus the read fails.
                        // There is code for LogicalLog streams to work
                        // around this but the  RWTAsync test does not take advantage of that code. 
                        //
                        ULONG index = (ULONG)-1;
                        for (ULONG i = 0; i < _RecordCountPerIteration; i++)
                        {
                            if (&Async == _PR[i]->_ReadContext.RawPtr())
                            {
                                index = i;
                                break;
                            }
                        }

                        if (index != (ULONG)-1)
                        {
                            if (_PR[index]->_RetryCount == 0)
                            {
                                _PR[index]->_ReadContext->Reuse();
                                _PR[index]->_ReadContext->StartRead(_PR[index]->_RecordAsn,
                                                         &_PR[index]->_Version,
                                                         _PR[index]->_MetadataSize,
                                                         _PR[index]->_ReadMetadataIoBuffer,
                                                         _PR[index]->_ReadDataIoBuffer,
                                                         this,    // ParentAsync
                                                         completion);
                                _PR[index]->_RetryCount++;
                                return;
                            }
                        } else {
                            KInvariant(FALSE);
                        }
                    }
                    _LastStatus = Status;
                    KInvariant(FALSE);
                }
                _RecordsRead++;

                if (_RecordsRead == _RecordCountPerIteration)
                {
                    // Advance to Truncate state
                    NextState();
                    break;
                }
                
                return;
            }
            
            case Truncate:
            {
                KtlLogStream::AsyncTruncateContext::SPtr truncateContext;

                Status = _LogStream->CreateAsyncTruncateContext(truncateContext);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                }

                KtlLogAsn truncateAsn = _LastAsn.Get() - 1;
                truncateContext->Truncate(truncateAsn,
                                               truncateAsn);
                
                NextState();                
                break;
            }

            case AdvanceASN:
            {
                _LastAsn = _LastAsn.Get() + _RecordCountPerIteration;
                NextState();
                break;
            }

            case Completed:
            {
                _State = CloseStream;

                if (_LogStream)
                {
                    KtlLogStream::CloseCompletionCallback closeCompletion(this, &AsyncRWTStressContext::CloseCompletion);
                    _LogStream->StartClose(this,
                                           closeCompletion);
                    _LogStream = nullptr;
                    return;
                }

                // fall through
            }

            case CloseStream:
            {
                DoComplete(_LastStatus);
                return;
            }

            default:
            {
                KAssert(FALSE);

                Status = STATUS_UNSUCCESSFUL;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
        }
#pragma warning(disable:4127)   // C4127: conditional expression is constant
    } while (TRUE);
}

VOID AsyncRWTStressContext::OnStart()
{   
    RWTStressFSM(STATUS_SUCCESS, *this); 
}

VOID AsyncRWTStressContext::StartStress(
    __in KtlLogContainer::SPtr& LogContainer,
    __in ULONG RepeatCount,
    __in ULONG RecordCountPerIteration,
    __in ULONG RecordSize,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _State = Initial;

    _LogContainer = LogContainer;
    _RepeatCount = RepeatCount;
    _RecordCountPerIteration = RecordCountPerIteration;
    _RecordSize = RecordSize;

    Start(ParentAsyncContext, CallbackPtr);
}
        

AsyncRWTStressContext::AsyncRWTStressContext() :
   _PR(GetThisAllocator())
{
}

AsyncRWTStressContext::~AsyncRWTStressContext()
{
}

VOID AsyncRWTStressContext::OnReuse()
{
    _LogStream = nullptr;
}

NTSTATUS AsyncRWTStressContext::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out AsyncRWTStressContext::SPtr& Context
    )
{
    NTSTATUS status;
    AsyncRWTStressContext::SPtr context;
    
    context = _new(AllocationTag, Allocator) AsyncRWTStressContext();
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

    context->_LastAsn = 10;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);     
}

VOID
RWTStress(
    KAllocator& Allocator,
    KGuid diskId,
    KtlLogManager::SPtr logManager,
    ULONG numberAsyncs = 64,
    ULONG numberRepeats = 8,
    ULONG numberRecordsPerIteration = 8,
    ULONG numberSizeOfRecord = 4 * 0x100000  // 4MB    
    )
{
    NTSTATUS status;

    KtlLogManager::AsyncCreateLogContainer::SPtr createAsync;
    LONGLONG logSize = DefaultTestLogFileSize;
    KtlLogContainer::SPtr logContainer;
    KGuid logContainerGuid;
    KtlLogContainerId logContainerId;
    KSynchronizer sync;
    
    logContainerGuid.CreateNew();
    logContainerId = static_cast<KtlLogContainerId>(logContainerGuid);

    status = logManager->CreateAsyncCreateLogContainerContext(createAsync);
    KInvariant(NT_SUCCESS(status));
    createAsync->StartCreateLogContainer(diskId,
                                         logContainerId,
                                         logSize,
                                             0,            // Max Number Streams
                                             0,            // Max Record Size
                                         logContainer,
                                         NULL,         // ParentAsync
                                         sync);
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    {
        KArray<KSynchronizer*> syncArr(Allocator);
        status = syncArr.Reserve(numberAsyncs);
        KInvariant(NT_SUCCESS(status));
        syncArr.SetCount(numberAsyncs);

        KArray<AsyncRWTStressContext::SPtr> asyncRWTStress(Allocator);
        asyncRWTStress.Reserve(numberAsyncs);
        asyncRWTStress.SetCount(numberAsyncs);
        KInvariant(NT_SUCCESS(status));
        
        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            syncArr[i] = _new(ALLOCATION_TAG, Allocator) KSynchronizer();
            KInvariant(syncArr[i] != NULL);
            KInvariant(NT_SUCCESS(syncArr[i]->Status()));
            
            status = AsyncRWTStressContext::Create(Allocator,
                                                     ALLOCATION_TAG,
                                                     asyncRWTStress[i]);
            KInvariant(NT_SUCCESS(status));
        }

        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            asyncRWTStress[i]->StartStress(logContainer,
                                           numberRepeats, // Repeat count
                                           numberRecordsPerIteration,
                                           numberSizeOfRecord,
                                           NULL,          // ParentAsync
                                           *syncArr[i]);
        }
        
        for (ULONG i = 0; i < numberAsyncs; i++)
        {
            status = syncArr[i]->WaitForCompletion();
            KInvariant(NT_SUCCESS(status));
            _delete(syncArr[i]);
        }
    }

    ContainerCloseSynchronizer closeContainerSync;
    logContainer->StartClose(NULL,
                        closeContainerSync.CloseCompletionCallback());

    status = closeContainerSync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));

    //
    // Delete the log container
    //
    {
        KtlLogManager::AsyncDeleteLogContainer::SPtr deleteAsync;
        
        status = logManager->CreateAsyncDeleteLogContainerContext(deleteAsync);
        KInvariant(NT_SUCCESS(status));
        deleteAsync->StartDeleteLogContainer(diskId,
                                             logContainerId,
                                             NULL,         // ParentAsync
                                             sync);
        status = sync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));
    }
}
