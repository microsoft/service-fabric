// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class DeployedApplicationsCache 
            : public HealthEntityCache<DeployedApplicationHealthId>
        {
            DENY_COPY(DeployedApplicationsCache);
            
        public:
            DeployedApplicationsCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~DeployedApplicationsCache();

            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(DeployedApplicationHealthId, DeployedApplicationEntity)
        };
    }
}
