// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("MovePrimaryDescriptionUsingNodeName");

MovePrimaryDescriptionUsingNodeName::MovePrimaryDescriptionUsingNodeName()
: newPrimaryNodeName_(L"")
, serviceName_()
, partitionId_()
, ignoreConstraints_(false)
{
}

MovePrimaryDescriptionUsingNodeName::MovePrimaryDescriptionUsingNodeName(MovePrimaryDescriptionUsingNodeName const & other)
: newPrimaryNodeName_(other.newPrimaryNodeName_)
, serviceName_(other.serviceName_)
, partitionId_(other.partitionId_)
, ignoreConstraints_(other.ignoreConstraints_)
{
}

MovePrimaryDescriptionUsingNodeName::MovePrimaryDescriptionUsingNodeName(MovePrimaryDescriptionUsingNodeName && other)
: newPrimaryNodeName_(move(other.newPrimaryNodeName_))
, serviceName_(move(other.serviceName_))
, partitionId_(move(other.partitionId_))
, ignoreConstraints_(move(other.ignoreConstraints_))
{
}

Common::ErrorCode MovePrimaryDescriptionUsingNodeName::FromPublicApi(
    FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME const & publicDescription)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.NodeName, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, newPrimaryNodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "Failed parsing nodename");
        return ErrorCodeValue::InvalidNameUri;
    }

    // ServiceName
    {
        hr = NamingUri::TryParse(publicDescription.ServiceName, L"TraceId", serviceName_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "MovePrimaryDescriptionUsingNodeName::FromPublicApi/Failed to parse ServiceName");
            return ErrorCodeValue::InvalidNameUri;
        }
    }

    partitionId_ = publicDescription.PartitionId;

    ignoreConstraints_ = publicDescription.IgnoreConstraints ? true : false;

    return ErrorCodeValue::Success;
}

void MovePrimaryDescriptionUsingNodeName::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME & result) const
{
    result.NodeName = heap.AddString(newPrimaryNodeName_);

    result.ServiceName = heap.AddString(serviceName_.Name);

    result.PartitionId = partitionId_;

    result.IgnoreConstraints = ignoreConstraints_ ? TRUE : FALSE;
}
