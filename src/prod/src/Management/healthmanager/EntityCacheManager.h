// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // CacheManager that keeps track of all entities where health entries were reported.
        // The cache manager follows the incoming requests to build the cache.
        // If the replica just became primary (due to failover or rebalancing),
        // the caches need to be built from replicated store.
        // LIFETIME:
        // The EntityCacheManager is build and held alive by the HealthManagerReplica.
        class EntityCacheManager
            : public Common::RootedObject
        {
            DENY_COPY(EntityCacheManager);

        public:
            EntityCacheManager(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~EntityCacheManager();
            
            __declspec (property(get = get_HealthManagerReplica)) HealthManagerReplica & HealthManagerReplicaObj;
            HealthManagerReplica & get_HealthManagerReplica() const { return healthManagerReplica_; }

            __declspec(property(get=get_PartitionedReplicaId)) Store::PartitionedReplicaId const & PartitionedReplicaId;
            Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return healthManagerReplica_.PartitionedReplicaId; }
            
            __declspec (property(get=get_HealthManagerCounters)) HealthManagerCountersSPtr const & HealthManagerCounters;
            HealthManagerCountersSPtr const & get_HealthManagerCounters() const { return healthManagerReplica_.HealthManagerCounters; }

            __declspec (property(get=get_Partitions)) PartitionsCache & Partitions;
            PartitionsCache & get_Partitions() { return partitions_; }

            __declspec (property(get=get_Replicas)) ReplicasCache const & Replicas;
            ReplicasCache const & get_Replicas() const { return replicas_; }

            __declspec (property(get=get_Nodes)) NodesCache const & Nodes;
            NodesCache const & get_Nodes() const { return nodes_; }

            __declspec (property(get=get_Services)) ServicesCache & Services;
            ServicesCache & get_Services() { return services_; }

            __declspec (property(get=get_Applications)) ApplicationsCache & Applications;
            ApplicationsCache & get_Applications() { return applications_; }

            __declspec (property(get=get_DeployedApplications)) DeployedApplicationsCache & DeployedApplications;
            DeployedApplicationsCache & get_DeployedApplications() { return deployedApplications_; }

            __declspec (property(get=get_DeployedServicePackages)) DeployedServicePackagesCache & DeployedServicePackages;
            DeployedServicePackagesCache & get_DeployedServicePackages() { return deployedServicePackages_; }

            __declspec (property(get=get_Cluster)) ClusterCache & Cluster;
            ClusterCache & get_Cluster() { return cluster_; }

            // State changes - the cache manager should only be enabled on the primary
            Common::ErrorCode Open();
            Common::ErrorCode Close();

            bool IsReady() const;
            void OnCacheLoadCompleted();

            // Give reports to the correct entities to process them
            void AddReports(
                std::vector<ReportRequestContext> && contexts);
            void OnContextCompleted(RequestContext const & context);
            
            // Sequence stream
            void AddSequenceStreamContext(
                SequenceStreamRequestContext && context);
            Common::ErrorCode GetSequenceStreamProgress(
                SequenceStreamId const & sequenceStreamId,
                FABRIC_INSTANCE_ID instance,
                bool getHighestLsn,
                __inout SequenceStreamResult & sequenceStreamResult);
            Common::ErrorCode AcceptSequenceStream(
                Common::ActivityId const & activityId,
                SequenceStreamId const & sequenceStreamId,
                FABRIC_INSTANCE_ID instance,
                FABRIC_SEQUENCE_NUMBER upToLsn);

            Common::ErrorCode TryCreateCleanupJob(Common::ActivityId const & activityId, std::wstring const & entityId);
            void OnCleanupJobCompleted();
            
        private:
            void ProcessReports(std::vector<ReportRequestContext> && contexts);
            void ProcessSequenceStream(SequenceStreamRequestContext && context);

            void TryAccept(
                std::vector<ReportRequestContext> && contexts,
                __in std::vector<ReportRequestContext> & acceptedReports,
                __in std::vector<ReportRequestContext> & rejectedReports);
            bool TryAccept(RequestContext const & context);
      
        private:
            HealthManagerReplica & healthManagerReplica_;

            MUTABLE_RWLOCK(HM.CacheManager, lock_);
            volatile LONG loadingCacheCount_;

            // Queued reports and ss while the initial store loading is not complete.
            // Once the data from store is completely loaded, the blocked contexts 
            // will be enqueued in the job queue for processing.
            std::vector<ReportRequestContext> blockedReports_;
            std::vector<SequenceStreamRequestContext> blockedStreams_;

            // Cache with all node entities
            NodesCache nodes_;

            // Cache with all partition entities
            PartitionsCache partitions_;

            // Cache with all replica entities
            ReplicasCache replicas_;

            // Cache with all service entities. Non-persisted
            ServicesCache services_;
            
            // Cache with all application entities. Non-persisted
            ApplicationsCache applications_;

            // Cache with all deployed applications entities
            DeployedApplicationsCache deployedApplications_;

            // Cache with all deployed service packages entities
            DeployedServicePackagesCache deployedServicePackages_;

            // Cache with just one entity, the cluster
            ClusterCache cluster_;

            // Track the reports and sequence streams given to job queue manager for processing.
            // When the max limit is reached, reports and ss are rejected.
            // Represents the count or the size of the reports depending on EnableMaxPendingHealthReportSizeEstimation flag.
            Common::atomic_uint64 pendingReportAmount_;
            
            // Special reports (like system authority reports for components) and ss are queued separately
            // to ensure they are not dropped because of too many lower priority reports.
            // Represents the count or the size of the reports depending on EnableMaxPendingHealthReportSizeEstimation flag.
            Common::atomic_uint64 pendingCriticalReportAmount_;

            Common::atomic_uint64 pendingCleanupJobCount_;
        };
    }
}
