// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

class RvdLogStreamShim  : public RvdLogStream
{
    K_FORCE_SHARED_WITH_INHERITANCE(RvdLogStreamShim);

public:
    //
    // Control interfaces
    //
    inline VOID SetRvdLogStream(
        __in RvdLogStream& CoreLogStream
    )
    {
        _CoreLogStream = &CoreLogStream;
    }

    inline RvdLogStream::SPtr GetRvdLogStream()
    {
        return(_CoreLogStream);
    }
    
    inline VOID SetTestCaseData(
        __in ULONG TestCaseId,
        __in PVOID TestCaseData
    )
    {
        _TestCaseId = TestCaseId;
        _TestCaseData = TestCaseData;
    }

    inline ULONG GetTestCaseId()
    {
        return(_TestCaseId);
    }

    inline PVOID GetTestCaseData()
    {
        return(_TestCaseData);
    }
    
protected:
    ULONG _TestCaseId;
    PVOID _TestCaseData;
    
    //
    // RvdLogStream Interfaces
    //
public:
    LONGLONG
    QueryLogStreamUsage() override ;
    
    VOID
    QueryLogStreamType(__out RvdLogStreamType& LogStreamType) override ;

    VOID
    QueryLogStreamId(__out RvdLogStreamId& LogStreamId) override ;

    NTSTATUS
    QueryRecordRange(
        __out_opt RvdLogAsn* const LowestAsn,
        __out_opt RvdLogAsn* const HighestAsn,
        __out_opt RvdLogAsn* const LogTruncationAsn) override ;

    ULONGLONG QueryCurrentReservation() override ;

    NTSTATUS
    SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal) override ;
    

#if 0   // TODO; AsyncWriteContextShim
    class AsyncWriteContextShim : public RvdLogStream::AsyncWriteContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContextShim);

    public:

        VOID
        StartWrite(
            __in BOOLEAN LowPriorityIO,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartWrite(
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        VOID
        StartReservedWrite(
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
        
        VOID
        StartReservedWrite(
            __in BOOLEAN LowPriorityIO,
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
    };
#endif
    
    NTSTATUS
    CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) override ;

#if 0   // TODO: AsyncReadContextShim
    class AsyncReadContextShim : public RvdLogStream::AsyncReadContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadContextShim);

    public:
        VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

        typedef enum { ReadTypeNotSpecified, ReadExactRecord, ReadNextRecord, ReadPreviousRecord, ReadContainingRecord, 
                       ReadNextFromSpecificAsn, ReadPreviousFromSpecificAsn }
        ReadType;
        
        VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __in RvdLogStream::AsyncReadContext::ReadType Type,
            __out_opt RvdLogAsn* const ActualRecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;        
    };
#endif
    
    NTSTATUS
    CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) override ;    
            
    NTSTATUS
    QueryRecord(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize = nullptr,
        __out_opt ULONGLONG* const DebugInfo1 = nullptr) override ;

    
    NTSTATUS
    QueryRecord(
        __inout RvdLogAsn& RecordAsn,
        __in RvdLogStream::AsyncReadContext::ReadType Type,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize = nullptr,
        __out_opt ULONGLONG* const DebugInfo1 = nullptr) override ;

    
    NTSTATUS
    QueryRecords(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RvdLogStream::RecordMetadata>& ResultsArray) override ;

    NTSTATUS
    DeleteRecord(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version) override ;
    
    VOID
    Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint) override ;

    VOID
    TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version) override ;

#if 0    // TODO; AsyncReservationContextShim
    class AsyncReservationContextShim : public RvdLogStream::AsyncReservationContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncReservationContextShim);

    public:
        VOID
        StartUpdateReservation(
            __in LONGLONG Delta,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
    };
#endif
    
    NTSTATUS
    CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context) override ;

    private:
        RvdLogStream::SPtr _CoreLogStream;
};
