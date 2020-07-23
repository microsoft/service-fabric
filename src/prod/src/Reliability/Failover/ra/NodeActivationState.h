// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class NodeActivationState
        {
            DENY_COPY(NodeActivationState);

        public:
            enum Enum
            {
                Activated = 0,
                Deactivated = 1,
            };

            NodeActivationState(ReconfigurationAgent & ra);

            // Return true if the sequence number is greater than the current
            bool TryChangeActivationStatus(
                bool isFmm, 
                NodeActivationState::Enum newState, 
                int64 sequenceNumber, 
                NodeActivationState::Enum & currentActivationState, 
                int64 & currentSequenceNumber);

            void StartReplicaCloseOperation(
                std::wstring const & activityId, 
                bool isFmm, 
                int64 sequenceNumber, 
                Infrastructure::EntityEntryBaseList && ftsToMonitor);

            void CancelReplicaCloseOperation(
                std::wstring const & activityId, 
                bool isFmm, 
                int64 sequenceNumber);

            bool IsActivated(Reliability::FailoverUnitId const & ftId) const;

            bool IsActivated(bool isFmm) const;

            // Compare and see if the sequence number provided is still current 
            // Returns true if the sequence number matches
            // Returns false if the node activation state has changed from the earlier sequence number
            bool IsCurrent(Reliability::FailoverUnitId const & ftId, int64 sequenceNumber) const;

            static bool IsActivate(NodeActivationState::Enum state)
            {
                return state == NodeActivationState::Activated;
            }

            void Test_GetStatus(bool isFmm, bool & isActivatedOut, int64 & currentSequenceNumberOut)
            {
                Common::AcquireReadLock grab(lock_);
                if (isFmm)
                {
                    isActivatedOut = IsActivate(fmmState_.State);
                    currentSequenceNumberOut = fmmState_.SequenceNumber;
                }
                else
                {
                    isActivatedOut = IsActivate(fmState_.State);
                    currentSequenceNumberOut = fmState_.SequenceNumber;
                }
            }

            Common::AsyncOperationSPtr Test_GetReplicaCloseMonitorAsyncOperation(bool isFmm)
            {
                Common::AcquireReadLock grab(lock_);
                return GetState(isFmm).ReplicaCloseMonitorAsyncOperation;
            }

            void Close();

        private:

            struct InnerState
            {
                NodeActivationState::Enum State;
                int64 SequenceNumber;
                Common::AsyncOperationSPtr ReplicaCloseMonitorAsyncOperation;

                InnerState()
                : State(NodeActivationState::Activated),
                  SequenceNumber(Constants::InvalidNodeActivationSequenceNumber)
                {
                }
            };

            InnerState const & GetStateForFTId(Reliability::FailoverUnitId const & ftId) const
            {
                return ftId.IsFM ? fmmState_ : fmState_;
            }

            InnerState & GetState(bool isFmm)
            {
                return isFmm ? fmmState_ : fmState_;
            }

            InnerState const & GetState(bool isFmm) const
            {
                return isFmm ? fmmState_ : fmState_;
            }

            void AssertInvariants()
            {                
            }

            Common::RwLock mutable lock_;
            
            InnerState fmState_;
            InnerState fmmState_;
            ReconfigurationAgent & ra_;
        };
    }
}

