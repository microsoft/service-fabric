// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class ProgressVectorEntry final 
            : public KObject<ProgressVectorEntry>
        {

        public:

            static ProgressVectorEntry const & ZeroProgressVectorEntry();

            static ULONG const SerializedByteCount;

            static LONG64 const SizeOfStringInBytes();

            ProgressVectorEntry();

            ProgressVectorEntry(__in ProgressVectorEntry const & other);

            ProgressVectorEntry(__in UpdateEpochLogRecord const & record);

            ProgressVectorEntry(
                __in TxnReplicator::Epoch const & epoch,
                __in LONG64 lsn,
                __in LONG64 primaryReplicaId,
                __in Common::DateTime timeStamp);

            __declspec(property(get = get_Lsn, put = set_Lsn)) LONG64 Lsn;
            LONG64 get_Lsn() const
            {
                return lsn_;
            }

            __declspec(property(get = get_Epoch)) TxnReplicator::Epoch & CurrentEpoch;
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

            bool operator==(__in ProgressVectorEntry const & other) const;
            bool operator>(__in ProgressVectorEntry const & other) const;
            bool operator>=(__in ProgressVectorEntry const & other) const;
            bool operator!=(__in ProgressVectorEntry const & other) const;
            bool operator<(__in ProgressVectorEntry const & other) const;
            bool operator<=(__in ProgressVectorEntry const & other) const;

            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            void Write(__in Utilities::BinaryWriter & binaryWriter);

            std::wstring ToString() const;

            bool IsDataLossBetween(__in ProgressVectorEntry const & other) const;

        private:

            static ProgressVectorEntry const ZeroProgressVectorEntry_;

            static LONG64 SizeOfStringInBytes_;

            TxnReplicator::Epoch epoch_;
            LONG64 lsn_;
            LONG64 primaryReplicaId_;
            Common::DateTime timeStamp_;
        };
    }
}
