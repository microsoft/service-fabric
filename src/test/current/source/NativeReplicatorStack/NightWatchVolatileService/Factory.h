// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace V1ReplPerf
{
    class Factory : public Common::ComponentRoot
    {
    public:
        static std::shared_ptr<Factory> Create();

        virtual ~Factory()
        {
        }

        ServiceSPtr CreateReplica(
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId,
            Common::ComponentRootSPtr const & root);

    private:
        Factory()
        {
        }
    };
}
