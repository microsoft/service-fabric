// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION( NodeAggregatedHealthState )

StringLiteral const TraceSource("NodeAggregatedHealthState");

NodeAggregatedHealthState::NodeAggregatedHealthState()
    : nodeName_()
    , nodeId_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

NodeAggregatedHealthState::NodeAggregatedHealthState(
    wstring const & nodeName,
    Federation::NodeId const & nodeId,
    FABRIC_HEALTH_STATE aggregatedHealthState)
    : nodeName_(nodeName)
    , nodeId_(nodeId)
    , aggregatedHealthState_(aggregatedHealthState)
{
}

NodeAggregatedHealthState::~NodeAggregatedHealthState()
{
}

Common::ErrorCode NodeAggregatedHealthState::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_NODE_HEALTH_STATE & publicNodeAggregatedHealthState) const 
{
    // Entity information
    
    publicNodeAggregatedHealthState.NodeName = heap.AddString(nodeName_);

    // Health state
    publicNodeAggregatedHealthState.AggregatedHealthState = aggregatedHealthState_;
    
    return ErrorCode::Success();
}

Common::ErrorCode NodeAggregatedHealthState::FromPublicApi(
    FABRIC_NODE_HEALTH_STATE const & publicNodeAggregatedHealthState)
{
    // Entity information
    auto hr = StringUtility::LpcwstrToWstring(publicNodeAggregatedHealthState.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    // Health state
    aggregatedHealthState_ = publicNodeAggregatedHealthState.AggregatedHealthState;

    return ErrorCode::Success();
}

void NodeAggregatedHealthState::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("NodeAggregatedHealthState({0} : {1})", nodeName_, aggregatedHealthState_);
}

std::wstring NodeAggregatedHealthState::ToString() const
{
    return wformatString(*this);
}

std::wstring NodeAggregatedHealthState::CreateContinuationToken() const
{
    return PagingStatus::CreateContinuationToken<Federation::NodeId>(nodeId_);
}
