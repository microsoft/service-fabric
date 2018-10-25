// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            /*
            
                This class represents the activation state of a node with respect
                to a particular failover manager
            
            */
            class NodeDeactivationState
            {
                DENY_COPY(NodeDeactivationState);

            public:
                NodeDeactivationState(
                    Reliability::FailoverManagerId const & fm,
                    ReconfigurationAgent & ra);

                __declspec(property(get = get_IsActivated)) bool IsActivated;
                bool get_IsActivated() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return state_.IsActivated;
                }                                                            

                bool IsCurrent(int64 sequenceNumber) const
                {
                    Common::AcquireReadLock grab(lock_);
                    return state_.SequenceNumber == sequenceNumber;
                }

                __declspec(property(get = get_FailoverManagerId)) Reliability::FailoverManagerId const & FailoverManagerId;
                Reliability::FailoverManagerId const & get_FailoverManagerId() const { return fm_; }

                __declspec(property(get = get_DeactivationInfo)) NodeDeactivationInfo DeactivationInfo;
                NodeDeactivationInfo get_DeactivationInfo() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return state_;
                }

                Common::AsyncOperationSPtr Test_GetAsyncOp()
                {
                    Common::AcquireReadLock grab(lock_);
                    return replicaCloseMonitorAsyncOperation_;
                }

                // Returns true if the incoming sequence number is greater
                bool TryStartChange(
                    std::wstring const & activityId,
                    NodeDeactivationInfo const & newActivationInfo);

                void FinishChange(
                    std::wstring const & activityId,
                    NodeDeactivationInfo const & activationInfo,
                    Infrastructure::EntityEntryBaseList && ftsToMonitor);

                void Close();

            private:
                void StartReplicaCloseOperation(                    
                    std::wstring const & activityId,
                    NodeDeactivationInfo const & activationInfo,
                    Infrastructure::EntityEntryBaseList && ftsToMonitor);

                bool ShouldSendNodeDeactivationReply(
                    NodeDeactivationInfo const & activationInfo);

                void SendNodeDeactivationReply(
                    std::wstring const & activityId,
                    NodeDeactivationInfo const & activationInfo);

                void CancelReplicaCloseOperation(
                    NodeDeactivationInfo const & activationInfo,
                    Infrastructure::EntityEntryBaseList && ftsToMonitor);

                bool CheckStalenessAtFinish(
                    Common::AcquireWriteLock const &,
                    NodeDeactivationInfo const & incoming) const;

                mutable Common::RwLock lock_;
                Reliability::FailoverManagerId fm_;
                ReconfigurationAgent & ra_;
                NodeDeactivationInfo state_;
                Common::AsyncOperationSPtr replicaCloseMonitorAsyncOperation_;
            };
        }
    } 
}



