// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class CopyContextParameters final
            : public KObject<CopyContextParameters>
        {
            K_DENY_COPY(CopyContextParameters)

        public:

            CopyContextParameters(
                __in ProgressVector & pv,
                __in TxnReplicator::Epoch const & logHeadEpoch,
                __in LONG64 logHeadLsn,
                __in LONG64 logTailLsn)
                : KObject()
                , progressVector_(&pv)
                , logHeadEpoch_(logHeadEpoch)
                , logHeadLsn_(logHeadLsn)
                , logTailLsn_(logTailLsn)
            {
            }

            __declspec(property(get = get_ProgressVector)) ProgressVector::SPtr ProgressVectorData;
            ProgressVector::SPtr get_ProgressVector() const
            {
                return progressVector_;
            }

            __declspec(property(get = get_LogHeadEpoch)) TxnReplicator::Epoch & LogHeadEpoch;
            TxnReplicator::Epoch const & get_LogHeadEpoch() const
            {
                return logHeadEpoch_;
            }

            __declspec(property(get = get_LogHeadLsn)) LONG64 LogHeadLsn;
            LONG64 get_LogHeadLsn() const
            {
                return logHeadLsn_;
            }

            __declspec(property(get = get_LogTailLsn)) LONG64 LogTailLsn;
            LONG64 get_LogTailLsn() const
            {
                return logTailLsn_;
            }

        private:
            
            ProgressVector::SPtr progressVector_;
            TxnReplicator::Epoch logHeadEpoch_;
            LONG64 logHeadLsn_;
            LONG64 logTailLsn_;
        };
    }
}
