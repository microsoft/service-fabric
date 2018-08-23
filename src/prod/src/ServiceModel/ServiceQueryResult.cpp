// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServiceQueryResult)

ServiceQueryResult::ServiceQueryResult()
    : serviceKind_(FABRIC_SERVICE_KIND_INVALID)
    , serviceName_()
    , serviceTypeName_()
    , serviceManifestVersion_()
    , hasPersistedState_(false)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , serviceStatus_(FABRIC_QUERY_SERVICE_STATUS_UNKNOWN)
    , isServiceGroup_(false)
{
}

ServiceQueryResult::ServiceQueryResult(
    Common::Uri const & serviceName,        
    std::wstring const & serviceTypeName,
    std::wstring const & serviceManifestVersion,
    bool hasPersistedState,
    FABRIC_QUERY_SERVICE_STATUS serviceStatus)
    : serviceKind_(FABRIC_SERVICE_KIND_STATEFUL)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , serviceManifestVersion_(serviceManifestVersion)
    , hasPersistedState_(hasPersistedState)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , serviceStatus_(serviceStatus)
    , isServiceGroup_(false)
{
}

ServiceQueryResult::ServiceQueryResult(
    Common::Uri const & serviceName,        
    std::wstring const & serviceTypeName,
    std::wstring const & serviceManifestVersion,
    FABRIC_QUERY_SERVICE_STATUS serviceStatus)
    : serviceKind_(FABRIC_SERVICE_KIND_STATELESS)
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , serviceManifestVersion_(serviceManifestVersion)
    , hasPersistedState_(false)
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
    , serviceStatus_(serviceStatus)
    , isServiceGroup_(false)
{
}

ServiceQueryResult::~ServiceQueryResult()
{
}

ServiceQueryResult ServiceQueryResult::CreateStatelessServiceQueryResult(
    Common::Uri const & serviceName,        
    std::wstring const & serviceTypeName,
    std::wstring const & serviceManifestVersion,
    FABRIC_QUERY_SERVICE_STATUS serviceStatus,
    bool isServiceGroup)
{
    ServiceQueryResult serviceQueryResult (serviceName, serviceTypeName, serviceManifestVersion, serviceStatus);
    serviceQueryResult.IsServiceGroup = isServiceGroup;
    return serviceQueryResult;
}

ServiceQueryResult ServiceQueryResult::CreateStatefulServiceQueryResult(
    Common::Uri const & serviceName,        
    std::wstring const & serviceTypeName,
    std::wstring const & serviceManifestVersion,
    bool hasPersistedState,
    FABRIC_QUERY_SERVICE_STATUS serviceStatus,
    bool isServiceGroup)
{
    ServiceQueryResult serviceQueryResult (serviceName, serviceTypeName, serviceManifestVersion, hasPersistedState, serviceStatus);
    serviceQueryResult.IsServiceGroup = isServiceGroup;
    return serviceQueryResult;
}

void ServiceQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_SERVICE_QUERY_RESULT_ITEM & publicServiceQueryResult) const 
{
    publicServiceQueryResult.Kind = serviceKind_;

    if (serviceKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        auto publicStatelessService = heap.AddItem<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM>();
        publicStatelessService->ServiceName = heap.AddString(serviceName_.ToString());
        publicStatelessService->ServiceTypeName = heap.AddString(serviceTypeName_);
        publicStatelessService->ServiceManifestVersion = heap.AddString(serviceManifestVersion_);
        publicStatelessService->HealthState = healthState_;

        auto publicStatelessServiceEx1 = heap.AddItem<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX1>();
        publicStatelessServiceEx1->ServiceStatus = serviceStatus_;

        auto publicStatelessServiceEx2 = heap.AddItem<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX2>();
        publicStatelessServiceEx2->IsServiceGroup = isServiceGroup_ ? TRUE : FALSE;

        publicStatelessServiceEx1->Reserved = publicStatelessServiceEx2.GetRawPointer();
        publicStatelessService->Reserved = publicStatelessServiceEx1.GetRawPointer();
        publicServiceQueryResult.Value = publicStatelessService.GetRawPointer();
    }
    else if (serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        auto publicStatefulService = heap.AddItem<FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM>();
        publicStatefulService->ServiceName = heap.AddString(serviceName_.ToString());
        publicStatefulService->ServiceTypeName = heap.AddString(serviceTypeName_);
        publicStatefulService->ServiceManifestVersion = heap.AddString(serviceManifestVersion_);
        publicStatefulService->HasPersistedState = hasPersistedState_ ? TRUE : FALSE;    
        publicStatefulService->HealthState = healthState_;

        auto publicStatefulServiceEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX1>();
        publicStatefulServiceEx1->ServiceStatus = serviceStatus_;

        auto publicStatelessServiceEx2 = heap.AddItem<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX2>();
        publicStatelessServiceEx2->IsServiceGroup = isServiceGroup_ ? TRUE : FALSE;

        publicStatefulServiceEx1->Reserved = publicStatelessServiceEx2.GetRawPointer();
        publicStatefulService->Reserved = publicStatefulServiceEx1.GetRawPointer();
        publicServiceQueryResult.Value = publicStatefulService.GetRawPointer();
    }
}

