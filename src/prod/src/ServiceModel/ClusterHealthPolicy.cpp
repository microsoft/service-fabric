// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ClusterHealthPolicy");

ClusterHealthPolicy::ClusterHealthPolicy()
    : considerWarningAsError_(false)
    , maxPercentUnhealthyNodes_(0)
    , maxPercentUnhealthyApplications_(0)
    , applicationTypeMap_()
{
}

ClusterHealthPolicy::ClusterHealthPolicy(
    bool considerWarningAsError, 
    BYTE maxPercentUnhealthyNodes,
    BYTE maxPercentUnhealthyApplications)
    : considerWarningAsError_(considerWarningAsError)
    , maxPercentUnhealthyNodes_(maxPercentUnhealthyNodes)
    , maxPercentUnhealthyApplications_(maxPercentUnhealthyApplications)
    , applicationTypeMap_()
{
}

ClusterHealthPolicy::~ClusterHealthPolicy()
{
}

Common::ErrorCode ClusterHealthPolicy::AddApplicationTypeHealthPolicy(std::wstring && applicationTypeName, BYTE maxPercentUnhealthyApplications)
{
    auto error = ParameterValidator::ValidatePercentValue(maxPercentUnhealthyApplications, Constants::MaxPercentUnhealthyApplications);
    if (!error.IsSuccess()) { return error; }

    if (applicationTypeName.empty())
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    applicationTypeMap_.insert(make_pair(move(applicationTypeName), maxPercentUnhealthyApplications));
    return ErrorCode::Success();
}

bool ClusterHealthPolicy::operator == (ClusterHealthPolicy const & other) const
{
    bool equals = (this->considerWarningAsError_ == other.considerWarningAsError_);
    if (!equals) return equals;

    equals = (this->maxPercentUnhealthyNodes_ == other.maxPercentUnhealthyNodes_);
    if (!equals) return equals;

    equals = (this->maxPercentUnhealthyApplications_ == other.maxPercentUnhealthyApplications_);
    if (!equals) return equals;

    equals = (this->applicationTypeMap_.size() == other.applicationTypeMap_.size());
    if (!equals) return equals;

    for (auto it = applicationTypeMap_.begin(), itOther = other.applicationTypeMap_.begin(); it != applicationTypeMap_.end(); ++it, ++itOther)
    {
        equals = (it->first == itOther->first) && (it->second == itOther->second);
        if (!equals) return equals;
    }

    return equals;
}

bool ClusterHealthPolicy::operator != (ClusterHealthPolicy const & other) const
{
    return !(*this == other);
}

void ClusterHealthPolicy::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write("ClusterHealthPolicy = {0}", ToString());
}

wstring ClusterHealthPolicy::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ClusterHealthPolicy&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ClusterHealthPolicy::FromString(wstring const & clusterHealthPolicyStr, __out ClusterHealthPolicy & clusterHealthPolicy)
{
    return JsonHelper::Deserialize(clusterHealthPolicy, clusterHealthPolicyStr);
}

