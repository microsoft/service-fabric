// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("UpdateEpochAsyncOperation");

    void ReplicatedStore::UpdateEpochAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.Test_BlockUpdateEpochIfNeeded();

        auto error = owner_.LoadCachedCurrentEpoch();

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);

            return;
        }

        oldEpoch_ = owner_.GetCachedCurrentEpoch();

        if (newEpoch_.DataLossNumber < oldEpoch_.DataLossNumber
            || (newEpoch_.DataLossNumber == oldEpoch_.DataLossNumber && newEpoch_.ConfigurationNumber < oldEpoch_.ConfigurationNumber))
        {
            TRACE_ERROR_AND_TESTASSERT(
                TraceComponent,
                "{0} UpdateEpoch called with lower epoch ({1}.{2:X} < {3}.{4:X})", 
                owner_.TraceId,
                newEpoch_.DataLossNumber, 
                newEpoch_.ConfigurationNumber,
                oldEpoch_.DataLossNumber,
                oldEpoch_.ConfigurationNumber);

            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidState);

            return;
        }

        WriteInfo(
            TraceComponent, 
            "{0} UpdateEpoch: new = {1}.{2:X} previous = {3}.{4:X} last operation LSN = {5}",
            owner_.TraceId, 
            newEpoch_.DataLossNumber, 
            newEpoch_.ConfigurationNumber, 
            oldEpoch_.DataLossNumber,
            oldEpoch_.ConfigurationNumber,
            previousEpochLastSequenceNumber_);

        error = owner_.CreateLocalStoreTransaction(transactionSPtr_);
        
        // For both the epoch history and current epoch data entries,
        // ensure that the Local Store sequence number stays at 1
        // since we do not want ReplicatedStore "metadata" to count towards
        // user data progress
        //
        if (error.IsSuccess())
        {
            error = UpdateEpochHistory();
        }

        if (error.IsSuccess())
        {
            error = UpdateCurrentEpoch();
        }

        if (error.IsSuccess())
        {
            auto commitOperation = transactionSPtr_->BeginCommit(
                Common::TimeSpan::MaxValue, 
                [this](AsyncOperationSPtr const & operation){ this->CommitCallback(operation); },
                thisSPtr);

            if (commitOperation->CompletedSynchronously)
            {
                FinishCommit(commitOperation);
            }
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    ErrorCode ReplicatedStore::UpdateEpochAsyncOperation::UpdateEpochHistory()
    {
        ProgressVectorData epochHistory;

        ErrorCode error = owner_.dataSerializerUPtr_->TryReadData(
            transactionSPtr_, 
            Constants::ProgressDataType, 
            Constants::EpochHistoryDataName,
            epochHistory);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            auto & progressVector = epochHistory.ProgressVector;

            if (!progressVector.empty())
            {
                auto const & lastProgress = progressVector.back();

                if (newEpoch_.DataLossNumber == oldEpoch_.DataLossNumber &&
                    newEpoch_.ConfigurationNumber == oldEpoch_.ConfigurationNumber &&
                    previousEpochLastSequenceNumber_ == lastProgress.LastOperationLSN)
                {
                    // This can happen if the primary restarts immediately after update epoch succeeds
                    //
                    WriteInfo(
                        TraceComponent, 
                        "{0} epoch history already up-to-date: {1}.{2:X}.{3}",
                        owner_.TraceId, 
                        oldEpoch_.DataLossNumber,
                        oldEpoch_.ConfigurationNumber,
                        lastProgress.LastOperationLSN);
                    
                    return ErrorCodeValue::Success;
                }
            }

            progressVector.push_back(ProgressVectorEntry(oldEpoch_, previousEpochLastSequenceNumber_));

            if (progressVector.size() > static_cast<size_t>(StoreConfig::GetConfig().MaxEpochHistoryCount))
            {
                progressVector.erase(progressVector.begin());
            }

            error = owner_.dataSerializerUPtr_->WriteData(
                transactionSPtr_, 
                epochHistory, 
                Constants::ProgressDataType,
                Constants::EpochHistoryDataName);
        }

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to update epoch history: {1}",
                owner_.TraceId, 
                error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::UpdateEpochAsyncOperation::UpdateCurrentEpoch()
    {
        auto error = owner_.dataSerializerUPtr_->WriteData(
            transactionSPtr_,
            CurrentEpochData(newEpoch_),
            Constants::ProgressDataType,
            Constants::CurrentEpochDataName);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to update current epoch: {1}",
                owner_.TraceId, 
                error);
        }

        return error;
    }

    void ReplicatedStore::UpdateEpochAsyncOperation::CommitCallback(AsyncOperationSPtr const& asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            FinishCommit(asyncOperation);
        }
    }
   
    void ReplicatedStore::UpdateEpochAsyncOperation::FinishCommit(AsyncOperationSPtr const& asyncOperation)
    {
        auto error = transactionSPtr_->EndCommit(asyncOperation);
        transactionSPtr_.reset();

        if (error.IsSuccess())
        {
            owner_.UpdateCachedCurrentEpoch(newEpoch_);
        }

        this->TryComplete(asyncOperation->Parent, error);
    }

    ErrorCode ReplicatedStore::UpdateEpochAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<UpdateEpochAsyncOperation>(operation)->Error;
    }
}

