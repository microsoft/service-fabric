// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ClusterUpgradeHealthPolicy");

ClusterUpgradeHealthPolicy::ClusterUpgradeHealthPolicy()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , maxPercentDeltaUnhealthyNodes_(0)
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(0)
{
}

ClusterUpgradeHealthPolicy::ClusterUpgradeHealthPolicy(
    BYTE maxPercentDeltaUnhealthyNodes,
    BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , maxPercentDeltaUnhealthyNodes_(maxPercentDeltaUnhealthyNodes)
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(maxPercentUpgradeDomainDeltaUnhealthyNodes)
{
}

ClusterUpgradeHealthPolicy::~ClusterUpgradeHealthPolicy()
{
}

bool ClusterUpgradeHealthPolicy::operator == (ClusterUpgradeHealthPolicy const & other) const
{
    bool equals;

    equals = (this->maxPercentDeltaUnhealthyNodes_ == other.maxPercentDeltaUnhealthyNodes_);
    if (!equals) return equals;

    equals = (this->maxPercentUpgradeDomainDeltaUnhealthyNodes_ == other.maxPercentUpgradeDomainDeltaUnhealthyNodes_);
    if (!equals) return equals;

    return equals;
}

bool ClusterUpgradeHealthPolicy::operator != (ClusterUpgradeHealthPolicy const & other) const
{
    return !(*this == other);
}

std::wstring ClusterUpgradeHealthPolicy::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ClusterUpgradeHealthPolicy&>(*this), objectString);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "Error serializing ClusterUpgradeHealthPolicy to JSON: {0}", error);
        return wstring();
    }

    return objectString;
}

ErrorCode ClusterUpgradeHealthPolicy::FromString(
    std::wstring const & clusterHealthPolicyStr, 
    __inout ClusterUpgradeHealthPolicy & clusterHealthPolicy)
{
    return JsonHelper::Deserialize(clusterHealthPolicy, clusterHealthPolicyStr);
}

void ClusterUpgradeHealthPolicy::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ClusterUpgradeHealthPolicy = {0}", ToString());
}

ErrorCode ClusterUpgradeHealthPolicy::FromPublicApi(FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY const & publicClusterUpgradeHealthPolicy)
{
    ErrorCode error(ErrorCodeValue::Success);

    error = ParameterValidator::ValidatePercentValue(publicClusterUpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes, Constants::MaxPercentDeltaUnhealthyNodes);
    if (!error.IsSuccess()) { return error; }
    maxPercentDeltaUnhealthyNodes_ = publicClusterUpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes;

    error = ParameterValidator::ValidatePercentValue(publicClusterUpgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes, Constants::MaxPercentUpgradeDomainDeltaUnhealthyNodes);
    if (!error.IsSuccess()) { return error; }
    maxPercentUpgradeDomainDeltaUnhealthyNodes_ = publicClusterUpgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes;

    return error;
}

ErrorCode ClusterUpgradeHealthPolicy::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY & publicClusterUpgradeHealthPolicy) const
{
    UNREFERENCED_PARAMETER(heap);
    publicClusterUpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes = maxPercentDeltaUnhealthyNodes_;
    publicClusterUpgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes = maxPercentUpgradeDomainDeltaUnhealthyNodes_;

    return ErrorCode::Success();
}

bool ClusterUpgradeHealthPolicy::TryValidate(__inout std::wstring & validationErrorMessage) const
{
    auto error = ParameterValidator::ValidatePercentValue(MaxPercentDeltaUnhealthyNodes, Constants::MaxPercentDeltaUnhealthyNodes);
    if (error.IsSuccess())
    {
        error = ParameterValidator::ValidatePercentValue(MaxPercentUpgradeDomainDeltaUnhealthyNodes, Constants::MaxPercentUpgradeDomainDeltaUnhealthyNodes);
    }

    if (!error.IsSuccess())
    {
        validationErrorMessage = error.TakeMessage();
        return false;
    }

    return true;
}


