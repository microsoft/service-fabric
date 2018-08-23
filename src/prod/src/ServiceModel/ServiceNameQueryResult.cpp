// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceNameQueryResult::ServiceNameQueryResult()
    : serviceName_()
{
}

ServiceNameQueryResult::ServiceNameQueryResult(Uri const & serviceName)
    : serviceName_(serviceName)
{
}

void ServiceNameQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_SERVICE_NAME_QUERY_RESULT & publicQueryResult) const 
{
    publicQueryResult.ServiceName = heap.AddString(serviceName_.ToString());
}

ErrorCode ServiceNameQueryResult::FromPublicApi(__in FABRIC_SERVICE_NAME_QUERY_RESULT const& publicQueryResult)
{
    if (!Uri::TryParse(publicQueryResult.ServiceName, serviceName_))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode::Success();
}

void ServiceNameQueryResult::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ServiceNameQueryResult::ToString() const
{
    return wformatString("ServiceName = '{0}'", serviceName_.ToString()); 
}
