// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReportLoadInfo : public Serialization::FabricSerializable
    {
    public:
        ReportLoadInfo()
        {
        }

        ReportLoadInfo(
            Reliability::FailoverUnitId const & failoverUnitId,
            bool isStateful,
            Reliability::ReplicaRole::Enum const & replicaRole,
            std::vector<Reliability::LoadBalancingComponent::LoadMetric> && loadMetrics) :
            isStateful_(isStateful),
            failoverUnitId_(failoverUnitId),
            replicaRole_(replicaRole),
            loadMetrics_(std::move(loadMetrics))
        {
        }

        ReportLoadInfo(ReportLoadInfo && other) :
            isStateful_(other.isStateful_),
            failoverUnitId_(other.failoverUnitId_),
            replicaRole_(other.replicaRole_),
            loadMetrics_(std::move(other.loadMetrics_))
        {
        }

        ReportLoadInfo & operator=(ReportLoadInfo && other)
        {
            if (this != &other)
            {
                isStateful_ = other.isStateful_;
                failoverUnitId_ = other.failoverUnitId_;
                replicaRole_ = other.replicaRole_;
                loadMetrics_ = std::move(other.loadMetrics_);
            }

            return *this;
        }

        __declspec (property(get=get_IsStateful)) bool IsStateful;;
        bool get_IsStateful() const { return isStateful_; }

        __declspec (property(get=get_FailoverUnitId)) Reliability::FailoverUnitId const & FailoverUnitId;
        Reliability::FailoverUnitId const & get_FailoverUnitId() const { return failoverUnitId_; }

        __declspec (property(get=get_ReplicaRole)) Reliability::ReplicaRole::Enum ReplicaRole;
        Reliability::ReplicaRole::Enum get_ReplicaRole() const { return replicaRole_; }

        __declspec (property(get=get_LoadMetrics)) std::vector<Reliability::LoadBalancingComponent::LoadMetric> const & LoadMetrics;
        std::vector<Reliability::LoadBalancingComponent::LoadMetric> const & get_LoadMetrics() const { return loadMetrics_; }

        void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
        {
            writer.Write("{0}/{1}:", failoverUnitId_, replicaRole_);

            for (Reliability::LoadBalancingComponent::LoadMetric const & loadMetric : loadMetrics_)
            {
                writer.WriteLine("{0} ", loadMetric);
            }
        }

        FABRIC_FIELDS_03(failoverUnitId_, replicaRole_, loadMetrics_);

    private:
        Reliability::FailoverUnitId failoverUnitId_;
        Reliability::ReplicaRole::Enum replicaRole_;
        std::vector<Reliability::LoadBalancingComponent::LoadMetric> loadMetrics_;
        bool isStateful_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ReportLoadInfo);
