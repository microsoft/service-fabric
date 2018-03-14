// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Constants.h"
#include "ApplicationCapacitiesDescription.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ApplicationCapacitiesDescription::ApplicationCapacitiesDescription(
    std::wstring && metricName,
    int totalCapacity,
    int maxInstanceCapacity,
    int reservationCapacity)
    : metricName_(move(metricName)),
    totalCapacity_(totalCapacity),
    maxInstanceCapacity_(maxInstanceCapacity),
    reservationCapacity_(reservationCapacity)
{
}

ApplicationCapacitiesDescription::ApplicationCapacitiesDescription(ApplicationCapacitiesDescription const & other)
: metricName_(other.metricName_),
totalCapacity_(other.totalCapacity_),
maxInstanceCapacity_(other.maxInstanceCapacity_),
reservationCapacity_(other.reservationCapacity_)
{
}

ApplicationCapacitiesDescription::ApplicationCapacitiesDescription(ApplicationCapacitiesDescription && other)
: metricName_(move(other.metricName_)),
totalCapacity_(other.totalCapacity_),
maxInstanceCapacity_(other.maxInstanceCapacity_),
reservationCapacity_(other.reservationCapacity_)
{
}

ApplicationCapacitiesDescription & ApplicationCapacitiesDescription::operator = (ApplicationCapacitiesDescription && other)
{
    if (this != &other)
    {
        metricName_ = move(other.metricName_);
        totalCapacity_ = other.totalCapacity_;
        maxInstanceCapacity_ = other.maxInstanceCapacity_;
        reservationCapacity_ = other.reservationCapacity_;
    }

    return *this;
}

bool ApplicationCapacitiesDescription::operator == (ApplicationCapacitiesDescription const & other) const
{
    // it is only used when we update the service description
    ASSERT_IFNOT(metricName_ == other.metricName_, "Comparison between two different applications");

    return (
        metricName_ == other.metricName_ &&
        totalCapacity_ == other.totalCapacity_ &&
        maxInstanceCapacity_ == other.maxInstanceCapacity_ &&
        reservationCapacity_ == other.reservationCapacity_);

}

bool ApplicationCapacitiesDescription::operator != (ApplicationCapacitiesDescription const & other) const
{
    return !(*this == other);
}

void ApplicationCapacitiesDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("Application capacity for metric:{0} totalCapacity:{1} maxInstanceCapacity:{2} reservationCapacity:{3}",
        metricName_, totalCapacity_, maxInstanceCapacity_, reservationCapacity_);
}

void ApplicationCapacitiesDescription::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
