// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace TXRStatefulServiceBase
{
    typedef std::function<StatefulServiceBaseCPtr(FABRIC_PARTITION_ID, FABRIC_REPLICA_ID, Common::ComponentRootSPtr const &)> CreateReplicaCallback;

    class Factory 
        : public Common::ComponentRoot
    {
    public:

        static std::shared_ptr<Factory> Create(__in CreateReplicaCallback createReplicaCallback);

        virtual ~Factory();
         
        StatefulServiceBaseCPtr CreateReplica(
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId,
            Common::ComponentRootSPtr const & root);

    private:
        Factory(__in CreateReplicaCallback createReplicaCallback);

        CreateReplicaCallback createReplicaCallBack_;
    };
}
