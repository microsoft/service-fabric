// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            class ServiceTypeUpdateStalenessChecker
            {
                DENY_COPY(ServiceTypeUpdateStalenessChecker);

            public:
                ServiceTypeUpdateStalenessChecker(
                    Infrastructure::IClock & clock,
                    TimeSpanConfigEntry const& stalenessEntryTtl);

                bool TryUpdateServiceTypeSequenceNumber(
                    ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                    uint64 sequenceNumber);
                bool TryUpdatePartitionSequenceNumber(
                    PartitionId& partitionId,
                    uint64 sequenceNumber);

                void PerformCleanup();

            private:
                template<class K>
                bool TryUpdateSequenceNumber(
                    Infrastructure::ExpiringMap<K, uint64> & map,
                    K const& key,
                    uint64 sequenceNumber)
                {
                    auto it = map.find(key);
                    if (it != map.end() && sequenceNumber <= it->second.first)
                    {
                        TESTASSERT_IF(
                            sequenceNumber == it->second.first,
                            "RA received same sequence number twice for Hosting event. Key = {0}. LastUpdated = {1}.",
                            key,
                            it->second.second);

                        return false;
                    }

                    map.insert_or_assign(it, key, sequenceNumber, clock_.Now());
                    return true;
                }

                Infrastructure::IClock & clock_;
                TimeSpanConfigEntry const& stalenessEntryTtl_;

                // Staleness map for service type updates
                Infrastructure::ExpiringMap<ServiceModel::ServiceTypeIdentifier, uint64> serviceTypeStalenessMap_;

                // Staleness map for partition updates
                Infrastructure::ExpiringMap<PartitionId, uint64> partitionStalenessMap_;
            };
        }
    }
}
