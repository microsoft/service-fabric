// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodeHealthStateFilter");

NodeHealthStateFilter::NodeHealthStateFilter()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(0)
    , nodeNameFilter_()
{
}

NodeHealthStateFilter::NodeHealthStateFilter(
    DWORD healthStateFilter)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(healthStateFilter)
    , nodeNameFilter_()
{
}

NodeHealthStateFilter::~NodeHealthStateFilter()
{
}

wstring NodeHealthStateFilter::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeHealthStateFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
         return wstring();
    }

    return objectString;
}

ErrorCode NodeHealthStateFilter::FromJsonString(wstring const & str, __inout NodeHealthStateFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

bool NodeHealthStateFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    bool returnAllOnDefault = (!nodeNameFilter_.empty());
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState, returnAllOnDefault);
}

ErrorCode NodeHealthStateFilter::FromPublicApi(FABRIC_NODE_HEALTH_STATE_FILTER const & publicNodeHealthStateFilter)
{
    healthStateFilter_ = publicNodeHealthStateFilter.HealthStateFilter;
    // NodeNameFilter
    {
        auto hr = StringUtility::LpcwstrToWstring(publicNodeHealthStateFilter.NodeNameFilter, true, 0, ParameterValidator::MaxStringSize, nodeNameFilter_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr, wformatString("{0} {1}, {2}, {3}.", GET_COMMON_RC(Invalid_LPCWSTR_Length), L"NodeNameFilter", 0, ParameterValidator::MaxStringSize));
            Trace.WriteInfo(TraceSource, "NodeNameFilter::FromPublicApi error: {0}", error);
            return error;
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode NodeHealthStateFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_NODE_HEALTH_STATE_FILTER & publicFilter) const
{
    publicFilter.HealthStateFilter = healthStateFilter_;
    publicFilter.NodeNameFilter = heap.AddString(nodeNameFilter_);
    return ErrorCode::Success();
}
