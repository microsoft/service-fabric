// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;

StringLiteral const TraceFirewallSecurityProvider("FirewallSecurityProvider");
wstring FirewallSecurityProvider::firewallGroup_(L"WindowsFabricApplicationExplicitPort");
LONG FirewallSecurityProvider::allProfiles_[] = { NET_FW_PROFILE2_DOMAIN, NET_FW_PROFILE2_PRIVATE, NET_FW_PROFILE2_PUBLIC };

#if defined(PLATFORM_UNIX)
const int IPTABLES_LINE_SIZE = 2048;
static Common::RwLock IptablesOpLock;

static void IptablesOutputSplit(const string &str, char delimiter, vector<string> &tokens)
{
    stringstream ss;
    ss.str(str);
    string token;
    while (getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }
}

static HRESULT AddIptablesEntryBase(const string &iptables, const string &name, int protocol, int port, bool outgoing)
{
    Common::AcquireWriteLock acquire(IptablesOpLock);
    int retryCnt = 20;
    HRESULT status = S_OK;
    do
    {
        string addcmd(" -I ");
        addcmd += (outgoing ? "OUTPUT 1 " : "INPUT 1 ");
        addcmd += (protocol == NET_FW_IP_PROTOCOL_TCP ? "-p tcp " : "-p udp ");
        addcmd += "--dport " + std::to_string(port) + " ";
        addcmd += "-j ACCEPT ";
        addcmd += "-m comment --comment " + name + " ";
        status = system((iptables + addcmd).c_str());
        if (status)
        {
            retryCnt--;
            Sleep(100);
        }
    } while (status && (retryCnt > 0));
    return status;
}

static HRESULT AddIptablesEntry(const string &name, int protocol, int port, bool outgoing)
{
    return AddIptablesEntryBase("iptables", name, protocol, port, outgoing);
}

static HRESULT AddIp6tablesEntry(const string &name, int protocol, int port, bool outgoing)
{
    return AddIptablesEntryBase("ip6tables", name, protocol, port, outgoing);
}

static HRESULT RemoveIptablesEntryBase(const string &iptables, const string &name)
{
    Common::AcquireWriteLock acquire(IptablesOpLock);
    HRESULT hr = S_OK;
    bool found = false;
    bool retry = false;
    int retryCnt = 20;
    do
    {
        if (retry) Sleep(100);

        retry = found = false;
        string listcmd = iptables + (" -L --line-numbers ");
        FILE *pipe = popen(listcmd.c_str(), "r");
        if (!pipe)
        {
            retry = true;
            retryCnt--;
            hr = errno;
            continue;
        }
        vector<string> lines;
        char buf[IPTABLES_LINE_SIZE];
        while (fgets(buf, IPTABLES_LINE_SIZE, pipe))
        {
            lines.push_back(string(buf));
        }
        hr = pclose(pipe);
        if (hr)
        {
            retry = true;
            retryCnt--;
            continue;
        }

        string chainName;
        string chainTag("Chain");
        for(string line: lines)
        {
            vector<string> segments;
            IptablesOutputSplit(line, ' ', segments);
            if (!segments.size()) continue;
            if (segments[0].compare(chainTag) == 0)
            {
                chainName = segments[1];
                continue;
            }
            if (line.find(name) != string::npos)
            {
                found = true;
                string delcmd = iptables + (" -D ");
                delcmd += chainName + " " + segments[0];
                hr = system(delcmd.c_str());
                if (hr)
                {
                    retry = true;
                    retryCnt--;
                    continue;
                }
            }
            if (found) break;
        }
    } while ((found || retry) && (retryCnt > 0));
    return hr;
}

static HRESULT RemoveIptablesEntry(const string & name)
{
    return RemoveIptablesEntryBase("iptables", name);
}

HRESULT RemoveIp6tablesEntry(const string & name)
{
    return RemoveIptablesEntryBase("ip6tables", name);
}

#endif

FirewallSecurityProvider::FirewallSecurityProvider(
    ComponentRoot const & root)
    :
    profilesEnabled_(),
    RootedObject(root),
    FabricComponent()
{
    if (!SecurityConfig::GetConfig().DisableFirewallRuleForDomainProfile)
    {
        profilesEnabled_.push_back(NET_FW_PROFILE2_DOMAIN);
    }
    if (!SecurityConfig::GetConfig().DisableFirewallRuleForPublicProfile)
    {
        profilesEnabled_.push_back(NET_FW_PROFILE2_PUBLIC);
    }
    if (!SecurityConfig::GetConfig().DisableFirewallRuleForPrivateProfile)
    {
        profilesEnabled_.push_back(NET_FW_PROFILE2_PRIVATE);
    }

#if defined(PLATFORM_UNIX)
    if (profilesEnabled_.size() > 0)
    {
        profilesEnabled_.clear();
        profilesEnabled_.push_back(NET_FW_PROFILE2_PUBLIC);
    }
#endif

    allprotocols_.push_back(NET_FW_IP_PROTOCOL_TCP);
    allprotocols_.push_back(NET_FW_IP_PROTOCOL_UDP);
}

