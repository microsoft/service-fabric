// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReplicaUp
        {
            class ReplicaUpMessageBuilder
            {
                DENY_COPY(ReplicaUpMessageBuilder);

            public:
                typedef void (*ReplicaUpSendFunctionPtr) (IFederationWrapper &, ReplicaUpMessageBody const &);

                ReplicaUpMessageBuilder(ReplicaUpSendFunctionPtr sendFunc);

                void ProcessJobItem(ReplicaUpMessageRetryJobItem & jobItem);

                void FinalizeAndSend(LastReplicaUpState const & lastReplicaUpState, ReconfigurationAgent & ra);

            private:
                bool ShouldSendMessage(bool lastReplicaUp) const;

                std::vector<FailoverUnitInfo> upReplicas_;
                std::vector<FailoverUnitInfo> droppedReplicas_;
                std::set<FailoverUnitId> ftsInMessage_;

                ReplicaUpSendFunctionPtr sendFunc_;
            };
        }
    }
}

