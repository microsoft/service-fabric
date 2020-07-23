// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <KtlPhysicalLog.h>

class KtlLogDestager abstract : public KObject<KtlLogDestager>, public KShared<KtlLogDestager>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KtlLogDestager);

public:

    struct ResourceCounter
    {
        ResourceCounter()
        {
            Zero();
        }

        VOID Zero()
        {
            memset(this, 0, sizeof(ResourceCounter));
        }

        // Memory usage of this destager in bytes. It is the total size of all pending writes in the destaging queue.
        LONGLONG LocalMemoryUsage;

        // The number of pending writes in the destaging queue.
        ULONG PendingWriteCount;
    };

    class AsyncWriteContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContext);

    public:

        //
        // This method puts a log record in the destaging queue and eventually writes the record to the dedicated log stream.
        // The decision to flush a write request to the dideciated log stream comes from outside.
        // It is completed successfully only after the record has been written to the dedicated log stream.
        // In all other cases, it returns an error.
        //
        // Parameters:
        //
        //  RecordAsn: Supplies Asn of the log record
        //
        //  Version: Supplies version the log record
        //
        //  MetaDataBuffer: Supplies meta data buffer part of the record
        //
        //  IoBuffer: Supplies IO buffer part of the record
        //
        //  Counter: Optionally returns the resource usage of this destager when this async completes
        //
        //  ParentAsyncContext: Optionally supplies a parent async context
        //
        //  CallbackPtr: Optionally supplies an async completion callback
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
        //  STATUS_CANCELLED             - The write request has been cancelled.
        //  K_STATUS_LOG_STRUCTURE_FAULT - Underlying LOG file has a potential structural fault
        //
        virtual VOID
        StartWrite(
            __in RvdLogAsn RecordAsn,
            __in ULONGLONG Version,
            __in const KBuffer::SPtr& MetaDataBuffer,
            __in const KIoBuffer::SPtr& IoBuffer,
            __out_opt ResourceCounter* Counter,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
            ) = 0;
    };

    class AsyncCloseContext abstract : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncCloseContext);

    public:

        //
        // This method logically closes this destager and drains the queue. 
        // Once this async is started, no more log records can be queued into this destager.
        // After this async completes, the destager is guranteed not to hold any reference to any log stream.
        //
        // Parameters:
        //
        //  CancelIo:   If TRUE, all in-progress log record writes are cancelled and all queued log records are dropped.
        //              If FALSE, this async will start flushing all pending writes to the dedicated log stream.
        //              It will not complete until all pending writes are completed.
        //
        //  ParentAsyncContext: Optionally supplies a parent async context
        //
        //  CallbackPtr: Optionally supplies an async completion callback
        //
        virtual VOID
        StartClose(
            __in BOOLEAN CancelIo,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
            ) = 0;
    };

    //
    // This method allocates an AsyncWriteContext.
    //
    virtual NTSTATUS
    CreateAsyncWriteContext(
        __out AsyncWriteContext::SPtr& Result,
        __in ULONG AllocationTag = KTL_TAG_LOGGER
        ) = 0;

    //
    // This method allocates an AsyncCloseContext.
    //
    virtual NTSTATUS
    CreateAsyncCloseContext(
        __out AsyncCloseContext::SPtr& Result,
        __in ULONG AllocationTag = KTL_TAG_LOGGER
        ) = 0;

    //
    // This method reads an existing log record from the internal queue
    // where the RecordAsn passed exactly matches the Asn of
    // the record.
    //
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
    // Completion status:
    //
    //  STATUS_SUCCESS - The operation completed successfully.
    //  STATUS_NOT_FOUND - The specified log record does not exist.
    //
    virtual NTSTATUS
    Read(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) = 0;

    //
    // This method synchronously reads an existing log record from the internal queue
    // where the RecordAsn passed is the Asn of the record that
    // is immediately after the record read.
    //
    // If the record has been overwritten multiple times, only the latest version
    // will be returned. The record buffers are allocated by the logger.
    // Caller of this method should release the buffers when they are no longer needed.
    //
    // Parameters:
    //
    //  NextRecordAsn - Supplies the Asn (Application-assigned
    //  Sequence Number) of the record previous to the record
    //  read.
    //
    //  RecordAsn - returns the Asn of the record actually read
    //
    //  Version - Optionally returns the version number of the record.
    //
    //  MetaDataBuffer - Returns the meta data buffer block of the record.
    //
    //  IoBuffer - Returns the IoBuffer part of the record.
    //
    // Completion status:
    //
    //  STATUS_SUCCESS - The operation completed successfully.
    //  STATUS_NO_MATCH - NextRecordAsn does not exist in the queue.
    //  STATUS_NOT_FOUND - There is no record prior to NextRecordAsn.
    //
    virtual NTSTATUS
    ReadPrevious(
        __in RvdLogAsn NextRecordAsn,
        __out_opt RvdLogAsn* RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) = 0;

    //
    // This method synchronously reads an existing log record from the internal queue
    // where the RecordAsn passed is the Asn of the record that
    // is previous to the record read.
    //
    // If the record has been overwritten multiple times, only the latest version
    // will be returned. The record buffers are allocated by the logger.
    // Caller of this method should release the buffers when they are no longer needed.
    //
    // Parameters:
    //
    //  PreviousRecordAsn - Supplies the Asn (Application-assigned
    //  Sequence Number) of the record previous to the record
    //  read.
    //
    //  RecordAsn - returns the Asn of the record actually read
    //
    //  Version - Optionally returns the version number of the record.
    //
    //  MetaDataBuffer - Returns the meta data buffer block of the record.
    //
    //  IoBuffer - Returns the IoBuffer part of the record.
    //
    // Completion status:
    //
    //  STATUS_SUCCESS - The operation completed successfully.
    //  STATUS_NO_MATCH - PreviousRecordAsn does not exist in the queue.
    //  STATUS_NOT_FOUND - There is no record next to PreviousRecordAsn.
    //
    virtual NTSTATUS
    ReadNext(
        __in RvdLogAsn PreviousRecordAsn,
        __out_opt RvdLogAsn* RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) = 0;
            
    //
    // This method synchronously reads an existing log record from the internal queue
    // where the RecordAsn passed is within the range of the of
    // the log record read.
    //
    // If the record has been overwritten multiple times, only the latest version
    // will be returned. The record buffers are allocated by the logger.
    // Caller of this method should release the buffers when they are no longer needed.
    //
    // Parameters:
    //
    //  SpecifiedRecordAsn - Supplies the Asn
    //  (Application-assigned Sequence Number) that is within
    //  the ASN range of the record.
    //
    //  RecordAsn - returns the first Asn of the record which
    //  is the ASN at which the record was written
    //
    //  Version - Optionally returns the version number of the record.
    //
    //  MetaDataBuffer - Returns the meta data buffer block of the record.
    //
    //  IoBuffer - Returns the IoBuffer part of the record.
    //
    // Completion status:
    //
    //  STATUS_SUCCESS - The operation completed successfully.
    //  STATUS_NOT_FOUND - There is no record that contains SpecifiedRecordAsn.
    //
    virtual NTSTATUS
    ReadContaining(
        __in RvdLogAsn SpecifiedRecordAsn,
        __out_opt RvdLogAsn* RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        ) = 0;

    //
    // This method drops all log records whose Asn's are less than or equal to PurgePoint from the destaging queue.
    // All outstanding writes for the purged log records are also cancelled.
    //
    virtual VOID 
    PurgeQueuedWrites(
        __in RvdLogAsn PurgePoint
        ) = 0;

    //
    // This method returns the current resource counter.
    //
    virtual VOID
    QueryResourceCounter(
        __out ResourceCounter& Counter
        ) = 0;

    //
    // The following methods are mechanisms to flush queued log records to the dedicated log stream.
    // It is up to the external caller to decide when to call these methods and how much to flush.
    // Each queued log record write will eventually complete as its AsyncWriteContext completes.
    // There is no separate notification mechanism for flush completion.
    //
    virtual VOID FlushByTime(__in LONG MaxQueueingTimeInMs) = 0;

    virtual VOID FlushByMemory(__in LONGLONG TargetLocalMemoryUsage) = 0;

    virtual VOID FlushByAsn(__in RvdLogAsn FlushToRecordAsn) = 0;
};

inline KtlLogDestager::KtlLogDestager() {}
inline KtlLogDestager::~KtlLogDestager() {}

inline KtlLogDestager::AsyncWriteContext::AsyncWriteContext() {}
inline KtlLogDestager::AsyncWriteContext::~AsyncWriteContext() {}

inline KtlLogDestager::AsyncCloseContext::AsyncCloseContext() {}
inline KtlLogDestager::AsyncCloseContext::~AsyncCloseContext() {}
