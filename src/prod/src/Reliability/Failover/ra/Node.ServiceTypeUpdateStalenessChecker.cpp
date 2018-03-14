// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Node;
using namespace Infrastructure;
using namespace ServiceModel;

ServiceTypeUpdateStalenessChecker::ServiceTypeUpdateStalenessChecker(
    Infrastructure::IClock & clock,
    TimeSpanConfigEntry const& stalenessEntryTtl)
    : clock_(clock)
    , stalenessEntryTtl_(stalenessEntryTtl)
    , serviceTypeStalenessMap_()
    , partitionStalenessMap_()
{
}

bool ServiceTypeUpdateStalenessChecker::TryUpdateServiceTypeSequenceNumber(
    ServiceTypeIdentifier const& serviceTypeId,
    uint64 sequenceNumber)
{
    return TryUpdateSequenceNumber<ServiceTypeIdentifier>(
        serviceTypeStalenessMap_,
        serviceTypeId,
        sequenceNumber);
}

bool ServiceTypeUpdateStalenessChecker::TryUpdatePartitionSequenceNumber(
    PartitionId & partitionId,
    uint64 sequenceNumber)
{
    return TryUpdateSequenceNumber<PartitionId>(
        partitionStalenessMap_,
        partitionId,
        sequenceNumber);
}

void ServiceTypeUpdateStalenessChecker::PerformCleanup()
{
    auto expiredTime = clock_.Now() - stalenessEntryTtl_.GetValue();
    serviceTypeStalenessMap_.PerformCleanup(expiredTime);
    partitionStalenessMap_.PerformCleanup(expiredTime);
}
