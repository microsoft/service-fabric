// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ClusterCache 
            : public HealthEntityCache<ClusterHealthId>
        {
            DENY_COPY(ClusterCache);
            
        public:
            ClusterCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~ClusterCache();

            // TODO: locking weak ptr should be protected by a lock?
            __declspec (property(get=get_ClusterObj)) ClusterEntitySPtr ClusterObj;
            ClusterEntitySPtr get_ClusterObj() const { return cluster_.lock(); }

            Common::ErrorCode GetClusterHealthPolicy(
                __inout ServiceModel::ClusterHealthPolicySPtr & healthPolicy) const;

        protected:
            HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS(ClusterHealthId, ClusterEntity)

            void CreateDefaultEntities();

        private: 
            // Keep a weak pointer to cluster to avoid going to the cache to get the entity
            // The entity is by default created on Enable cache
            ClusterEntityWPtr cluster_; 
        };
    }
}