ErrorCode ServiceQueryResult::FromPublicApi(__in FABRIC_SERVICE_QUERY_RESULT_ITEM const& publicServiceQueryResult)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::InvalidArgument);
    serviceKind_ = publicServiceQueryResult.Kind ;
    if (publicServiceQueryResult.Kind == FABRIC_SERVICE_KIND_STATELESS)
    {
        FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM *publicStatelessService =
            reinterpret_cast<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM *>(publicServiceQueryResult.Value);

        if (!Uri::TryParse(publicStatelessService->ServiceName, serviceName_))
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }

        serviceTypeName_ = publicStatelessService->ServiceTypeName;
        serviceManifestVersion_ = publicStatelessService->ServiceManifestVersion;
        healthState_ = publicStatelessService->HealthState;

        if (publicStatelessService->Reserved != NULL)
        {
            FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX1 * publicStatelessServiceEx1 =
                reinterpret_cast<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX1 *>(publicStatelessService->Reserved);

            serviceStatus_ = publicStatelessServiceEx1->ServiceStatus;

            if (publicStatelessServiceEx1->Reserved != NULL)
            {
                FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX2 * publicStatelessServiceEx2 =
                    reinterpret_cast<FABRIC_STATELESS_SERVICE_QUERY_RESULT_ITEM_EX2 *>(publicStatelessServiceEx1->Reserved);
                isServiceGroup_ = publicStatelessServiceEx2->IsServiceGroup ? true : false;
            }
        }
    }
    else if (publicServiceQueryResult.Kind == FABRIC_SERVICE_KIND_STATEFUL)
    {
        FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM *publicStatefulService =
            reinterpret_cast<FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM *>(publicServiceQueryResult.Value);

        if (!Uri::TryParse(publicStatefulService->ServiceName, serviceName_))
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }

        serviceTypeName_ = publicStatefulService->ServiceTypeName;
        serviceManifestVersion_ = publicStatefulService->ServiceManifestVersion;
        hasPersistedState_ = publicStatefulService->HasPersistedState ? true : false;
        healthState_ = publicStatefulService->HealthState;

        if (publicStatefulService->Reserved != NULL)
        {
            FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX1 * publicStatefulServiceEx1 =
                reinterpret_cast<FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX1 *>(publicStatefulService->Reserved);

            serviceStatus_ = publicStatefulServiceEx1->ServiceStatus;

            if (publicStatefulServiceEx1->Reserved != NULL)
            {
                FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX2 * publicStatefulServiceEx2 =
                    reinterpret_cast<FABRIC_STATEFUL_SERVICE_QUERY_RESULT_ITEM_EX2 *>(publicStatefulServiceEx1->Reserved);
                isServiceGroup_ = publicStatefulServiceEx2->IsServiceGroup ? true : false;
            }
        }
    }

    return error;
}

void ServiceQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring ServiceQueryResult::ToString() const
{
    wstring serviceType = L"Invalid";
    wstring serviceString = L"";

    if (serviceKind_ == FABRIC_SERVICE_KIND_STATELESS)
    {
        serviceType = L"Stateless";
        serviceString = wformatString("ServiceName = '{0}', ServiceTypeName = '{1}', ServiceManifestVersion = '{2}', HealthState = '{3}', ServiceStatus = '{4}'",
            serviceName_.ToString(),
            serviceTypeName_,
            serviceManifestVersion_,
            healthState_,
            serviceStatus_);
    }
    else if (serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
    {
        serviceType = L"Stateful";
        serviceString = wformatString("ServiceName = '{0}', ServiceTypeName = '{1}', ServiceManifestVersion = '{2}', HasPersistedState = '{3}, HealthState = '{4}', ServiceStatus = '{5}'",
            serviceName_.ToString(),
            serviceTypeName_,
            serviceManifestVersion_,
            hasPersistedState_,
            healthState_,
            serviceStatus_);
    }

    return wformatString(
        "Type = '{0}', {1}",
        serviceType,
        serviceString);
}

std::wstring ServiceQueryResult::CreateContinuationToken() const
{
    return PagingStatus::CreateContinuationToken<Uri>(serviceName_);
}
