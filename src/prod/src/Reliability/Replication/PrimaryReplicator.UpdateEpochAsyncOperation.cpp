// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ComPointer;
    using Common::ErrorCode;

    using std::move;

    PrimaryReplicator::UpdateEpochAsyncOperation::UpdateEpochAsyncOperation(
        __in PrimaryReplicator & parent,
        FABRIC_EPOCH const & epoch,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state, true),
            parent_(parent),
            epoch_(epoch),
            apiNameDesc_(Common::ApiMonitoring::ApiNameDescription(Common::ApiMonitoring::InterfaceName::IStateProvider, Common::ApiMonitoring::ApiName::UpdateEpoch, L""))
    {
    }

    void PrimaryReplicator::UpdateEpochAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (parent_.CheckReportedFault())
        {
            TryComplete(thisSPtr, Common::ErrorCodeValue::OperationFailed);
            return;
        }

        FABRIC_SEQUENCE_NUMBER lastSequenceInPreviousEpoch;
        ErrorCode error = parent_.GetLastSequenceNumber(epoch_, lastSequenceInPreviousEpoch);

        if (error.IsSuccess())
        {
            auto callDesc = ApiMonitoringWrapper::GetApiCallDescriptionFromName(parent_.endpointUniqueId_, apiNameDesc_, parent_.config_->SlowApiMonitoringInterval);
            parent_.apiMonitor_->StartMonitoring(callDesc);

            ReplicatorEventSource::Events->StateProviderUpdateEpoch(
                parent_.partitionId_,
                parent_.endpointUniqueId_, 
                epoch_.DataLossNumber,
                epoch_.ConfigurationNumber,
                lastSequenceInPreviousEpoch);

            AsyncOperationSPtr inner = parent_.stateProvider_.BeginUpdateEpoch(
                epoch_, 
                lastSequenceInPreviousEpoch,
                [this, callDesc](AsyncOperationSPtr const & asyncOperation)
                {
                    this->FinishUpdateEpoch(asyncOperation, false, callDesc);
                },
                thisSPtr);
            FinishUpdateEpoch(inner, true, callDesc);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    void PrimaryReplicator::UpdateEpochAsyncOperation::FinishUpdateEpoch(
        Common::AsyncOperationSPtr const & asyncOperation,
        bool completedSynchronously,
        Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc)
    {
        if (asyncOperation->CompletedSynchronously == completedSynchronously)
        {
            ErrorCode error = parent_.stateProvider_.EndUpdateEpoch(asyncOperation);
            
            parent_.apiMonitor_->StopMonitoring(callDesc, error);

            if (!error.IsSuccess())
            {
                parent_.SetReportedFault(error, L"Primary ServiceProvider EndUpdateEpoch");
            }
            else
            {
                ReplicatorEventSource::Events->StateProviderUpdateEpochCompleted(parent_.partitionId_, parent_.endpointUniqueId_);
            }
            asyncOperation->Parent->TryComplete(asyncOperation->Parent, error);
        }
    }

    ErrorCode PrimaryReplicator::UpdateEpochAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<UpdateEpochAsyncOperation>(asyncOperation);
        return casted->Error;
    }
    
} // end namespace ReplicationComponent
} // end namespace Reliability
