// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

public:
    virtual LONGLONG
    QueryLogStreamUsage() = 0;
    
    virtual VOID
    QueryLogStreamType(__out RvdLogStreamType& LogStreamType) = 0;

    virtual VOID
    QueryLogStreamId(__out RvdLogStreamId& LogStreamId) = 0;

    virtual NTSTATUS
    QueryRecordRange(
        __out_opt RvdLogAsn* const LowestAsn,
        __out_opt RvdLogAsn* const HighestAsn,
        __out_opt RvdLogAsn* const LogTruncationAsn) = 0;

    virtual ULONGLONG QueryCurrentReservation() = 0;

    class AsyncWriteContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContext);

    public:
        //
        // This method asynchronously writes a new log record.
        //
        // Parameters:
        //
        //  LowPriorityIO - If TRUE, IO will be written through the
        //                  KBlockFile background queue. If FALSE it will be written
        //                  through the foreground queue.
        //
        //  RecordAsn - Supplies the Asn (Application-assigned Sequence Number) of the record.
        //
        //  Version - Supplies the version number of the record. If a record with the same Asn
        //      already exists, the outcome depends on the Version value. If the existing record
        //      has a lower version, it will be overwritten. Otherwise, the new write will fail.
        //
        //  MetaDataBuffer - Supplies an optional meta data buffer block of the record.
        //
        //  IoBuffer - Supplies an optional IoBuffer part of the record.
        //
        //  ParentAsyncContext - Supplies an optional parent async context.
        //
        //  ParentAsyncContext - Supplies an optional callback delegate to be notified
        //      when the log record is persisted on disk.
        //
        // Completion status:
        //  STATUS_SUCCESS               - The operation completed successfully.
        //  STATUS_LOG_FULL              - The write is rejected because the log space is exhausted.
        //                                 the log is still in an usable state.
        //  STATUS_BUFFER_OVERFLOW       - The log record buffer size exceeds the per-record size limit.
        //  STATUS_OBJECT_NAME_COLLISION - A log record with the same Asn already exists,
        //                                 but the supplied Version value is not higher than the
        //                                 existing version. The log write is rejected.
        //  STATUS_DELETE_PENDING        - The Stream is in the process of being deleted.
        //  K_STATUS_LOG_STRUCTURE_FAULT - Underlying LOG file has a potential structural fault
        //
        virtual VOID
        StartWrite(
            __in BOOLEAN LowPriorityIO,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        virtual VOID
        StartWrite(
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        //
        // This method asynchronously writes a new log record against the current reserve of
        // the stream
        //
        // Parameters:
        //
        //  LowPriorityIO - If TRUE, IO will be written through the
        //                  KBlockFile background queue. If FALSE it will be written
        //                  through the foreground queue.
        //
        //  ReserveToUse - Supplies the amount of stream reserve to use on a write. All of the
        //                 reserve indicated will be used no matter the size of the actual write.
        //
        //  RecordAsn - Supplies the Asn (Application-assigned Sequence Number) of the record.
        //
        //  Version - Supplies the version number of the record. If a record with the same Asn
        //      already exists, the outcome depends on the Version value. If the existing record
        //      has a lower version, it will be overwritten. Otherwise, the new write will fail.
        //
        //  MetaDataBuffer - Supplies an optional meta data buffer block of the record.
        //
        //  IoBuffer - Supplies an optional IoBuffer part of the record.
        //
        //  ParentAsyncContext - Supplies an optional parent async context.
        //
        //  ParentAsyncContext - Supplies an optional callback delegate to be notified
        //      when the log record is persisted on disk.
        //
        // Completion status:
        //  STATUS_SUCCESS               - The operation completed successfully - ReserveToUse relieved.
        //  STATUS_LOG_FULL              - The write is rejected because the log space is exhausted.
        //                                 the log is still in an usable state - ReserveToUse not relieved.
        //  STATUS_BUFFER_OVERFLOW       - The log record buffer size exceeds the per-record size limit - 
        //                                 ReserveToUse not relieved.
        //  STATUS_OBJECT_NAME_COLLISION - A log record with the same Asn already exists,
        //                                 but the supplied Version value is not higher than the
        //                                 existing version. The log write is rejected - ReserveToUse not relieved.
        //  STATUS_DELETE_PENDING          - The Stream is in the process of being deleted - ReserveToUse not relieved.
        //  K_STATUS_LOG_STRUCTURE_FAULT   - Underlying LOG file has a potential structural fault - Impact of ReserveToUse
        //                                   not determinate - log is faulted and can't be used further
        //  K_STATUS_LOG_RESERVE_TOO_SMALL - The stream does not have enough reserve to grant the write request
        //
        virtual VOID
        StartReservedWrite(
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        virtual VOID
        StartReservedWrite(
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __out ULONGLONG& LogSize,
            __out ULONGLONG& LogSpaceRemaining,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
        
        virtual VOID
        StartReservedWrite(
            __in BOOLEAN LowPriorityIO,
            __in ULONGLONG ReserveToUse,
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    virtual NTSTATUS
    CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) = 0;

    class AsyncReadContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadContext);

    public:
        //
        // This method asynchronously reads an existing log record.
        // If the record has been overwritten multiple times, only the latest version
        // will be returned. The record buffers are allocated by the logger.
        // Caller of this method should release the buffers when they are no longer needed.
        //
        // Parameters:
        //
        //  RecordAsn - Supplies the Asn (Application-assigned Sequence Number) of the record.
        //
        //  Version - Optionally returns the version number of the record.
        //
        //  MetaDataBuffer - Returns the meta data buffer block of the record.
        //
        //  IoBuffer - Returns the IoBuffer part of the record.
        //
        //  ParentAsyncContext - Supplies an optional parent async context.
        //
        //  CallbackPtr - Supplies an optional callback delegate to be notified
        //      when the read completes.
        //
        // Completion status:
        //
        //  STATUS_SUCCESS - The operation completed successfully.
        //  STATUS_NOT_FOUND - The specified log record does not exist.
        //  STATUS_CRC_ERROR - The log record meta data failed CRC verificaiton.
        //  STATUS_BUFFER_TOO_SMALL - Internal record metadata indicates a record too large
        //  STATUS_DATA_ERROR - Data on disk is not a valid log record.
        //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
        //
        virtual VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

        typedef enum { ReadTypeNotSpecified, ReadExactRecord, ReadNextRecord, ReadPreviousRecord, ReadContainingRecord, 
                       ReadNextFromSpecificAsn, ReadPreviousFromSpecificAsn }
        ReadType;
        
        virtual VOID
        StartRead(
            __in RvdLogAsn RecordAsn,
            __in RvdLogStream::AsyncReadContext::ReadType Type,
            __out_opt RvdLogAsn* const ActualRecordAsn,
            __out_opt ULONGLONG* const Version,
            __out KBuffer::SPtr& MetaDataBuffer,
            __out KIoBuffer::SPtr& IoBuffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;        
    };

    virtual NTSTATUS
    CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) = 0;

    //
    // This method queries the meta data information of a specfic log record.
    // For the current version, record meta data is assumed to be small enough
    // to fit into memory. Therefore, this method is synchronous.
    //
    // Parameters:
    //
    //  RecordAsn - Supplies the Asn of the record.
    //
    //  Version - Optionally returns the version number of the record.
    //
    //  Disposition - Optionally returns the current disposition of the record.
    //
    enum RecordDisposition : ULONG
    {
        eDispositionPending = 0,    // The log record is being written to a stable storage.
        eDispositionPersisted = 1,  // The log record has been persisted on a stable storage.
        eDispositionNone = 2,       // The log record does not exist.
    };

    virtual NTSTATUS
    QueryRecord(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize = nullptr,
        __out_opt ULONGLONG* const DebugInfo1 = nullptr) = 0;

    
    virtual NTSTATUS
    QueryRecord(
        __inout RvdLogAsn& RecordAsn,
        __in RvdLogStream::AsyncReadContext::ReadType Type,
        __out_opt ULONGLONG* const Version,
        __out_opt RvdLogStream::RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize = nullptr,
        __out_opt ULONGLONG* const DebugInfo1 = nullptr) = 0;
    
    //
    // This method queries the meta data information for a range of log records.
    //
    // Parameters:
    //  LowestAsn - Supplies the lowest Asn of the record metadata to be returned.
    //
    //  HighestAsn - Supplies the highest Asn of the record metadata to be returned.
    //
    //  ResultsArray - Supplies the KArray<RecordMetadata> that the results of this
    //                 method will be returned. NOTE: This array is cleared before use.
    //
    struct RecordMetadata;

    virtual NTSTATUS
    QueryRecords(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RvdLogStream::RecordMetadata>& ResultsArray) = 0;

    //
    // This method will delete a record from the core logger metadata.
    //
    // Parameters:
    //  RecordAsn is the ASN for the record to delete
    //
    //  Version is the version number for the record to delete
    //
    virtual NTSTATUS
    DeleteRecord(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version) = 0;
    
    //
    // This method truncates a log stream.
    // Log truncation is voluntary and can happen in the background.
    // If there is enough log space, the caller would like to keep all records
    // whose Asn's are higher than PreferredTruncationPoint. If there is not enough space,
    // any record with an Asn strictly less than or equal to TruncationPoint can be truncated.
    //
    // Parameters:
    //
    //  TruncationPoint - Supplies the highest Asn that can be truncated.
    //
    //  PreferredTruncationPoint - Supplies the preferred log truncation point.
    //      PreferredTruncationPoint is normally lower than TruncationPoint.
    //
    //  Version - Supplies the version associated with the ASN to truncate. If the stream
    //            has any records below the truncation point with a higher version then no
    //            truncation occurs.
    virtual VOID
    Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint) = 0;

    virtual VOID
    TruncateBelowVersion(__in RvdLogAsn TruncationPoint, __in ULONGLONG Version) = 0;

    class AsyncReservationContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncReservationContext);

    public:
        //
        // This method asynchronously adjusts a stream's log
        // reservation space.             
        //
        // Parameters:
        //
        //  Delta - difference to adjust reserve by
        //  
        //  ParentAsyncContext - Supplies an optional parent async context.
        //
        //  CallbackPtr - Supplies an optional callback delegate to be notified
        //      when a notification event occurs
        //
        // Completion status:
        //
        //  STATUS_SUCCESS - The operation completed successfully.
        //  STATUS_LOG_FULL - There is not enough log file space to extend the reservation
        //
        virtual VOID
        StartUpdateReservation(
            __in LONGLONG Delta,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
    };

    virtual NTSTATUS
    CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context) = 0;        

    virtual NTSTATUS
    SetTruncationCompletionEvent(__in_opt KAsyncEvent* const EventToSignal) = 0;
    