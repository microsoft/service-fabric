// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class UpdateEpochLogRecord final 
            : public LogicalLogRecord
        {
            K_FORCE_SHARED(UpdateEpochLogRecord)

        public:

            static UpdateEpochLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            static UpdateEpochLogRecord::SPtr Create(
                __in TxnReplicator::Epoch const & epoch,
                __in LONG64 primaryReplicaId,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            static UpdateEpochLogRecord::SPtr CreateZeroUpdateEpochLogRecord(
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            virtual std::wstring ToString() const override;

            __declspec(property(get = get_Epoch)) TxnReplicator::Epoch & EpochValue;
            TxnReplicator::Epoch const & get_Epoch() const
            {
                return epoch_;
            }

            __declspec(property(get = get_PrimaryReplicaId)) LONG64 PrimaryReplicaId;
            LONG64 get_PrimaryReplicaId() const
            {
                return primaryReplicaId_;
            }

            __declspec(property(get = get_TimeStamp)) Common::DateTime TimeStamp;
            Common::DateTime get_TimeStamp() const
            {
                return timeStamp_;
            }

            bool Test_Equals(__in LogRecord const & other) const override;

        private:

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

            ULONG GetSizeOnWire() const override;

            UpdateEpochLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            UpdateEpochLogRecord(
                __in TxnReplicator::Epoch const & epoch,
                __in LONG64 primaryReplicaId,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            UpdateEpochLogRecord(__in PhysicalLogRecord & invalidPhysicalLogRecord);

            static const ULONG DiskSpaceUsed;
            static const ULONG SizeOnWireIncrement;

            void UpdateApproximateDiskSize();

            void ReadPrivate(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            TxnReplicator::Epoch epoch_;
            LONG64 primaryReplicaId_;
            Common::DateTime timeStamp_;
        };
    }
}
