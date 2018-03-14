// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(NodeHealthStateChunk)

StringLiteral const TraceSource("NodeHealthStateChunk");

NodeHealthStateChunk::NodeHealthStateChunk()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , nodeName_()
    , healthState_(FABRIC_HEALTH_STATE_INVALID)
{
}

NodeHealthStateChunk::NodeHealthStateChunk(
    std::wstring const & nodeName,
    FABRIC_HEALTH_STATE healthState)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , nodeName_(nodeName)
    , healthState_(healthState)
{
}

NodeHealthStateChunk::NodeHealthStateChunk(NodeHealthStateChunk && other)
    : Common::IFabricJsonSerializable(move(other))
    , Serialization::FabricSerializable(move(other))
    , nodeName_(move(other.nodeName_))
    , healthState_(move(other.healthState_))
{
}

NodeHealthStateChunk & NodeHealthStateChunk::operator =(NodeHealthStateChunk && other)
{
    if (this != &other)
    {
        nodeName_ = move(other.nodeName_);
        healthState_ = move(other.healthState_);
    }

    Common::IFabricJsonSerializable::operator=(move(other));
    Serialization::FabricSerializable::operator=(move(other));
    return *this;
}

NodeHealthStateChunk::~NodeHealthStateChunk()
{
}

ErrorCode NodeHealthStateChunk::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_NODE_HEALTH_STATE_CHUNK & publicNodeHealthStateChunk) const
{
    publicNodeHealthStateChunk.NodeName = heap.AddString(nodeName_);

    publicNodeHealthStateChunk.HealthState = healthState_;

    return ErrorCode::Success();
}

ErrorCode NodeHealthStateChunk::FromPublicApi(FABRIC_NODE_HEALTH_STATE_CHUNK const & publicNodeHealthStateChunk)
{
    auto hr = StringUtility::LpcwstrToWstring(publicNodeHealthStateChunk.NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteInfo(TraceSource, "Error parsing node name in FromPublicAPI: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    healthState_ = publicNodeHealthStateChunk.HealthState;

    return ErrorCode::Success();
}

void NodeHealthStateChunk::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("NodeHealthStateChunk({0} : {1})", nodeName_, healthState_);
}
