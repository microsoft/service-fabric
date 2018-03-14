// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationLoadInformationQueryResult::ApplicationLoadInformationQueryResult()
{
}

ApplicationLoadInformationQueryResult::ApplicationLoadInformationQueryResult(
    std::wstring const & applicationName,
    uint minimumNodes,
    uint maximumNodes,
    uint nodeCount,
    std::vector<ServiceModel::ApplicationLoadMetricInformation> && applicationLoadMetricInformation)
    : applicationName_(applicationName)
    , minimumNodes_(minimumNodes)
    , maximumNodes_(maximumNodes)
    , nodeCount_(nodeCount)
    , applicationLoadMetricInformation_(move(applicationLoadMetricInformation))
{
}
    
ApplicationLoadInformationQueryResult::ApplicationLoadInformationQueryResult(ApplicationLoadInformationQueryResult const & other)
    : applicationName_(other.applicationName_)
    , minimumNodes_(other.minimumNodes_)
    , maximumNodes_(other.maximumNodes_)
    , nodeCount_(other.nodeCount_)
    , applicationLoadMetricInformation_(other.applicationLoadMetricInformation_)
{
}

ApplicationLoadInformationQueryResult::ApplicationLoadInformationQueryResult(ApplicationLoadInformationQueryResult && other)
    : applicationName_(move(other.applicationName_))
    , minimumNodes_(other.minimumNodes_)
    , maximumNodes_(other.maximumNodes_)
    , nodeCount_(other.nodeCount_)
    , applicationLoadMetricInformation_(move(other.applicationLoadMetricInformation_))
{
}

ApplicationLoadInformationQueryResult const & ApplicationLoadInformationQueryResult::operator = (ApplicationLoadInformationQueryResult const & other)
{
    if (this != & other)
    {
        applicationName_ = other.applicationName_;
        minimumNodes_ = other.minimumNodes_;
        maximumNodes_ = other.maximumNodes_;
        nodeCount_ = other.nodeCount_;
        applicationLoadMetricInformation_ = other.applicationLoadMetricInformation_;
    }

    return *this;
}

ApplicationLoadInformationQueryResult const & ApplicationLoadInformationQueryResult::operator=(ApplicationLoadInformationQueryResult && other)
{
    if (this != &other)
    {
        applicationName_ = move(other.applicationName_);
        minimumNodes_ = other.minimumNodes_;
        maximumNodes_ = other.maximumNodes_;
        nodeCount_ = other.nodeCount_;
        applicationLoadMetricInformation_ = move(other.applicationLoadMetricInformation_);
    }

    return *this;
}

void ApplicationLoadInformationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_APPLICATION_LOAD_INFORMATION & publicApplicationLoadInformationQueryResult) const 
{
    publicApplicationLoadInformationQueryResult.Name = heap.AddString(applicationName_);
    publicApplicationLoadInformationQueryResult.MinimumNodes = minimumNodes_;
    publicApplicationLoadInformationQueryResult.MaximumNodes = maximumNodes_;
    publicApplicationLoadInformationQueryResult.NodeCount = nodeCount_;

    auto applicationLoadMetricInformationList = heap.AddItem<::FABRIC_APPLICATION_LOAD_METRIC_INFORMATION_LIST>();
    applicationLoadMetricInformationList->Count = static_cast<ULONG>(applicationLoadMetricInformation_.size());
    auto applicationLoadMetricInformationArray = heap.AddArray<FABRIC_APPLICATION_LOAD_METRIC_INFORMATION>(applicationLoadMetricInformation_.size());
    int index = 0;

    for (auto metric : applicationLoadMetricInformation_)
    {
        metric.ToPublicApi(heap, applicationLoadMetricInformationArray[index]);
        index++;
    }

    applicationLoadMetricInformationList->LoadMetrics = applicationLoadMetricInformationArray.GetRawArray();
    publicApplicationLoadInformationQueryResult.ApplicationLoadMetricInformation = applicationLoadMetricInformationList.GetRawPointer();

    publicApplicationLoadInformationQueryResult.Reserved = NULL;
}

ErrorCode ApplicationLoadInformationQueryResult::FromPublicApi(__in FABRIC_APPLICATION_LOAD_INFORMATION const& publicQueryResult)
{
    auto hr = StringUtility::LpcwstrToWstring(publicQueryResult.Name, false, applicationName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    minimumNodes_ = publicQueryResult.MinimumNodes;
    maximumNodes_ = publicQueryResult.MaximumNodes;
    nodeCount_ = publicQueryResult.NodeCount;

    FABRIC_APPLICATION_LOAD_METRIC_INFORMATION_LIST const & applicationLoadMetricInformationList = *(FABRIC_APPLICATION_LOAD_METRIC_INFORMATION_LIST *) publicQueryResult.ApplicationLoadMetricInformation;
    applicationLoadMetricInformation_.clear();
    auto loadMetrics = (FABRIC_APPLICATION_LOAD_METRIC_INFORMATION *)applicationLoadMetricInformationList.LoadMetrics;

    for(uint i = 0; i != applicationLoadMetricInformationList.Count; ++i)
    {
        ServiceModel::ApplicationLoadMetricInformation applicationLoadMetricInformation;
        applicationLoadMetricInformation.FromPublicApi(*(loadMetrics + i));
        applicationLoadMetricInformation_.push_back(applicationLoadMetricInformation);
    }

    return ErrorCode::Success();
}

void ApplicationLoadInformationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring ApplicationLoadInformationQueryResult::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ApplicationLoadInformationQueryResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}
