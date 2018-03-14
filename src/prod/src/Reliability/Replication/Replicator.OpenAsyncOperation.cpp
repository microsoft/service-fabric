// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::AcquireWriteLock;

    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ErrorCode;

    using std::wstring;

    Replicator::OpenAsyncOperation::OpenAsyncOperation(
        __in Replicator & parent,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state, true),
            parent_(parent)
    {
    }

    void Replicator::OpenAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        Common::ApiMonitoring::MonitoringComponentConstructorParameters monitoringParameters;

        {
            AcquireWriteLock lock(parent_.lock_);
            error = parent_.state_.TransitionToOpened();

            if (!error.IsSuccess())
            {
                ReplicatorEventSource::Events->ReplicatorInvalidState(
                    parent_.partitionId_,
                    parent_, // need to print the current state
                    L"Open",
                    (const int) ReplicatorState::Created);
            }
            else
            {
                ReplicatorEventSource::Events->ReplicatorOpen(
                    parent_.partitionId_,
                    parent_.endpointUniqueId_,
                    parent_.hasPersistedState_);

                parent_.transport_->GeneratePublishEndpoint(
                    parent_.endpointUniqueId_,
                    parent_.endpoint_);
                
                parent_.CreateMessageProcessorCallerHoldsLock();
                parent_.transport_->RegisterMessageProcessor(parent_);
                parent_.perfCounters_ = REPerformanceCounters::CreateInstance(
                    parent_.partitionId_.ToString(), 
                    parent_.replicaId_);
                parent_.perfCounters_->Role.RawValue = static_cast<int>(::FABRIC_REPLICA_ROLE_UNKNOWN);

                parent_.apiMonitor_ = ApiMonitoringWrapper::Create(parent_.healthClient_, parent_.config_, parent_.partitionId_, parent_.endpointUniqueId_);
            }
        }

        // Complete outside the lock
        TryComplete(thisSPtr, error);
    }

    ErrorCode Replicator::OpenAsyncOperation::End(AsyncOperationSPtr const & asyncOperation, __out wstring & endpoint)
    {
        auto casted = AsyncOperation::End<OpenAsyncOperation>(asyncOperation);
        if(casted->Error.IsSuccess())
        {
            Common::StringWriter(endpoint).Write("{0}{1}{2}",
                casted->parent_.endpoint_,
                Constants::ReplicationEndpointDelimiter,
                casted->parent_.transport_->Id);
        }

        return casted->Error;
    }

} // end namespace ReplicationComponent
} // end namespace Reliability
