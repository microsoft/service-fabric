//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(GatewayDestination)

StringLiteral const TraceSource("GatewayDestination");

ErrorCode GatewayDestination::TryValidate(wstring const &traceId) const
{
    if (applicationName_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ApplicationNameNotSpecified), traceId));
    }

    if (serviceName_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(ServiceNameNotSpecified), traceId));
    }

    return ErrorCode::Success();
}

ErrorCode GatewayDestination::FromPublicApi(
    TRAFFIC_ROUTING_DESTINATION const & publicRoutingDestination)
{
    ErrorCode error = ErrorCode::Success();
    auto hr = StringUtility::LpcwstrToWstring(publicRoutingDestination.ApplicationName, true /* acceptNull */, applicationName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing GatewayDestination ApplicationName in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicRoutingDestination.ServiceName, true /* acceptNull */, serviceName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing GatewayDestination ServiceName in FromPublicAPI: {0}", error);
        return error;
    }

    hr = StringUtility::LpcwstrToWstring(publicRoutingDestination.EndpointName, true /* acceptNull */, endpointName_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing GatewayDestination EndpointName in FromPublicAPI: {0}", error);
        return error;
    }

    return ErrorCode::Success();
}

void GatewayDestination::ToPublicApi(
    __in ScopedHeap & heap,
    __out TRAFFIC_ROUTING_DESTINATION & publicRoutingDestination) const
{
    publicRoutingDestination.ApplicationName = heap.AddString(applicationName_);
    publicRoutingDestination.ServiceName = heap.AddString(serviceName_);
    publicRoutingDestination.EndpointName = heap.AddString(endpointName_);
}
