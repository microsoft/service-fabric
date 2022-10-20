// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

class LongRunningWriteStress : public WorkerAsync
{
    K_FORCE_SHARED(LongRunningWriteStress);

public:
    //
    // Parameters
    //
    struct StartParameters
    {
        KtlLogStream::SPtr LogStream;
        ULONG MaxRandomRecordSize;
        BOOLEAN UseFixedRecordSize;
        ULONGLONG LogSpaceAllowed;
        ULONGLONG HighestAsn;
        ULONG WaitTimerInMs;
    };

    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;

        _LogStream = startParameters->LogStream;
        _MaxRandomRecordSize = startParameters->MaxRandomRecordSize;
        _LogSpaceAllowed = startParameters->LogSpaceAllowed;
        _HighestAsn = startParameters->HighestAsn;
        _UseFixedRecordSize = startParameters->UseFixedRecordSize;
        _WaitTimerInMs = startParameters->WaitTimerInMs;

        Start(ParentAsyncContext, CallbackPtr);
    }
    
private:
    KtlLogStream::SPtr _LogStream;
    ULONG _MaxRandomRecordSize;
    ULONGLONG _LogSpaceAllowed;
    ULONGLONG _HighestAsn;
    BOOLEAN _UseFixedRecordSize;
    ULONG _WaitTimerInMs;

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out LongRunningWriteStress::SPtr& Context
        )
    {
            NTSTATUS status;
            LongRunningWriteStress::SPtr context;

            context = _new(AllocationTag, Allocator) LongRunningWriteStress();
            if (context == nullptr)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return(status);
            }

            status = context->Status();
            if (!NT_SUCCESS(status))
            {
                return(status);
            }

            context->_LogStream = nullptr;
            context->_MaxRandomRecordSize = 0;
            context->_LogSpaceAllowed = 0;
            context->_HighestAsn = 0;
            context->_UseFixedRecordSize = FALSE;

            context->_WriteContext = nullptr;
            context->_TruncateContext = nullptr;
            context->_EmptyIoBuffer = nullptr;
            context->_IoBuffer = nullptr;
            context->_Metadata = nullptr;
            context->_DataWritten = nullptr;
            context->_WaitTimerInMs = 0;
            context->_WaitTimer = nullptr;

            Context = context.RawPtr();

            return(STATUS_SUCCESS);
        }

protected:
    //
    // State Machine
    //
    VOID OnReuse() override
    {
    }

    enum  FSMState { Initial = 0, WriteRecord = 1, WriteRecordWait = 2, Completed = 3 };
    FSMState _State;
    
    VOID FSMContinue(
        __in NTSTATUS Status
        ) override
    {
        switch (_State)
        {
            case Initial:
            {
                _OffsetToStreamBlockHeaderM = _LogStream->QueryReservedMetadataSize() + sizeof(KLogicalLogInformation::MetadataBlockHeader);

                _Asn = KtlLogAsn::Min().Get();
                _Version = 0;

                _LastTruncationAsn = KtlLogAsn::Min().Get();
                
                Status = _LogStream->CreateAsyncWriteContext(_WriteContext);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));


                Status = _LogStream->CreateAsyncTruncateContext(_TruncateContext);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));


                Status = KIoBuffer::CreateEmpty(_EmptyIoBuffer, *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                ULONG maxIoBufferSize = (_MaxRandomRecordSize + 0xfff) &~ 0xfff;
                Status = KBuffer::Create(maxIoBufferSize + KLogicalLogInformation::FixedMetadataSize,
                                         _DataWritten,
                                         *g_Allocator);
                VERIFY_IS_TRUE(NT_SUCCESS(Status));

                Status = KTimer::Create(_WaitTimer, GetThisAllocator(), GetThisAllocationTag());
                VERIFY_IS_TRUE(NT_SUCCESS(Status));
                
                _State = WriteRecord;
                // fall through
            }

            case WriteRecord:
            {
WriteRecord:
                _RetryCount = 0;
                ULONGLONG nextTruncation = _LastTruncationAsn + _LogSpaceAllowed;
                if (_Asn >= nextTruncation)
                {
                    ULONGLONG truncationAsn = _Asn - (_LogSpaceAllowed / 2);

                    _TruncateContext->Reuse();                  
                    _TruncateContext->Truncate(truncationAsn, truncationAsn);
                    _LastTruncationAsn = truncationAsn;
                }
                
                if (_Asn < _HighestAsn)
                {
                    ULONG dataSizeWritten;
                    ULONG maxIoBufferSize;
                    
                    maxIoBufferSize = (_MaxRandomRecordSize + 0xfff) &~ 0xfff;
                    if (_UseFixedRecordSize)
                    {
                        dataSizeWritten = _MaxRandomRecordSize;
                    } else {
                        dataSizeWritten = rand() % _MaxRandomRecordSize; 
                    }
                    
                    PVOID p;
                    
                    Status = KIoBuffer::CreateSimple(KLogicalLogInformation::FixedMetadataSize,
                                                     _Metadata,
                                                     p,
                                                     *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));
                    
                    Status = KIoBuffer::CreateSimple(maxIoBufferSize, _IoBuffer, p, *g_Allocator);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status));
                    
                    if (dataSizeWritten <= (_OffsetToStreamBlockHeaderM + sizeof(KLogicalLogInformation::StreamBlockHeader)))
                    {
                        _IoBuffer = _EmptyIoBuffer;
                    }

                    PUCHAR dataWrittenPtr;

                    dataWrittenPtr = (PUCHAR)_DataWritten->GetBuffer();
                    for (ULONG i = 0; i < dataSizeWritten; i++)
                    {
                        dataWrittenPtr[i] = (UCHAR)(_Asn + i);  // TODO: a better way to unique this
                    }
                    
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
                                                 NULL,
                                                 [](KLogicalLogInformation::StreamBlockHeader* streamBlockHeader){ UNREFERENCED_PARAMETER(streamBlockHeader);});

                    VERIFY_IS_TRUE(NT_SUCCESS(Status));

                    _State = WriteRecordWait;
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
                    
                } else {
                    _State = Completed;
                    Complete(STATUS_SUCCESS);
                    return;
                }
                break;
            }

            case WriteRecordWait:
            {
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
                }

                _State = WriteRecord;
                if (_WaitTimerInMs != 0)
                {
                    _WaitTimer->Reuse();
                    _WaitTimer->StartTimer(_WaitTimerInMs, this, _Completion);
                } else {
                    goto WriteRecord;
                }
                
                break;
            }

            default:
            {
                VERIFY_IS_TRUE(FALSE);
            }
        }

        return;
    }

    VOID OnCompleted() override
    {
        _WriteContext = nullptr;
        _TruncateContext = nullptr;
        
        _EmptyIoBuffer = nullptr;
        _IoBuffer = nullptr;
        _Metadata = nullptr;
        _DataWritten = nullptr;
    }

public:
    ULONGLONG GetAsn() { return(_Asn); };
    ULONGLONG GetVersion() { return(_Version); };
    ULONGLONG GetTruncationAsn() { return(_LastTruncationAsn); };
    ULONGLONG ForceCompletion() { ULONGLONG a = _Asn; _HighestAsn = _Asn; return(a); };
private:
    //
    // Internal
    //
    KTimer::SPtr _WaitTimer;
    KtlLogStream::AsyncWriteContext::SPtr _WriteContext;
    KtlLogStream::AsyncTruncateContext::SPtr _TruncateContext;
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
};

