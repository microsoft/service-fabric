 // ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


//
// When USE_RVDLOGGER_OBJECTS is defined the underlying RVD objects are
// used and the KTL object are aliased to them. This is a temporary
// measure until the RVD code base is moved to use the KTL object
//

#pragma once

#include "KtlPhysicalLog.h"
#define USE_RVDLOGGER_OBJECTS

#ifdef USE_RVDLOGGER_OBJECTS
#define KtlLogStreamType RvdLogStreamType
#define KtlLogStreamId RvdLogStreamId
#define KtlLogContainerId RvdLogId
#define KtlLogAsn RvdLogAsn
#endif


class KtlLogManager;
class KtlLogContainerId;
class KtlLogStreamId;
class KtlLogStreamType;
class KtlLogAsn;

//**
// A log stream is a logical sequence of log records. A client can use an KtlLogStream
// object to write and read log records. It can also use it to truncate a log stream.
// Log streams are physically multiplexed into a single log container, but they are
// logically separate.
//
class KtlLogStream abstract : public KObject<KtlLogStream>, public KShared<KtlLogStream>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KtlLogStream);

    public:
        //
        // This method returns a boolean that if TRUE, indicates that
        // the stream is in a functional and usable state. If FALSE
        // then the stream is not.
        //
        virtual
        BOOLEAN IsFunctional() = 0;

        //
        // This method returns the log stream id for the stream
        //
        virtual VOID
        QueryLogStreamId(__out KtlLogStreamId& LogStreamId) = 0;


        //
        // This method returns a fixed value that specifies how much
        // metadata space is needed for the log stream functionality.
        // If the application has its own meta data to be included in
        // the record then applications must pass a metadata buffer of at least this
        // length when writing log records. The application places its
        // own metadata after the log stream metadata header.
        //
        virtual ULONG
        QueryReservedMetadataSize() = 0;

        class AsyncQueryRecordRangeContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncQueryRecordRangeContext);

        public:
            //
            // This method asynchronously queries the record ranges for this stream             
            //
            // Parameters:
            //
            //  LowestAsn - returns the ASN of the lowest record in the stream
            //  
            //  HighestAsn - returns the ASN of the highest record in the stream
            //
            //  LogTruncationAsn - returns the ASN to which the stream is truncated
            //  
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            virtual VOID
            StartQueryRecordRange(
                __out_opt KtlLogAsn* const LowestAsn,
                __out_opt KtlLogAsn* const HighestAsn,
                __out_opt KtlLogAsn* const LogTruncationAsn,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
        };

        virtual NTSTATUS
        CreateAsyncQueryRecordRangeContext(__out AsyncQueryRecordRangeContext::SPtr& Context) = 0;        



        class AsyncReservationContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReservationContext);

        public:
            //
            // This method asynchronously allocates additional log
            // reservation space and includes it with any existing
            // reservation space associated with this stream
            //
            // Parameters:
            //
            //  ReservationSize - the number of additional log space bytes to reserve.
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
            StartExtendReservation(
                __in ULONGLONG ReservationSize,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

            //
            // This method asynchronously releases log
            // reservation space from this stream.
            //
            // Parameters:
            //
            //  ReservationSize - the number of log space bytes to release.
            //  
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //  K_STATUS_LOG_RESERVE_TOO_SMALL - The amount of
            //      reservation space currently allocated is less than
            //      the amount specified to contract
            //  
            //
            virtual VOID
            StartContractReservation(
                __in ULONGLONG ReservationSize,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
        };

        virtual NTSTATUS
        CreateAsyncReservationContext(__out AsyncReservationContext::SPtr& Context) = 0;        

        virtual ULONGLONG
        QueryReservationSpace( ) = 0;
        
        class AsyncWriteContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContext);

        public:
            //
            // This method asynchronously writes a new log record.
            //
            // Parameters:
            //
            //  RecordAsn - Supplies the Asn (Application-assigned Sequence Number) of the record.
            //
            //  Version - Supplies the version number of the record. If a record with the same Asn
            //      already exists, the outcome depends on the Version value. If the existing record
            //      has a lower version, it will be overwritten. Otherwise, the new write will fail.
            //
            //  MetaDataLength - Supplies the length of the optional meta data block of the record.
            //  
            //  MetaDataBuffer - Supplies an optional meta data buffer
            //                   block of the record. If specified then the meta data
            //                   buffer must be at least as large as the value specified
            //                   by QueryReservedMetaDataSize() and the application meta
            //                   data placed after the log stream meta data header.
            //
            //  IoBuffer - Supplies an optional IoBuffer part of the
            //             record.
            //
            //  Reservation - Supplies the number of bytes of
            //                reservation space to use as part of this write
            //                operation. Callers allocate reservation space for this
            //                stream by using the AsyncReservationContext and then
            //                use the reservation space allocated in this api. If
            //                this api returns successfully then all reservation
            //                space specified is deducted from the total reservation
            //                space currently held by this stream. If the api fails
            //                then the reservations space is not deducted from the
            //                total reservation space held by this stream. If the
            //                reservation space passed is less than the amount of
            //                space needed for this write then the logger will
            //                attempt to write the record using the reservation space
            //                and new space if it is available; in this case the
            //                write may fail with an out of log space error. 
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
            //  K_STATUS_LOG_RESERVE_TOO_SMALL - The amount of
            //      reservation space specified to use is larger than
            //      the amount of reservation space available for the
            //      stream.
            //
            virtual VOID
            StartWrite(
                __in KtlLogAsn RecordAsn,
                __in ULONGLONG Version,
                __in ULONG MetaDataLength,
                __in const KIoBuffer::SPtr& MetaDataBuffer,
                __in const KIoBuffer::SPtr& IoBuffer,
                __in ULONGLONG ReservationSpace,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

            virtual VOID
            StartWrite(
                __in KtlLogAsn RecordAsn,
                __in ULONGLONG Version,
                __in ULONG MetaDataLength,
                __in const KIoBuffer::SPtr& MetaDataBuffer,
                __in const KIoBuffer::SPtr& IoBuffer,
                __in ULONGLONG ReservationSpace,
                __out ULONGLONG& LogSize,
                __out ULONGLONG& LogSpaceRemaining,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            WriteAsync(
                __in KtlLogAsn RecordAsn,
                __in ULONGLONG Version,
                __in ULONG MetaDataLength,
                __in const KIoBuffer::SPtr& MetaDataBuffer,
                __in const KIoBuffer::SPtr& IoBuffer,
                __in ULONGLONG ReservationSpace,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;

            virtual ktl::Awaitable<NTSTATUS>
            WriteAsync(
                __in KtlLogAsn RecordAsn,
                __in ULONGLONG Version,
                __in ULONG MetaDataLength,
                __in const KIoBuffer::SPtr& MetaDataBuffer,
                __in const KIoBuffer::SPtr& IoBuffer,
                __in ULONGLONG ReservationSpace,
                __out ULONGLONG& LogSize,
                __out ULONGLONG& LogSpaceRemaining,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) = 0;

        
        class AsyncReadContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadContext);

        public:
            //
            // This method asynchronously reads an existing log record
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
            //  MetaDataLength - Returns the length of the metadata buffer block.
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
            //  STATUS_CRC_ERROR - The log record meta data failed CRC verification.
            //  STATUS_BUFFER_TOO_SMALL - Internal record metadata indicates a record too large
            //  STATUS_DATA_ERROR - Data on disk is not a valid log record.
            //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
            //
            virtual VOID
            StartRead(
                __in KtlLogAsn RecordAsn,
                __out_opt ULONGLONG* const Version,
                __out ULONG& MetaDataLength,
                __out KIoBuffer::SPtr& MetaDataBuffer,
                __out KIoBuffer::SPtr& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;


            //
            // This method asynchronously reads an existing log record
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
            //  MetaDataLength - Returns the length of the metadata buffer block.
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
            //  STATUS_CRC_ERROR - The log record meta data failed CRC verification.
            //  STATUS_BUFFER_TOO_SMALL - Internal record metadata indicates a record too large
            //  STATUS_DATA_ERROR - Data on disk is not a valid log record.
            //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
            //
            virtual VOID
            StartReadPrevious(
                __in KtlLogAsn NextRecordAsn,
                __out_opt KtlLogAsn* RecordAsn,
                __out_opt ULONGLONG* const Version,
                __out ULONG& MetaDataLength,
                __out KIoBuffer::SPtr& MetaDataBuffer,
                __out KIoBuffer::SPtr& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

            //
            // This method asynchronously reads an existing log record
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
            //  MetaDataLength - Returns the length of the metadata buffer block.
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
            //  STATUS_CRC_ERROR - The log record meta data failed CRC verification.
            //  STATUS_BUFFER_TOO_SMALL - Internal record metadata indicates a record too large
            //  STATUS_DATA_ERROR - Data on disk is not a valid log record.
            //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
            //
            virtual VOID
            StartReadNext(
                __in KtlLogAsn PreviousRecordAsn,
                __out_opt KtlLogAsn* RecordAsn,
                __out_opt ULONGLONG* const Version,
                __out ULONG& MetaDataLength,
                __out KIoBuffer::SPtr& MetaDataBuffer,
                __out KIoBuffer::SPtr& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            
            //
            // This method asynchronously reads an existing log record
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
            //  MetaDataLength - Returns the length of the metadata buffer block.
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
            //  STATUS_CRC_ERROR - The log record meta data failed CRC verification.
            //  STATUS_BUFFER_TOO_SMALL - Internal record metadata indicates a record too large
            //  STATUS_DATA_ERROR - Data on disk is not a valid log record.
            //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
            //
            virtual VOID
            StartReadContaining(
                __in KtlLogAsn SpecifiedRecordAsn,
                __out_opt KtlLogAsn* RecordAsn,
                __out_opt ULONGLONG* const Version,
                __out ULONG& MetaDataLength,
                __out KIoBuffer::SPtr& MetaDataBuffer,
                __out KIoBuffer::SPtr& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            
#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            ReadContainingAsync(
                __in KtlLogAsn SpecifiedRecordAsn,
                __out_opt KtlLogAsn* RecordAsn,
                __out_opt ULONGLONG* const Version,
                __out ULONG& MetaDataLength,
                __out KIoBuffer::SPtr& MetaDataBuffer,
                __out KIoBuffer::SPtr& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) = 0;

        class AsyncMultiRecordReadContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncMultiRecordReadContext);

        public:
            //
            // This method asynchronously reads an one or more logical
            // log records from the offset of the RecordAsn forward.
            // Record data is returned from subsequent records into the Metadata and IoBuffers
            // are until they are filled.
            //
            // NOTE: This is only valid when using logical log streams
            //
            //
            // Parameters:
            //
            //  RecordAsn - Supplies the Asn (Application-assigned Sequence Number) of the record.
            //
            //  MetaDataBuffer - Returns filled with the meta data buffer block of the record.
            //
            //  IoBuffer - Returns filled with the IoBuffer part of the record.
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
            //  STATUS_CRC_ERROR - The log record meta data failed CRC verification.
            //  STATUS_BUFFER_TOO_SMALL - Internal record metadata indicates a record too large
            //  STATUS_DATA_ERROR - Data on disk is not a valid log record.
            //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
            //
            virtual VOID
            StartRead(
                __in KtlLogAsn RecordAsn,
                __inout KIoBuffer& MetaDataBuffer,
                __inout KIoBuffer& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            ReadAsync(
                __in KtlLogAsn RecordAsn,
                __inout KIoBuffer& MetaDataBuffer,
                __inout KIoBuffer& IoBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };
        
        virtual NTSTATUS
        CreateAsyncMultiRecordReadContext(__out AsyncMultiRecordReadContext::SPtr& Context) = 0;
        
        enum RecordDisposition : ULONG
        {
            eDispositionPending = 0,    // The log record is being written to a stable storage.
            eDispositionPersisted = 1,  // The log record has been persisted on a stable storage.
            eDispositionNone = 2,       // The log record does not exist.
        };

        class AsyncQueryRecordContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncQueryRecordContext);

        public:
            //
            // This method asynchronously queries the meta data information of a specific log record.
            // For the current version, record meta data is assumed to be small enough
            // to fit into memory. 
            //
            // Parameters:
            //
            //  RecordAsn - Supplies the Asn of the record.
            //
            //  Version - Optionally returns the version number of the record.
            //
            //  Disposition - Optionally returns the current disposition of the record.
            //
            //  
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            virtual VOID
            StartQueryRecord(
                __in KtlLogAsn RecordAsn,
                __out_opt ULONGLONG* const Version,
#ifdef USE_RVDLOGGER_OBJECTS
                __out_opt RvdLogStream::RecordDisposition* const Disposition,
#else
                __out_opt KtlLogStream::RecordDisposition* const Disposition,
#endif
                __out_opt ULONG* const IoBufferSize,
                __out_opt ULONGLONG* const DebugInfo,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            };

        virtual NTSTATUS
        CreateAsyncQueryRecordContext(__out AsyncQueryRecordContext::SPtr& Context) = 0;        
                

        struct RecordMetadata;

        class AsyncQueryRecordsContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncQueryRecordsContext);

        public:
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
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            virtual VOID
            StartQueryRecords(
                __in KtlLogAsn LowestAsn,
                __in KtlLogAsn HighestAsn,
#ifdef USE_RVDLOGGER_OBJECTS
                __in KArray<RvdLogStream::RecordMetadata>& ResultsArray,
#else
                __in KArray<RecordMetadata>& ResultsArray,     
#endif
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            };

        virtual NTSTATUS
        CreateAsyncQueryRecordsContext(__out AsyncQueryRecordsContext::SPtr& Context) = 0;        


        class AsyncTruncateContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncTruncateContext);

        public:
            //
            // This method requests asynchronous truncation of a log
            // stream. Truncation will occur in the background,
            // potentially after the call returns.
            //
            // If there is enough log space, the caller would like to keep all records
            // whose Asn's are higher than PreferredTruncationPoint. If there is not enough space,
            // any record with an Asn strictly less than or equal to TruncationPoint can be truncated.
            //
            //
            // Parameters:
            //
            //  TruncationPoint - Supplies the highest Asn that can be truncated.
            //
            //  PreferredTruncationPoint - Supplies the preferred log truncation point.
            //      PreferredTruncationPoint is normally lower than TruncationPoint.
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            virtual VOID
            Truncate(
                __in KtlLogAsn TruncationPoint,
                __in KtlLogAsn PreferredTruncationPoint
            ) = 0;          
        };

        virtual NTSTATUS
        CreateAsyncTruncateContext(__out AsyncTruncateContext::SPtr& Context) = 0;

        
        class AsyncStreamNotificationContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncStreamNotificationContext);

        public:
            //
            // This method starts an async which completes when a
            // specific stream threshold has been reached. The async is
            // completed once when the threshold is reached. If the
            // caller would like a subsequent completion it needs to
            // Reuse() and start the async again.
            //
            // Note that a stream will not be destructed properly if
            // there is an AsyncStreamNotificationContext pending and
            // the stream was not first closed
            //
            // Supported ThresholdTypeEnum Values:
            //
            //    LogSpaceUtilization - this specifies that when usage
            //    of log space for the stream crosses a value that the
            //    async is completed. ThresholdValue specifies the
            //    percentage of log space used that triggers
            //    completion. Threshold value must be an integer
            //    between 0 and 99.
            //
            // Parameters:
            //
            //  ThresholdType - Supplies the type of threshold
            //      that should trigger this completion
            //
            //  ThresholdValue - Supplies the value for the threshold
            //      type to trigger this completion
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            
            enum ThresholdTypeEnum { LogSpaceUtilization };
            
            virtual VOID
            StartWaitForNotification(
                __in ThresholdTypeEnum ThresholdType,
                __in ULONGLONG ThresholdValue,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            };

        virtual NTSTATUS
        CreateAsyncStreamNotificationContext(__out AsyncStreamNotificationContext::SPtr& Context) = 0;        

        
        class AsyncWriteMetadataContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteMetadataContext);

        public:
            //
            // This method asynchronously writes the per stream metadata data
            //
            // Parameters:
            //
            //  MetaDataBuffer - Supplies meta data buffer block of the stream.
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
            //  STATUS_BUFFER_OVERFLOW       - The meta data buffer size exceeds the size limit.
            //  STATUS_DELETE_PENDING        - The Stream is in the process of being deleted.
            //  K_STATUS_LOG_STRUCTURE_FAULT - Underlying LOG file has a potential structural fault
            //
            virtual VOID
            StartWriteMetadata(
                __in const KIoBuffer::SPtr& MetaDataBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
        };

        virtual NTSTATUS
        CreateAsyncWriteMetadataContext(__out AsyncWriteMetadataContext::SPtr& Context) = 0;

        class AsyncReadMetadataContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadMetadataContext);

        public:
            //
            // This method asynchronously reads the per stream meta
            // data.  The meta data buffer is allocated by the logger.
            // Caller of this method should release the buffers when they are no longer needed.
            //
            // Parameters:
            //
            //  MetaDataBuffer - Returns the meta data buffer block of the stream
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
            //  STATUS_CRC_ERROR - The log record meta data failed CRC verification.
            //  STATUS_BUFFER_TOO_SMALL - Internal record metadata indicates a record too large
            //  STATUS_DATA_ERROR - Data on disk is not a valid log record.
            //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
            //
            virtual VOID
            StartReadMetadata(
                __out KIoBuffer::SPtr& MetaDataBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
        };

        virtual NTSTATUS
        CreateAsyncReadMetadataContext(__out AsyncReadMetadataContext::SPtr& Context) = 0;
        

        class AsyncIoctlContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncIoctlContext);

        public:
            //
            // This method asynchronously sends a control message to
            // the stream object. Certain stream objects may support
            // different control codes depending upon the LogStreamType
            // used to create the log streams. The format for the
            // requests are defined by a contract between the caller
            // and the LogStreamType; the contents of the call are
            // opaque to the other layers.
            //
            // LogicalLogStreamType control are defined in klogicallog.h
            //
            // Parameters:
            //
            //  ControlCode is a ULONG value that specifies which
            //  operation the log stream type should perform.
            //
            //  InBuffer is an optional input buffer whose contents
            //  depend upon the ControlCode
            //
            //  OutBuffer is the output buffer whose contents
            //  depend upon the ControlCode
            //
            //  Result is a ULONG that describes the result of the
            //  Ioctl
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when the read completes.
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //  STATUS_INVALID_PARAMETER - A parameter passed is incorrect
            //  STATUS_DELETE_PENDING - The Stream is in the process of being deleted.
            //
            virtual VOID
            StartIoctl(
                __in ULONG ControlCode,
                __in_opt KBuffer* const InBuffer,
                __out ULONG& Result,
                __out KBuffer::SPtr& OutBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            IoctlAsync(
                __in ULONG ControlCode,
                __in_opt KBuffer* const InBuffer,
                __out ULONG& Result,
                __out KBuffer::SPtr& OutBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncIoctlContext(__out AsyncIoctlContext::SPtr& Context) = 0;                

        typedef KDelegate<VOID(
            KAsyncContextBase* const,           // Parent; can be nullptr
            KtlLogStream&,                      // Log Stream instance
            NTSTATUS                            // Completion status of the Close
            )> CloseCompletionCallback;
        
        virtual NTSTATUS StartClose(
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt CloseCompletionCallback CloseCallbackPtr
        ) = 0;
        
#if defined(K_UseResumable)
        virtual ktl::Awaitable<NTSTATUS>
        CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
};

//
// An KtlLogContainer object is a container of many logically separated log streams.
// It manages the log container in terms of physical space, cache, etc.
//
// Once a client gets an SPtr to a KtlLogContainer object, the log is already mounted.
// It does not have to and cannot call mount(). Only the KtlLogManager
// can mount a physical log.
//
class KtlLogContainer abstract : public KObject<KtlLogContainer>, public KShared<KtlLogContainer>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KtlLogContainer);

    public:     
        //
        // This method returns a boolean that if TRUE, indicates that
        // the log container is in a functional and usable state. If FALSE
        // then the stream is not.
        //
        virtual
        BOOLEAN IsFunctional() = 0;     

        //
        // Define constants for throttling log container usage.
        //
        static const ULONG MinimumThrottleAmountInBytes = 16 * 1024 * 1024;  // 16MB
        
        class AsyncCreateLogStreamContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLogStreamContext);
            
        public:

            //
            // Log creation flags
            //
            static const DWORD FlagSparseFile = 1;
            
            //
            // This method asynchronously create a new log stream.
            //
            // Parameters:
            //
            //  LogStreamId - Supplies the log stream KGuid id.
            //
            //  LogStreamType - Supplies the type of log stream.
            //      Different log stream types have different
            //      behaviors.
            //
            //           LogicalLogLogStreamType is used for the SF
            //           logical log and implements specific behaviors.
            //
            //  LogStreamAlias - An optional string that may serve as
            //      an alias for the LogStreamId
            //
            //
            //  Path - An optional path in which to create the files
            //         associated with the log stream.
            //
            //  SecurityDescriptor - A buffer that contains a security
            //         descriptor to associate with the log stream.
            //
            //  MaximumMetaDataLength - Supplies the maximum number of bytes
            //      that will be consumed by per-stream meta data for this
            //      stream.
            //
            //  MaximumStreamSize - Supplies the maximum number of
            //      record data and metadata that can be stored in the
            //      stream.
            //  MaximumRecordSize - Supplies the maximum number of
            //      bytes that can be part of a log record.
            //
            //  LogStream - Returns a pointer to a KTL log stream
            //      object. When the last reference to this is removed the
            //      LogStream will be closed.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when the log stream is created.
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            //  STATUS_LOG_FULL - There is not enough space to record the new log stream information.
            //      The log is still in an usable state.
            //
            //  STATUS_OBJECT_NAME_COLLISION - A log stream with the same stream ID
            //      in the same log already exists.
            //
            //  STATUS_DELETE_PENDING - Subject Stream is in the process of being deleted
            //
            virtual VOID
            StartCreateLogStream(
                __in const KtlLogStreamId& LogStreamId,
                __in_opt const KString::CSPtr& LogStreamAlias,
                __in_opt const KString::CSPtr& Path,
                __in_opt KBuffer::SPtr& SecurityDescriptor,
                __in ULONG MaximumMetaDataLength,
                __in LONGLONG MaximumStreamSize,
                __in ULONG MaximumRecordSize,
                __out KtlLogStream::SPtr& LogStream,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            
            virtual VOID
            StartCreateLogStream(
                __in const KtlLogStreamId& LogStreamId,
                __in const KtlLogStreamType& LogStreamType,
                __in_opt const KString::CSPtr& LogStreamAlias,
                __in_opt const KString::CSPtr& Path,
                __in_opt KBuffer::SPtr& SecurityDescriptor,
                __in ULONG MaximumMetaDataLength,
                __in LONGLONG MaximumStreamSize,
                __in ULONG MaximumRecordSize,
                __out KtlLogStream::SPtr& LogStream,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            
            virtual VOID
            StartCreateLogStream(
                __in const KtlLogStreamId& LogStreamId,
                __in const KtlLogStreamType& LogStreamType,
                __in_opt const KString::CSPtr& LogStreamAlias,
                __in_opt const KString::CSPtr& Path,
                __in_opt KBuffer::SPtr& SecurityDescriptor,
                __in ULONG MaximumMetaDataLength,
                __in LONGLONG MaximumStreamSize,
                __in ULONG MaximumRecordSize,
                __in DWORD Flags,
                __out KtlLogStream::SPtr& LogStream,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
                
#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            CreateLogStreamAsync(
                __in const KtlLogStreamId& LogStreamId,
                __in const KtlLogStreamType& LogStreamType,
                __in_opt const KString::CSPtr& LogStreamAlias,
                __in_opt const KString::CSPtr& Path,
                __in_opt KBuffer::SPtr& SecurityDescriptor,
                __in ULONG MaximumMetaDataLength,
                __in LONGLONG MaximumStreamSize,
                __in ULONG MaximumRecordSize,
                __in DWORD Flags,
                __out KtlLogStream::SPtr& LogStream,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context) = 0;

        class AsyncDeleteLogStreamContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLogStreamContext);
        public:

            //
            // This method asynchronously deletes an existing log stream.
            //
            // Parameters:
            //
            //  LogStreamId - Supplies the log stream KGuid id.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when the log stream is deleted.
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            //  STATUS_LOG_FULL - There is not enough space to record the log stream deletion information.
            //      The log is still in an usable state.
            //
            //  STATUS_NOT_FOUND - The specified log stream does not exist.
            //
            //  NOTES: This operation is potentially long running in that it will not complete (successfully)
            //         until any interest in the underlying KtlLogStream instance has been released - the point
            //         at which time that such KtlLogStream instance would normally close - meaning all refs, both
            //         direct (via Open/Create log stream apis) and indirect (KtlLogStream::Async*Contexts), 
            //         have been released. Only then will the such an outstanding StartDeleteLogStream operation complete.
            //
            virtual VOID
            StartDeleteLogStream(
                __in const KtlLogStreamId& LogStreamId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            DeleteLogStreamAsync(
                __in const KtlLogStreamId& LogStreamId,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context) = 0;

        class AsyncOpenLogStreamContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLogStreamContext);
        public:
            //
            //
            // This method opens an existing log stream. Use
            // CloseLogStream() to close access to the log stream.
            //
            // Parameters:
            //
            //  LogStreamId - Supplies the log stream KGuid id.
            //
            //  MaximumMetaDataSize - Returns the maximum number of
            //      bytes that can be accommodated in the per-stream
            //      metadata.
            //
            //  LogStream - Returns a pointer to a KTL log stream
            //      object. When the last reference to this is removed the
            //      LogStream will be closed.
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //
            //  STATUS_NOT_FOUND - The specified log stream does not exist.
            //
            //  STATUS_DELETE_PENDING - Subject Stream is in the process of being deleted
            //
            virtual VOID
            StartOpenLogStream(
                __in const KtlLogStreamId& LogStreamId,
                __out ULONG* MaximumMetaDataSize,
                __out KtlLogStream::SPtr& LogStream,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            OpenLogStreamAsync(
                __in const KtlLogStreamId& LogStreamId,
                __out ULONG* MaximumMetaDataSize,
                __out KtlLogStream::SPtr& LogStream,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context) = 0;


        //
        // Length of the alias name is limited to 128 characters
        //
        static const ULONG MaxAliasLength = 128;
        
        class AsyncAssignAliasContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncAssignAliasContext);

            public:
                //
                // This method asynchronously assigns a string that serves
                // as an alias to the LogStreamId. The alias can be
                // resolved to the LogStreamId using the
                // AsyncResolveAliasContext. The alias can be remove
                // using the AsyncRemoveAliasContext.
                // If the stream already has an alias that alias is overwritten.
                //
                // Parameters:
                //
                //  Alias - the alias to associate with the Log stream id
                //
                //  LogStreamId - the log stream id to associate with the alias
                //
                //  ParentAsyncContext - Supplies an optional parent async context.
                //
                //  CallbackPtr - Supplies an optional callback delegate to be notified
                //      when the read completes.
                //
                // Completion status:
                //
                //  STATUS_SUCCESS - The operation completed successfully.
                //
                virtual VOID
                StartAssignAlias(
                    __in KString::CSPtr& Alias,
                    __in KtlLogStreamId LogStreamId,                             
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
                    
#if defined(K_UseResumable)
                virtual ktl::Awaitable<NTSTATUS>
                AssignAliasAsync(
                    __in KString::CSPtr& Alias,
                    __in KtlLogStreamId LogStreamId,                             
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncAssignAliasContext(__out AsyncAssignAliasContext::SPtr& Context) = 0;

        class AsyncRemoveAliasContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncRemoveAliasContext);

            public:
                //
                // This method asynchronously removes a string that serves
                // as an alias to the LogStreamId. 
                //
                // Parameters:
                //
                //  Alias - the alias to remove association with the Log stream id
                //
                //  ParentAsyncContext - Supplies an optional parent async context.
                //
                //  CallbackPtr - Supplies an optional callback delegate to be notified
                //      when the read completes.
                //
                // Completion status:
                //
                //  STATUS_SUCCESS - The operation completed successfully.
                //  STATUS_NOT_FOUND - An alias to remove was not found
                //
                virtual VOID
                StartRemoveAlias(
                    __in KString::CSPtr& Alias,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
                    
#if defined(K_UseResumable)
                virtual ktl::Awaitable<NTSTATUS>
                RemoveAliasAsync(
                    __in KString::CSPtr& Alias,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncRemoveAliasContext(__out AsyncRemoveAliasContext::SPtr& Context) = 0;
        
        class AsyncResolveAliasContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncResolveAliasContext);

            public:
                //
                // This method asynchronously resolves the alias to the
                // LogStreamId if the alias had been previously set by
                // the AsyncAssignAliasContext
                //
                // Parameters:
                //
                //  Alias - the alias to associate with the log stream id
                //
                //  LogStreamId - returns the log stream id associated with the alias
                //
                //  ParentAsyncContext - Supplies an optional parent async context.
                //
                //  CallbackPtr - Supplies an optional callback delegate to be notified
                //      when the read completes.
                //
                // Completion status:
                //
                //  STATUS_SUCCESS - The operation completed successfully.
                //  STATUS_NOT_FOUND - An alias for the log stream id was not found
                //
                virtual VOID
                StartResolveAlias(
                    __in KString::CSPtr& Alias,
                    __out KtlLogStreamId* LogStreamId,                           
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
                virtual ktl::Awaitable<NTSTATUS>
                ResolveAliasAsync(
                    __in KString::CSPtr& Alias,
                    __out KtlLogStreamId* LogStreamId,                           
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncResolveAliasContext(__out AsyncResolveAliasContext::SPtr& Context) = 0;


        class AsyncEnumerateStreamsContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncEnumerateStreamsContext);

            public:
                //
                // This method asynchronously resolves the alias to the
                // LogStreamId if the alias had been previously set by
                // the AsyncAssignAliasContext
                //
                // Parameters:
                //
                //  LogStreamIds - returns array of log stream ids
                //                 representing streams in the container
                //
                //  ParentAsyncContext - Supplies an optional parent async context.
                //
                //  CallbackPtr - Supplies an optional callback delegate to be notified
                //      when the read completes.
                //
                // Completion status:
                //
                //  STATUS_SUCCESS - The operation completed successfully.
                //  STATUS_NOT_FOUND - An alias for the log stream id was not found
                //
                virtual VOID
                StartEnumerateStreams(
                    __out KArray<KtlLogStreamId>& LogStreamIds,                           
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
                virtual ktl::Awaitable<NTSTATUS>
                EnumerateStreamsAsync(
                    __out KArray<KtlLogStreamId>& LogStreamIds,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncEnumerateStreamsContext(__out AsyncEnumerateStreamsContext::SPtr& Context) = 0;
        
        
        typedef KDelegate<VOID(
            KAsyncContextBase* const,           // Parent; can be nullptr
            KtlLogContainer&,                   // Log Container instance
            NTSTATUS                            // Completion status of the Close
            )> CloseCompletionCallback;
        
        virtual NTSTATUS StartClose(
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt CloseCompletionCallback CloseCallbackPtr
        ) = 0;

#if defined(K_UseResumable)
        virtual ktl::Awaitable<NTSTATUS>
        CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
};

//
// The KtlLogManager object is the root entry into the Ktl log system instance.
// All other Ktl log objects are created directly or indirectly
// through the KtlLogManager interface.
//
// There is typically one global KtlLogManager per machine/process.
// This is to ensure singleton properties of all other objects
// created through the KtlLogManager interface. This singleton property
// is important to coordinate all I/Os from different log users
// to the same physical log, maintain log LSN consistency etc. It is the
// responsibility of the user of the KtlLog facility to manage the lifetime
// of KtlLogManager instances in order to maintain this property.
//
class KtlLogManager abstract : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KtlLogManager);

    
    public:
        //
        // This is the maximum length in characters for the pathname of a container or stream
        //
        static const ULONG MaxPathnameLengthInChar = 296;
        static const DWORD FlagSparseFile = 1;
        

        //
        // Factory for creation of KtlLogManager Service instances.
        //
        // Service is opened and closed using KAsyncServiceBase functionality (see
        // KasyncService.h).
        //
        // To use the KtlLogManager you would do the following:
        //
        //     1. KtlLogManager::Create() to create an instance
        //     2. KtlLogManager::StartOpenLogManager() to initialize the log manager.
        //        When the OpenCompletionCallback is invoked, the log manager is
        //        ready for use.
        //     3. KtlLogManager::CloseLogManager() to shutdown the log manager.
        //        CloseCompletionCallback is invoked when the log
        //        manager and all log containers and log streams and
        //        log operations are shutdown.
        //
        static NTSTATUS
        CreateInproc(__in ULONG AllocationTag, __in KAllocator& Allocator, __out KtlLogManager::SPtr& Result);

        static NTSTATUS
        CreateDriver(__in ULONG AllocationTag, __in KAllocator& Allocator, __out KtlLogManager::SPtr& Result);

        virtual NTSTATUS
        StartOpenLogManager(
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in_opt OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr) = 0;

#if defined(K_UseResumable)
        virtual ktl::Awaitable<NTSTATUS>
        OpenLogManagerAsync(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr) = 0;
#endif

        virtual NTSTATUS
        StartCloseLogManager(
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in_opt CloseCompletionCallback CloseCallbackPtr) = 0;

#if defined(K_UseResumable)
        virtual ktl::Awaitable<NTSTATUS>
        CloseLogManagerAsync(
            __in_opt KAsyncContextBase* const ParentAsync) = 0;
#endif
        
        class AsyncCreateLogContainer abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCreateLogContainer);

            public:
                
                //
                // This method asynchronously create a new log container.
                // Once it completes, the log container is created and formatted appropriately.
                //
                // Parameters:
                //      DiskId              - Supplies KGuid ID of the disk on which the log will reside. 
                //      Path                - Supplies fully qualified pathname to the logger file
                //      LogContainerId      - Supplies ID of the log.
                //      LogSize             - Supplies the desired size of the log container in bytes.
                //      MaximumRecordSize   - Supplies the maximum size of a record that can be written to a stream in
                //                            the log container
                //      MaximumNumberStreams- Supplies the maximum number of streams that can be created in the log container
                //      Flags               - Supplies flags used to affect how log container file is created. Valid
                //                            values are 0 or FlagSparseFile
                //      ParentAsyncContext  - Supplies an optional parent async context.
                //      CallbackPtr         - Supplies an optional callback delegate to be notified
                //                            when the log is created on disk.
                //
                // Returns:
                //      LogContainer        - Returns pointer to the log container object. When the last reference to the
                //                            log container is released then the log container is closed.
                //
                // Completion status:
                //      STATUS_SUCCESS                  - The operation completed successfully.
                //      STATUS_OBJECT_NAME_COLLISION    - A log with the same {DiskId, LogContainerId} already exists.
                //
                virtual VOID
                StartCreateLogContainer(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

                virtual VOID
                StartCreateLogContainer(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
                
                virtual VOID
                StartCreateLogContainer(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

                virtual VOID
                StartCreateLogContainer(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
            
#if defined(K_UseResumable)
                virtual ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;

                virtual ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;

                virtual ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;

                virtual ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncCreateLogContainerContext(__out AsyncCreateLogContainer::SPtr& Context) = 0;

        class AsyncOpenLogContainer abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLogContainer);

        public:
            //
            // This method asynchronously opens an existing log.
            // Once it completes, the returned log object has finished its
            // crash recovery and is ready to use.
            //
            // Parameters:
            //
            //      DiskId              - Supplies KGuid ID of the disk on which the log resides.
            //      Path                - Supplies fully qualified pathname to the logger file
            //      LogContainerId      - Supplies ID of the log container.
            //      LogContainer        - Returns a pointer to the log container object
            //      ParentAsyncContext  - Supplies an optional parent async context.
            //      CallbackPtr         - Supplies an optional callback delegate to be notified
            //                            when the log is opened and ready to use.
            //
            // Completion status:
            //      STATUS_SUCCESS                  - The operation completed successfully.
            //      STATUS_NOT_FOUND                - The speficied log does not exist.
            //      STATUS_CRC_ERROR                - The log file failed CRC verification.
            //      K_STATUS_LOG_STRUCTURE_FAULT    - The log file's structure is incorrect
            //      K_STATUS_LOG_INVALID_LOGID      - The log file's structure's log container id is incorrect
            //
            virtual VOID
            StartOpenLogContainer(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogContainerId,
                __out KtlLogContainer::SPtr& LogContainer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            OpenLogContainerAsync(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __out KtlLogContainer::SPtr& LogContainer) = 0;
#endif
            
            virtual VOID
            StartOpenLogContainer(
                __in KString const & Path,
                __in const KtlLogContainerId& LogContainerId,
                __out KtlLogContainer::SPtr& LogContainer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            OpenLogContainerAsync(
                __in KString const & Path,
                __in const KtlLogContainerId& LogId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __out KtlLogContainer::SPtr& LogContainer) = 0;
#endif
        };

        virtual NTSTATUS
        CreateAsyncOpenLogContainerContext(__out AsyncOpenLogContainer::SPtr& Context) = 0;

        class AsyncDeleteLogContainer abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncDeleteLogContainer);

        public:
            //
            // This method asynchronously deletes an existing log.
            //
            // Parameters:
            //
            //      DiskId              - Supplies KGuid ID of the disk on which the log resides.
            //      Path                - Supplies fully qualified pathname to the logger file
            //      LogContainerId      - Supplies ID of the log.
            //      ParentAsyncContext  - Supplies an optional parent async context.
            //      CallbackPtr         - Supplies an optional callback delegate to be notified
            //                            when the log is deleted.
            //
            // Completion status:
            //      STATUS_SUCCESS      - The operation completed successfully.
            //      STATUS_NOT_FOUND    - The speficied log does not exist.
            //

            virtual VOID
            StartDeleteLogContainer(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

            virtual VOID
            StartDeleteLogContainer(
                __in KString const & Path,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

#if defined(K_UseResumable)
            virtual ktl::Awaitable<NTSTATUS>
            DeleteLogContainerAsync(
                __in KString const & Path,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext) = 0;
#endif      
        };

        virtual NTSTATUS
        CreateAsyncDeleteLogContainerContext(__out AsyncDeleteLogContainer::SPtr& Context) = 0;

        class AsyncEnumerateLogContainers abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncEnumerateLogContainers);

        public:
            // This method asynchronously enumerates all log containers
            // on a disk. The list of log containers includes those log
            // containers created as a consequence of creating a
            // destaged logical log as well as those log containers
            // created on this disk by other applications.
            //
            // Parameters:
            //      DiskId              - Supplies KGuid ID of the disk.
            //      ParentAsyncContext  - Supplies an optional parent async context.
            //      CallbackPtr         - Supplies an optional callback delegate to be notified
            //                            when the enumeration is complete.
            // Returns:
            //      LogIdArray          - Array of log IDs on the selected disk.
            //
            // Completion status:
            //      STATUS_SUCCESS      - The operation completed successfully.
            //
            virtual VOID
            StartEnumerateLogContainers(
                __in const KGuid& DiskId,
                __out KArray<KtlLogContainerId>& LogIdArray,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
        };

        virtual NTSTATUS
        CreateAsyncEnumerateLogContainersContext(__out AsyncEnumerateLogContainers::SPtr& Context)= 0;


        typedef enum
        {
            ConfigureSharedLogContainerSettings,
            ConfigureMemoryThrottleLimits,
            ConfigureMemoryThrottleLimits2,
            QueryMemoryThrottleUsage,
            QueryDebugUnitTestInformation,
            ConfigureMemoryThrottleLimits3,
            ConfigureAcceleratedFlushLimits
        } ConfigureCode;

        //
        // ConfigureSharedLogContainerSettings
        //
        typedef struct _SharedLogContainerSettings
        {
            //
            // This is the minimum size for the shared log as enforced
            // by the configuration control code
            //
#if DBG
            static const LONGLONG _DefaultLogSizeMin = 0x10000000;   // 256MB. Values lower than this are rejected by the core logger
#else
            static const LONGLONG _DefaultLogSizeMin = 0x20000000;   // 512MB
#endif
            static const LONGLONG _DefaultLogSize =    0x200000000;  // 8GB

            //
            // These are the minimum and maximum number of streams to
            // create the log. They are enforced by the configuration
            // control code
            //
            static const ULONG _DefaultMaximumNumberStreamsMin = 3 * 512;
            static const ULONG _DefaultMaximumNumberStreamsMax = 3 * 8192;
            
            WCHAR Path[512];
            GUID DiskId;
            KtlLogContainerId LogContainerId;
            LONGLONG LogSize;
            ULONG MaximumNumberStreams;
            ULONG MaximumRecordSize;
            DWORD Flags;
        } SharedLogContainerSettings;

        //
        // ConfigureMemoryThrottleLimits
        //
        typedef struct _MemoryThrottleLimits
        {
            //
            // This is the default, minimum and maximum value for the timer that flushes the
            // write memory buffers after a period of inactivity.
            //
            static const ULONG _DefaultPeriodicFlushTimeInSec = 1 * 60;
            static const ULONG _DefaultPeriodicFlushTimeInSecMin = 15;
            static const ULONG _DefaultPeriodicFlushTimeInSecMax = 5 * 60;
            
            //
            // This is the default, minimum and maximum value for the
            // timer interval that underlies the timer that flushes the
            // write memory buffers after a period of inactivity.
            //
            static const ULONG _DefaultPeriodicTimerIntervalInSec = 10;
            static const ULONG _DefaultPeriodicTimerIntervalInSecMin = 1;
            static const ULONG _DefaultPeriodicTimerIntervalInSecMax = 60;

            //
            // This value indicates that the setting has no limit
            //
            static const LONGLONG _NoLimit = -1;

            //
            // These values define the default size values and
            // configuration ranges for the write buffer pool.
            // The WriteBufferMemoryPool is a pool of memory available
            // for all streams to use when allocating memory to store
            // data being coalesced into a larger record for the
            // dedicated log. The WriteBufferMemoryPool has a total
            // allocation limit which is the maximum amount of memory
            // that can be allocated at one time. If there is more
            // demand for memory above the total allocation limit then
            // allocation requests will wait until memory is freed. The
            // total allocation limit may grow whenever a stream is
            // opened and shrink when it is closed by
            // WriteBufferMemoryPoolPerStream. However the total
            // allocation limit will never grow above
            // WriteBufferMemoryPoolMax.
            //

            //
            // Default maximum and minimum value for the WriteBufferMemoryPool
            // as enforced by the configuration control.
            //
            static const LONGLONG _DefaultWriteBufferMemoryPoolMax = _NoLimit;
            static const LONGLONG _DefaultWriteBufferMemoryPoolMin = 0x200000000;  // 8GB
            //
            // This defines the minimum value configurable for the
            // minimum size of the WriteBufferMemoryPool
            //
            static const LONGLONG _DefaultWriteBufferMemoryPoolMinMin = 0x1000000;  // 16MB

            //
            // Default size for the amount of memory to grow and shrink
            // the WriteBufferMemoryPool when a stream opens and
            // closes.
            //
            static const LONG     _DefaultWriteBufferMemoryPoolPerStream = 0x100000;     // 1MB
            //
            // Minimum configurable size of the amount of memory added
            // and removed to the total allocation limit for each stream open and
            // close.
            //
            static const LONG     _DefaultWriteBufferMemoryPoolPerStreamMin = 0x100000;     // 1MB

            //
            // When a call is made from user to kernel mode, memory is
            // pinned so that kernel mode code can use it. This is the
            // default size for  the number of bytes of memory that can
            // be pinned at one time.
            //
            static const LONGLONG _DefaultPinnedMemoryLimit = _NoLimit;
#if DBG
            static const LONGLONG _PinnedMemoryLimitMin = (256 * 1024);            // 256K
#else
            static const LONGLONG _PinnedMemoryLimitMin = 64 * (1024 * 1024);       // 64MB
#endif

            //
            // Allocations to the WriteBufferMemoryPool may have a
            // timeout that will fail the allocation request after a
            // period of time. 
            //

            //
            // This value indicates that the allocation timeout is
            // infinite.
            //
            static const ULONG _NoAllocationTimeoutInMs = 0;
            //
            // This value indicates that the default allocation timeout
            // should be used.
            //
            static const ULONG _UseDefaultAllocationTimeoutInMs = (ULONG)-1;
            //
            // These are the default allocation timeout, minimum and
            // maximum values.
            //
            static const ULONG _DefaultAllocationTimeoutInMs = 5 * 60 * 1000;  // 5 minutes         
            static const ULONG _DefaultAllocationTimeoutInMsMin = _NoAllocationTimeoutInMs;
            static const ULONG _DefaultAllocationTimeoutInMsMax = 60 * 60 * 1000;  // 60 minutes            
            
            LONGLONG WriteBufferMemoryPoolMax;
            LONGLONG WriteBufferMemoryPoolMin;
            LONG WriteBufferMemoryPoolPerStream;
            LONGLONG PinnedMemoryLimit;
            ULONG PeriodicFlushTimeInSec;
            ULONG PeriodicTimerIntervalInSec;
            ULONG AllocationTimeoutInMs;

            // ConfigureMemoryThrottleLimits2

            static const LONGLONG _DefaultMaximumDestagingWriteOutstanding = _NoLimit;
            static const LONGLONG _DefaultMaximumDestagingWriteOutstandingMin = 16 * (1024 * 1024);
            LONGLONG MaximumDestagingWriteOutstanding;
            
            // ConfigureMemoryThrottleLimits3

            static const ULONG _NoSharedLogThrottleLimit = 100;      // 100% implies no throttling
            static const ULONG _DefaultSharedLogThrottleLimit = 90;  //  90% shared log usage
            ULONG SharedLogThrottleLimit;
            ULONG Reserved;
            
        } MemoryThrottleLimits;

        typedef struct 
        {
            MemoryThrottleLimits ConfiguredLimits;
            LONGLONG CurrentAllocations;
            LONGLONG TotalAllocationLimit;
            LONGLONG PinnedMemoryAllocations;
            BOOLEAN IsUnderMemoryPressure;          
        } MemoryThrottleUsage;

        typedef struct
        {
            LONG PinMemoryFailureCount;
        } DebugUnitTestInformation;

        typedef struct _AcceleratedFlushLimits
        {
            static const ULONG AccelerateFlushActiveTimerInMsNoAction = 0;
            static const ULONG AccelerateFlushActiveTimerInMsMin = 1000;
            static const ULONG AccelerateFlushActiveTimerInMsMax = 10000;
            static const ULONG DefaultAccelerateFlushActiveTimerInMs = 1000;
            
            static const ULONG AccelerateFlushPassiveTimerInMsMin = 1000;
            static const ULONG AccelerateFlushPassiveTimerInMsMax = 60000;
            static const ULONG DefaultAccelerateFlushPassiveTimerInMs = 15000;
            
            static const ULONG AccelerateFlushActivePercentMin = 1;
            static const ULONG AccelerateFlushActivePercentMax = 99;
            static const ULONG DefaultAccelerateFlushActivePercent = 70;
            
            static const ULONG AccelerateFlushPassivePercentMin = 1;
            static const ULONG AccelerateFlushPassivePercentMax = 99;
            static const ULONG DefaultAccelerateFlushPassivePercent = 30;
            
            ULONG AccelerateFlushActiveTimerInMs;
            ULONG AccelerateFlushPassiveTimerInMs;
            ULONG AccelerateFlushActivePercent;
            ULONG AccelerateFlushPassivePercent;
        } AcceleratedFlushLimits;
        
        class AsyncConfigureContext abstract : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncConfigureContext);

        public:

            //
            // This method asynchronously sends a configuration message to
            // the KtlLogManager object. 
            //
            // Parameters:
            //
            //  Code is a ULONG value that specifies which
            //  configuration operation should be performed.
            //
            //  InBuffer is an optional input buffer whose contents
            //  depend upon the Code
            //
            //  OutBuffer is the output buffer whose contents
            //  depend upon the Code
            //
            //  Result is a ULONG that describes the result of the
            //  Configure
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when the read completes.
            //
            // Completion status:
            //
            //  STATUS_SUCCESS - The operation completed successfully.
            //  STATUS_INVALID_PARAMETER - A parameter passed is incorrect
            //
            virtual VOID
            StartConfigure(
                __in ULONG Code,
                __in_opt KBuffer* const InBuffer,
                __out ULONG& Result,
                __out KBuffer::SPtr& OutBuffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;
        };

        virtual NTSTATUS
        CreateAsyncConfigureContext(__out AsyncConfigureContext::SPtr& Context) = 0;                

    protected:
        using KAsyncContextBase::Cancel;
};


class KtlLogInformation
{
    public:
        //
        // This specifies the default log stream type if one is not
        // specified in the StartCreateLogStream()
        //
        static RvdLogStreamType const&    KtlLogDefaultStreamType()
        {
            // {B81BF9BA-CDA3-45ae-ADBB-09CAC3B11C01}
            static GUID const WfLLStreamType = { 0xb81bf9ba, 0xcda3, 0x45ae, { 0xad, 0xbb, 0x9, 0xca, 0xc3, 0xb1, 0x1c, 0x1 } };
            return *(reinterpret_cast<RvdLogStreamType const*>(&WfLLStreamType));
        }
};

#ifndef USE_RVDLOGGER_OBJECTS
// NOTE: Keep ktl strong types and fix rvdlogger; move these to
// rvdlogger side

class KtlLogStreamId : public KStrongType<KtlLogStreamId, KGuid>
{
    K_STRONG_TYPE(KtlLogStreamId, KGuid);

public:
    KtlLogStreamId() {}
    ~KtlLogStreamId() {}
};

// Strong unique ID and scalar types used in public API
class KtlLogStreamType : public KStrongType<KtlLogStreamType, KGuid>
{
    K_STRONG_TYPE(KtlLogStreamType, KGuid);

public:
    KtlLogStreamType() {}
    ~KtlLogStreamType() {}
};

class KtlLogContainerId : public KStrongType<KtlLogContainerId, KGuid>
{
    K_STRONG_TYPE(KtlLogContainerId, KGuid);

public:
    KtlLogContainerId() {}
    ~KtlLogContainerId() {}
};


class KtlLogAsn : public KStrongType<KtlLogAsn, ULONGLONG volatile>
{
    K_STRONG_TYPE(KtlLogAsn, ULONGLONG volatile);

public:
    KtlLogAsn() : KStrongType<KtlLogAsn, ULONGLONG volatile>(KtlLogAsn::Null()) {}

    // All valid log ASNs must be between Null and Max, exclusively.
    static KtlLogAsn const&
    Null()
    {
        static KtlLogAsn const nullAsn((ULONGLONG)0);
        return nullAsn;
    }

    static KtlLogAsn const&
    Min()
    {
        static KtlLogAsn const minAsn((ULONGLONG)1);
        return minAsn;
    }

    static KtlLogAsn const&
    Max()
    {
        static KtlLogAsn const maxAsn(_UI64_MAX);
        return maxAsn;
    }

    BOOLEAN
    IsNull()
    {
        return (*this) == Null();
    }

    __inline BOOLEAN
    SetIfLarger(ULONGLONG newValue)
    {
        ULONGLONG volatile  oldValue;
        while (newValue > (oldValue = _Value))
        {
            if (InterlockedCompareExchange64(
                (LONGLONG volatile*)&_Value,
                (LONGLONG)newValue,
                (LONGLONG)oldValue) == oldValue)
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    __inline BOOLEAN
    SetIfLarger(KtlLogAsn& NewSuggestedAsn)
    {
        return SetIfLarger(NewSuggestedAsn.Get());
    }
};
#endif

struct KtlLogStream::RecordMetadata
{
    KtlLogAsn                       Asn;
    ULONGLONG                       Version;
    KtlLogStream::RecordDisposition Disposition;
    ULONG                           Size;
    LONGLONG                        Debug_LsnValue;
};

