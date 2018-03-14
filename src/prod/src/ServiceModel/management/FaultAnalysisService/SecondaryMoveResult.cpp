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

StringLiteral const TraceComponent("SecondaryMoveResult");

SecondaryMoveResult::SecondaryMoveResult()
: currentSecondaryNodeName_(L"")
, newSecondaryNodeName_(L"")
, serviceName_()
, partitionId_()
{
}

SecondaryMoveResult::SecondaryMoveResult(SecondaryMoveResult && other)
: currentSecondaryNodeName_(move(other.currentSecondaryNodeName_))
, newSecondaryNodeName_(move(other.newSecondaryNodeName_))
, serviceName_(move(other.serviceName_))
, partitionId_(move(other.partitionId_))
{
}

ErrorCode SecondaryMoveResult::FromPublicApi(FABRIC_MOVE_SECONDARY_RESULT const & publicResult)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicResult.CurrentNodeName, true, currentSecondaryNodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "SecondaryMoveResult::FromPublicApi/Failed to parse NodeName");
        return ErrorCode::FromHResult(hr);
    }

    hr = StringUtility::LpcwstrToWstring(publicResult.NewNodeName, true, newSecondaryNodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "SecondaryMoveResult::FromPublicApi/Failed to parse NodeName");
        return ErrorCode::FromHResult(hr);
    }

    hr = NamingUri::TryParse(publicResult.ServiceName, L"TraceId", serviceName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "SecondaryMoveResult::FromPublicApi/Failed to parse ApplicationName");
        return ErrorCodeValue::InvalidNameUri;
    }

    partitionId_ = publicResult.PartitionId;

    return ErrorCodeValue::Success;
}

void SecondaryMoveResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_MOVE_SECONDARY_RESULT & publicResult) const
{
    publicResult.CurrentNodeName = heap.AddString(currentSecondaryNodeName_);

    publicResult.NewNodeName = heap.AddString(newSecondaryNodeName_);

    publicResult.ServiceName = heap.AddString(serviceName_.Name);

    publicResult.PartitionId = partitionId_;
}

