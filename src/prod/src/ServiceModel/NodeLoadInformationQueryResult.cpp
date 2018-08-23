// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

NodeLoadInformationQueryResult::NodeLoadInformationQueryResult()
{
}

NodeLoadInformationQueryResult::NodeLoadInformationQueryResult(
    std::wstring const & nodeName,
    std::vector<ServiceModel::NodeLoadMetricInformation> && nodeLoadMetricInformation):
    nodeName_(nodeName),
    nodeLoadMetricInformation_(move(nodeLoadMetricInformation))
{
}

void NodeLoadInformationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_NODE_LOAD_INFORMATION & publicNodeLoadInformationQueryResult) const 
{
    publicNodeLoadInformationQueryResult.NodeName = heap.AddString(nodeName_);
    auto nodeLoadMetricInformationList = heap.AddItem<::FABRIC_NODE_LOAD_METRIC_INFORMATION_LIST>();
    nodeLoadMetricInformationList->Count = static_cast<ULONG>(nodeLoadMetricInformation_.size());
    auto nodeLoadMetricInformationArray = heap.AddArray<FABRIC_NODE_LOAD_METRIC_INFORMATION>(nodeLoadMetricInformation_.size());
    int i = 0;

    for (auto iter = nodeLoadMetricInformation_.begin(); iter != nodeLoadMetricInformation_.end(); ++iter)
    {
        iter->ToPublicApi(heap, nodeLoadMetricInformationArray[i]);
        i++;
    }

    nodeLoadMetricInformationList->Items = nodeLoadMetricInformationArray.GetRawArray();
    publicNodeLoadInformationQueryResult.NodeLoadMetricInformation = nodeLoadMetricInformationList.GetRawPointer();

    publicNodeLoadInformationQueryResult.Reserved = NULL;    
}

ErrorCode NodeLoadInformationQueryResult::FromPublicApi(__in FABRIC_NODE_LOAD_INFORMATION const& publicQueryResult)
{    
    auto hr = StringUtility::LpcwstrToWstring(publicQueryResult.NodeName, false, nodeName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    FABRIC_NODE_LOAD_METRIC_INFORMATION_LIST const & nodeLoadMetricInformationList = *(FABRIC_NODE_LOAD_METRIC_INFORMATION_LIST *) publicQueryResult.NodeLoadMetricInformation;
    nodeLoadMetricInformation_.clear();
    auto item = (FABRIC_NODE_LOAD_METRIC_INFORMATION *)nodeLoadMetricInformationList.Items;

    for(uint i=0; i!= nodeLoadMetricInformationList.Count; ++i)
    {
        ServiceModel::NodeLoadMetricInformation nodeLoadMetricInformation;
        nodeLoadMetricInformation.FromPublicApi(*(item+i));
        nodeLoadMetricInformation_.push_back(nodeLoadMetricInformation);
    }

    return ErrorCode::Success();
}

void NodeLoadInformationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring NodeLoadInformationQueryResult::ToString() const
{    
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeLoadInformationQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}
