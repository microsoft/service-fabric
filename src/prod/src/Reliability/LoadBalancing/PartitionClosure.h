// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "PLBSchedulerAction.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Service;
        class FailoverUnit;
        class Application;
        class ServicePackage;

        struct PartitionClosureType
        {
        public:
            enum Enum
            {
                None = 0,
                NewReplicaPlacement = 1,
                ConstraintCheck = 2,
                Full = 3,
                // NewReplicaPlacementWithMove closure(used for placement with move) includes two closures: Partial and Full closure
                NewReplicaPlacementWithMove = 4,
            };

            static PartitionClosureType::Enum FromPLBSchedulerAction(PLBSchedulerActionType::Enum action);
        };

        class PartitionClosure;
        typedef std::unique_ptr<PartitionClosure> PartitionClosureUPtr;

        /// <summary>
        /// Class to represent a transitive closure of partitions and services obtained from an initial set of partitions and services,
        /// where the transitive relation is defined as two partitions must be considered together under some constraints.
        /// </summary>
        class PartitionClosure
        {
            DENY_COPY(PartitionClosure);

        public:
            PartitionClosure(
                std::vector<Service const*> && services,
                std::vector<FailoverUnit const*> && partitions,
                std::vector<Application const*> && applications,
                std::set<Common::Guid> && partialClosureFailoverUnits,
                std::vector<ServicePackage const*> && servicePackages,
                PartitionClosureType::Enum closureType);

            PartitionClosure(PartitionClosure && other);

            __declspec (property(get=get_Services)) std::vector<Service const*> const& Services;
            std::vector<Service const*> const& get_Services() const { return services_; }

            __declspec (property(get=get_Partitions)) std::vector<FailoverUnit const*> const& Partitions;
            std::vector<FailoverUnit const*> const& get_Partitions() const { return partitions_; }

            __declspec (property(get = get_Applications)) std::vector<Application const*> const& Applications;
            std::vector<Application const*> const& get_Applications() const { return applications_; }

            __declspec (property(get = get_PartialClosureFailoverUnits)) std::set<Common::Guid> const& PartialClosureFailoverUnits;
            std::set<Common::Guid> const& get_PartialClosureFailoverUnits() const { return partialClosureFailoverUnits_; }

            __declspec (property(get = get_ServicePackages)) std::vector<ServicePackage const*> const& ServicePackages;
            std::vector<ServicePackage const*> const& get_ServicePackages() const { return servicePackages_; }

            __declspec (property(get = get_Type)) PartitionClosureType::Enum const& Type;
            PartitionClosureType::Enum const& get_Type() const { return type_; }

            bool IsForConstraintCheck() const { return type_ == PartitionClosureType::ConstraintCheck; }

        private:
            std::vector<Application const*> applications_;
            std::vector<Service const*> services_;
            std::vector<FailoverUnit const*> partitions_;
            // partition used for NewReplicaPlacementWithMove phase with partial closure
            std::set<Common::Guid> partialClosureFailoverUnits_;
            std::vector<ServicePackage const*> servicePackages_;
            PartitionClosureType::Enum type_;
        };
    }
}
