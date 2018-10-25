// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedApplicationHealthStateFilter");

DeployedApplicationHealthStateFilter::DeployedApplicationHealthStateFilter()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(0)
    , nodeNameFilter_()
    , deployedServicePackageFilters_()
{
}

DeployedApplicationHealthStateFilter::DeployedApplicationHealthStateFilter(
    DWORD healthStateFilter)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(healthStateFilter)
    , nodeNameFilter_()
    , deployedServicePackageFilters_()
{
}

DeployedApplicationHealthStateFilter::~DeployedApplicationHealthStateFilter()
{
}

wstring DeployedApplicationHealthStateFilter::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<DeployedApplicationHealthStateFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
         return wstring();
    }

    return objectString;
}

ErrorCode DeployedApplicationHealthStateFilter::FromJsonString(wstring const & str, __inout DeployedApplicationHealthStateFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

bool DeployedApplicationHealthStateFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    bool returnAllOnDefault = false;
    if (!nodeNameFilter_.empty() || !deployedServicePackageFilters_.empty())
    {
        // If this is a specific filter or has children filter specified, consider default as all
        returnAllOnDefault = true;
    }

    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState, returnAllOnDefault);
}

ErrorCode DeployedApplicationHealthStateFilter::FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER const & publicDeployedApplicationHealthStateFilter)
{
    healthStateFilter_ = publicDeployedApplicationHealthStateFilter.HealthStateFilter;

    // NodeNameFilter
    {
        auto hr = StringUtility::LpcwstrToWstring(publicDeployedApplicationHealthStateFilter.NodeNameFilter, true, 0, ParameterValidator::MaxStringSize, nodeNameFilter_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr, wformatString("{0} {1}, {2}, {3}.", GET_COMMON_RC(Invalid_LPCWSTR_Length), L"NodeNameFilter", 0, ParameterValidator::MaxStringSize));
            Trace.WriteInfo(TraceSource, "NodeNameFilter::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // DeployedServicePackageFilters
    {
        auto error = PublicApiHelper::FromPublicApiList<DeployedServicePackageHealthStateFilter, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_LIST>(publicDeployedApplicationHealthStateFilter.DeployedServicePackageFilters, deployedServicePackageFilters_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "DeployedServicePackageFilters::FromPublicApi error: {0}", error);
            return error;
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode DeployedApplicationHealthStateFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER & publicFilter) const
{
    ErrorCode error(ErrorCodeValue::Success);

    publicFilter.HealthStateFilter = healthStateFilter_;
    publicFilter.NodeNameFilter = heap.AddString(nodeNameFilter_);

    auto publicDeployedServicePackageFilters = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_LIST>();
    error = PublicApiHelper::ToPublicApiList<DeployedServicePackageHealthStateFilter, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_LIST>(
        heap,
        deployedServicePackageFilters_,
        *publicDeployedServicePackageFilters);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "DeployedServicePackageFilters::ToPublicApi error: {0}", error);
        return error;
    }

    publicFilter.DeployedServicePackageFilters = publicDeployedServicePackageFilters.GetRawPointer();
    return ErrorCode::Success();
}
