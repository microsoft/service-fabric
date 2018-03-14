// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        typedef Common::Guid PartitionHealthId;

        typedef Common::LargeInteger NodeHealthId;

        typedef std::wstring ClusterHealthId;
             
        class HealthManagerReplica;
        typedef std::shared_ptr<HealthManagerReplica> HealthManagerReplicaSPtr;

        class IHealthAggregator;
        typedef std::shared_ptr<IHealthAggregator> IHealthAggregatorSPtr;

        class HealthManagerCounters;
        typedef std::shared_ptr<HealthManagerCounters> HealthManagerCountersSPtr;
    }
}
