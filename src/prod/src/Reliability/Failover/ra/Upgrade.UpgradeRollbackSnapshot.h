// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            class RollbackSnapshot
            {
            public:
                RollbackSnapshot(UpgradeStateName::Enum state, bool wereReplicasClosed)
                : state_(state),
                  wereReplicasClosed_(wereReplicasClosed)
                {
                }

                __declspec(property(get = get_State)) UpgradeStateName::Enum State;
                UpgradeStateName::Enum get_State() const { return state_; }

                __declspec(property(get = get_WereReplicasClosed)) bool WereReplicasClosed;
                bool get_WereReplicasClosed() const { return wereReplicasClosed_; }

            private:
                UpgradeStateName::Enum state_;
                bool wereReplicasClosed_;
            };
        }
    }
}

