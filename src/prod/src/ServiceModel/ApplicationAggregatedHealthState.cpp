// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ApplicationAggregatedHealthState)

StringLiteral const TraceSource("ApplicationAggregatedHealthState");

ApplicationAggregatedHealthState::ApplicationAggregatedHealthState()
    : applicationName_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

ApplicationAggregatedHealthState::ApplicationAggregatedHealthState(
    std::wstring const & applicationName,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : applicationName_(applicationName)
    , aggregatedHealthState_(aggregatedHealthState)
{
}

ApplicationAggregatedHealthState::~ApplicationAggregatedHealthState()
{
}

Common::ErrorCode ApplicationAggregatedHealthState::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_APPLICATION_HEALTH_STATE & publicApplicationAggregatedHealthState) const
{
    // Entity information
    publicApplicationAggregatedHealthState.ApplicationName = heap.AddString(applicationName_);

    // Health state
    publicApplicationAggregatedHealthState.AggregatedHealthState = aggregatedHealthState_;

    return ErrorCode::Success();
}

Common::ErrorCode ApplicationAggregatedHealthState::FromPublicApi(
    FABRIC_APPLICATION_HEALTH_STATE const & publicApplicationAggregatedHealthState)
{
    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicApplicationAggregatedHealthState.ApplicationName, true /* acceptNull */, applicationName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing application name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    // Health state
    aggregatedHealthState_ = publicApplicationAggregatedHealthState.AggregatedHealthState;

    return ErrorCode::Success();
}

void ApplicationAggregatedHealthState::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("ApplicationAggregatedHealthState({0}: {1})", applicationName_, aggregatedHealthState_);
}

std::wstring ApplicationAggregatedHealthState::ToString() const
{
    return wformatString(*this);
}

std::wstring ApplicationAggregatedHealthState::CreateContinuationToken() const
{
    return applicationName_;
}
