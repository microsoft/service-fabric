// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServiceAggregatedHealthState)

StringLiteral const TraceSource("ServiceAggregatedHealthState");

ServiceAggregatedHealthState::ServiceAggregatedHealthState()
    : serviceName_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

ServiceAggregatedHealthState::ServiceAggregatedHealthState(
    std::wstring const & serviceName,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : serviceName_(serviceName)
    , aggregatedHealthState_(aggregatedHealthState)
{
}

ServiceAggregatedHealthState::ServiceAggregatedHealthState(
    std::wstring && serviceName,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : serviceName_(move(serviceName))
    , aggregatedHealthState_(aggregatedHealthState)
{
}

ServiceAggregatedHealthState::~ServiceAggregatedHealthState()
{
}

Common::ErrorCode ServiceAggregatedHealthState::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SERVICE_HEALTH_STATE & publicServiceAggregatedHealthState) const
{
    // Entity information
    publicServiceAggregatedHealthState.ServiceName = heap.AddString(serviceName_);

    // Health state
    publicServiceAggregatedHealthState.AggregatedHealthState = aggregatedHealthState_;

    return ErrorCode::Success();
}

Common::ErrorCode ServiceAggregatedHealthState::FromPublicApi(
    FABRIC_SERVICE_HEALTH_STATE const & publicServiceAggregatedHealthState)
{
    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicServiceAggregatedHealthState.ServiceName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, serviceName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing service name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    // Health state
    aggregatedHealthState_ = publicServiceAggregatedHealthState.AggregatedHealthState;

    return ErrorCode::Success();
}

void ServiceAggregatedHealthState::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("ServiceAggregatedHealthState({0}: {1})", serviceName_, aggregatedHealthState_);
}

std::wstring ServiceAggregatedHealthState::ToString() const
{
    return wformatString(*this);
}

std::wstring ServiceAggregatedHealthState::CreateContinuationToken() const
{
    return serviceName_;
}
