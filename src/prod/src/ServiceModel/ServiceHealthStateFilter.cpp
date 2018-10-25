// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceHealthStateFilter");

ServiceHealthStateFilter::ServiceHealthStateFilter()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(0)
    , serviceNameFilter_()
    , partitionFilters_()
{
}

ServiceHealthStateFilter::ServiceHealthStateFilter(
    DWORD healthStateFilter)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(healthStateFilter)
    , serviceNameFilter_()
    , partitionFilters_()
{
}

ServiceHealthStateFilter::~ServiceHealthStateFilter()
{
}

wstring ServiceHealthStateFilter::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ServiceHealthStateFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
         return wstring();
    }

    return objectString;
}

ErrorCode ServiceHealthStateFilter::FromJsonString(wstring const & str, __inout ServiceHealthStateFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

bool ServiceHealthStateFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    bool returnAllOnDefault = false;
    if (!serviceNameFilter_.empty() || !partitionFilters_.empty())
    {
        // If this is a specific filter or has children filter specified, consider default as all
        returnAllOnDefault = true;
    }

    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState, returnAllOnDefault);
}

ErrorCode ServiceHealthStateFilter::FromPublicApi(FABRIC_SERVICE_HEALTH_STATE_FILTER const & publicServiceHealthStateFilter)
{
    healthStateFilter_ = publicServiceHealthStateFilter.HealthStateFilter;

    // ServiceNameFilter
    {
        auto hr = StringUtility::LpcwstrToWstring(publicServiceHealthStateFilter.ServiceNameFilter, true, 0, CommonConfig::GetConfig().MaxNamingUriLength, serviceNameFilter_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr, wformatString("{0} {1}, {2}, {3}.", GET_COMMON_RC(Invalid_LPCWSTR_Length), L"ServiceNameFilter", 0, CommonConfig::GetConfig().MaxNamingUriLength));
            Trace.WriteInfo(TraceSource, "ServiceNameFilter::FromPublicApi error: {0}", error);
            return error;
        }

        if (!serviceNameFilter_.empty())
        {
            // Check that this is a NamingUri
            NamingUri nameUri;
            if (!NamingUri::TryParse(serviceNameFilter_, /*out*/nameUri))
            {
                ErrorCode error(ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}.", GET_NAMING_RC(Invalid_Uri), serviceNameFilter_));
                Trace.WriteWarning(Uri::TraceCategory, "NamingUri::TryPase: {0}: {1}", error, error.Message);
                return error;
            }
        }
    }

    // PartitionFilters
    {
        auto error = PublicApiHelper::FromPublicApiList<PartitionHealthStateFilter, FABRIC_PARTITION_HEALTH_STATE_FILTER_LIST>(publicServiceHealthStateFilter.PartitionFilters, partitionFilters_);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "PartitionFilters::FromPublicApi error: {0}", error);
            return error;
        }
    }

    return ErrorCode::Success();
}

Common::ErrorCode ServiceHealthStateFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SERVICE_HEALTH_STATE_FILTER & publicFilter) const
{
    ErrorCode error(ErrorCodeValue::Success);

    publicFilter.HealthStateFilter = healthStateFilter_;
    publicFilter.ServiceNameFilter = heap.AddString(serviceNameFilter_);

    auto publicPartitionFilters = heap.AddItem<FABRIC_PARTITION_HEALTH_STATE_FILTER_LIST>();
    error = PublicApiHelper::ToPublicApiList<PartitionHealthStateFilter, FABRIC_PARTITION_HEALTH_STATE_FILTER, FABRIC_PARTITION_HEALTH_STATE_FILTER_LIST>(
        heap,
        partitionFilters_,
        *publicPartitionFilters);
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceSource, "PartitionFilters::ToPublicApi error: {0}", error);
        return error;
    }

    publicFilter.PartitionFilters = publicPartitionFilters.GetRawPointer();
    return ErrorCode::Success();
}
