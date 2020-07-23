//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Microsoft::WRL;

StringLiteral TraceType_FirewallSecurityProviderHelper = "FirewallSecurityProviderHelper";
wstring FirewallSecurityProviderHelper::firewallGroup_(L"WindowsFabricApplicationExplicitPort");

ErrorCode FirewallSecurityProviderHelper::GetRules(INetFwPolicy2** fwPolicy, INetFwRules** pFwRules)
{
    TraceInfo(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Trying to GetRules");

    *fwPolicy = nullptr;
    *pFwRules = nullptr;

    ComPtr<INetFwPolicy2> fwPolicyLocal;
    ComPtr<INetFwRules> pFwRulesLocal;

    auto hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        (void**)&fwPolicyLocal);

    if (FAILED(hr))
    {
        ErrorCode error = ErrorCode::FromHResult(hr);
        TraceError(
            TraceTaskCodes::Hosting,
            TraceType_FirewallSecurityProviderHelper,
            "Failed to create INetFwPolicy2 {0}",
            error);
        return error;
    }

    TraceInfo(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Invoking GetRules");

    // Retrieve INetFwRules
    hr = fwPolicyLocal->get_Rules(&pFwRulesLocal);
    if (FAILED(hr))
    {
        TraceWarning(
            TraceTaskCodes::Hosting,
            TraceType_FirewallSecurityProviderHelper,
            "Failed to get firewall rules, hresult {0}",
            hr);
        return ErrorCode::FromHResult(hr);
    }

    long fwRuleCount;
    hr = pFwRulesLocal->get_Count(&fwRuleCount);
    if (FAILED(hr))
    {
        TraceError(
            TraceTaskCodes::Hosting,
            TraceType_FirewallSecurityProviderHelper,
            "Failed to get number of firewall rules, hresult {0}",
            hr);
        return ErrorCode::FromHResult(hr);
    }

    TraceInfo(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Total number of firewall rules in system {0}",
        fwRuleCount);

    *fwPolicy = fwPolicyLocal.Detach();
    *pFwRules = pFwRulesLocal.Detach();

    return ErrorCodeValue::Success;
}

ErrorCode FirewallSecurityProviderHelper::GetOrRemoveFirewallRules(
    vector<BSTR> & rules,
    wstring const& groupName,
    uint64 nodePrefix,
    bool isRemove)
{
    auto error = InitializeCOM();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = GetOrRemoveFirewallRulesInternal(rules, groupName, nodePrefix, isRemove);

    UninitializeCOM();
    return error;
}

//This method should perform synchronous operations and COM should be initialize before.
ErrorCode FirewallSecurityProviderHelper::GetOrRemoveFirewallRulesInternal(
    vector<BSTR> & rules,
    wstring const& groupName,
    uint64 nodePrefix,
    bool isRemove)
{
    ComPtr<INetFwPolicy2> fwPolicy;
    ComPtr<INetFwRules> pFwRules;

    auto error = GetRules(&fwPolicy, &pFwRules);
    if (!error.IsSuccess())
    {
        return error;
    }

    long fwRuleCount;
    IUnknown *pEnumerator;
    ComPtr<INetFwRule2> pFwRule;
    ComPtr<IEnumVARIANT> pVariant;
    ULONG cFetched = 0;
    CComVariant var;
    BSTR bstrName;
    BSTR bstrDescription;

    vector<wstring> ruleToRemove;
    // Obtain the number of Firewall rules
    auto hr = pFwRules->get_Count(&fwRuleCount);
    if (FAILED(hr))
    {
        TraceError(
            TraceTaskCodes::Hosting,
            TraceType_FirewallSecurityProviderHelper,
            "Failed to get number of firewall rules, hresult {0}",
            hr);
        return ErrorCode::FromHResult(hr);
    }

    TraceInfo(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Total number of firewall rules {0}",
        fwRuleCount);

    // Iterate through all of the rules in pFwRules
    pFwRules->get__NewEnum(&pEnumerator);

    if (pEnumerator)
    {
        hr = pEnumerator->QueryInterface(__uuidof(IEnumVARIANT), (void **)&pVariant);
    }

    while (SUCCEEDED(hr) && hr != S_FALSE)
    {
        var.Clear();
        hr = pVariant->Next(1, &var, &cFetched);

        if (S_FALSE != hr)
        {
            if (SUCCEEDED(hr))
            {
                hr = var.ChangeType(VT_DISPATCH);
            }
            if (SUCCEEDED(hr))
            {
                hr = (V_DISPATCH(&var))->QueryInterface(__uuidof(INetFwRule), (void**)&pFwRule);
            }

            if (SUCCEEDED(hr))
            {
                // Output the properties of this rule
                hr = pFwRule->get_Name(&bstrName);
                if (SUCCEEDED(hr))
                {
                    if (nodePrefix!=0 && StringUtility::StartsWith(((wstring)bstrName), wformatString(nodePrefix)))
                    {
                        rules.push_back(bstrName);
                        TraceInfo(
                            TraceTaskCodes::Hosting,
                            TraceType_FirewallSecurityProviderHelper,
                            "Adding rule to remove {0}",
                            bstrName);
                    }
                    else
                    {
                        hr = pFwRule->get_Grouping(&bstrDescription);
                        if (SUCCEEDED(hr) && bstrDescription)
                        {
                            if (!groupName.empty() && StringUtility::AreEqualCaseInsensitive((wstring)bstrDescription, groupName))
                            {
                                rules.push_back(bstrName);
                                TraceInfo(
                                    TraceTaskCodes::Hosting,
                                    TraceType_FirewallSecurityProviderHelper,
                                    "Adding rule to remove {0}",
                                    bstrName);
                            }
                        }
                    }
                }
            }
        }
    }

    if (isRemove)
    {
        error = RemoveRulesInternal(rules, pFwRules);
    }

    return error;
}

ErrorCode  FirewallSecurityProviderHelper::RemoveRules(
    vector<BSTR> const& rules,
    ComPtr<INetFwRules> pFwRules)
{
    auto error = InitializeCOM();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = RemoveRulesInternal(rules, pFwRules);

    UninitializeCOM();

    return error;
}

//This method should perform synchronous operations and COM should be initialize before.
ErrorCode FirewallSecurityProviderHelper::RemoveRulesInternal(
    vector<BSTR> const& rules,
    ComPtr<INetFwRules> pFwRules)
{
    ComPtr<INetFwPolicy2> fwPolicy;

    TraceInfo(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Number of Firewall Rules to Remove {0}", rules.size());

    if (pFwRules == nullptr)
    {
        auto error = GetRules(&fwPolicy, &pFwRules);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    for (auto it = rules.begin(); it != rules.end(); it++)
    {
        auto hr = pFwRules->Remove(*it);
        if (FAILED(hr))
        {
            TraceWarning(
                TraceTaskCodes::Hosting,
                TraceType_FirewallSecurityProviderHelper,
                "Failed to remove outgoing firewall rule , hresult {0} rule {1}", hr, (wstring)*it);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode FirewallSecurityProviderHelper::InitializeCOM()
{
    TraceNoise(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Invoking CoInitializeEx");

    HRESULT hr = CoInitializeEx(
        0,
        COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        ErrorCode error = ErrorCode::FromHResult(hr);
        TraceError(
            TraceTaskCodes::Hosting,
            TraceType_FirewallSecurityProviderHelper,
            "Failed to CoInitialize {0}",
            error);
        return error;
    }

    TraceNoise(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Done Invoking CoInitializeEx");

    return ErrorCodeValue::Success;
}

void FirewallSecurityProviderHelper::UninitializeCOM()
{
    TraceNoise(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Invoking CoUnInitializeEx");
    
    CoUninitialize();

    TraceNoise(
        TraceTaskCodes::Hosting,
        TraceType_FirewallSecurityProviderHelper,
        "Done Invoking CoUnInitializeEx");
}