ErrorCode ClusterHealthPolicy::FromPublicApi(FABRIC_CLUSTER_HEALTH_POLICY const & publicClusterHealthPolicy)
{
    if (publicClusterHealthPolicy.ConsiderWarningAsError)
    {
        this->considerWarningAsError_ = true;
    }
    else
    {
        this->considerWarningAsError_ = false;
    }

    auto error = ParameterValidator::ValidatePercentValue(publicClusterHealthPolicy.MaxPercentUnhealthyNodes, Constants::MaxPercentUnhealthyNodes);
    if (!error.IsSuccess()) { return error; }
    this->maxPercentUnhealthyNodes_ = publicClusterHealthPolicy.MaxPercentUnhealthyNodes;

    error = ParameterValidator::ValidatePercentValue(publicClusterHealthPolicy.MaxPercentUnhealthyApplications, Constants::MaxPercentUnhealthyApplications);
    if (!error.IsSuccess()) { return error; }
    this->maxPercentUnhealthyApplications_ = publicClusterHealthPolicy.MaxPercentUnhealthyApplications;

    if (publicClusterHealthPolicy.Reserved != NULL)
    {
        auto ex1 = reinterpret_cast<FABRIC_CLUSTER_HEALTH_POLICY_EX1*>(publicClusterHealthPolicy.Reserved);
        if (ex1->ApplicationTypeHealthPolicyMap != NULL)
        {
            for (size_t i = 0; i < ex1->ApplicationTypeHealthPolicyMap->Count; ++i)
            {
                FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP_ITEM const & mapItem = ex1->ApplicationTypeHealthPolicyMap->Items[i];
                
                wstring applicationTypeName;
                auto hr = StringUtility::LpcwstrToWstring(mapItem.ApplicationTypeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, applicationTypeName);
                if (FAILED(hr))
                {
                    error = ErrorCode::FromHResult(hr, wformatString("{0} {1}.", HMResource::GetResources().InvalidApplicationTypeHealthPolicyMapItem, i));
                    Trace.WriteInfo(TraceSource, "Error parsing item {0} from app policy map in FromPublicAPI: {1} {2}", i, error, error.Message);
                    return error;
                }

                error = ParameterValidator::ValidatePercentValue(mapItem.MaxPercentUnhealthyApplications, Constants::MaxPercentUnhealthyApplications);
                if (!error.IsSuccess()) { return error; }

                applicationTypeMap_.insert(make_pair(move(applicationTypeName), mapItem.MaxPercentUnhealthyApplications));
            }
        }
    }

    return error;
}

void ClusterHealthPolicy::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_CLUSTER_HEALTH_POLICY & publicClusterHealthPolicy) const
{
    publicClusterHealthPolicy.ConsiderWarningAsError = this->considerWarningAsError_ ? TRUE : FALSE;
    publicClusterHealthPolicy.MaxPercentUnhealthyNodes = this->maxPercentUnhealthyNodes_;
    publicClusterHealthPolicy.MaxPercentUnhealthyApplications = this->maxPercentUnhealthyApplications_;

    auto ex1 = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY_EX1>();
    publicClusterHealthPolicy.Reserved = ex1.GetRawPointer();

    auto publicAppTypePolicyMap = heap.AddItem<FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP>();
    ex1->ApplicationTypeHealthPolicyMap = publicAppTypePolicyMap.GetRawPointer();
    publicAppTypePolicyMap->Count = (ULONG)applicationTypeMap_.size();

    if (!applicationTypeMap_.empty())
    {
        auto mapItems = heap.AddArray<FABRIC_APPLICATION_TYPE_HEALTH_POLICY_MAP_ITEM>(publicAppTypePolicyMap->Count);
        publicAppTypePolicyMap->Items = mapItems.GetRawArray();

        int index = 0;
        for (auto it = applicationTypeMap_.begin(); it != applicationTypeMap_.end(); ++it, ++index)
        {
            mapItems[index].ApplicationTypeName = heap.AddString(it->first);
            mapItems[index].MaxPercentUnhealthyApplications = it->second;
        }
    }
    else
    {
        publicAppTypePolicyMap->Items = NULL;
    }
}

bool ClusterHealthPolicy::TryValidate(__out wstring & validationErrorMessage) const
{
    auto error = ParameterValidator::ValidatePercentValue(MaxPercentUnhealthyNodes, Constants::MaxPercentUnhealthyNodes);
    if (error.IsSuccess())
    {
        error = ParameterValidator::ValidatePercentValue(MaxPercentUnhealthyApplications, Constants::MaxPercentUnhealthyApplications);
    }

    if (error.IsSuccess())
    {
        for (auto it = applicationTypeMap_.begin(); it != applicationTypeMap_.end(); ++it)
        {
            error = ParameterValidator::ValidatePercentValue(MaxPercentUnhealthyApplications, Constants::MaxPercentUnhealthyApplications);
            if (!error.IsSuccess()) break;
        }
    }

    if (!error.IsSuccess())
    {
        validationErrorMessage = error.TakeMessage();
        return false;
    }

    return true;
}
