// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ApplicationsCache 
            : public HealthEntityCache<ApplicationHealthId>
        {
            DENY_COPY(ApplicationsCache);
            
        public:
            ApplicationsCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~ApplicationsCache();

            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(ApplicationHealthId, ApplicationEntity)
        };
    }
}
