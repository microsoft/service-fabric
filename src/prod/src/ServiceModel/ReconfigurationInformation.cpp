// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ReconfigurationInformation::ReconfigurationInformation()
    : previousConfigurationRole_(FABRIC_REPLICA_ROLE_NONE)
    , reconfigurationPhase_(FABRIC_RECONFIGURATION_PHASE_INVALID)
    , reconfigurationType_(FABRIC_RECONFIGURATION_TYPE_INVALID)
    , reconfigurationStartTimeUtc_(DateTime::Zero)
{
}

ReconfigurationInformation::ReconfigurationInformation(
    FABRIC_REPLICA_ROLE previousConfigurationRole,
    FABRIC_RECONFIGURATION_PHASE reconfigurationPhase,
    FABRIC_RECONFIGURATION_TYPE reconfigurationType,
    DateTime reconfigurationStartTime)
    : previousConfigurationRole_(previousConfigurationRole)
    , reconfigurationPhase_(reconfigurationPhase)
    , reconfigurationType_(reconfigurationType)
    , reconfigurationStartTimeUtc_(reconfigurationStartTime)
{
}

void ReconfigurationInformation::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_RECONFIGURATION_INFORMATION_QUERY_RESULT & reconfigurationInformationQueryResult) const
{
    UNREFERENCED_PARAMETER(heap);
    reconfigurationInformationQueryResult.PreviousConfigurationRole = previousConfigurationRole_;
    reconfigurationInformationQueryResult.ReconfigurationPhase = reconfigurationPhase_;
    reconfigurationInformationQueryResult.ReconfigurationType = reconfigurationType_;
    reconfigurationInformationQueryResult.ReconfigurationStartTimeUtc = reconfigurationStartTimeUtc_.AsFileTime;
}

void ReconfigurationInformation::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring ReconfigurationInformation::ToString() const
{
    return wformatString("PreviousConfigurationRole = {0}, ReconfigurationPhase = {1}, ReconfigurationType = {2}, ReconfigurationStartTimeUtc = {3}",
        previousConfigurationRole_,
        reconfigurationPhase_,
        reconfigurationType_,
        reconfigurationStartTimeUtc_);
}
