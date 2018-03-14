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

StringLiteral const TraceComponent("PrimaryMoveResult");

PrimaryMoveResult::PrimaryMoveResult()
: nodeName_(L"")
, serviceName_()
, partitionId_()
{
}

PrimaryMoveResult::PrimaryMoveResult(PrimaryMoveResult && other)
: nodeName_(move(other.nodeName_))
, serviceName_(move(other.serviceName_))
, partitionId_(move(other.partitionId_))
{
}

ErrorCode PrimaryMoveResult::FromPublicApi(FABRIC_MOVE_PRIMARY_RESULT const & publicResult)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(publicResult.NodeName, true, nodeName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "PrimaryMoveResult::FromPublicApi/Failed to parse NodeName");
        return ErrorCode::FromHResult(hr);
    }

    hr = NamingUri::TryParse(publicResult.ServiceName, L"TraceId", serviceName_);
    if (FAILED(hr))
    {
        Trace.WriteWarning(TraceComponent, "PrimaryMoveResult::FromPublicApi/Failed to parse ApplicationName");
        return ErrorCodeValue::InvalidNameUri;
    }

    partitionId_ = publicResult.PartitionId;

    return ErrorCodeValue::Success;
}

void PrimaryMoveResult::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_MOVE_PRIMARY_RESULT & publicResult) const
{
    publicResult.NodeName = heap.AddString(nodeName_);

    publicResult.ServiceName = heap.AddString(serviceName_.Name);

    publicResult.PartitionId = partitionId_;
}

