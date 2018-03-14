// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServicePartitionInformation)

ServicePartitionInformation::ServicePartitionInformation()
    : partitionKind_(FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
    , partitionId_()
    , partitionName_()
    , lowKey_(0)
    , highKey_(0)
{
}

ServicePartitionInformation::ServicePartitionInformation(Common::Guid const & partitionId)
    : partitionKind_(FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
    , partitionId_(partitionId)
    , partitionName_()
    , lowKey_(0)
    , highKey_(0)
{
}

ServicePartitionInformation::ServicePartitionInformation(Common::Guid const & partitionId, std::wstring const & partitionName)
    : partitionKind_(FABRIC_SERVICE_PARTITION_KIND_NAMED)
    , partitionId_(partitionId)
    , partitionName_(partitionName)
    , lowKey_(0)
    , highKey_(0)
{
}

ServicePartitionInformation::ServicePartitionInformation(Common::Guid const & partitionId, int64 lowKey, int64 highKey)
    : partitionKind_(FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
    , partitionId_(partitionId)
    , partitionName_()
    , lowKey_(lowKey)
    , highKey_(highKey)
{
}

ServicePartitionInformation::ServicePartitionInformation(ServicePartitionInformation && other)
    : partitionKind_(move(other.partitionKind_))
    , partitionId_(move(other.partitionId_))
    , partitionName_(move(other.partitionName_))
    , lowKey_(move(other.lowKey_))
    , highKey_(move(other.highKey_))
{
}

ServicePartitionInformation & ServicePartitionInformation::operator=(ServicePartitionInformation && other)
{
    if (this != &other)
    {
        partitionKind_ = move(other.partitionKind_);
        partitionId_ = move(other.partitionId_);
        partitionName_ = move(other.partitionName_);
        lowKey_ = move(other.lowKey_);
        highKey_ = move(other.highKey_);
    }

    return *this;
}

bool ServicePartitionInformation::AreEqualIgnorePartitionId(ServicePartitionInformation const & other) const
{
    bool equals = (partitionKind_ == other.partitionKind_);
    if (!equals) return equals;

    equals = (partitionName_ == other.partitionName_);
    if (!equals) return equals;

    equals = (lowKey_ == other.lowKey_);
    if (!equals) return equals;

    equals = (highKey_ == other.highKey_);
    if (!equals) return equals;

    return equals;
}

void ServicePartitionInformation::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_SERVICE_PARTITION_INFORMATION & servicePartitionInformation) const
{
    servicePartitionInformation.Kind = partitionKind_;

    if (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_INVALID) { return; }

    if (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
    {
        auto singletonPartition = heap.AddItem<FABRIC_SINGLETON_PARTITION_INFORMATION>();
        singletonPartition->Id = partitionId_.AsGUID();
        servicePartitionInformation.Value = singletonPartition.GetRawPointer();
    }
    else if(partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_NAMED)
    {
        auto namedPartition = heap.AddItem<FABRIC_NAMED_PARTITION_INFORMATION>();
        namedPartition->Id = partitionId_.AsGUID();
        namedPartition->Name = heap.AddString(partitionName_);
        servicePartitionInformation.Value = namedPartition.GetRawPointer();
    }
    else if(partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
    {
        auto int64RangePartition = heap.AddItem<FABRIC_INT64_RANGE_PARTITION_INFORMATION>();
        int64RangePartition->Id = partitionId_.AsGUID();
        int64RangePartition->LowKey = lowKey_;
        int64RangePartition->HighKey = highKey_;
        servicePartitionInformation.Value = int64RangePartition.GetRawPointer();
    }
}

ErrorCode ServicePartitionInformation::FromPublicApi(__in FABRIC_SERVICE_PARTITION_INFORMATION const& servicePartitionInformation)
{  
    if (servicePartitionInformation.Kind == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
    {
        FABRIC_SINGLETON_PARTITION_INFORMATION *partition = 
            reinterpret_cast<FABRIC_SINGLETON_PARTITION_INFORMATION *>(servicePartitionInformation.Value);

        partitionKind_ = FABRIC_SERVICE_PARTITION_KIND_SINGLETON;
        partitionId_ = Guid(partition->Id);
    }
    else if (servicePartitionInformation.Kind == FABRIC_SERVICE_PARTITION_KIND_NAMED)
    {
        FABRIC_NAMED_PARTITION_INFORMATION *partition = 
            reinterpret_cast<FABRIC_NAMED_PARTITION_INFORMATION *>(servicePartitionInformation.Value);

        partitionKind_ = FABRIC_SERVICE_PARTITION_KIND_NAMED;
        partitionId_ = Guid(partition->Id);
        partitionName_ = partition->Name;
    }
    else if (servicePartitionInformation.Kind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
    {
        FABRIC_INT64_RANGE_PARTITION_INFORMATION *partition = 
            reinterpret_cast<FABRIC_INT64_RANGE_PARTITION_INFORMATION *>(servicePartitionInformation.Value);

        partitionKind_ = FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE;
        partitionId_ = Guid(partition->Id);
        lowKey_ = partition->LowKey;
        highKey_ = partition->HighKey;
    }
    else
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode::Success();
}

void ServicePartitionInformation::FromPartitionInfo(Guid const &partitionId, Naming::PartitionInfo const &partitionInfo)
{
    partitionId_ = partitionId;
    if (partitionInfo.PartitionScheme == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
    {
        partitionKind_ = FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE;
        lowKey_ = partitionInfo.LowKey;
        highKey_ = partitionInfo.HighKey;
    }
    else if (partitionInfo.PartitionScheme == FABRIC_PARTITION_SCHEME_NAMED)
    {
        partitionKind_ = FABRIC_SERVICE_PARTITION_KIND_NAMED;
        partitionName_ = partitionInfo.PartitionName;
    }
    else
    {
        partitionKind_ = FABRIC_SERVICE_PARTITION_KIND_SINGLETON;
    }
}

void ServicePartitionInformation::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());    
}

std::wstring ServicePartitionInformation::ToString() const
{
    wstring partitionKind = L"Invalid";
    wstring partitionInformation = L"";

    if (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_SINGLETON)
    {
        partitionKind = L"Singleton";
        partitionInformation = wformatString("ID = '{0}'", partitionId_);
    }
    else if (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_NAMED)
    {
        partitionKind = L"Named";
        partitionInformation = wformatString("ID = '{0}', Name = '{1}'", partitionId_, partitionName_);
    }
    else if (partitionKind_ == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
    {
        partitionKind = L"Int64 Range";
        partitionInformation = wformatString("ID = '{0}', LowKey = '{1}', HighKey = {2}", partitionId_, lowKey_, highKey_);
    }

    return wformatString("Kind = '{0}', {1}", partitionKind, partitionInformation);
}
