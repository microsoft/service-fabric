// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::AcquireWriteLock;
    using Common::AcquireReadLock;
    using Common::Assert;
    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ComPointer;
    using Common::ErrorCode;
    
    using std::vector;
    
    PrimaryReplicator::BuildIdleAsyncOperation::BuildIdleAsyncOperation(
        __in PrimaryReplicator & parent,
        ReplicaInformation const & replica,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state, true),
            healthReporter_(BatchedHealthReporter::Create(
                parent.partitionId_,
                parent.endpointUniqueId_,
                HealthReportType::RemoteReplicatorConnectionStatus,
                parent.config_->RetryInterval,
                parent.healthClient_)),
            parent_(parent),
            replica_(replica),
            isCancelled_(false),
            isCancelledLock_(),
            session_()
    {
    }

    void PrimaryReplicator::BuildIdleAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (parent_.CheckReportedFault())
        {
            TryComplete(thisSPtr, Common::ErrorCodeValue::OperationFailed);
            return;
        }

        FABRIC_SEQUENCE_NUMBER replicationStartSeq;
        ErrorCode error = parent_.replicaManagerSPtr_->TryAddIdleReplica(
            replica_, 
            session_,
            replicationStartSeq);
        if (error.IsSuccess())
        {
            auto inner = session_->BeginEstablishCopy(
                replicationStartSeq,
                parent_.hasPersistedState_,
                [this, thisSPtr](Common::ErrorCode const & error, Transport::MessageUPtr &&)
                {
                    if (error.IsSuccess())
                    {
                        healthReporter_->ScheduleOKReport();
                    }
                    else
                    {
                        healthReporter_->ScheduleWarningReport(Common::wformatString("Replica {0} cannot be reached to start the copy process. Error Code: {1}, Target listen address: {2}. Verify that ReplicatorAddress config is valid.", session_->ReplicaId, error, session_->TransportTarget));
                    }
                },
                [this, replicationStartSeq](AsyncOperationSPtr const & operation)
                {
                    this->FinishEstablishCopy(operation, replicationStartSeq, false);
                },
                thisSPtr);
            FinishEstablishCopy(inner, replicationStartSeq, true);
        }
        else
        {
            TryComplete(thisSPtr, error);
        }
    }

    void PrimaryReplicator::BuildIdleAsyncOperation::FinishEstablishCopy(
        AsyncOperationSPtr const & asyncOperation,
        FABRIC_SEQUENCE_NUMBER replicationStartSeq,
        bool completedSynchronously)
    {
        if (completedSynchronously == asyncOperation->CompletedSynchronously)
        {
            AsyncOperationSPtr const & thisSPtr = asyncOperation->Parent;
            ComPointer<IFabricOperationDataStream> context;
            ErrorCode error = session_->EndEstablishCopy(asyncOperation, context);
            healthReporter_->ScheduleOKReport();
            healthReporter_->Close();

            if (IsCancelledLockedRead())
            {
                error = Common::ErrorCodeValue::OperationCanceled;
            }

            if (!error.IsSuccess())
            {
                CleanupCCReceiverAndCompleteOperation(
                    session_,
                    thisSPtr,
                    error);
                return;
            }
           
            ComProxyAsyncEnumOperationData asyncEnumOperationData;
            error = parent_.stateProvider_.GetCopyOperations(
                std::move(context),
                replicationStartSeq - 1,
                asyncEnumOperationData);

            if (!error.IsSuccess())
            {
                CleanupCCReceiverAndCompleteOperation(
                    session_,
                    thisSPtr,
                    error);
                parent_.SetReportedFault(error, L"GetCopyState");
                return;
            }
            
            // Copy is done when the last copy and the last replication operations 
            // are ACKed by the session.
            // The last replication operation is the last operation
            // given to the primary at the time last copy operation is received from the 
            // state provider.
            // This is necessary because subsequent operations may override
            // operations provided by copy,
            // so that the state when copy is finished is inconsistent
            EnumeratorLastOpCallback enumeratorLastOpCallback = 
                [this](FABRIC_SEQUENCE_NUMBER lastCopySequnceNumber)
                {
                    FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber;
                    ErrorCode error = this->parent_.replicaManagerSPtr_->GetLastSequenceNumber(lastReplicationSequenceNumber);
                    session_->OnLastEnumeratorCopyOp(
                        lastCopySequnceNumber, 
                        lastReplicationSequenceNumber, 
                        error);
                };
    
            session_->BeginCopy(
                std::move(asyncEnumOperationData),
                enumeratorLastOpCallback,
                [this](AsyncOperationSPtr const & asyncOperation)
                {
                    Common::ErrorCode faultErrorCode;
                    std::wstring faultDesc;
                    Common::ErrorCode error = session_->EndCopy(asyncOperation, faultErrorCode, faultDesc);
                    
                    if (!faultErrorCode.IsSuccess())
                    {
                        this->parent_.SetReportedFault(faultErrorCode, faultDesc);
                    }

                    // It is possible the replication session was removed after it was added 
                    // and before start copy was sent out.  In this case we need to clean up
                    // start copy reliable operation sender and the start copy operation.
                    session_->CloseStartCopyReliableOperationSender();
                    asyncOperation->Parent->TryComplete(asyncOperation->Parent, error);
                }, 
                thisSPtr);
        }
    }

    ErrorCode PrimaryReplicator::BuildIdleAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<BuildIdleAsyncOperation>(asyncOperation);
        return casted->Error;
    }

    void PrimaryReplicator::BuildIdleAsyncOperation::OnCancel()
    {
        {
            AcquireWriteLock grab(isCancelledLock_);
            isCancelled_ = true;
        }
        
        // On Start and On cancel can never race. So the session has to be valid
        //
        // This will ensure that the copy context getnext returns a failure, which would break the cycle if this async op is waiting for 
        // copy async op to complete, which in turn is waiting for the state provider get next call to complete, which is waiting for the copycontext
        // get next call to complete
        if (session_)
        {
            CleanupCCReceiver(
                session_,
                Common::ErrorCodeValue::OperationCanceled);
        }
    }
    
    // 
    //  Helper Functions
    //
    void PrimaryReplicator::BuildIdleAsyncOperation::CleanupCCReceiverAndCompleteOperation(
        ReplicationSessionSPtr const & session,
        Common::AsyncOperationSPtr const & thisSPtr,
        Common::ErrorCode const & errorCode)
    {
        CleanupCCReceiver(session, errorCode);
        thisSPtr->TryComplete(thisSPtr, errorCode);
    }

    void PrimaryReplicator::BuildIdleAsyncOperation::CleanupCCReceiver(
        ReplicationSessionSPtr const & session,
        Common::ErrorCode const & errorCode)
    {
        session->CleanupCopyContextReceiver(errorCode);
    }

    inline bool PrimaryReplicator::BuildIdleAsyncOperation::IsCancelledLockedRead()
    {
        AcquireReadLock grab(isCancelledLock_);
        return isCancelled_;
    }
} // end namespace ReplicationComponent
} // end namespace Reliability
