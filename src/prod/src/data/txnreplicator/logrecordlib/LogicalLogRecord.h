// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        //
        // Represents a logical record that is replicated (With the exception that UpdateEpoch is a logical log record that is not replicated)
        //
        class LogicalLogRecord 
            : public LogRecord
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED_WITH_INHERITANCE(LogicalLogRecord)

        public:

            //
            // Returns true if the record can be throttled or not
            // Some records like commit records are never throttled
            // 
            static bool IsAllowedToThrottle(__in LogicalLogRecord const & logicalRecord);

            virtual std::wstring ToString() const override;

            __declspec(property(get = get_IsReplicated)) bool IsReplicated;
            bool get_IsReplicated()
            {
                ASSERT_IF(
                    replicationTask_ == nullptr,
                    "Cannot check for replicated flag before starting replication");

                return replicationTask_->IsCompleted;
            }

            __declspec(property(get = get_ReplicationResult)) NTSTATUS ReplicationResult;
            NTSTATUS get_ReplicationResult()
            {
                ASSERT_IF(
                    replicationTask_ == nullptr,
                    "Cannot check for replication result before starting replication");

                return replicationTask_->CompletionCode;
            }

            __declspec(property(get = get_IsLatencySensitiveRecord)) bool IsLatencySensitiveRecord;
            virtual bool get_IsLatencySensitiveRecord() const
            {
                return false;
            }

            Utilities::OperationData::CSPtr SerializeLogicalData();

            void ReleaseSerializedLogicalDataBuffers();

            // 
            // Used to avoid dynamic cast and fast casting
            //
            LogicalLogRecord * AsLogicalLogRecord() override
            {
                return this;
            }

            // 
            // Used to avoid dynamic cast and fast casting
            //
            PhysicalLogRecord * AsPhysicalLogRecord() override
            {
                return nullptr;
            }

            void SetReplicationTask(TxnReplicator::CompletionTask::SPtr && replicationTask)
            {
                replicationTask_ = Ktl::Move(replicationTask);
            }

            ktl::Awaitable<NTSTATUS> AwaitReplication();

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

            LogicalLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            LogicalLogRecord(
                __in LogRecordType::Enum recordType,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            virtual void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            virtual void ReadLogical(
                __in Utilities::OperationData const & operationData,
                __inout INT & index) override;

            virtual void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

            virtual ULONG GetSizeOnWire() const;

        private:

            void ReadPrivate(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            static const ULONG DiskSpaceUsed;
            static const ULONG SizeOnWire;

            void UpdateApproximateDiskSize();

            TxnReplicator::CompletionTask::SPtr replicationTask_;
            KBuffer::SPtr replicatedData_; // buffers the replicated data that can also be used during the log write and avoids 2 serializations
            Utilities::BinaryWriter replicatedDataBinaryWriter_; // binary writer with the memory backing the replicated data
        };
    }
}
