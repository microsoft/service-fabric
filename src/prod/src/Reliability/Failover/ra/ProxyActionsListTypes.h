// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ProxyActionsListTypes
        {
            enum Enum
            {
                Empty = 0,
                StatelessServiceOpen = 1,
                StatelessServiceClose = 2,
                StatelessServiceAbort = 3,
                StatefulServiceOpenIdle = 4,
                StatefulServiceOpenPrimary = 5,
                StatefulServiceReopen = 6,
                StatefulServiceClose = 7,
                StatefulServiceDrop = 8,
                StatefulServiceAbort = 9,
                StatefulServiceChangeRole = 10,
                StatefulServicePromoteToPrimary = 11,
                StatefulServiceDemoteToSecondary = 12,
                StatefulServiceFinishDemoteToSecondary = 13,
                StatefulServiceEndReconfiguration = 14,
                ReplicatorBuildIdleReplica = 15,
                ReplicatorRemoveIdleReplica = 16,
                ReplicatorGetStatus = 17,
                ReplicatorUpdateEpochAndGetStatus = 18,
                ReplicatorUpdateReplicas = 19,
                ReplicatorUpdateAndCatchupQuorum = 20,
                CancelCatchupReplicaSet = 21,
                ReplicatorGetQuery = 22,
                UpdateServiceDescription = 23,
                StatefulServiceFinalizeDemoteToSecondary = 24,
                PROXY_ACTION_LIST_TYPES_COUNT = 25, // This must be the last entry in the enum; it is the number of entries
            };

            static size_t const ProxyActionsListTypesCount = static_cast<size_t>(PROXY_ACTION_LIST_TYPES_COUNT);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(ProxyActionsListTypes);
        }
    }
}
