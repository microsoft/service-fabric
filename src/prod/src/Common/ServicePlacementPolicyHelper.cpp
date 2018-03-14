// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

HRESULT ServicePlacementPolicyHelper::PolicyDescriptionToPlacementConstraints(
    __in const FABRIC_PLACEMENT_POLICY_TYPE & policyType,
    __in const std::wstring & domainName,
    __out std::wstring & placementConstraints
    )
{
    switch (policyType)
    {
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
        {
            placementConstraints.append(L"FaultDomain !^ ");
            placementConstraints.append(domainName);
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
        {
            placementConstraints.append(L"FaultDomain ^ ");
            placementConstraints.append(domainName);
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
        {
            placementConstraints.append(L"FaultDomain ^P ");
            placementConstraints.append(domainName);
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
        {
            placementConstraints.append(L"FDPolicy ~ Nonpacking ");
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
        {
            placementConstraints.append(L"PlacePolicy ~ NonPartially");
            break;
           }

    default:
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(
    __in const FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & placementPolicyDescription,
    __out std::wstring & domainName)
{
    switch (placementPolicyDescription.Type)
    {
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
        {
            auto policyDomain = reinterpret_cast<FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN_DESCRIPTION*>(placementPolicyDescription.Value);
            HRESULT hr = StringUtility::LpcwstrToWstring(policyDomain->InvalidFaultDomain, true, domainName);
            if (FAILED(hr)) { return hr; }
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
        {
            auto policyDomain = reinterpret_cast<FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DESCRIPTION*>(placementPolicyDescription.Value);
            HRESULT hr = StringUtility::LpcwstrToWstring(policyDomain->RequiredFaultDomain, true, domainName);
            if (FAILED(hr)) { return hr; }
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
        {
            auto policyDomain = reinterpret_cast<FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN_DESCRIPTION*>(placementPolicyDescription.Value);
            HRESULT hr = StringUtility::LpcwstrToWstring(policyDomain->PreferredPrimaryFaultDomain, true, domainName);
            if (FAILED(hr)) { return hr; }
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
        {
            break;
        }
    case FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
        {
            break;
        }

    default:
        return E_INVALIDARG;
    }

    return S_OK;

}

bool ServicePlacementPolicyHelper::IsValidPlacementPolicyType(const FABRIC_PLACEMENT_POLICY_TYPE & policyType)
{
    return (policyType == FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN ||
        policyType == FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN ||
        policyType == FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN ||
        policyType == FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION ||
        policyType == FABRIC_PLACEMENT_POLICY_TYPE::FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE
        );
}
