// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ProxyActions
        {
            enum Enum
            {
                // Stateless Service
                OpenInstance,
                CloseInstance,
                
                // Stateful Service
                OpenReplica,
                ChangeReplicaRole,
                CloseReplica,
                
                // Replicator (IFabricReplicator)
                OpenReplicator,
                ChangeReplicatorRole,
                UpdateEpoch,
                CloseReplicator,
                AbortReplicator,
                AbortReplica,
                AbortInstance,
                GetReplicatorStatus,
                
                // Replicator (IFabricPrimaryReplicator)
                ReplicatorPreWriteStatusRevokeUpdateConfiguration,
                ReplicatorUpdateCurrentConfiguration,
                ReplicatorUpdateCatchUpConfiguration,
                ReplicatorUpdateConfiguration,
                CatchupReplicaSetQuorum,
                CatchupReplicaSetAll,
                PreWriteStatusRevokeCatchup,
                CancelCatchupReplicaSet,
                BuildIdleReplica,
                RemoveIdleReplica,
                ReportAnyDataLoss,
                
                // FailoverUnit proxy hosting a StatefulService
                ReconfigurationStarting,
                ReconfigurationEnding,
                UpdateReadWriteStatus,
                UpdateServiceDescription,
                OpenPartition,
                ClosePartition,
                FinalizeDemoteToSecondary,

                // IFabricInternalReplicator
                GetReplicatorQuery,

                LastValidEnum = GetReplicatorQuery
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(ProxyActions);
        }
    }
}

DEFINE_AS_BLITTABLE(Reliability::ReconfigurationAgentComponent::ProxyActions::Enum);
