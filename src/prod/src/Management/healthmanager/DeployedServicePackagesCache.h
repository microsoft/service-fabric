// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class DeployedServicePackagesCache 
            : public HealthEntityCache<DeployedServicePackageHealthId>
        {
            DENY_COPY(DeployedServicePackagesCache);
            
        public:
            DeployedServicePackagesCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~DeployedServicePackagesCache();

            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(DeployedServicePackageHealthId, DeployedServicePackageEntity)
        };
    }
}
