// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class TestStateProvider::UndoOperationData final
            : public Data::Utilities::OperationData
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TestSession>
        {
            K_FORCE_SHARED(UndoOperationData);

        public: // Static creators
            // Creates an UndoOperationData::CSPtr from intputs.
            // Used in the IN path.
            static __checkReturn NTSTATUS Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KString const & value,
                __in FABRIC_SEQUENCE_NUMBER lsn,
                __in KAllocator & allocator,
                __out CSPtr & result);

            // Creates an UndoOperationData::SPtr by deserializing the OperationData from LoggingReplicator.
            // Used in the OUT path.
            static __checkReturn NTSTATUS Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Data::Utilities::OperationData const * const operationData,
                __in KAllocator & allocator,
                __out TestStateProvider::UndoOperationData::CSPtr & result);

        public:
            __declspec(property(get = get_Value)) KString::CSPtr Value;
            KString::CSPtr get_Value() const;

            __declspec(property(get = get_Version)) FABRIC_SEQUENCE_NUMBER Version;
            FABRIC_SEQUENCE_NUMBER get_Version() const;

        private:
            static __checkReturn NTSTATUS Deserialize(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in OperationData const * const operationData,
                __in KAllocator & allocator,
                __out KString::SPtr & value,
                __out FABRIC_SEQUENCE_NUMBER & version) noexcept;

        private: // Constructor
            FAILABLE UndoOperationData(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KString const & value,
                __in FABRIC_SEQUENCE_NUMBER lsn,
                __in bool serializeData) noexcept;

        private:
            __checkReturn NTSTATUS Serialize() noexcept;

        private:
            KString::CSPtr value_;
            FABRIC_SEQUENCE_NUMBER version_;
        };
    }
}
