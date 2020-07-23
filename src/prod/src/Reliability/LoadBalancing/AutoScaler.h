// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {

        class ServiceDomain;
        class Service;

        // All the information needed to invoke the client call for repartitioning.
        // Client will be invoked outside of lock: all needed info is here.
        struct AutoScalingRepartitionInfo
        {
            AutoScalingRepartitionInfo(uint64_t id, std::wstring const& name, bool stateful, uint currentCount, int change)
                : ServiceId(id), ServiceName(name), IsStateful(stateful), CurrentPartitionCount(currentCount), Change(change)
            {}

            // The Id of the service that is repartitioned.
            uint64_t ServiceId;
            // Service name
            std::wstring ServiceName;
            // Is the service stateful?
            bool IsStateful;
            // Current partition count
            uint CurrentPartitionCount;
            // Change in number of partitions
            int Change;
        };

        struct FailoverUnitAndExpiry
        {
            Common::StopwatchTime expiry;
            Common::Guid FUid;

            FailoverUnitAndExpiry() = default;
            FailoverUnitAndExpiry(Common::Guid FUid_, Common::StopwatchTime expiry_) : FUid(FUid_), expiry(expiry_)
            {

            }
            
            bool operator <(FailoverUnitAndExpiry const& other) const
            {
                if (expiry != other.expiry)
                {
                    return expiry < other.expiry;
                }
                return FUid < other.FUid;
            }
        };

        struct ServiceAndExpiry
        {
            Common::StopwatchTime Expiry;
            uint64_t ServiceId;

            ServiceAndExpiry() = default;
            ServiceAndExpiry(uint64_t id, Common::StopwatchTime exp) : ServiceId(id), Expiry(exp) {}

            bool operator<(ServiceAndExpiry const& other) const
            {
                if (Expiry != other.Expiry)
                {
                    return Expiry < other.Expiry;
                }
                return ServiceId < other.ServiceId;
            }
        };

        class AutoScaler
        {
        public:
            AutoScaler();
            ~AutoScaler();

            // Called on each Refresh to process auto scaling
            void Refresh(Common::StopwatchTime timestamp, ServiceDomain & serviceDomain_, size_t upNodeCount);

            // Add a new FT with auto scaling.
            void AddFailoverUnit(Common::Guid failoverUnitId, Common::StopwatchTime expiry);
            // Remove a FT (when it is deleted, or when auto scaling is removed).
            void RemoveFailoverUnit(Common::Guid failoverUnitId, Common::StopwatchTime expiry);
            // Called on FT update when FM acknowledges change in target count.
            void ProcessFailoverUnitTargetUpdate(Common::Guid const & failoverUnitId, int targetReplicaSetSize);

            // Add new service with auto scaling
            void AddService(uint64_t serviceId, Common::StopwatchTime expiry);
            // Remove service with auto scaling
            void RemoveService(uint64_t serviceId, Common::StopwatchTime expiry);
            // Update partition count when FM updates the service
            void UpdateServicePartitionCount(uint64_t serviceId, int change);
            // Auto scaling operation has failed
            void UpdateServiceAutoScalingOperationFailed(uint64_t serviceId);
            // Used for merging auto scalers from different service domains.
            void MergeAutoScaler(AutoScaler && other);
        private:
            void RefreshFTs(Common::StopwatchTime timestamp, ServiceDomain & serviceDomain_, size_t upNodeCount);
            void RefreshServices(Common::StopwatchTime timestamp, ServiceDomain & serviceDomain_);

            // we want to have access to the FU with the lowest expiry
            std::set<FailoverUnitAndExpiry> pendingAutoScaleSet_;
            //used to track ongoing target replica count updates to FM for auto scale
            std::map<Common::Guid, int> ongoingTargetUpdates_;

            // Pending set of services
            std::set<ServiceAndExpiry> pendingAutoScaleServices_;
            std::map<uint64_t, int> ongoingServiceUpdates_;
        };

    }
}
