// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationAggregatedHealthState)

StringLiteral const TraceSource("DeployedApplicationAggregatedHealthState");

DeployedApplicationAggregatedHealthState::DeployedApplicationAggregatedHealthState()
    : applicationName_()
    , nodeName_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

DeployedApplicationAggregatedHealthState::DeployedApplicationAggregatedHealthState(
    std::wstring const & applicationName,
    std::wstring const & nodeName,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : applicationName_(applicationName)
    , nodeName_(nodeName)
    , aggregatedHealthState_(aggregatedHealthState)
{
}

DeployedApplicationAggregatedHealthState::~DeployedApplicationAggregatedHealthState()
{
}

Common::ErrorCode DeployedApplicationAggregatedHealthState::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE & publicDeployedApplicationAggregatedHealthState) const
{
    // Entity information
    publicDeployedApplicationAggregatedHealthState.ApplicationName = heap.AddString(applicationName_);
    publicDeployedApplicationAggregatedHealthState.NodeName = heap.AddString(nodeName_);

    // Health state
    publicDeployedApplicationAggregatedHealthState.AggregatedHealthState = aggregatedHealthState_;

    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationAggregatedHealthState::FromPublicApi(
    FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE const & publicDeployedApplicationAggregatedHealthState)
{
    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationAggregatedHealthState.ApplicationName, false, applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationAggregatedHealthState.NodeName, false, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    // Health state
    aggregatedHealthState_ = publicDeployedApplicationAggregatedHealthState.AggregatedHealthState;

    return ErrorCode::Success();
}

void DeployedApplicationAggregatedHealthState::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("DeployedApplicationAggregatedHealthState({0}+{1}: {2})", applicationName_, nodeName_, aggregatedHealthState_);
}

std::wstring DeployedApplicationAggregatedHealthState::ToString() const
{
    return wformatString(*this);
}

std::wstring DeployedApplicationAggregatedHealthState::CreateContinuationToken() const
{
    return nodeName_;
}
