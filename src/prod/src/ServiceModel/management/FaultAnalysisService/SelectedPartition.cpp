// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("SelectedPartition");

SelectedPartition::SelectedPartition()
: serviceName_()
, partitionId_()
{
}


SelectedPartition::SelectedPartition(SelectedPartition const & other)
: serviceName_(other.serviceName_)
, partitionId_(other.partitionId_)
{
}

bool SelectedPartition::operator == (SelectedPartition const & other) const
{
    return (serviceName_ == other.ServiceName)
        && (partitionId_ == other.PartitionId);
}

bool SelectedPartition::operator != (SelectedPartition const & other) const
{
    return !(*this == other);
}

SelectedPartition & SelectedPartition::operator = (SelectedPartition const & other)
{
    if (this != &other)
    {
        serviceName_ = other.serviceName_;
        partitionId_ = other.partitionId_;
    }

    return *this;
}

SelectedPartition::SelectedPartition(SelectedPartition && other)
: serviceName_(move(other.serviceName_))
, partitionId_(move(other.partitionId_))
{
}

SelectedPartition & SelectedPartition::operator = (SelectedPartition && other)
{
    if (this != &other)
    {
        serviceName_ = move(other.serviceName_);
        partitionId_ = move(other.partitionId_);
    }

    return *this;
}

Common::ErrorCode SelectedPartition::FromPublicApi(
    FABRIC_SELECTED_PARTITION const & publicSelectedPartition)
{
    FABRIC_PARTITION_ID publicPartitionId = publicSelectedPartition.PartitionId;

    partitionId_ = Guid(publicPartitionId);

    wstring name(publicSelectedPartition.ServiceName);

    // ServiceName
    {
        auto hr = NamingUri::TryParse(publicSelectedPartition.ServiceName, L"TraceId", serviceName_);
        if (FAILED(hr))
        {
            return ErrorCodeValue::InvalidNameUri;
        }
    }

    return ErrorCode::Success();
}

void SelectedPartition::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SELECTED_PARTITION & result) const
{
    result.ServiceName = heap.AddString(serviceName_.Name);
    result.PartitionId = partitionId_.AsGUID();
}
