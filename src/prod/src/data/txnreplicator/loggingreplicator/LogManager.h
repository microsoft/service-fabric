// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // A base class for all the log managers
        //
        class LogManager
            : public KObject<LogManager>
            , public KShared<LogManager>
            , public KWeakRefType<LogManager>
            , public Utilities::IDisposable
            , public IFlushCallbackProcessor
            , public ILogManager
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(LogManager)
                K_SHARED_INTERFACE_IMP(IDisposable)
                K_SHARED_INTERFACE_IMP(IFlushCallbackProcessor)
                K_SHARED_INTERFACE_IMP(ILogManagerReadOnly)
                K_SHARED_INTERFACE_IMP(ILogManager)
                K_WEAKREF_INTERFACE_IMP(IFlushCallbackProcessor, LogManager)

        public:
            static KString::CSPtr GenerateLogFileAlias(
                __in KGuid const & partitionId,
                __in LONG64 const & replicaId,
                __in KAllocator & allocator);

            //
            // Generate backup/copy alias' using either the currentLogFileAlias_ or baseLogFileAlias_ 
            // By default the baseLogFileAlias is used
            // 
            static KString::SPtr Concat(
                __in KString const & str1,
                __in KStringView const & str2,
                __in KAllocator & allocator);

            __declspec(property(get = get_CurrentLastPhysicalRecord)) LogRecordLib::PhysicalLogRecord::SPtr CurrentLastPhysicalRecord;
            LogRecordLib::PhysicalLogRecord::SPtr get_CurrentLastPhysicalRecord() const
            {
                ASSERT_IFNOT(physicalLogWriter_ != nullptr, "Unexpected null physical log writer getting current last physical record in getter");

                return physicalLogWriter_->CurrentLastPhysicalRecord;
            }

            __declspec(property(get = get_CurrentLogTailPosition)) ULONG64 CurrentLogTailPosition;
            ULONG64 get_CurrentLogTailPosition() const
            {
                // Once set in PrepareToLog, this can change during log rename. However, log is renamed only on secondary and during that time, the replica is quiesced.
                // Hence no one else can be accesing this field

                ASSERT_IFNOT(physicalLogWriter_ != nullptr, "Unexpected null physical log writer getting current log tail position in getter");

                LogRecordLib::LogRecord::SPtr tailRecord = physicalLogWriter_->CurrentLogTailRecord;

                ASSERT_IFNOT(tailRecord != nullptr, "Unexpected null tail log record getting current log tail position in getter");

                return tailRecord->RecordPosition;
            }

            __declspec(property(get = get_Length)) LONG64 Length;
            LONG64 get_Length() const
            {
                return logicalLog_->GetLength();
            }

            __declspec(property(get = get_BufferedBytes)) LONG64 BufferedBytes;
            LONG64 get_BufferedBytes()
            {
                return physicalLogWriter_->BufferedRecordsBytes;
            }

            __declspec(property(get = get_PendingFlushBytes)) LONG64 PendingFlushBytes;
            LONG64 get_PendingFlushBytes()
            {
                return physicalLogWriter_->PendingFlushRecordsBytes;
            }

            __declspec(property(get = get_PhysicalLogWriter)) PhysicalLogWriter::SPtr PhysicalLogWriter;
            PhysicalLogWriter::SPtr get_PhysicalLogWriter() const
            {
                // Once set in PrepareToLog, this can change during log rename. However, log is renamed only on secondary and during that time, the replica is quiesced.
                // Hence no one else can be accesing this field
                ASSERT_IFNOT(physicalLogWriter_ != nullptr, "Unexpected null physical log writer in getter");

                return physicalLogWriter_;
            }

            __declspec(property(get = get_IsCompletelyFlushed)) bool IsCompletelyFlushed;
            bool get_IsCompletelyFlushed() const
            {
                return physicalLogWriter_->IsCompletelyFlushed;
            }

            __declspec(property(get = get_ShouldThrottleWrites)) bool ShouldThrottleWrites;
            bool get_ShouldThrottleWrites()
            {
                return physicalLogWriter_->ShouldThrottleWrites;
            }

            // 
            // Initializes the Log Manager with the full path to the dedicated log
            //
            virtual ktl::Awaitable<NTSTATUS> InitializeAsync(__in KString const & dedicatedLogPath);

            //
            // OpenAsync is where the log file is created or opened if it already exists
            //
            virtual ktl::Awaitable<NTSTATUS> OpenAsync(__out KSharedPtr<LogRecordLib::RecoveryPhysicalLogReader> & result);

            //
            // OpenAsync is where the log file is created with the log records from the restoreFileArray.
            //
            virtual ktl::Awaitable<NTSTATUS> OpenWithRestoreFilesAsync(
                __in KArray<KString::CSPtr> const & restoreFileArray,
                __in LONG64 blockSizeInKB,
                __in LogRecordLib::InvalidLogRecords & invalidLogRecords,
                __in ktl::CancellationToken const & cancellationToken,
                __out KSharedPtr<LogRecordLib::RecoveryPhysicalLogReader> & result) noexcept;

            //
            // Deletes the log resource
            //
            virtual ktl::Awaitable<NTSTATUS> DeleteLogAsync() = 0;

            //
            // During a full copy, the idle replica first creates a copy log, which is then renamed once the copy is completed
            //
            virtual ktl::Awaitable<NTSTATUS> CreateCopyLogAsync(
                __in TxnReplicator::Epoch const startingEpoch,
                __in LONG64 startingLsn,
                __out LogRecordLib::IndexingLogRecord::SPtr & newHead) = 0;

            //
            // Replaces the copy log (which is currently being used to log) to the actual log file
            // Marks the existing log file as log_backup
            //
            virtual ktl::Awaitable<NTSTATUS> RenameCopyLogAtomicallyAsync() = 0;

            //
            // Invoked by the replicator before closing
            //
            virtual ktl::Awaitable<NTSTATUS> CloseLogAsync();

            //
            // Initializes the log manager to accept log write operations
            //
            void PrepareToLog(
                __in LogRecordLib::LogRecord & tailRecord,
                __in PhysicalLogWriterCallbackManager & callbackManager);

            //
            // Stops accepting new log records
            //
            void PrepareToClose();

            // 
            // Adds the record to the list of buffered records without issuing a write to the disk
            // The input is not a const because the record's previous physical log record and psn are updated in this method
            //
            // Returns the number of bytes that are buffered currently
            //
            LONG64 InsertBufferedRecord(__in LogRecordLib::LogRecord & record);

            //
            // Returns the flush completion task from the underlying physical log writer
            //
            ktl::AwaitableCompletionSource<void>::SPtr GetFlushCompletionTask();

            //
            // Performs the log tail truncation during false progress scenario of idle replicas
            //
            ktl::Awaitable<NTSTATUS> PerformLogTailTruncationAsync(__in LogRecordLib::LogRecord & newTailRecord);

            //
            // Performs log head truncation.
            //
            // On the primary, there might be pending readers that are used to build other replicas.
            // This method ensures the head is truncated only after all readers that have a handle before the new head are destructed
            //
            ktl::AwaitableCompletionSource<void>::SPtr ProcessLogHeadTruncationAsync(__in LogRecordLib::TruncateHeadLogRecord & newHeadRecord);

            virtual void Dispose() override;

            //
            // If there are multiple full copies in progress, returns the earliest LSN amongst them. Otherwise, just return the full copy starting lsn
            // If there is no full copy, returns LONG64MAXVALUE
            //
            // This is used by the state manager to perform hard deletes of items that have been soft deleted
            //
            LONG64 GetEarliestFullCopyStartingLsn();

            //
            // Returns the earliest log reader 
            //
            LogReaderRange GetEarliestLogReaderRange();

            void SetLogHeadRecordPosition(__in ULONG64 newHeadPosition);

        public: // ILogManagerReadOnly Interface

            __declspec(property(get = get_CurrentLogTailRecord)) LogRecordLib::LogRecord::SPtr CurrentLogTailRecord;
            LogRecordLib::LogRecord::SPtr get_CurrentLogTailRecord() const override
            {
                // Once set in PrepareToLog, this can change during log rename. However, log is renamed only on secondary and during that time, the replica is quiesced.
                // Hence no one else can be accesing this field
                ASSERT_IFNOT(physicalLogWriter_ != nullptr, "Unexpected null physical log writer getting current log tail record in getter");

                return physicalLogWriter_->CurrentLogTailRecord;
            }

            __declspec(property(get = get_InvalidLogRecords)) LogRecordLib::InvalidLogRecords::SPtr InvalidLogRecords;
            LogRecordLib::InvalidLogRecords::SPtr get_InvalidLogRecords() const override
            {
                return invalidLogRecords_;
            }

            //
            // Returns a log reader between the 2 positions below
            //
            KSharedPtr<LogRecordLib::IPhysicalLogReader> GetPhysicalLogReader(
                __in ULONG64 startingRecordPosition,
                __in ULONG64 endingRecordPosition,
                __in LONG64 startingLsn,
                __in KStringView const & readerName,
                __in LogRecordLib::LogReaderType::Enum readerType) override;

            Data::Log::ILogicalLogReadStream::SPtr CreateReaderStream() override;

            void SetSequentialAccessReadSize(
                __in Data::Log::ILogicalLogReadStream & readStream,
                __in LONG64 sequentialReadSize) override;

            //
            // Adds a Ref counted reader lock on the start and end position to ensure the log is not truncated while there are pending readers for it
            //
            bool AddLogReader(
                __in LONG64 startingLsn,
                __in ULONG64 startingRecordPosition,
                __in ULONG64 endingRecordPosition,
                __in KStringView const & readerName,
                __in LogRecordLib::LogReaderType::Enum readerType) override;

            //
            // Decrements Ref count on the start position.
            // If the count goes to 0, the reader is removed
            //
            void RemoveLogReader(__in ULONG64 startingRecordPosition) override;

        public: // ILogManager Interface
            // 
            // Issues a flush for all the buffered records
            //
            ktl::Awaitable<void> FlushAsync(__in KStringView const & initiator) override;

        protected:
            static const int DefaultWriteCacheSizeMB;

            LogManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            __declspec(property(get = get_IsDisposed)) bool IsDisposed;
            bool get_IsDisposed() const
            {
                return isDisposed_;
            }

            //
            // Creates the logical log file stream depending of the kind of log manager - file/memory/ktl
            //
            virtual ktl::Awaitable<NTSTATUS> CreateLogFileAsync(
                __in bool createNew,
                __in ktl::CancellationToken const & cancellationToken,
                __out Data::Log::ILogicalLog::SPtr & result) = 0;

            //
            // Used by the derived classes to close the existing log, before creating a new copy log or renaming the existing copy log
            // 
            ktl::Awaitable<NTSTATUS> CloseCurrentLogAsync();

            void SetCurrentLogFileAliasToCopy();

            static const KStringView BackupSuffix;
            static const KStringView CopySuffix;
            static const KStringView LogFileNameSuffix;

            KString::CSPtr baseLogFileAlias_;
            KString::CSPtr currentLogFileAlias_;
            KString::CSPtr logFileDirectoryPath_;

            ULONG64 logHeadRecordPosition_;
            PhysicalLogWriter::SPtr physicalLogWriter_;
            Data::Log::ILogicalLog::SPtr logicalLog_;

            LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;

        private:

            class PendingLogHeadTruncationContext
                : public KObject<PendingLogHeadTruncationContext>
                , public KShared<PendingLogHeadTruncationContext>
            {
                K_FORCE_SHARED(PendingLogHeadTruncationContext)

            public:

                static PendingLogHeadTruncationContext::SPtr Create(
                    __in KAllocator & allocator,
                    __in ktl::AwaitableCompletionSource<void> & tcs,
                    __in LogRecordLib::TruncateHeadLogRecord & truncateHeadRecord);

                __declspec(property(get = get_ProposedPosition)) ULONG64 ProposedPosition;
                ULONG64 get_ProposedPosition() const
                {
                    return truncateHeadRecord_->HeadRecord->RecordPosition;
                }

                __declspec(property(get = get_NewHead)) LogRecordLib::IndexingLogRecord::SPtr NewHead;
                LogRecordLib::IndexingLogRecord::SPtr get_NewHead() const
                {
                    return truncateHeadRecord_->HeadRecord;
                }

                __declspec(property(get = get_Tcs)) ktl::AwaitableCompletionSource<void>::SPtr & Tcs;
                ktl::AwaitableCompletionSource<void>::SPtr const & get_Tcs() const
                {
                    return tcs_;
                }

            private:

                PendingLogHeadTruncationContext(
                    __in ktl::AwaitableCompletionSource<void> & tcs,
                    __in LogRecordLib::TruncateHeadLogRecord & truncateHeadRecord);

                ktl::AwaitableCompletionSource<void>::SPtr tcs_;
                LogRecordLib::TruncateHeadLogRecord::SPtr truncateHeadRecord_;
            };

            void ProcessFlushedRecordsCallback(__in LoggedRecords const & loggedRecords) noexcept override;

            ktl::Task PerformLogHeadTruncationAsync(__in PendingLogHeadTruncationContext & truncationContext);

            ktl::Awaitable<void> DisposeLogicalLog();

            bool isDisposed_;
            PhysicalLogWriterCallbackManager::SPtr emptyCallbackManager_;

            KSpinLock readersLock_; // protects the earliest log reader and the ranges below
            LogReaderRange earliestLogReader_;
            KArray<LogReaderRange> logReaderRanges_;

            // Truncation can get blocked behind a reader who is still using the log
            // This context is used to resume the truncation after all the readers that are blocking the truncation get disposed
            PendingLogHeadTruncationContext::SPtr pendingLogHeadTruncationContext_;

            TxnReplicator::TRPerformanceCountersSPtr const perfCounters_;

            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;
        };
    }
}
