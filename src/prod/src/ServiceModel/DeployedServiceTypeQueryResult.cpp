// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DeployedServiceTypeQueryResult::DeployedServiceTypeQueryResult()
    : serviceTypeName_()    
    , codePackageName_()
    , serviceManifestName_()
    , servicePackageActivationId_()
    , registrationStatus_()
{
}

DeployedServiceTypeQueryResult::DeployedServiceTypeQueryResult(
    std::wstring const & serviceTypeName,
    std::wstring const & codePackageName,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    FABRIC_SERVICE_TYPE_REGISTRATION_STATUS registrationStatus)
    : serviceTypeName_(serviceTypeName)    
    , codePackageName_(codePackageName)  
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , registrationStatus_(registrationStatus)
{
}

void DeployedServiceTypeQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM & publicDeployedServiceTypeQueryResult) const 
{
    publicDeployedServiceTypeQueryResult.ServiceTypeName = heap.AddString(serviceTypeName_);
    publicDeployedServiceTypeQueryResult.CodePackageName = heap.AddString(codePackageName_);
    publicDeployedServiceTypeQueryResult.ServiceManifestName = heap.AddString(serviceManifestName_);
    publicDeployedServiceTypeQueryResult.Status = registrationStatus_;

    auto queryResultEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM_EX1>();

    queryResultEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
    queryResultEx1->Reserved = nullptr;
    publicDeployedServiceTypeQueryResult.Reserved = queryResultEx1.GetRawPointer();
}

void DeployedServiceTypeQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring DeployedServiceTypeQueryResult::ToString() const
{
    return wformatString(
        "ServiceTypeName = {0}, CodePackageName = {1}, ServiceManifestName = {2}, ServicePackageActivationId = {3} Status = {4}", 
        serviceTypeName_, 
        codePackageName_,
        serviceManifestName_,
        servicePackageActivationId_,
        static_cast<LONG>(registrationStatus_));
}

Common::ErrorCode DeployedServiceTypeQueryResult::FromPublicApi(__in FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM const & publicDeployedServiceTypeQueryResult)
{
    serviceTypeName_ = wstring(publicDeployedServiceTypeQueryResult.ServiceTypeName);
    codePackageName_ = wstring(publicDeployedServiceTypeQueryResult.CodePackageName);
    serviceManifestName_ = wstring(publicDeployedServiceTypeQueryResult.ServiceManifestName);
    registrationStatus_ = publicDeployedServiceTypeQueryResult.Status;

    if (publicDeployedServiceTypeQueryResult.Reserved == nullptr)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto queryResultEx1 = (FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM_EX1*)publicDeployedServiceTypeQueryResult.Reserved;

    servicePackageActivationId_ = queryResultEx1->ServicePackageActivationId;

    return ErrorCode(ErrorCodeValue::Success);
}