FirewallSecurityProvider::~FirewallSecurityProvider()
{
}

ErrorCode FirewallSecurityProvider::OnOpen()
{
    WriteNoise(TraceFirewallSecurityProvider, Root.TraceId, "Opening FirewallSecurityProvider");
    return ErrorCode(ErrorCodeValue::Success);
}


ErrorCode FirewallSecurityProvider::OnClose()
{
    WriteNoise(TraceFirewallSecurityProvider, Root.TraceId, "Closing FirewallSecurityProvider");
    return ErrorCode(ErrorCodeValue::Success);
}

void FirewallSecurityProvider::OnAbort()
{
    WriteNoise(TraceFirewallSecurityProvider, Root.TraceId, "Aborting FirewallSecurityProvider.");
}

ErrorCode FirewallSecurityProvider::ConfigurePortFirewallPolicy(
    wstring const & policyName,
    vector<LONG> ports)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteInfo(
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "FirewallSecurityProvider cannot open ports for policy '{0}' since provider is not open", policyName);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    HRESULT hr = S_OK;

#if defined(PLATFORM_UNIX)
    auto err = CleanupRulesForAllProfiles(policyName, 0);
    if (!err.IsSuccess())
    {
        WriteInfo(
                TraceFirewallSecurityProvider,
                Root.TraceId,
                "Failed to remove firewall rule, error {0}, rulename", err, policyName);
    }

    wstring ruleDescription = wformatString("Rule for WindowsFabric service package {0}", policyName);
    for (auto it = profilesEnabled_.begin(); it != profilesEnabled_.end() && SUCCEEDED(hr); ++it)
    {
        for (auto iter = ports.begin(); iter != ports.end(); ++iter)
        {
            for (auto it1 = allprotocols_.begin(); it1 != allprotocols_.end(); it1++)
            {
                hr = this->AddRule(0,
                                   FirewallSecurityProvider::GetFirewallRuleName(policyName, false, *it, *it1),
                                   ruleDescription,
                                   *it,
                                   *it1,
                                   *iter,
                                   false);
                if (SUCCEEDED(hr))
                {
                    hr = this->AddRule(0,
                                       FirewallSecurityProvider::GetFirewallRuleName(policyName, true, *it, *it1),
                                       ruleDescription,
                                       *it,
                                       *it1,
                                       *iter,
                                       true);
                }
                if (FAILED(hr))
                {
                    break;
                }
            }
        }
    }
    return ErrorCode::FromHResult(hr);
#else

    hr = CoInitializeEx(
        0,
        COINIT_MULTITHREADED
    );
    if (FAILED(hr))
    {
        ErrorCode error = ErrorCode::FromHResult(hr);
        WriteError(TraceFirewallSecurityProvider, Root.TraceId, "Failed to create CoInitialize {0}", error);
        return error;
    }
    INetFwPolicy2* fwPolicy;
    hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        (void**)&fwPolicy);
    if (FAILED(hr))
    {
        ErrorCode error = ErrorCode::FromHResult(hr);
        WriteError(TraceFirewallSecurityProvider, Root.TraceId, "Failed to create INetFwPolicy2 {0}", error);
        return error;
    }
    INetFwRules *pFwRules = NULL;

    LONG currentProfilesBitMask = 0;

    // Retrieve INetFwRules
    hr = fwPolicy->get_Rules(&pFwRules);
    if (FAILED(hr))
    {
        WriteWarning(
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "Failed to get firewall rules, hresult {0}", hr);
    }
    else
    {
        auto err = CleanupRulesForAllProfiles(policyName, pFwRules);
        if (!err.IsSuccess())
        {
            WriteInfo(
                TraceFirewallSecurityProvider,
                Root.TraceId,
                "Failed to remove firewall rule, error {0}, rulename", err, policyName);
        }
        // Retrieve Current Profiles bitmask
        hr = fwPolicy->get_CurrentProfileTypes(&currentProfilesBitMask);
        if (SUCCEEDED(hr))
        {

            wstring ruleDescription = wformatString("Rule for WindowsFabric service package {0}", policyName);
            bool currentUserProfileEnabled = false;
            for (auto it = profilesEnabled_.begin(); it != profilesEnabled_.end() && SUCCEEDED(hr); ++it)
            {
                for (auto iter = ports.begin(); iter != ports.end(); ++iter)
                {
                    for (auto it1 = allprotocols_.begin(); it1 != allprotocols_.end(); it1++)
                    {
                        hr = this->AddRule(
                            pFwRules,
                            FirewallSecurityProvider::GetFirewallRuleName(policyName, false, *it, *it1),
                            ruleDescription,
                            *it,
                            *it1,
                            *iter,
                            false);
                        if (SUCCEEDED(hr))
                        {
                            hr = this->AddRule(
                                pFwRules,
                                FirewallSecurityProvider::GetFirewallRuleName(policyName, true, *it, *it1),
                                ruleDescription,
                                *it,
                                *it1,
                                *iter,
                                true);
                        }
                        if (FAILED(hr))
                        {
                            break;
                        }
                    }
                }
                if (*it & currentProfilesBitMask)
                {
                    currentUserProfileEnabled = true;
                }
            }
            if (!currentUserProfileEnabled)
            {
                WriteWarning(
                    TraceFirewallSecurityProvider,
                    Root.TraceId,
                    "Did not enable firewallpolicy for current profile {0}", currentProfilesBitMask);
            }
        }
        else
        {
            WriteWarning(
                TraceFirewallSecurityProvider,
                Root.TraceId,
                "Failed to get current profile types, hresult {0}", hr);
        }
        // Release the INetFwRules object
        if (pFwRules != NULL)
        {
            pFwRules->Release();
        }
        if (fwPolicy != NULL)
        {
            fwPolicy->Release();
        }
    }
    return ErrorCode::FromHResult(hr);
