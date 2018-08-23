// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ApplicationHealthStateFilter");

ApplicationHealthStateFilter::ApplicationHealthStateFilter()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(0)
    , applicationNameFilter_()
    , serviceFilters_()
    , deployedApplicationFilters_()
    , applicationTypeNameFilter_()
{
}

ApplicationHealthStateFilter::ApplicationHealthStateFilter(
    DWORD healthStateFilter)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(healthStateFilter)
    , applicationNameFilter_()
    , serviceFilters_()
    , deployedApplicationFilters_()
    , applicationTypeNameFilter_()
{
}

ApplicationHealthStateFilter::~ApplicationHealthStateFilter()
{
}

wstring ApplicationHealthStateFilter::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ApplicationHealthStateFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
         return wstring();
    }

    return objectString;
}

ErrorCode ApplicationHealthStateFilter::FromJsonString(wstring const & str, __inout ApplicationHealthStateFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

bool ApplicationHealthStateFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    bool returnAllOnDefault = false;
    if (!applicationNameFilter_.empty() || !serviceFilters_.empty() || !deployedApplicationFilters_.empty() || !applicationTypeNameFilter_.empty())
    {
        // If this is a specific filter or has children filter specified, consider default as all
        returnAllOnDefault = true;
    }

    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState, returnAllOnDefault);
}

ErrorCode ApplicationHealthStateFilter::FromPublicApi(FABRIC_APPLICATION_HEALTH_STATE_FILTER const & publicApplicationHealthStateFilter)
{
    healthStateFilter_ = publicApplicationHealthStateFilter.HealthStateFilter;

    // ApplicationNameFilter
    {
        auto hr = StringUtility::LpcwstrToWstring(publicApplicationHealthStateFilter.ApplicationNameFilter, true, 0, CommonConfig::GetConfig().MaxNamingUriLength, applicationNameFilter_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr, wformatString("{0} {1}, {2}, {3}.", GET_COMMON_RC(Invalid_LPCWSTR_Length), L"ApplicationNameFilter", 0, CommonConfig::GetConfig().MaxNamingUriLength));
            Trace.WriteInfo(TraceSource, "ApplicationNameFilter::FromPublicApi error: {0}{1}", error, error.Message);
            return error;
        }

        if (!applicationNameFilter_.empty())
        {
            // Check that this is a NamingUri
            NamingUri nameUri;
            if (!NamingUri::TryParse(applicationNameFilter_, /*out*/nameUri))
            {
                ErrorCode error(ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}.", GET_NAMING_RC(Invalid_Uri), applicationNameFilter_));
                Trace.WriteWarning(Uri::TraceCategory, "NamingUri::TryPase: {0}: {1}", error, error.Message);
                return error;
            }
        }
    }

    // ServiceFilters
    {
        auto error = PublicApiHelper::FromPublicApiList<ServiceHealthStateFilter, FABRIC_SERVICE_HEALTH_STATE_FILTER_LIST>(publicApplicationHealthStateFilter.ServiceFilters, serviceFilters_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ServiceFilters::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // DeployedApplicationFilters
    {
        auto error = PublicApiHelper::FromPublicApiList<DeployedApplicationHealthStateFilter, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_LIST>(publicApplicationHealthStateFilter.DeployedApplicationFilters, deployedApplicationFilters_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "DeployedApplicationFilters::FromPublicApi error: {0}", error);
            return error;
        }
    }

    // ApplicationTypeNameFilter
    if (publicApplicationHealthStateFilter.Reserved != NULL)
    {
        auto ex1 = static_cast<FABRIC_APPLICATION_HEALTH_STATE_FILTER_EX1*>(publicApplicationHealthStateFilter.Reserved);

        auto hr = StringUtility::LpcwstrToWstring(ex1->ApplicationTypeNameFilter, true, 0, ParameterValidator::MaxStringSize, applicationTypeNameFilter_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr, wformatString("{0} {1}, {2}, {3}.", GET_COMMON_RC(Invalid_LPCWSTR_Length), L"ApplicationTypeNameFilter", 0, ParameterValidator::MaxStringSize));
            Trace.WriteInfo(TraceSource, "ApplicationTyoeNameFilter::FromPublicApi error: {0} {1}", error, error.Message);
            return error;
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ApplicationHealthStateFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_APPLICATION_HEALTH_STATE_FILTER & publicFilter) const
{
    ErrorCode error(ErrorCodeValue::Success);

    publicFilter.HealthStateFilter = healthStateFilter_;

    publicFilter.ApplicationNameFilter = heap.AddString(applicationNameFilter_);

    auto publicServiceFilters = heap.AddItem<FABRIC_SERVICE_HEALTH_STATE_FILTER_LIST>();
    error = PublicApiHelper::ToPublicApiList<ServiceHealthStateFilter, FABRIC_SERVICE_HEALTH_STATE_FILTER, FABRIC_SERVICE_HEALTH_STATE_FILTER_LIST>(
        heap,
        serviceFilters_,
        *publicServiceFilters);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "ServiceFilters::ToPublicApi error: {0}", error);
        return error;
    }

    publicFilter.ServiceFilters = publicServiceFilters.GetRawPointer();

    auto publicDeployedApplicationFilters = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_LIST>();
    error = PublicApiHelper::ToPublicApiList<DeployedApplicationHealthStateFilter, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_LIST>(
        heap,
        deployedApplicationFilters_,
        *publicDeployedApplicationFilters);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "DeployedApplicationFilters::ToPublicApi error: {0}", error);
        return error;
    }

    publicFilter.DeployedApplicationFilters = publicDeployedApplicationFilters.GetRawPointer();

    if (!applicationTypeNameFilter_.empty())
    {
        auto ex1 = heap.AddItem<FABRIC_APPLICATION_HEALTH_STATE_FILTER_EX1>();
        ex1->ApplicationTypeNameFilter = heap.AddString(applicationTypeNameFilter_);
        publicFilter.Reserved = ex1.GetRawPointer();
    }

    return ErrorCode::Success();
}
