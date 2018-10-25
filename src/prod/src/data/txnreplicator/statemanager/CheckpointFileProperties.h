// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        // CheckpointFileProperties contains properties of checkpoint files.
        // CheckpointFileProperties used key-value pairs.
        // Here is the properties format:
        //
        //         |--Properties Keys--|--Size--|--Properties Value--|...
        //
        // For the Size, we used VarInt, which uses msb of each byte to tell us whether it is the end of the number
        //         | 1 x x x x x x x | 1 x x x x x x x | 0 x x x x x x x |
        //
        // CheckpointFileProperties can only read the properties it knows, for the unknown properties, it just skips the specific size
        class CheckpointFileProperties:
            public Utilities::FileProperties
        {
            K_FORCE_SHARED(CheckpointFileProperties)

        public:
            // Keep this Create function since it is used in the FileProperties Read path.
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            static NTSTATUS Create(
                __in Utilities::BlockHandle * const blocksHandle,
                __in Utilities::BlockHandle * const metadataHandle,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in ULONG32 rootStateProviderCount,
                __in ULONG32 stateProviderCount,
                __in bool doNotWritePrepareCheckpointLSN,
                __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public:
            __declspec(property(get = get_BlocksHandle)) Utilities::BlockHandle::SPtr BlocksHandle;
            Utilities::BlockHandle::SPtr get_BlocksHandle() const;

            __declspec(property(get = get_MetadataHandle)) Utilities::BlockHandle::SPtr MetadataHandle;
            Utilities::BlockHandle::SPtr get_MetadataHandle() const;

            // KGuid is 16 bytes, using copy by reference
            __declspec(property(get = get_PartitionId)) KGuid PartitionId;
            KGuid get_PartitionId() const;

            __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID CheckpointFileProperties::get_ReplicaId() const;

            __declspec(property(get = get_RootStateProviderCount)) ULONG32 RootStateProviderCount;
            ULONG32 get_RootStateProviderCount() const;

            __declspec(property(get = get_StateProviderCount)) ULONG32 StateProviderCount;
            ULONG32 get_StateProviderCount() const;

            __declspec(property(get = get_PrepareCheckpointLSN)) FABRIC_SEQUENCE_NUMBER PrepareCheckpointLSN;
            FABRIC_SEQUENCE_NUMBER get_PrepareCheckpointLSN() const;

            __declspec(property(put = put_Test_Ignore)) bool Test_Ignore;
            void put_Test_Ignore(__in bool test_Ignore);

        public: // Override funtions
            // Write the checkpoint file properties
            void Write(__in Utilities::BinaryWriter & writer) override;

            // Read the checkpoint file properties, skip the unknown properties
            void ReadProperty(
                __in Utilities::BinaryReader & reader, 
                __in KStringView const & property,
                __in ULONG32 valueSize) override;

        private:
            CheckpointFileProperties(
                __in Utilities::BlockHandle * const blocksHandle,
                __in Utilities::BlockHandle * const metadataHandle,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in ULONG32 rootStateProviderCount,
                __in ULONG32 stateProviderCount,
                __in bool doNotWritePrepareCheckpointLSN,
                __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN);

        private:
            // Keys for the properties
            const KStringView BlocksHandlePropertyName_ = L"blocks";
            const KStringView MetadataHandlePropertyName_ = L"metadata";
            const KStringView PartitionIdPropertyName_ = L"partitionid";
            const KStringView ReplicaIdPropertyName_ = L"replicaid";
            const KStringView RootStateProviderCountPropertyName_ = L"roots";
            const KStringView StateProviderCountPropertyName_ = L"count";
            const KStringView PrepareCheckpointLSNPropertyName_ = L"checkpointlsn";

        private:
            const bool doNotWritePrepareCheckpointLSN_;

            // Values for the properties
            Utilities::ThreadSafeSPtrCache<Utilities::BlockHandle> blocksHandle_;
            Utilities::ThreadSafeSPtrCache<Utilities::BlockHandle> metadataHandle_;
            KGuid partitionId_;
            FABRIC_REPLICA_ID replicaId_;
            ULONG32 rootStateProviderCount_;
            ULONG32 stateProviderCount_;
            FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN_;

            // Testing purpose only.
            // In the change of adding the prepareCheckpointLSN property, we need to verify the file with prepareCheckpointLSN
            // property can be read from the code version does not have the property, and the file without prepareCheckpointLSN
            // property can be read from the code version which include the property
            bool test_Ignore_;
        };
    }
}
