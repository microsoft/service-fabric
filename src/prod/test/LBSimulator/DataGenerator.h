// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class FM;

    class DataGenerator
    {
        DENY_COPY(DataGenerator)

    public:
        static std::wstring GetMetricName(int metricIndex);
        DataGenerator(FM & fm, int seed, Reliability::LoadBalancingComponent::PLBConfig & plbConfig);
        void Parse(std::wstring const & fileName);
        void Generate();

    private:

        static std::wstring const MetricCountStr;
        static std::wstring const NodeCountStr;
        static std::wstring const FaultDomainsStr;
        static std::wstring const PartitionCountStr;
        static std::wstring const ReplicaCountPerPartitionStr;
        static std::wstring const NodeCapacityRatioRangeStr;
        static std::wstring const NodeCapacityRangeStr;
        static std::wstring const PrimaryLoadRangeStr;
        static std::wstring const SecondaryLoadRangeStr;
        static std::wstring const AffinitizedServicePairStr;

        int MetricCount;
        int NodeCount;
        std::vector<Common::Uri> FaultDomains;
        std::vector<int> PartitionCount;
        std::vector<int> ReplicaCountPerPartition;
        std::vector<LoadDistribution> NodeCapacityRange;
        std::vector<LoadDistribution> PrimaryLoadRange;
        std::vector<LoadDistribution> SecondaryLoadRange;
        int AffinitizedServicePair;
        int ReplicaCountPerAffinitizedService;

        void ReadIntVec(std::wstring & line, std::vector<int> & vecInt);
        void ReadLoadInputVec(std::wstring & line, std::vector<LoadDistribution> & vecLoad);

        void GenerateNodes();
        void GenerateServices();
        void GenerateFailoverUnits();
        void GeneratePlacements();
        void GenerateService(int serviceIndex, int partitionCount, int replicaCount, wstring affinitizedService);
        void GenerateFailoverUnit(int failoverUnitIndex, int serviceIndex, int replicaCount);

        FM & fm_;
        Common::Random random_;
        Reliability::LoadBalancingComponent::PLBConfig & config_;
    };
}
