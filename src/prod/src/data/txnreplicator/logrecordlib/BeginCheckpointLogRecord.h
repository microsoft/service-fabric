// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class BeginCheckpointLogRecord final
            : public PhysicalLogRecord
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED(BeginCheckpointLogRecord)

        public:

            //
            // Used during initialization of a brand new log by the logmanager
            //
            static BeginCheckpointLogRecord::SPtr CreateZeroBeginCheckpointLogRecord(
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BeginTransactionOperationLogRecord & invalidBeginTx,
                __in KAllocator & allocator);

            //
            // Used during recovery
            //
            static BeginCheckpointLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BeginTransactionOperationLogRecord & invalidBeginTx,
                __in KAllocator & allocator);

            static BeginCheckpointLogRecord::SPtr Create(
                __in bool isFirstCheckpointOnFullCopy,
                __in ProgressVector & progressVector,
                __in_opt BeginTransactionOperationLogRecord * const earliestPendingTransaction,
                __in TxnReplicator::Epoch const & headEpoch,
                __in TxnReplicator::Epoch const & epoch,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BackupLogRecord const & lastCompletedBackupLogRecord,
                __in ULONG progressVectorMaxEntries,
                __in LONG64 periodicCheckpointTimeTicks,
                __in LONG64 periodicTruncationTimeTicks,
                __in KAllocator & allocator);

            ktl::Awaitable<NTSTATUS> AwaitCompletionPhase1FirstCheckpointOnFullCopy();

            virtual std::wstring ToString() const override;

            void SignalExceptionForPhase1OfFirstCheckpointOnFullCopy(__in NTSTATUS error);

            void SignalCompletionPhase1FirstCheckpointOnFullCopy();

            //
            // Number of log records that have been backed up.
            // Used for deciding whether incremental backup can be allowed
            //
            __declspec(property(get = get_BackupLogRecordCount)) UINT BackupLogRecordCount;
            UINT get_BackupLogRecordCount() const
            {
                return backupLogRecordCount_;
            }

            //
            // Current size of the backup log in KB
            //
            __declspec(property(get = get_BackupLogSize)) UINT BackupLogSize;
            UINT get_BackupLogSize() const
            {
                return backupLogSize_;
            }

            //
            // Epoch of the highest log record that is backed up.
            // This will be used to decide whether incremental backup is possible
            //
            __declspec(property(get = get_HighestBackedUpEpoch)) TxnReplicator::Epoch & HighestBackedUpEpoch;
            TxnReplicator::Epoch const & get_HighestBackedUpEpoch() const
            {
                return highestBackedUpEpoch_;
            }

            //
            // Highest logical sequence number that has been backed up.
            // This will be used to decide whether incremental backup is possible
            //
            __declspec(property(get = get_HighestBackedUpLsn)) LONG64 HighestBackedUpLsn;
            LONG64 get_HighestBackedUpLsn() const
            {
                return highestBackedUpLsn_;
            }

            //
            // Id of the backup
            //
            __declspec(property(get = get_BackupId)) KGuid & LastBackupId;
            KGuid const & get_BackupId() const
            {
                return backupId_;
            }

            __declspec(property(get = get_LastStableLsn, put = set_LastStableLsn)) LONG64 LastStableLsn;
            LONG64 get_LastStableLsn() const
            {
                return lastStableLsn_;
            }
            void set_LastStableLsn(LONG64 lsn)
            {
                lastStableLsn_ = lsn;
            }

            __declspec(property(get = get_EarliestPendingTransaction, put = set_EarliestPendingTransaction)) BeginTransactionOperationLogRecord::SPtr EarliestPendingTransaction;
            BeginTransactionOperationLogRecord::SPtr get_EarliestPendingTransaction() const
            {
                ASSERT_IF(
                    earliestPendingTransactionInvalidated_.load(),
                    "Cannot access earliest pending tx for checkpoint record as it is invalidated lsn: {0} psn: {1}",
                    Lsn,
                    Psn);

                return earliestPendingTransaction_;
            }
            void set_EarliestPendingTransaction(__in_opt BeginTransactionOperationLogRecord * const value)
            {
                ASSERT_IFNOT(
                    LogRecord::IsInvalid(earliestPendingTransaction_.RawPtr()), 
                    "Invalid earliest pending xact log record in setter");

                earliestPendingTransaction_ = value;
                if (earliestPendingTransaction_ != nullptr)
                {
                    earliestPendingTransaction_->RecordEpoch = epoch_;
                }
            }

            __declspec(property(get = get_EarliestPendingTransactionLsn)) LONG64 EarliestPendingTransactionLsn;
            LONG64 get_EarliestPendingTransactionLsn()
            {
                ASSERT_IF(
                    earliestPendingTransactionInvalidated_.load(),
                    "Cannot access earliest pending tx lsn for checkpoint record as it is invalidated lsn: {0} psn: {1}",
                    Lsn,
                    Psn);

                if (earliestPendingTransaction_ == nullptr || LogRecord::IsInvalid(earliestPendingTransaction_.RawPtr()))
                {
                    return Lsn;
                }

                return earliestPendingTransaction_->Lsn;
            }

            __declspec(property(get = get_CheckpointState, put = set_CheckpointState)) CheckpointState::Enum CheckpointState;
            CheckpointState::Enum const & get_CheckpointState() const
            {
                return checkpointState_;
            }
            void set_CheckpointState(CheckpointState::Enum value)
            {
                ASSERT_IFNOT(
                    checkpointState_ < value,
                    "New value of checkpoint state should be > existing state in setter: {0} {1}",
                    checkpointState_, value);

                checkpointState_ = value;
            }

            __declspec(property(get = get_EarliestPendingTransactionOffset)) ULONG64 EarliestPendingTransactionOffest;
            ULONG64 get_EarliestPendingTransactionOffset() const
            {
                return earliestPendingTransactionOffset_;
            }

            __declspec(property(get = get_EpochValue)) TxnReplicator::Epoch & EpochValue;
            TxnReplicator::Epoch const & get_EpochValue() const
            {
                return epoch_;
            }

            __declspec(property(get = get_EarliestPendingTransactionRecordPosition)) ULONG64 EarliestPendingTransactionRecordPosition;
            ULONG64 get_EarliestPendingTransactionRecordPosition() const
            {
                return RecordPosition - earliestPendingTransactionOffset_;
            }

            __declspec(property(get = get_ProgressVector)) ProgressVector::SPtr CurrentProgressVector;
            ProgressVector::SPtr get_ProgressVector()
            {
                return progressVector_;
            }

            __declspec(property(get = get_FirstCheckpointPhase1CompletionTcs)) ktl::AwaitableCompletionSource<NTSTATUS>::SPtr & FirstCheckpointPhase1CompletionTcs;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr const & get_FirstCheckpointPhase1CompletionTcs() const
            {
                return firstCheckpointPhase1CompletionTcs_.Get();
            }

            __declspec(property(get = get_IsFirstCheckpointOnFullCopy)) bool IsFirstCheckpointOnFullCopy;
            bool get_IsFirstCheckpointOnFullCopy()
            {
                return isFirstCheckpointOnFullCopy_;
            }

            __declspec(property(get = get_periodicCheckpointTimeTicks)) LONG64 PeriodicCheckpointTimeTicks;
            LONG64 get_periodicCheckpointTimeTicks() const
            {
                return periodicCheckpointTimeTicks_;
            }

            __declspec(property(get = get_PeriodicTruncationTimeStamp)) LONG64 PeriodicTruncationTimeStampTicks;
            LONG64 get_PeriodicTruncationTimeStamp()
            {
                return periodicTruncationTimeTicks_;
            }

            bool FreePreviousLinksLowerThanPsn(
                __in LONG64 newHeadPsn,
                __in InvalidLogRecords & invalidLogRecords) override;

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

        private:

            BeginCheckpointLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BeginTransactionOperationLogRecord & invalidBeginTx);

            BeginCheckpointLogRecord(
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BeginTransactionOperationLogRecord & invalidBeginTx,
                __in bool dummy);

            BeginCheckpointLogRecord(
                __in bool isFirstCheckpointOnFullCopy,
                __in ProgressVector & progressVector,
                __in_opt BeginTransactionOperationLogRecord * const earliestPendingTransaction,
                __in TxnReplicator::Epoch const & headEpoch,
                __in TxnReplicator::Epoch const & epoch,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BackupLogRecord const & lastCompletedBackupLogRecord,
                __in ULONG progressVectorMaxEntries,
                __in LONG64 periodicCheckpointTimeTicks,
                __in LONG64 periodicTruncationTimeTicks);

            static const ULONG DiskSpaceUsed;

            // Indicates if this is the first checkpoint on a full copy
            const bool isFirstCheckpointOnFullCopy_;

            void UpdateApproximateDiskSize();

            KGuid backupId_;
            UINT backupLogRecordCount_;
            UINT backupLogSize_;
            CheckpointState::Enum checkpointState_;
            TxnReplicator::Epoch epoch_;
            ULONG64 earliestPendingTransactionOffset_;
            TxnReplicator::Epoch highestBackedUpEpoch_;
            LONG64 highestBackedUpLsn_;
            LONG64 lastStableLsn_;
            ProgressVector::SPtr progressVector_;
            LONG64 periodicCheckpointTimeTicks_;
            LONG64 periodicTruncationTimeTicks_;

            // Phase1 for first checkpoint on full copy on idle is when prepare and perform checkpoint are completed.
            // Phase2 is when copy pump drains the last lsn from the v1 repl and invokes complete checkpoint along with log rename
            Data::Utilities::ThreadSafeSPtrCache<ktl::AwaitableCompletionSource<NTSTATUS>> firstCheckpointPhase1CompletionTcs_;

            // The following fields are not persisted
            BeginTransactionOperationLogRecord::SPtr earliestPendingTransaction_;

            // Once the earliest pending tx chain is released due to a truncation of the head, we mark this flag
            // Anyone trying to access this later can get incorrect results and should hence assert
            Common::atomic_bool earliestPendingTransactionInvalidated_;
        };
    }
}
