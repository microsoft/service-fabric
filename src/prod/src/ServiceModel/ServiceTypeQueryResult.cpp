// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceTypeQueryResult::ServiceTypeQueryResult() 
    : serviceTypeDescription_()
    , serviceManifestVersion_()
    , isServiceGroup_(false)
{
}

ServiceTypeQueryResult::ServiceTypeQueryResult(
    ServiceTypeDescription const & serviceTypeDescription, 
    wstring const & serviceManifestVersion,
    wstring const & serviceManifestName)
    : serviceTypeDescription_(serviceTypeDescription)
    , serviceManifestVersion_(serviceManifestVersion)
    , serviceManifestName_(serviceManifestName)
    , isServiceGroup_(false)
{
}

void ServiceTypeQueryResult::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM & publicServiceTypeQueryResult) const
{
    publicServiceTypeQueryResult.ServiceManifestVersion = heap.AddString(serviceManifestVersion_);

    auto publicServiceTypeDescription = heap.AddItem<FABRIC_SERVICE_TYPE_DESCRIPTION>();
    serviceTypeDescription_.ToPublicApi(heap,*publicServiceTypeDescription);
    publicServiceTypeQueryResult.ServiceTypeDescription = publicServiceTypeDescription.GetRawPointer();

    auto publicServiceTypeQueryResultEx1 = heap.AddItem<FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_EX1>();
    publicServiceTypeQueryResult.Reserved = publicServiceTypeQueryResultEx1.GetRawPointer();

    publicServiceTypeQueryResultEx1->ServiceManifestName = heap.AddString(serviceManifestName_);

    auto publicServiceTypeQueryResultEx2 = heap.AddItem<FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_EX2>();
    publicServiceTypeQueryResultEx1->Reserved = publicServiceTypeQueryResultEx2.GetRawPointer();
    publicServiceTypeQueryResultEx2->IsServiceGroup = isServiceGroup_ ? TRUE : FALSE;
}

ErrorCode ServiceTypeQueryResult::FromPublicApi(__in FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM const& publicServiceTypeQueryResult)
{
    auto hr = StringUtility::LpcwstrToWstring(
        publicServiceTypeQueryResult.ServiceManifestVersion,
        false /*acceptNull*/, 
        ParameterValidator::MinStringSize,
        ParameterValidator::MaxFilePathSize,
        serviceManifestVersion_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    if (publicServiceTypeQueryResult.Reserved != NULL)
    {
        auto publicServiceTypeQueryResultEx = reinterpret_cast<FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_EX1 *>(publicServiceTypeQueryResult.Reserved);

        hr = StringUtility::LpcwstrToWstring(
            publicServiceTypeQueryResultEx->ServiceManifestName,
            false /*acceptNull*/, 
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxFilePathSize,
            serviceManifestName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        if (publicServiceTypeQueryResultEx->Reserved != NULL)
        {
            auto publicServiceTypeQueryResultEx2 = reinterpret_cast<FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_EX2 *>(publicServiceTypeQueryResultEx->Reserved);

            isServiceGroup_ = publicServiceTypeQueryResultEx2->IsServiceGroup ? true : false;
        }
    }

    auto publicServiceTypeDescription = reinterpret_cast<FABRIC_SERVICE_TYPE_DESCRIPTION *>(publicServiceTypeQueryResult.ServiceTypeDescription);
    
    return serviceTypeDescription_.FromPublicApi(*publicServiceTypeDescription);
}

void ServiceTypeQueryResult::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ServiceTypeQueryResult::ToString() const
{
   return wformatString(
       "ServiceTypeDescription = '{0}'; ServiceManifestVersion = '{1}'; ServiceManifestName = '{2}", 
       serviceTypeDescription_.ToString(), 
       serviceManifestVersion_,
       serviceManifestName_);    
}
