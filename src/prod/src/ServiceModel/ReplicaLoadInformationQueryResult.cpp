// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ReplicaLoadInformationQueryResult::ReplicaLoadInformationQueryResult()
{
}

ReplicaLoadInformationQueryResult::ReplicaLoadInformationQueryResult(
    Common::Guid partitionId,
    int64 replicaOrInstanceId,
    std::vector<ServiceModel::LoadMetricReport> && loadMetricReports)    
    : partitionId_(partitionId)
    , replicaOrInstanceId_(move(replicaOrInstanceId))
    , loadMetricReports_(move(loadMetricReports))
{
}


ReplicaLoadInformationQueryResult::ReplicaLoadInformationQueryResult(ReplicaLoadInformationQueryResult const & other) :
    partitionId_(other.partitionId_),
    replicaOrInstanceId_(other.replicaOrInstanceId_),
    loadMetricReports_(other.loadMetricReports_)
{
}

ReplicaLoadInformationQueryResult::ReplicaLoadInformationQueryResult(ReplicaLoadInformationQueryResult && other) :
    partitionId_(move(other.partitionId_)),
    replicaOrInstanceId_(move(other.replicaOrInstanceId_)),
    loadMetricReports_(move(other.loadMetricReports_))
{
}

ReplicaLoadInformationQueryResult const & ReplicaLoadInformationQueryResult::operator = (ReplicaLoadInformationQueryResult const & other)
{
    if (&other != this)
    {
        partitionId_ = other.partitionId_;
        replicaOrInstanceId_ = other.replicaOrInstanceId_;
        loadMetricReports_ = other.loadMetricReports_;
    }

    return *this;
}

ReplicaLoadInformationQueryResult const & ReplicaLoadInformationQueryResult::operator = (ReplicaLoadInformationQueryResult && other)
{
    if (&other != this)
    {
        partitionId_ = move(other.partitionId_);
        replicaOrInstanceId_ = move(other.replicaOrInstanceId_);
        loadMetricReports_ = move(other.loadMetricReports_);
    }

    return *this;
}

void ReplicaLoadInformationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_REPLICA_LOAD_INFORMATION & queryResult) const 
{
    queryResult.PartitionId = partitionId_.AsGUID();
    queryResult.ReplicaOrInstanceId = replicaOrInstanceId_;

    auto loadMetricReportsList = heap.AddItem<::FABRIC_LOAD_METRIC_REPORT_LIST>();
    loadMetricReportsList->Count = static_cast<ULONG>(loadMetricReports_.size());

    auto loadMetricReportsArray = heap.AddArray<FABRIC_LOAD_METRIC_REPORT>(loadMetricReports_.size());
    int i = 0;
    for (auto iter = loadMetricReports_.begin(); iter != loadMetricReports_.end(); ++iter)
    {
        iter->ToPublicApi(heap, loadMetricReportsArray[i]);
        i++;
    }
    loadMetricReportsList->Items = loadMetricReportsArray.GetRawArray();

    queryResult.LoadMetricReports = loadMetricReportsList.GetRawPointer();
}

ErrorCode ReplicaLoadInformationQueryResult::FromPublicApi(__in FABRIC_REPLICA_LOAD_INFORMATION const& queryResult)
{
    ErrorCode error = ErrorCode::Success();
    partitionId_ = Guid(queryResult.PartitionId);
    loadMetricReports_.clear();     
    loadMetricReports_.clear();

    auto loadMetricReportsList = queryResult.LoadMetricReports;
    auto loadItem =(FABRIC_LOAD_METRIC_REPORT*) loadMetricReportsList->Items;

    for(uint i=0;i!=loadMetricReportsList->Count;++i)
    {
        LoadMetricReport load;
        load.FromPublicApi(*(loadItem+i));
        loadMetricReports_.push_back(load);
    }

    return ErrorCode::Success();
}

void ReplicaLoadInformationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());    
}

std::wstring ReplicaLoadInformationQueryResult::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ReplicaLoadInformationQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}
