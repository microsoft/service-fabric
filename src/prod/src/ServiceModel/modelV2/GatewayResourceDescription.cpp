//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(GatewayProperties)
INITIALIZE_SIZE_ESTIMATION(GatewayResourceDescription)

StringLiteral const TraceSource("GatewayResourceDescription");

ErrorCode GatewayProperties::TryValidate(wstring const &traceId) const
{
    unordered_set<uint> ports;
    ErrorCode error = sourceNetwork_.TryValidate(traceId);
    if (!error.IsSuccess()) { return error; }

    error = destinationNetwork_.TryValidate(traceId);
    if (!error.IsSuccess()) { return error; }

    for (auto &config : tcp_)
    {
        error = config.TryValidate(traceId);
        if (!error.IsSuccess()) { return error; }

        auto result = ports.insert(config.ListenPort);
        if (!result.second)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(PortNotUnique), traceId, config.ListenPort));
        }
    }

    for (auto &config : http_)
    {
        error = config.TryValidate(traceId);
        if (!error.IsSuccess()) { return error; }

        auto result = ports.insert(config.ListenPort);
        if (!result.second)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(PortNotUnique), traceId, config.ListenPort));
        }
    }

    return ErrorCode::Success();
}

ErrorCode GatewayResourceDescription::TryValidate(wstring const &traceId) const
{
    ErrorCode error = __super::TryValidate(traceId);
    if (!error.IsSuccess()) { return error; }

    return this->Properties.TryValidate(GetNextTraceId(traceId, Name));
}

void GatewayResourceDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        Name);
}

wstring GatewayResourceDescription::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<GatewayResourceDescription&>(*this), objectString, static_cast<JsonSerializerFlags>(JsonSerializerFlags::DateTimeInIsoFormat | JsonSerializerFlags::EnumInStringFormat));
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error serializing GatewayResourcedescription: {0}", error);
        return wstring();
    }

    return objectString;
}

ErrorCode GatewayResourceDescription::FromString(
    wstring const & descriptionStr,
    __out GatewayResourceDescription & description)
{
    return JsonHelper::Deserialize(description, descriptionStr, static_cast<JsonSerializerFlags>(JsonSerializerFlags::DateTimeInIsoFormat | JsonSerializerFlags::EnumInStringFormat));
}
