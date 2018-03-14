// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ClusterLoadInformationQueryResult::ClusterLoadInformationQueryResult()
{
}

ClusterLoadInformationQueryResult::ClusterLoadInformationQueryResult(
            Common::DateTime lastBalancingStartTimeUtc,
            Common::DateTime lastBalancingEndTimeUtc,
            std::vector<LoadMetricInformation> && loadMetricInformation):
    lastBalancingStartTimeUtc_(lastBalancingStartTimeUtc),
    lastBalancingEndTimeUtc_(lastBalancingEndTimeUtc),
    loadMetricInformation_(move(loadMetricInformation))
{
}
    
ClusterLoadInformationQueryResult::ClusterLoadInformationQueryResult(ClusterLoadInformationQueryResult const & other) :
    lastBalancingStartTimeUtc_(other.lastBalancingStartTimeUtc_),
    lastBalancingEndTimeUtc_(other.lastBalancingEndTimeUtc_),
    loadMetricInformation_(other.loadMetricInformation_)
{
}

ClusterLoadInformationQueryResult::ClusterLoadInformationQueryResult(ClusterLoadInformationQueryResult && other) :
    lastBalancingStartTimeUtc_(move(other.lastBalancingStartTimeUtc_)),
    lastBalancingEndTimeUtc_(move(other.lastBalancingEndTimeUtc_)),
    loadMetricInformation_(move(other.loadMetricInformation_))
{
}

ClusterLoadInformationQueryResult const & ClusterLoadInformationQueryResult::operator = (ClusterLoadInformationQueryResult const & other)
{
    if (this != & other)
    {
        lastBalancingStartTimeUtc_ = other.lastBalancingStartTimeUtc_;
        lastBalancingEndTimeUtc_ = other.lastBalancingEndTimeUtc_;
        loadMetricInformation_ = other.loadMetricInformation_;
    }

    return *this;
}

ClusterLoadInformationQueryResult const & ClusterLoadInformationQueryResult::operator=(ClusterLoadInformationQueryResult && other)
{
    if (this != &other)
    {
        lastBalancingStartTimeUtc_ = move(other.lastBalancingStartTimeUtc_);
        lastBalancingEndTimeUtc_ = move(other.lastBalancingEndTimeUtc_);
        loadMetricInformation_ = move(other.loadMetricInformation_);
    }

    return *this;
}

void ClusterLoadInformationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_CLUSTER_LOAD_INFORMATION & publicClusterLoadInformationQueryResult) const 
{
    publicClusterLoadInformationQueryResult.LastBalancingStartTimeUtc = lastBalancingStartTimeUtc_.AsFileTime;
    publicClusterLoadInformationQueryResult.LastBalancingEndTimeUtc = lastBalancingEndTimeUtc_.AsFileTime;
    auto loadMetricInformationList = heap.AddItem<::FABRIC_LOAD_METRIC_INFORMATION_LIST>();
    loadMetricInformationList->Count = static_cast<ULONG>(loadMetricInformation_.size());
    auto loadMetricInformationArray = heap.AddArray<FABRIC_LOAD_METRIC_INFORMATION>(loadMetricInformation_.size());
    int i = 0;

    for (auto iter = loadMetricInformation_.begin(); iter != loadMetricInformation_.end(); ++iter)
    {
        iter->ToPublicApi(heap, loadMetricInformationArray[i]);
        i++;
    }

    loadMetricInformationList->Items = loadMetricInformationArray.GetRawArray();
    publicClusterLoadInformationQueryResult.LoadMetricInformation = loadMetricInformationList.GetRawPointer();

    publicClusterLoadInformationQueryResult.Reserved = NULL;    
}

ErrorCode ClusterLoadInformationQueryResult::FromPublicApi(__in FABRIC_CLUSTER_LOAD_INFORMATION const& publicQueryResult)
{    
    lastBalancingStartTimeUtc_ = DateTime(publicQueryResult.LastBalancingStartTimeUtc);
    lastBalancingEndTimeUtc_ = DateTime(publicQueryResult.LastBalancingStartTimeUtc);
    FABRIC_LOAD_METRIC_INFORMATION_LIST * loadMetricInformationList = (FABRIC_LOAD_METRIC_INFORMATION_LIST *) publicQueryResult.LoadMetricInformation;
    loadMetricInformation_.clear();
    auto item = (FABRIC_LOAD_METRIC_INFORMATION *)loadMetricInformationList->Items;

    for(uint i=0; i!= loadMetricInformationList->Count; ++i)
    {
        LoadMetricInformation loadMetricInformation;
        loadMetricInformation.FromPublicApi(*(item+i));
        loadMetricInformation_.push_back(loadMetricInformation);
    }

    return ErrorCode::Success();
}

void ClusterLoadInformationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring ClusterLoadInformationQueryResult::ToString() const
{    
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ClusterLoadInformationQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}
