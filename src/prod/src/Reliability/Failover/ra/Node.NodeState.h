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
            class NodeState
            {
                DENY_COPY(NodeState);
            public:
                NodeState(
                    ReconfigurationAgent & ra,
                    Infrastructure::EntitySetCollection & setCollection,
                    FailoverConfig const & config) :
                    fmDeactivationState_(Common::make_unique<NodeDeactivationState>(*FailoverManagerId::Fm, ra)),
                    fmmDeactivationState_(Common::make_unique<NodeDeactivationState>(*FailoverManagerId::Fmm, ra)),
                    fmPendingReplicaUploadProcessor_(Common::make_unique<PendingReplicaUploadStateProcessor>(ra, setCollection, *FailoverManagerId::Fm)),
                    fmmPendingReplicaUploadProcessor_(Common::make_unique<PendingReplicaUploadStateProcessor>(ra, setCollection, *FailoverManagerId::Fmm)),
                    fmmMessageThrottle_(std::make_shared<FMMessageThrottle>(config)),
                    fmMessageThrottle_(std::make_shared<FMMessageThrottle>(config))
                {
                }

                NodeDeactivationState & GetNodeDeactivationState(FailoverManagerId const & id)
                {
                    return id.IsFmm ? *fmmDeactivationState_ : *fmDeactivationState_;
                }

                NodeDeactivationState const & GetNodeDeactivationState(FailoverManagerId const & id) const
                {
                    return id.IsFmm ? *fmmDeactivationState_ : *fmDeactivationState_;
                }

                PendingReplicaUploadStateProcessor & GetPendingReplicaUploadStateProcessor(FailoverManagerId const & id)
                {
                    return id.IsFmm ? *fmmPendingReplicaUploadProcessor_ : *fmPendingReplicaUploadProcessor_;
                }

                PendingReplicaUploadStateProcessor const & GetPendingReplicaUploadStateProcessor(FailoverManagerId const & id) const
                {
                    return id.IsFmm ? *fmmPendingReplicaUploadProcessor_ : *fmPendingReplicaUploadProcessor_;
                }

                Infrastructure::IThrottleSPtr GetMessageThrottle(FailoverManagerId const & id)
                {
                    return id.IsFmm ? fmmMessageThrottle_ : fmMessageThrottle_;
                }

                void Close()
                {
                    fmDeactivationState_->Close();
                    fmmDeactivationState_->Close();

                    fmPendingReplicaUploadProcessor_->Close();
                    fmmPendingReplicaUploadProcessor_->Close();
                }

            private:
                NodeDeactivationStateUPtr fmDeactivationState_;
                NodeDeactivationStateUPtr fmmDeactivationState_;

                PendingReplicaUploadStateProcessorUPtr fmPendingReplicaUploadProcessor_;
                PendingReplicaUploadStateProcessorUPtr fmmPendingReplicaUploadProcessor_;

                FMMessageThrottleSPtr fmMessageThrottle_;
                FMMessageThrottleSPtr fmmMessageThrottle_;
            };
        }
    }
}


