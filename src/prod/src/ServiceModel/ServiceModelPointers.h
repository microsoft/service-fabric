// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicatorStatusQueryResult;
    typedef std::shared_ptr<ReplicatorStatusQueryResult> ReplicatorStatusQueryResultSPtr;

    class ReplicatorQueueStatus;
    typedef std::shared_ptr<ReplicatorQueueStatus> ReplicatorQueueStatusSPtr;

    class ReplicaStatusQueryResult;
    typedef std::shared_ptr<ReplicaStatusQueryResult> ReplicaStatusQueryResultSPtr;

    class DeployedServiceReplicaQueryResult;
    typedef std::shared_ptr<DeployedServiceReplicaQueryResult> DeployedServiceReplicaQueryResultSPtr;
}

