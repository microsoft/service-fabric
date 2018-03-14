// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComTestReplicaMap
    {
    public:

        ComTestReplicaMap() = default;

        void ClearReplicaMap();
        void UpdateReplicaMap(FABRIC_REPLICA_ID replicaId);
        void UpdateCatchupReplicaSet(std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & ccReplicas, std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & pcReplicas, std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & pcAndccReplicas);
        void UpdateCurrentReplicaSet(std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> const & ccReplicas);
        void BeginBuildReplica(FABRIC_REPLICA_ID replicaId);
        void BuildReplicaOnComplete(FABRIC_REPLICA_ID replicaId, HRESULT hr);
        void EndBuildReplica(IFabricAsyncOperationContext * context, HRESULT hr);
        void RemoveReplica(FABRIC_REPLICA_ID replicaId);

        // asyncContext map
        void AsyncMapAdd(IFabricAsyncOperationContext * context, FABRIC_REPLICA_ID replicaId);

    private:
        std::map<FABRIC_REPLICA_ID, ComTestReplica> replicas_;
        std::map<IFabricAsyncOperationContext *, FABRIC_REPLICA_ID> asyncContextMap_;

        Common::RwLock lock_;
    };
};
