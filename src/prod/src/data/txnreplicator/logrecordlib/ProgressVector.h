// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class ProgressVector final 
            : public KObject<ProgressVector>
            , public KShared<ProgressVector>
        {
            K_FORCE_SHARED(ProgressVector)

        public:

            static ProgressVector::SPtr Create(__in KAllocator & allocator);

            static ProgressVector::SPtr CreateZeroProgressVector(__in KAllocator & allocator);
            
            // Copies the contents of the progress vector parameter into a new object and returns that
            static ProgressVector::SPtr Clone(
                __in ProgressVector & progressVector,
                __in ULONG progressVectorMaxEntries,
                __in TxnReplicator::Epoch const & highestBackedUpEpoch,
                __in TxnReplicator::Epoch const & headEpoch,
                __in KAllocator & allocator);

            static CopyModeResult FindCopyMode(
                __in CopyContextParameters const & sourceParameters,
                __in CopyContextParameters const & targetParameters,
                __in LONG64 lastRecoveredAtomicRedoOperationLsnAtTarget);

            static SharedProgressVectorEntry FindSharedVector(
                __in ProgressVector & sourceProgressVector,
                __in ProgressVector & targetProgressVector);

            __declspec(property(get = get_Length)) ULONG Length;
            ULONG get_Length() const
            {
                return vectors_.Count();
            }

            __declspec(property(get = get_ByteCount)) ULONG ByteCount;
            ULONG get_ByteCount() const
            {
                return sizeof(ULONG) + vectors_.Count() * ProgressVectorEntry::SerializedByteCount; 
            }

            __declspec(property(get = get_LastProgressVectorEntry)) ProgressVectorEntry & LastProgressVectorEntry;
            ProgressVectorEntry const & get_LastProgressVectorEntry() const
            {
                ASSERT_IFNOT(vectors_.Count() > 0, "No progress vector entries");

                return vectors_[vectors_.Count() - 1];
            }

            __declspec(property(get = get_ProgressVectorMaxEntries)) ULONG & ProgressVectorMaxEntries;
            ULONG get_ProgressVectorMaxEntries() const
            {
                return progressVectorMaxEntries_;
            }

            void Test_SetProgressVectorMaxEntries(__in ULONG progressVectorMaxEntries);

            bool Equals(__in ProgressVector & other) const;

            // TODO: Remove STL later
            std::wstring ToString() const;

            std::wstring ToString(
                __in LONG64 maxVectorStringLengthInKb) const;

            std::wstring ToString(
                __in ULONG startingIndex,
                __in LONG64 maxVectorStringLengthInKb) const;

            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            void Write(__in Utilities::BinaryWriter & binaryWriter);

            void Append(__in ProgressVectorEntry const & progressVectorEntry);

            bool Insert(__in ProgressVectorEntry const & progressVectorEntry);

            ProgressVectorEntry Find(__in TxnReplicator::Epoch const & epoch) const;

            TxnReplicator::Epoch FindEpoch(__in LONG64 lsn) const;

            void TruncateTail(__in ProgressVectorEntry const & lastProgressVectorEntry);

            void TrimProgressVectorIfNeeded(
                __in TxnReplicator::Epoch const & highestBackedUpEpoch,
                __in TxnReplicator::Epoch const & headEpoch);

        private:

            static CopyModeResult FindCopyModePrivate(
                __in CopyContextParameters const & sourceParameters,
                __in CopyContextParameters const & targetParameters,
                __in LONG64 lastRecoveredAtomicRedoOperationLsnAtTarget);

            static bool DecrementIndexUntilLeq(
                __inout ULONG & index,
                __in ProgressVector const & vector,
                __in ProgressVectorEntry const & comparand);

            static ProgressVectorEntry DecrementIndexUntilLsnIsEqual(
                __inout ULONG & index,
                __in ProgressVector const & vector,
                __in LONG64 lsn);

            static bool IsBrandNewReplica(CopyContextParameters const & copyContextParameters);

            static bool ValidateIfDebugEnabled(
                __in bool condition,
                __in std::string const & msg);

            KArray<ProgressVectorEntry> vectors_;
            ULONG progressVectorMaxEntries_;
        };
    }
}
