// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodeEntity");

INITIALIZE_SIZE_ESTIMATION(NodeEntityHealthInformation)

NodeEntityHealthInformation::NodeEntityHealthInformation()
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_NODE)
    , nodeId_()
    , nodeName_()
    , nodeInstanceId_(FABRIC_INVALID_NODE_INSTANCE_ID)
{
}

NodeEntityHealthInformation::NodeEntityHealthInformation(
    Common::LargeInteger const& nodeId,
    wstring const& nodeName,
    FABRIC_NODE_INSTANCE_ID nodeInstanceId)
    : EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND_NODE)
    , nodeId_(nodeId)
    , nodeName_(nodeName)
    , nodeInstanceId_(nodeInstanceId)
{
}

ErrorCode NodeEntityHealthInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in FABRIC_HEALTH_INFORMATION * healthInformation,
    __out FABRIC_HEALTH_REPORT & healthReport) const
{
    auto nodeHealthInformation = heap.AddItem<FABRIC_NODE_HEALTH_REPORT>();
    nodeHealthInformation->NodeName = heap.AddString(nodeName_);
    nodeHealthInformation->HealthInformation = healthInformation;

    healthReport.Kind = kind_;
    healthReport.Value = nodeHealthInformation.GetRawPointer();
    return ErrorCode::Success();
}

ErrorCode NodeEntityHealthInformation::FromPublicApi(
    FABRIC_HEALTH_REPORT const & healthReport,
    __inout HealthInformation & commonHealthInformation,
    __out AttributeList & attributes)
{
    kind_ = healthReport.Kind;

    auto nodeHealthInformation = reinterpret_cast<FABRIC_NODE_HEALTH_REPORT *>(healthReport.Value);
    auto hr = StringUtility::LpcwstrToWstring(nodeHealthInformation->NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
    if (!SUCCEEDED(hr))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    auto error = GenerateNodeId();
    if (!error.IsSuccess())
    {
        return error;
    }
    
    attributes.AddAttribute(*HealthAttributeNames::NodeName, nodeName_);

    return commonHealthInformation.FromCommonPublicApi(*nodeHealthInformation->HealthInformation);
}

Common::ErrorCode NodeEntityHealthInformation::GenerateNodeId()
{
    Federation::NodeId nodeId;
    ErrorCode error = Federation::NodeIdGenerator::GenerateFromString(nodeName_, nodeId);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error generating NodeId from NodeName {0}: {1}", nodeName_, error);
        return error;
    }

    if (nodeId != nodeId_)
    {
        Trace.WriteInfo(TraceSource, "Generate NodeId from NodeName {0}: {1} (previous {2})", nodeName_, nodeId, nodeId_);
        nodeId_ = nodeId.IdValue;
        entityId_.clear();
    }

    return ErrorCode::Success();
}

wstring const& NodeEntityHealthInformation::get_EntityId() const
{
    if (entityId_.empty())
    {
        entityId_ = nodeId_.ToString();
    }

    return entityId_;
}
