// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedServicePackageHealthStateFilter");

DeployedServicePackageHealthStateFilter::DeployedServicePackageHealthStateFilter()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(0)
    , serviceManifestNameFilter_()
    , servicePackageActivationIdFilterSPtr_()
{
}

DeployedServicePackageHealthStateFilter::DeployedServicePackageHealthStateFilter(
    DWORD healthStateFilter)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(healthStateFilter)
    , serviceManifestNameFilter_()
    , servicePackageActivationIdFilterSPtr_()
{
}

DeployedServicePackageHealthStateFilter::~DeployedServicePackageHealthStateFilter()
{
}

wstring DeployedServicePackageHealthStateFilter::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<DeployedServicePackageHealthStateFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
         return wstring();
    }

    return objectString;
}

ErrorCode DeployedServicePackageHealthStateFilter::FromJsonString(wstring const & str, __inout DeployedServicePackageHealthStateFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

bool DeployedServicePackageHealthStateFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
	bool returnAllOnDefault = !(this->IsDefaultFilter());
    
	return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState, returnAllOnDefault);
}

bool DeployedServicePackageHealthStateFilter::IsDefaultFilter() const
{
	return (serviceManifestNameFilter_.empty() && servicePackageActivationIdFilterSPtr_ == nullptr);
}

ErrorCode DeployedServicePackageHealthStateFilter::FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER const & publicDeployedServicePackageHealthStateFilter)
{
    healthStateFilter_ = publicDeployedServicePackageHealthStateFilter.HealthStateFilter;

    // ServiceManifestNameFilter
    {
        auto hr = StringUtility::LpcwstrToWstring(publicDeployedServicePackageHealthStateFilter.ServiceManifestNameFilter, true, 0, ParameterValidator::MaxStringSize, serviceManifestNameFilter_);
        if (FAILED(hr))
        {
            auto error = ErrorCode::FromHResult(hr, wformatString("{0} {1}, {2}, {3}.", GET_COMMON_RC(Invalid_LPCWSTR_Length), L"ServiceManifestNameFilter", 0, ParameterValidator::MaxStringSize));
            Trace.WriteInfo(TraceSource, "ServiceManifestNameFilter::FromPublicApi error: {0}", error);
            return error;
        }
    }

    if (publicDeployedServicePackageHealthStateFilter.Reserved == nullptr)
    {
        return ErrorCode::Success();
    }

    auto ex1 = (FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_EX1*)publicDeployedServicePackageHealthStateFilter.Reserved;

    // ServicePackageActivationIdFilter
    {
		if (ex1->ServicePackageActivationIdFilter != nullptr)
		{
			wstring servicePackageActivationIdFilter;
			auto error = StringUtility::LpcwstrToWstring2(ex1->ServicePackageActivationIdFilter, true, servicePackageActivationIdFilter);
			if (!error.IsSuccess())
			{
				Trace.WriteInfo(TraceSource, "ServicePackageActivationIdFilter::FromPublicApi error: {0}", error);
				return error;
			}

			servicePackageActivationIdFilterSPtr_ = std::make_shared<wstring>(std::move(servicePackageActivationIdFilter));
		}
    }

    return ErrorCode::Success();
}

Common::ErrorCode DeployedServicePackageHealthStateFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER & publicFilter) const
{
    publicFilter.HealthStateFilter = healthStateFilter_;
    publicFilter.ServiceManifestNameFilter = heap.AddString(serviceManifestNameFilter_);

	if (servicePackageActivationIdFilterSPtr_ != nullptr)
	{
		auto ex1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_EX1>();
		ex1->ServicePackageActivationIdFilter = heap.AddString(*servicePackageActivationIdFilterSPtr_);
		publicFilter.Reserved = ex1.GetRawPointer();
	}
    
    return ErrorCode::Success();
}
