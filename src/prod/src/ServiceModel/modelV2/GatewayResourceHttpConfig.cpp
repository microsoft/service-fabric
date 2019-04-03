//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(HttpConfig)

ErrorCode HttpConfig::TryValidate(wstring const &traceId) const
{
    ErrorCode error = ErrorCodeValue::Success;

    if (hostNames_.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(HttpHostsNotSpecified), traceId));
    }

    for (auto &host : hostNames_)
    {
        error = host.TryValidate(traceId);
        if (!error.IsSuccess()) { return error; }
    }

    if (listenPort_ == 0)
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(PortNotSpecified), traceId));
    }

    return error;
}

void HttpConfig::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0}",
        Name);
}
