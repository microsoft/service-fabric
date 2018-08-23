// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestGroupCommitValidationResult final 
    {

    public:

        TestGroupCommitValidationResult()
            : lsn_(-1)
            , pendingTxCount_(0)
            , committedTxCount_(0)
            , abortedTxCount_(0)
        {
        }

        TestGroupCommitValidationResult(
            LONG64 lsn,
            UINT pendingTxCount,
            UINT committedTxCount,
            UINT abortedTxCount)
            : lsn_(lsn)
            , pendingTxCount_(pendingTxCount)
            , committedTxCount_(committedTxCount)
            , abortedTxCount_(abortedTxCount)
        {
        }

        static bool Compare(
            KArray<TestGroupCommitValidationResult> const & expected,
            KArray<TestGroupCommitValidationResult> const & found)
        {
            if (found.Count() != expected.Count() + 1)
            {
                return false;
            }

            for (ULONG i = 0; i < expected.Count(); i++)
            {
                if (expected[i] == found[i])
                {
                    continue;
                }

                return false;
            }

            return true;
        }

        __declspec(property(get = get_Lsn, put = set_Lsn)) LONG64 Lsn;
        LONG64 get_Lsn() const
        {
            return lsn_;
        }
        void set_Lsn(LONG64 value)
        {
            lsn_ = value;
        }

        __declspec(property(get = get_PendingTxCount)) UINT PendingTxCount;
        UINT get_PendingTxCount() const
        {
            return pendingTxCount_;
        }

        __declspec(property(get = get_CommittedTxCount)) UINT CommittedTxCount;
        UINT get_CommittedTxCount() const
        {
            return committedTxCount_;
        }

        __declspec(property(get = get_AbortedTxCount)) UINT AbortedTxCount;
        UINT get_AbortedTxCount() const
        {
            return abortedTxCount_;
        }

        bool operator==(TestGroupCommitValidationResult const & other) const
        {
            return
                lsn_ == other.lsn_ &&
                pendingTxCount_ == other.pendingTxCount_ &&
                committedTxCount_ == other.committedTxCount_ &&
                abortedTxCount_ == other.abortedTxCount_;
        }

        UINT IncrementAbortedTxCount()
        {
            return InterlockedIncrement((volatile LONG*)&abortedTxCount_);
        }

        UINT IncrementCommittedTxCount()
        {
            return InterlockedIncrement((volatile LONG*)&committedTxCount_);
        }

        UINT IncrementPendingTxCount()
        {
            return InterlockedIncrement((volatile LONG*)&pendingTxCount_);
        }

        UINT DecrementPendingTxCount()
        {
            return InterlockedDecrement((volatile LONG*)&pendingTxCount_);
        }

    private:

        LONG64 lsn_;
        UINT pendingTxCount_;
        UINT committedTxCount_;
        UINT abortedTxCount_;
    };
}
