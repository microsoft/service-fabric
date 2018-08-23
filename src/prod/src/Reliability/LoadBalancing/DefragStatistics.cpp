// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DefragStatistics.h"
#include "PLBConfig.h"
#include "ServicePackageDescription.h"
#include "NodeDescription.h"
#include "ServiceDescription.h"
#include "Snapshot.h"
#include "Metric.h"


using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

DefragStatistics::DefragStatistics()
{
}

void DefragStatistics::Update(Snapshot const & snapshot)
{
    Clear();

    for (auto && itDomain = snapshot.ServiceDomainSnapshot.begin(); itDomain != snapshot.ServiceDomainSnapshot.end(); ++itDomain)
    {
        auto & placement = itDomain->second.State.PlacementObj;
        if (nullptr != placement) 
        {
            auto & domain = placement->BalanceCheckerObj->LBDomains.back();
            for (auto &metric : domain.Metrics)
            {
                if (metric.Weight <= 0)
                {
                    otherMetricsCount_++;
                }
                else
                {
                    auto placementStrategy = metric.placementStrategy;
                    switch (placementStrategy)
                    {
                    case Metric::PlacementStrategy::Balancing:
                        balancingMetricsCount_++;
                        break;
                    case Metric::PlacementStrategy::ReservationAndBalance:
                        balancingReservationMetricsCount_++;
                        break;
                    case Metric::PlacementStrategy::Reservation:
                        reservationMetricsCount_++;
                        break;
                    case Metric::PlacementStrategy::ReservationAndPack:
                        packReservationMetricsCount_++;
                        break;
                    case Metric::PlacementStrategy::Defragmentation:
                        defragMetricsCount_++;
                        break;
                    default: Assert::TestAssert("Placement strategy is not set properly");
                    }
                }
            }
        }
    }
}

void DefragStatistics::Clear()
{
    balancingMetricsCount_ = 0;
    balancingReservationMetricsCount_ = 0;
    reservationMetricsCount_ = 0;
    packReservationMetricsCount_ = 0;
    defragMetricsCount_ = 0;
    otherMetricsCount_ = 0;
}

string DefragStatistics::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "MetricPerStrategies={0}/{1}/{2}/{3}/{4}\r\nOtherMetrics={5}";
    size_t index = 0;

    traceEvent.AddEventField<uint64_t>(format, name + ".balancingMetricsCount", index);
    traceEvent.AddEventField<uint64_t>(format, name + ".balancingReservationMetricsCount", index);
    traceEvent.AddEventField<uint64_t>(format, name + ".reservationMetricsCount", index);
    traceEvent.AddEventField<uint64_t>(format, name + ".packReservationMetricsCount", index);
    traceEvent.AddEventField<uint64_t>(format, name + ".defragMetricsCount", index);
    
    traceEvent.AddEventField<uint64_t>(format, name + ".otherMetricsCount", index);

    return format;
}

void DefragStatistics::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(balancingMetricsCount_);
    context.Write(balancingReservationMetricsCount_);
    context.Write(reservationMetricsCount_);
    context.Write(packReservationMetricsCount_);
    context.Write(defragMetricsCount_);
    context.Write(otherMetricsCount_);
}

void DefragStatistics::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine("MetricPerStrategies={0}/{1}/{2}/{3}/{4} OtherMetrics={5}",
        balancingMetricsCount_,
        balancingReservationMetricsCount_,
        reservationMetricsCount_,
        packReservationMetricsCount_,
        defragMetricsCount_,
        otherMetricsCount_);
}
