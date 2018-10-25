// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        typedef enum PerfCounterName
        {
            Invalid,
            LogFlushBytes,
            AvgBytesPerFlush,
            AvgFlushLatency,
            AvgSerializationLatency

        } PerfCounterName;

        //
        // PhysicalLogWriter is the component that manages the logical log stream and primarily provides the WAL semantics to the replicator
        // The 2 primary functions are:
        //  1. InsertBufferedRecord(LogRecord) - It inserts a log record to the buffered list of records. It is not thread safe and the upper level must call this 
        //                                       method in the order of appending records in the log.
        //
        //  2. FlushAsync() - Flushes all the records that were inserted into the buffered list. If there is an outstanding flush pending, a new flush is not issued until the previous one completes
        //
        class PhysicalLogWriter final 
            : public Utilities::IDisposable
            , public KObject<PhysicalLogWriter>
            , public KShared<PhysicalLogWriter>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(PhysicalLogWriter)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            static PhysicalLogWriter::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Data::Log::ILogicalLog & logicalLogStream,
                __in PhysicalLogWriterCallbackManager & callbackManager,
                __in LONG maxWriteCacheSizeBytes,
                __in bool recomputeOffsets,
                __in LogRecordLib::LogRecord & invalidTailRecord,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

            static PhysicalLogWriter::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Data::Log::ILogicalLog & logicalLogStream,
                __in PhysicalLogWriterCallbackManager & callbackManager,
                __in LONG maxWriteCacheSizeBytes,
                __in bool recomputeOffsets,
                __in LogRecordLib::LogRecord & tailRecord,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in NTSTATUS closedError,
                __in KAllocator & allocator);

            __declspec(property(get = get_CurrentLogTailRecord)) LogRecordLib::LogRecord::SPtr CurrentLogTailRecord;
            LogRecordLib::LogRecord::SPtr get_CurrentLogTailRecord() const
            {
                return currentLogTailRecord_.Get();
            }

            __declspec(property(get = get_CurrentLastPhysicalRecord)) LogRecordLib::PhysicalLogRecord::SPtr CurrentLastPhysicalRecord;
            LogRecordLib::PhysicalLogRecord::SPtr get_CurrentLastPhysicalRecord() const
            {
                ASSERT_IF(
                    LogRecordLib::LogRecord::IsInvalid(lastPhysicalRecord_.RawPtr()),
                    "{0} LogRecord::IsInvalid(lastPhysicalRecord_.RawPtr)",
                    TraceId);

                return lastPhysicalRecord_;
            }

            __declspec(property(get = get_ClosedError)) NTSTATUS ClosedError;
            NTSTATUS get_ClosedError() const
            {
                return closedError_.load();
            }

            __declspec(property(get = get_CallbackManager)) PhysicalLogWriterCallbackManager::SPtr const & CallbackManager;
            PhysicalLogWriterCallbackManager::SPtr const & get_CallbackManager() const
            {
                return callbackManager_;
            }

            __declspec(property(get = get_BufferedRecordsBytes)) LONG64 BufferedRecordsBytes;
            LONG64 get_BufferedRecordsBytes()
            {
                return bufferedRecordsBytes_.load();
            }

            __declspec(property(get = get_PendingFlushRecordsBytes)) LONG64 PendingFlushRecordsBytes;
            LONG64 get_PendingFlushRecordsBytes()
            {
                return pendingFlushRecordsBytes_.load();
            }

            __declspec(property(get = get_IsCompletelyFlushed )) bool IsCompletelyFlushed;
            bool get_IsCompletelyFlushed() const
            {
                return (bufferedRecords_ == nullptr && pendingFlushRecords_ == nullptr && flushingRecords_ == nullptr);
            }

            __declspec(property(get = get_ShouldThrottleWrites)) bool ShouldThrottleWrites;
            bool get_ShouldThrottleWrites()
            {
                return PendingFlushRecordsBytes > maxWriteCacheSizeInBytes_;
            }

            __declspec(property(get = get_ResetCallbackManager, put = set_ResetCallbackManager )) bool ResetCallbackManager;
            bool get_ResetCallbackManager() const
            {
                return isCallbackManagerReset_;
            }

            void set_ResetCallbackManager(bool value)
            {
                isCallbackManagerReset_ = value;
            }

            __declspec(property(get = get_IsDisposed)) bool IsDisposed;
            bool get_IsDisposed() const
            {
                return isDisposed_;
            }

            void Dispose() override;

            //
            // Adds the record to the list of buffered records without issuing a write to the disk
            // The input is not a const because the record's previous physical log record and psn are updated in this method
            //
            // Returns the number of bytes that are buffered currently
            //
            LONG64 InsertBufferedRecord(__in LogRecordLib::LogRecord & logRecord);
            
            //
            // Issues a flush for all the buffered records
            //
            ktl::Awaitable<void> FlushAsync(KStringView const initiator);// This is const instead of const & as we call flushasync after a co_await
            
            // 
            // During truncate tail and re-initialization of a new log after full copy, the tail is set using this method
            //
            void SetTailRecord(__in LogRecordLib::LogRecord & tailRecord);

            // 
            // The head is truncated to the given position
            //
            ktl::Awaitable<NTSTATUS> TruncateLogHeadAsync(__in ULONG64 headRecordPosition);

            // 
            // The tail is truncated to the given position after a false progress during build
            //
            ktl::Awaitable<NTSTATUS> TruncateLogTail(__in LogRecordLib::LogRecord & newTailRecord);
            
            //
            // used by the group commit logic to ensure it waits until a pending flush is complete before issuing a new group commit
            //
            ktl::AwaitableCompletionSource<void>::SPtr GetFlushCompletionTask();

            //
            // Sets a closed exception, which ensures that further calls to flush or insertbufferedrecord does not accept new writes
            //
            void PrepareToClose();

            //
            // TEST ONLY
            // Aborts all buffered, flushing and pending flush records by setting them to nullptr
            // Used to allow PhysicalLogWriter dispose in LogTruncationManager.ShouldCheckpoint tests
            //
            void Test_ClearRecords();

        private:

            PhysicalLogWriter(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Data::Log::ILogicalLog & logicalLogStream,
                __in PhysicalLogWriterCallbackManager & callbackManager,
                __in LONG maxWriteCacheSizeBytes,
                __in bool recomputeOffsets,
                __in LogRecordLib::LogRecord & invalidTailRecord,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            PhysicalLogWriter(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Data::Log::ILogicalLog & logicalLogStream,
                __in PhysicalLogWriterCallbackManager & callbackManager,
                __in LONG maxWriteCacheSizeBytes,
                __in bool recomputeOffsets,
                __in LogRecordLib::LogRecord  & tailRecord,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in NTSTATUS closedError);

            static void WakeupFlushWaiters(__in_opt KSharedArray<ktl::AwaitableCompletionSource<void>::SPtr> const * const waitingTasks);

            static void InitializeCompletedTcs(
                __out ktl::AwaitableCompletionSource<void>::SPtr & tcs,
                __in KAllocator & allocator);

            ktl::Task FlushTask(__in ktl::AwaitableCompletionSource<void> & initiatingTcs);

            void FailedFlushTask(__inout KSharedArray<ktl::AwaitableCompletionSource<void>::SPtr>::SPtr & flushingTasks);

            void ProcessFlushedRecords(__in LoggedRecords const & loggedRecords);

            bool ProcessFlushCompletion(
                __in LoggedRecords const & flushedRecords,
                __inout KSharedArray<ktl::AwaitableCompletionSource<void>::SPtr> & flushedTasks,
                __out KSharedArray<ktl::AwaitableCompletionSource<void>::SPtr>::SPtr & flushingTasks);

            // Invoked after the flush task completes a flush
            void FlushCompleted();
            // Invoked before the flush task issues a flush
            void FlushStarting();

            Utilities::OperationData::CSPtr WriteRecord(
                __inout LogRecordLib::LogRecord & record, 
                __in ULONG bytesWritten);

            void UpdateWriteStats(
                __in Common::Stopwatch watch,
                __in ULONG size);

            void UpdatePerfCounter(
                __in PerfCounterName counterName,
                __in LONG64 value);
            
            void InitializeMovingAverageKArray();

            bool isDisposed_;

            //
            // Protects the flushing Notification tcs to enable the group commit code to wait on any pending flush before issuing a new group commit
            //
            mutable KSpinLock flushNotificationLock_;
            ktl::AwaitableCompletionSource<void>::SPtr completedFlushNotificationTcs_;

            // If this tcs is NULL, it means there is a pending flush going on.
            // If it is assigned to "completedFlushNotificationTcs_" above, it means there is no flush in progress
            // If this tcs is neither of the above, then there is a waiter who requested to be notified while a flush was in progress
            ktl::AwaitableCompletionSource<void>::SPtr flushNotificationTcs_;

            //
            // Protects the flushing logic to ensure that there is a single flush issued to the underlying logger at any point in time
            //
            mutable KSpinLock flushLock_;

            Common::atomic_long closedError_;
            Common::atomic_long loggingError_;

            bool isCallbackManagerReset_;

            LONG const maxWriteCacheSizeInBytes_;
            Common::atomic_long bufferedRecordsBytes_;
            Common::atomic_long pendingFlushRecordsBytes_;
            ULONG64 currentLogTailPosition_;
            LONG64 currentLogTailPsn_;
            
            // Since this field is accessible outside and can be set by another thread, ensure it is thread safe
            Utilities::ThreadSafeSPtrCache<LogRecordLib::LogRecord> currentLogTailRecord_;

            LogRecordLib::PhysicalLogRecord::SPtr lastPhysicalRecord_;
        
            Data::Log::ILogicalLog::SPtr logicalLogStream_;
            PhysicalLogWriterCallbackManager::SPtr callbackManager_;

            //
            // Records that have been buffered via the InsertBufferedRecord but not flushed yet
            // 
            KSharedArray<LogRecordLib::LogRecord::SPtr>::SPtr bufferedRecords_;

            //
            // Records that are currently being flushed to the log
            // 
            KSharedArray<LogRecordLib::LogRecord::SPtr>::SPtr flushingRecords_;

            //
            // Records that have been buffered and then moved to this list since there is currently a pending flush
            // 
            KSharedArray<LogRecordLib::LogRecord::SPtr>::SPtr pendingFlushRecords_;

            //
            // Tasks that represent the pending calls to FlushAsync()
            // 
            KSharedArray<ktl::AwaitableCompletionSource<void>::SPtr>::SPtr pendingFlushTasks_;

            // a binary writer backed by a memory channel that is used during serialization of log records in the write code path
            Utilities::BinaryWriter recordWriter_;

            TxnReplicator::TRPerformanceCountersSPtr const perfCounters_;

            LONG64 runningLatencySumMs_;
            LONG64 writeSpeedBytesPerSecondSum_;
            KArray<LONG64> avgRunningLatencyMilliseconds_;
            KArray<LONG64> avgWriteSpeedBytesPerSecond_;

            TxnReplicator::IOMonitor::SPtr ioMonitor_;
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;

            // Recalculate offsets between log records
            // Used during restore to ensure backwards compatiblity if additional fields are added
            bool const recomputeOffsets_;
        };
    }
}
