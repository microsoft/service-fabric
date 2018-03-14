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

StringLiteral const TraceComponent("MoveSecondaryDescriptionUsingNodeName");

MoveSecondaryDescriptionUsingNodeName::MoveSecondaryDescriptionUsingNodeName()
: currentSecondaryNodeName_(L"")
, newSecondaryNodeName_(L"")
, serviceName_()
, partitionId_()
, ignoreConstraints_(false)
{
}

MoveSecondaryDescriptionUsingNodeName::MoveSecondaryDescriptionUsingNodeName(MoveSecondaryDescriptionUsingNodeName const & other)
: currentSecondaryNodeName_(other.currentSecondaryNodeName_)
, newSecondaryNodeName_(other.newSecondaryNodeName_)
, serviceName_(other.serviceName_)
, partitionId_(other.partitionId_)
, ignoreConstraints_(other.ignoreConstraints_)
{
}

MoveSecondaryDescriptionUsingNodeName::MoveSecondaryDescriptionUsingNodeName(MoveSecondaryDescriptionUsingNodeName && other)
: currentSecondaryNodeName_(move(other.currentSecondaryNodeName_))
, newSecondaryNodeName_(move(other.newSecondaryNodeName_))
, serviceName_(move(other.serviceName_))
, partitionId_(move(other.partitionId_))
, ignoreConstraints_(move(other.ignoreConstraints_))
{
}

Common::ErrorCode MoveSecondaryDescriptionUsingNodeName::FromPublicApi(
    FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME const & publicDescription)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.CurrentNodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, currentSecondaryNodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "Failed parsing current secondary nodename");
        return ErrorCodeValue::InvalidNameUri;
    }

    hr = StringUtility::LpcwstrToWstring(publicDescription.NewNodeName, true, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, newSecondaryNodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "Failed parsing new secondary nodename");
        return ErrorCodeValue::InvalidNameUri;
    }

    // ServiceName
    {
        hr = NamingUri::TryParse(publicDescription.ServiceName, L"TraceId", serviceName_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "MoveSecondaryDescriptionUsingNodeName::FromPublicApi/Failed to parse ServiceName");
            return ErrorCodeValue::InvalidNameUri;
        }
    }

    partitionId_ = publicDescription.PartitionId;

    ignoreConstraints_ = publicDescription.IgnoreConstraints ? true : false;

    return ErrorCodeValue::Success;
}

void MoveSecondaryDescriptionUsingNodeName::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME & result) const
{
    result.CurrentNodeName = heap.AddString(currentSecondaryNodeName_);

    result.NewNodeName = heap.AddString(newSecondaryNodeName_);

    result.ServiceName = heap.AddString(serviceName_.Name);

    result.PartitionId = partitionId_;

    result.IgnoreConstraints = ignoreConstraints_ ? TRUE : FALSE;
}