#endif    
}


HRESULT FirewallSecurityProvider::AddRule(
    INetFwRules* pFwRules,
    wstring const & policyName,
    wstring const & ruleDescription,
    LONG currentProfilesBitMask,
    LONG protocol,
    LONG port,
    bool outgoing)
{
#if defined(PLATFORM_UNIX)
    string policyNameA = StringUtility::Utf16ToUtf8(policyName);
    HRESULT hr = AddIptablesEntry(policyNameA, protocol, port, outgoing);
    if (FAILED(hr))
    {
        WriteWarning(TraceFirewallSecurityProvider, Root.TraceId, "Failed to add firewall rule (ipv4), hresult {0}", hr);
    }
    else
    {
        hr = AddIp6tablesEntry(policyNameA, protocol, port, outgoing);
        if (FAILED(hr))
        {
            WriteWarning(TraceFirewallSecurityProvider, Root.TraceId, "Failed to add firewall rule (ipv6), hresult {0}", hr);
        }
    }
    return hr;
#else 
    HRESULT hr = S_OK;
    INetFwRule *pFwRule = NULL;


    hr = CoCreateInstance(
        __uuidof(NetFwRule),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwRule),
        (void**)&pFwRule);
    if (SUCCEEDED(hr))
    {

        // Populate the Firewall Rule object
        pFwRule->put_Name((BSTR)policyName.c_str());
        pFwRule->put_Description((BSTR)ruleDescription.c_str());
        pFwRule->put_ApplicationName(nullptr);
        pFwRule->put_Protocol(protocol);
        pFwRule->put_LocalPorts((BSTR)StringUtility::ToWString<UINT>(port).c_str());
        if (outgoing)
        {
            pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);
        }
        else
        {
            pFwRule->put_Direction(NET_FW_RULE_DIR_IN);
        }
        pFwRule->put_Grouping((BSTR)FirewallSecurityProvider::firewallGroup_.c_str());
        pFwRule->put_Profiles(currentProfilesBitMask);
        pFwRule->put_Action(NET_FW_ACTION_ALLOW);
        pFwRule->put_Enabled(VARIANT_TRUE);

        // Add the Firewall Rule
        hr = pFwRules->Add(pFwRule);
        if (FAILED(hr))
        {
            WriteWarning(
                TraceFirewallSecurityProvider,
                Root.TraceId,
                "Failed to add firewall rule, hresult {0}", hr);
        }
        if (pFwRule != NULL)
        {
            pFwRule->Release();
        }
    }
    else
    {
        WriteWarning(
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "Failed to create firewall rule instance, hresult {0}", hr);
    }
    return hr;
#endif    
}


