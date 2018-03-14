// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

PartitionLoadInformationQueryResult::PartitionLoadInformationQueryResult()
{
}

PartitionLoadInformationQueryResult::PartitionLoadInformationQueryResult(
    Common::Guid partitionId,
    std::vector<ServiceModel::LoadMetricReport> && primaryLoadMetricReports,
    std::vector<ServiceModel::LoadMetricReport> && secondaryLoadMetricReports)
    : partitionId_(partitionId)
    , primaryLoadMetricReports_(move(primaryLoadMetricReports))
    , secondaryLoadMetricReports_(move(secondaryLoadMetricReports))
{
}

PartitionLoadInformationQueryResult::PartitionLoadInformationQueryResult(PartitionLoadInformationQueryResult const & other) :
    partitionId_(other.partitionId_),
    primaryLoadMetricReports_(other.primaryLoadMetricReports_),
    secondaryLoadMetricReports_(other.secondaryLoadMetricReports_)
{
}

PartitionLoadInformationQueryResult::PartitionLoadInformationQueryResult(PartitionLoadInformationQueryResult && other) :
    partitionId_(move(other.partitionId_)),
    primaryLoadMetricReports_(move(other.primaryLoadMetricReports_)),
    secondaryLoadMetricReports_(move(other.secondaryLoadMetricReports_))
{
}

PartitionLoadInformationQueryResult const & PartitionLoadInformationQueryResult::operator = (PartitionLoadInformationQueryResult const & other)
{
    if (&other != this)
    {
        partitionId_ = other.partitionId_;
        primaryLoadMetricReports_ = other.primaryLoadMetricReports_;
        secondaryLoadMetricReports_ = other.secondaryLoadMetricReports_;
    }

    return *this;
}

PartitionLoadInformationQueryResult const & PartitionLoadInformationQueryResult::operator = (PartitionLoadInformationQueryResult && other)
{
    if (&other != this)
    {
        partitionId_ = move(other.partitionId_);
        primaryLoadMetricReports_ = move(other.primaryLoadMetricReports_);
        secondaryLoadMetricReports_ = move(other.secondaryLoadMetricReports_);
    }

    return *this;
}

void PartitionLoadInformationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_PARTITION_LOAD_INFORMATION & queryResult) const 
{
    queryResult.PartitionId = partitionId_.AsGUID();

    auto primaryLoadsList = heap.AddItem<::FABRIC_LOAD_METRIC_REPORT_LIST>();
    primaryLoadsList->Count = static_cast<ULONG>(primaryLoadMetricReports_.size());

    auto primaryLoadsArray = heap.AddArray<FABRIC_LOAD_METRIC_REPORT>(primaryLoadMetricReports_.size());
    int i = 0;
    for (auto iter = primaryLoadMetricReports_.begin(); iter != primaryLoadMetricReports_.end(); ++iter)
    {
        iter->ToPublicApi(heap, primaryLoadsArray[i]);
        i++;
    }
    primaryLoadsList->Items = primaryLoadsArray.GetRawArray();

    auto secondaryLoadsList = heap.AddItem<::FABRIC_LOAD_METRIC_REPORT_LIST>();
    secondaryLoadsList->Count = static_cast<ULONG>(secondaryLoadMetricReports_.size());

    auto secondaryLoadsArray = heap.AddArray<FABRIC_LOAD_METRIC_REPORT>(secondaryLoadMetricReports_.size());
    i = 0;
    for (auto iter = secondaryLoadMetricReports_.begin(); iter != secondaryLoadMetricReports_.end(); ++iter)
    {
        iter->ToPublicApi(heap, secondaryLoadsArray[i]);
        i++;
    }
    secondaryLoadsList->Items = secondaryLoadsArray.GetRawArray();

    queryResult.PrimaryLoadMetricReports = primaryLoadsList.GetRawPointer();
    queryResult.SecondaryLoadMetricReports = secondaryLoadsList.GetRawPointer();
}

ErrorCode PartitionLoadInformationQueryResult::FromPublicApi(__in FABRIC_PARTITION_LOAD_INFORMATION const& queryResult)
{
    ErrorCode error = ErrorCode::Success();
    partitionId_ = Guid(queryResult.PartitionId);
    primaryLoadMetricReports_.clear();     
    secondaryLoadMetricReports_.clear();

    auto primaryLoadList = queryResult.PrimaryLoadMetricReports;
    auto loadItem =(FABRIC_LOAD_METRIC_REPORT*) primaryLoadList->Items;

    for(uint i=0;i!=primaryLoadList->Count;++i)
    {
        LoadMetricReport load;
        load.FromPublicApi(*(loadItem+i));
        primaryLoadMetricReports_.push_back(load);
    }

    auto secondaryLoadList = queryResult.SecondaryLoadMetricReports;
    loadItem =(FABRIC_LOAD_METRIC_REPORT*) secondaryLoadList->Items;             

    for(uint i=0;i!=secondaryLoadList->Count;++i)
    {
        LoadMetricReport load;
        load.FromPublicApi(*(loadItem+i));
        secondaryLoadMetricReports_.push_back(load);
    }

    return ErrorCode::Success();
}

void PartitionLoadInformationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());    
}

std::wstring PartitionLoadInformationQueryResult::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<PartitionLoadInformationQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}
