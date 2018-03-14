// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::UpdateEpochAsyncOperation : public Common::AsyncOperation
    {
        DENY_COPY(UpdateEpochAsyncOperation);

    public:
        UpdateEpochAsyncOperation(
            __in ReplicatedStore & owner,
            FABRIC_EPOCH const & epoch,
            FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent, true)
            , owner_(owner)
            , newEpoch_(epoch)
            , previousEpochLastSequenceNumber_(previousEpochLastSequenceNumber)
            , oldEpoch_()
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        Common::ErrorCode UpdateEpochHistory();
        Common::ErrorCode UpdateCurrentEpoch();
        void CommitCallback(Common::AsyncOperationSPtr const& asyncOperation);
        void FinishCommit(Common::AsyncOperationSPtr const& asyncOperation);

        FABRIC_EPOCH newEpoch_;
        FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber_;
        FABRIC_EPOCH oldEpoch_;

        TransactionSPtr transactionSPtr_;
        ReplicatedStore & owner_;
    };
}

