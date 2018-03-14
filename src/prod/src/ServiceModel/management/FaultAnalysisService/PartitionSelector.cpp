// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("PartitionSelector");

PartitionSelector::PartitionSelector()
    : serviceName_()
    , partitionSelectorType_(PartitionSelectorType::Enum::None)
    , partitionKey_(L"")
{
}

PartitionSelector::PartitionSelector(Common::NamingUri && serviceName, PartitionSelectorType::Enum partitionSelectorType, std::wstring const & partitionKey)
    : serviceName_(move(serviceName))
    , partitionSelectorType_(partitionSelectorType)
    , partitionKey_(partitionKey)
{
}

PartitionSelector::PartitionSelector(PartitionSelector const & other)
    : serviceName_(other.serviceName_)
    , partitionSelectorType_(other.partitionSelectorType_)
    , partitionKey_(other.partitionKey_)
{
}

bool PartitionSelector::operator == (PartitionSelector const & other) const
{
    return (serviceName_ == other.ServiceName)
        && (partitionSelectorType_ == other.PartitionSelectorType)
        && (partitionKey_ == other.PartitionKey);
}

bool PartitionSelector::operator != (PartitionSelector const & other) const
{
    return !(*this == other);
}

PartitionSelector & PartitionSelector::operator = (PartitionSelector const & other)
{
    if (this != &other)
    {
        serviceName_ = other.serviceName_;
        partitionSelectorType_ = other.partitionSelectorType_;
        partitionKey_ = other.partitionKey_;
    }

    return *this;
}

PartitionSelector::PartitionSelector(PartitionSelector && other)
    : serviceName_(move(other.serviceName_))
    , partitionSelectorType_(move(other.partitionSelectorType_))
    , partitionKey_(move(other.partitionKey_))
{
}

PartitionSelector & PartitionSelector::operator = (PartitionSelector && other)
{
    if (this != &other)
    {
        serviceName_ = move(other.serviceName_);
        partitionSelectorType_ = move(other.partitionSelectorType_);
        partitionKey_ = move(other.partitionKey_);
    }

    return *this;
}

Common::ErrorCode PartitionSelector::FromPublicApi(
    FABRIC_PARTITION_SELECTOR const & publicPartitionSelector)
{
    wstring name(publicPartitionSelector.ServiceName);

    // ServiceName
    {
        auto hr = NamingUri::TryParse(publicPartitionSelector.ServiceName, L"TraceId", serviceName_);
        if (FAILED(hr))
        {
            Trace.WriteError(TraceComponent, "PartitionSelector::FromPublicApi/Failed to parse ServiceName: {0}", hr);
            return ErrorCodeValue::InvalidNameUri;
        }
    }

    switch (publicPartitionSelector.PartitionSelectorType)
    {
    case FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON:
        partitionSelectorType_ = PartitionSelectorType::Enum::Singleton;
        break;
    case FABRIC_PARTITION_SELECTOR_TYPE_NAMED:
        partitionSelectorType_ = PartitionSelectorType::Enum::Named;
        break;
    case FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64:
        partitionSelectorType_ = PartitionSelectorType::Enum::UniformInt64;
        break;
    case FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID:
        partitionSelectorType_ = PartitionSelectorType::Enum::PartitionId;
        break;
    case FABRIC_PARTITION_SELECTOR_TYPE_RANDOM:
        partitionSelectorType_ = PartitionSelectorType::Enum::Random;
        break;
    }

    HRESULT hr = StringUtility::LpcwstrToWstring(publicPartitionSelector.PartitionKey, true, partitionKey_);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "PartitionSelector::FromPublicApi/Failed to parse PartitionKey: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }
    
    return ErrorCode::Success();
}

void PartitionSelector::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_PARTITION_SELECTOR & result) const
{    
    result.ServiceName = heap.AddString(serviceName_.Name); 
    switch (partitionSelectorType_)
    {
    case PartitionSelectorType::Enum::Named:
        result.PartitionSelectorType = FABRIC_PARTITION_SELECTOR_TYPE_NAMED;
        break;
    case PartitionSelectorType::Enum::Singleton:
        result.PartitionSelectorType = FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON;
        break;
    case PartitionSelectorType::Enum::UniformInt64:
        result.PartitionSelectorType = FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64;
        break;
    case PartitionSelectorType::Enum::PartitionId:
        result.PartitionSelectorType = FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID;
        break;
    case PartitionSelectorType::Enum::Random:
        result.PartitionSelectorType = FABRIC_PARTITION_SELECTOR_TYPE_RANDOM;
        break;
    }
    
    result.PartitionKey = heap.AddString(partitionKey_);    
}

void PartitionSelector::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "[ " << serviceName_ << ", " << partitionSelectorType_ << ", " << partitionKey_ << " ]";
}
