// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceDomainLocalMetric
        {
            DENY_COPY(ServiceDomainLocalMetric);

        public:
            ServiceDomainLocalMetric();

            ServiceDomainLocalMetric(ServiceDomainLocalMetric && other);
            ServiceDomainLocalMetric &operator=(ServiceDomainLocalMetric && other);

            void AddLoad(Federation::NodeId, uint load);

            void DeleteLoad(Federation::NodeId, uint load);

            int64 GetLoad(Federation::NodeId node) const;

            void VerifyLoads(ServiceDomainLocalMetric const& baseLoads) const;
            void VerifyAreZero() const; // verify all loads are zero

        private:
            std::map<Federation::NodeId, int64> nodeLoads_;
        };
    }
}