ErrorCode FirewallSecurityProvider::RemoveFirewallRule(
    wstring const & policyName)
{
#if defined(PLATFORM_UNIX)
    return CleanupRulesForAllProfiles(policyName, 0);
#else
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteInfo(
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "FirewallSecurityProvider cannot remove rule '{0}' since provider is not open", policyName);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    HRESULT hr = S_OK;

    hr = CoInitializeEx(
        0,
        COINIT_MULTITHREADED
    );
    if (FAILED(hr))
    {
        ErrorCode error = ErrorCode::FromHResult(hr);
        WriteError(TraceFirewallSecurityProvider, Root.TraceId, "Failed to create CoInitialize {0}", error);
        return error;
    }

    INetFwPolicy2* fwPolicy;
    hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        (void**)&fwPolicy);
    if (FAILED(hr))
    {
        ErrorCode error = ErrorCode::FromHResult(hr);
        WriteError(TraceFirewallSecurityProvider, Root.TraceId, "Failed to create INetFwPolicy2 {0}", error);
        return error;
    }

    INetFwRules *pFwRules = NULL;

    // Retrieve INetFwRules
    hr = fwPolicy->get_Rules(&pFwRules);
    if (FAILED(hr))
    {
        WriteWarning(
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "Failed to get firewall rules, hresult {0}", hr);
        return ErrorCode::FromHResult(hr);
    }
    else
    {
        auto error = CleanupRulesForAllProfiles(policyName, pFwRules);
        WriteTrace(
            error.ToLogLevel(),
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "Removing firewall rules for policy {0} returned error {1}", policyName, error);
        // Release the INetFwRules object
        if (pFwRules != NULL)
        {
            pFwRules->Release();
        }
        if (fwPolicy != NULL)
        {
            fwPolicy->Release();
        }
        return error;
    }
#endif    
}

ErrorCode FirewallSecurityProvider::CleanupRulesForAllProfiles(wstring const & policyName, INetFwRules* pFwRules)
{
#if defined(PLATFORM_UNIX)
    HRESULT hr = S_OK;
    for (auto iter = profilesEnabled_.begin(); iter != profilesEnabled_.end(); ++iter)
    {
        for (auto it = allprotocols_.begin(); it != allprotocols_.end(); it++)
        {
            for (int dir = 0; dir <= 1; dir++)
            {
                string policyNameA = StringUtility::Utf16ToUtf8(FirewallSecurityProvider::GetFirewallRuleName(policyName, dir, *iter, *it));
                hr = RemoveIptablesEntry(policyNameA);
                if (FAILED(hr))
                {
                    WriteWarning(TraceFirewallSecurityProvider, Root.TraceId,
                                 "Failed to remove incoming firewall rule (ipv4), hresult {0} rule {1}", hr, policyName);
                }
                hr = RemoveIp6tablesEntry(policyNameA);
                if (FAILED(hr))
                {
                    WriteWarning(TraceFirewallSecurityProvider, Root.TraceId,
                                 "Failed to remove incoming firewall rule (ipv6), hresult {0} rule {1}", hr, policyName);
                }
            }
        }
    }
    return ErrorCode::FromHResult(hr);
#else
    HRESULT hr = S_OK;
    for (auto iter = profilesEnabled_.begin(); iter != profilesEnabled_.end(); ++iter)
    {
        for (auto it = allprotocols_.begin(); it != allprotocols_.end(); it++)
        {
            hr = pFwRules->Remove((BSTR)FirewallSecurityProvider::GetFirewallRuleName(policyName, false, *iter, *it).c_str());
            if (FAILED(hr))
            {

                WriteWarning(
                    TraceFirewallSecurityProvider,
                    Root.TraceId,
                    "Failed to remove incoming firewall rule , hresult {0} rule {1}", hr, policyName);
            }
            hr = pFwRules->Remove((BSTR)FirewallSecurityProvider::GetFirewallRuleName(policyName, true, *iter, *it).c_str());
            if (FAILED(hr))
            {

                WriteWarning(
                    TraceFirewallSecurityProvider,
                    Root.TraceId,
                    "Failed to remove outgoing firewall rule , hresult {0} rule {1}", hr, policyName);
            }
        }
    }
    return ErrorCode::FromHResult(hr);
#endif
}

wstring FirewallSecurityProvider::GetFirewallRuleName(
    wstring const & packageId,
    bool outgoing,
    LONG profileType,
    LONG protocol)
{
    wstring profileName = L"";

    if (profileType == NET_FW_PROFILE2_PRIVATE)
    {
        profileName = L"Private";
    }
    else if (profileType == NET_FW_PROFILE2_PUBLIC)
    {
        profileName = L"Public";
    }
    else if (profileType == NET_FW_PROFILE2_DOMAIN)
    {
        profileName = L"Domain";
    }
    if (protocol == NET_FW_IP_PROTOCOL_TCP)
    {
        profileName = wformatString("{0}_TCP", profileName);
    }
    else if (protocol == NET_FW_IP_PROTOCOL_UDP)
    {
        profileName = wformatString("{0}_UDP", profileName);
    }
    if (outgoing)
    {
        return wformatString("{0}_{1}_Out", packageId, profileName);
    }
    else
    {
        return wformatString("{0}_{1}_IN", packageId, profileName);
    }
}
