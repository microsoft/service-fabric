// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(NetworkRef)

StringLiteral const TraceSource("NetworkRef");

bool NetworkRef::operator==(NetworkRef const & other) const
{
    if (!(this->Name == other.Name))
    {
        return false;
    }

    return this->EndpointRefs == other.EndpointRefs;
}

ErrorCode NetworkRef::TryValidate(wstring const & traceId) const
{
    if (Name.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(NameNotSpecified), traceId));
    }

    return ErrorCodeValue::Success;
}

ErrorCode NetworkRef::FromPublicApi(
    NETWORK_REF const & publicNetworkRef)
{
    auto hr = StringUtility::LpcwstrToWstring(publicNetworkRef.Name, true /* acceptNull */, Name);
    if (FAILED(hr))
    {
        auto error = ErrorCode::FromHResult(hr);
        Trace.WriteInfo(TraceSource, "Error parsing NetworkRef name escription in FromPublicAPI: {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

void NetworkRef::ToPublicApi(
    __in ScopedHeap & heap,
    __out NETWORK_REF & publicNetworkRef) const
{
    publicNetworkRef.Name = heap.AddString(this->Name);
    publicNetworkRef.Reserved = NULL;
}
