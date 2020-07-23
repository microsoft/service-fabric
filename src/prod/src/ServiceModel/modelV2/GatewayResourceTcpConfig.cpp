//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(TcpConfig)

StringLiteral const TraceSource("TcpConfig");

ErrorCode TcpConfig::TryValidate(wstring const &traceId) const
{
    if (name_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }

    if (listenPort_ == 0)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(PortNotSpecified), traceId));
    }

    return this->destination_.TryValidate(GetNextTraceId(traceId, Name));
}

void TcpConfig::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        Name);
}

ErrorCode TcpConfig::FromPublicApi(FABRIC_GATEWAY_RESOURCE_TCP_CONFIG const & publicTcpConfig)
{
    ErrorCode error = ErrorCode::Success();
    auto hr = StringUtility::LpcwstrToWstring(publicTcpConfig.Name, true /* acceptNull */, name_);
    if (FAILED(hr))
    {
        error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing tcp config name in FromPublicAPI: {0}", error);
        return error;
    }

    listenPort_ = publicTcpConfig.ListenPort;

    return destination_.FromPublicApi(publicTcpConfig.Destination);
}

Common::ErrorCode TcpConfig::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_GATEWAY_RESOURCE_TCP_CONFIG & publicTcpConfig) const
{
    publicTcpConfig.Name = heap.AddString(name_);
    publicTcpConfig.ListenPort = listenPort_;
    destination_.ToPublicApi(heap, publicTcpConfig.Destination);
    return ErrorCode::Success();
}

