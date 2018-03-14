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
        // The copy metadata.
        //
        class CopyMetadata final
            : public KObject<CopyMetadata>
            , public KShared<CopyMetadata>
        {
            K_FORCE_SHARED(CopyMetadata)

        public:
            static CopyMetadata::SPtr Create(
                __in ULONG32 copyStateMetadataVersion,
                __in ProgressVector & progressVector,
                __in TxnReplicator::Epoch & startingEpoch,
                __in LONG64 startingLogicalSequenceNumber,
                __in LONG64 checkpointLsn,
                __in LONG64 uptoLsn,
                __in LONG64 highestStateProviderCopiedLsn,
                __in KAllocator & allocator);

            static CopyMetadata::SPtr Deserialize(
                __in Utilities::OperationData const & operationData,
                __in KAllocator & allocator);

            static KBuffer::SPtr Serialize(
                __in CopyMetadata const & copyMetadata,
                __in KAllocator & allocator);

            __declspec(property(get = get_CopyStateMetadataVersion)) ULONG32 CopyStateMetadataVersion;
            ULONG32 get_CopyStateMetadataVersion() const
            {
                return copyStateMetadataVersion_;
            }

            __declspec(property(get = get_ProgressVector)) ProgressVector::SPtr ProgressVectorValue;
            ProgressVector::SPtr get_ProgressVector() const
            {
                return progressVector_;
            }

            __declspec(property(get = get_StartingEpoch)) TxnReplicator::Epoch & StartingEpoch;
            TxnReplicator::Epoch const & get_StartingEpoch() const
            {
                return startingEpoch_;
            }

            __declspec(property(get = get_StartingLogicalSequenceNumber)) LONG64 StartingLogicalSequenceNumber;
            LONG64 get_StartingLogicalSequenceNumber() const
            {
                return startingLogicalSequenceNumber_;
            }

            __declspec(property(get = get_CheckpointLsn)) LONG64 CheckpointLsn;
            LONG64 get_CheckpointLsn() const
            {
                return checkpointLsn_;
            }

            __declspec(property(get = get_UptoLsn)) LONG64 UptoLsn;
            LONG64 get_UptoLsn() const
            {
                return uptoLsn_;
            }

            __declspec(property(get = get_HighestStateProviderCopiedLsn)) LONG64 HighestStateProviderCopiedLsn;
            LONG64 get_HighestStateProviderCopiedLsn() const
            {
                return highestStateProviderCopiedLsn_;
            }

        private:
            // Initializes a new instance of the CopyMetadata class.
            CopyMetadata(
                __in ULONG32 copyStateMetadataVersion,
                __in ProgressVector & progressVector,
                __in TxnReplicator::Epoch & startingEpoch,
                __in LONG64 startingLogicalSequenceNumber,
                __in LONG64 checkpointLsn,
                __in LONG64 uptoLsn,
                __in LONG64 highestStateProviderCopiedLsn);

            // The copy state metadata version.
            ULONG32 copyStateMetadataVersion_;

            // The progress vector.
            ProgressVector::SPtr progressVector_;

            // The starting epoch.
            TxnReplicator::Epoch startingEpoch_;

            // The starting logical sequence number.
            LONG64 startingLogicalSequenceNumber_;

            // The checkpoing lsn.
            LONG64 checkpointLsn_;

            // The upto lsn.
            LONG64 uptoLsn_;

            // The highest state provider copied lsn.
            LONG64 highestStateProviderCopiedLsn_;
        };
    }
}
