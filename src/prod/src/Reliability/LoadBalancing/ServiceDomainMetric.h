// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Accumulator.h"
#include "DynamicBitSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceDomainLocalMetric;
        class Node;

        class ServiceDomainMetric
        {
        public:
            ServiceDomainMetric(std::wstring && name);
            ServiceDomainMetric(ServiceDomainMetric const& other);
            ServiceDomainMetric(ServiceDomainMetric && other);

            static Common::GlobalWString const FormatHeader;

            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get=get_ServiceCount)) size_t ServiceCount;
            size_t get_ServiceCount() const { return serviceCount_; }

            __declspec (property(get = get_ApplicationCount, put = put_ApplicationCount)) size_t & ApplicationCount;
            size_t & get_ApplicationCount() { return applicationCount_; }
            void put_ApplicationCount(size_t applicationCount) { applicationCount_ = applicationCount; }

            __declspec (property(get=get_NodeLoadSum)) int64 NodeLoadSum;
            int64 get_NodeLoadSum() const { return nodeLoadSum_; }

            __declspec (property(get=get_AverageWeight)) double AverageWeight;
            double get_AverageWeight() const { return serviceCount_ == 0 ? 0.0 : std::max(totalWeight_ / serviceCount_, 0.0); }

            __declspec(property(get = get_NodeLoads)) std::map<Federation::NodeId, int64> const& NodeLoads;
            std::map<Federation::NodeId, int64> const& get_NodeLoads() const { return nodeLoads_; }

            __declspec(property(get = get_ShouldDisappearNodeLoads)) std::map<Federation::NodeId, int64> const& ShouldDisappearNodeLoads;
            std::map<Federation::NodeId, int64> const& get_ShouldDisappearNodeLoads() const { return shouldDisappearNodeLoads_; }

            void IncreaseServiceCount(double metricWeightInService, bool affectsBalancing);
            void DecreaseServiceCount(double metricWeightInService, bool affectsBalancing);

            void AddLoad(Federation::NodeId, uint load, bool shouldDisappear);
            void DeleteLoad(Federation::NodeId, uint load, bool shouldDisappear);

            void AddTotalLoad(uint load);
            void DeleteTotalLoad(uint load);

            int64 GetLoad(Federation::NodeId node, bool shouldDisappear = false) const;

            void UpdateBlockNodeServiceCount(uint64 nodeIndex, int delta);

            DynamicBitSet GetBlockNodeList() const;

            void VerifyLoads(ServiceDomainLocalMetric const& baseLoads, bool includeDisappearingLoad) const;
            void VerifyAreZero(bool includeDisappearingLoad) const;

            bool WriteFor(Node node, Common::StringWriterA& w) const;

            //TODO::Reversible Tracing/Writeto:#1955390
            //void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
            //void WriteToEtw(uint16 contextSequenceId) const;

        private:

            void AddLoadInternal(Federation::NodeId node, uint load, std::map<Federation::NodeId, int64> & loads);
            void DeleteLoadInternal(Federation::NodeId node, uint load, std::map<Federation::NodeId, int64> & loads);

            std::wstring name_;

            // count of services which reports this metric
            size_t serviceCount_;

            // count of stateless services that are spanned across all nodes - not important services for triggering balancing
            size_t notAffectingBalancingServiceCount_;

            size_t applicationCount_;
            double totalWeight_;
            // all replicas except MoveInProgress and ToBeDropped replicas load
            std::map<Federation::NodeId, int64> nodeLoads_;
            // MoveInProgress and ToBeDropped replicas load
            std::map<Federation::NodeId, int64> shouldDisappearNodeLoads_;
            int64 nodeLoadSum_;
            // how many services have been blocked on that node
            std::map<uint64, int64> blockNodeMap_;
        };
    }
}
