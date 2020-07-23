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
#ifndef PLATFORM_UNIX
using namespace Microsoft::WRL;
#endif

StringLiteral const TraceFirewallSecurityProvider("FirewallSecurityProvider");

const std::wstring LocalNatPolicyName = L"Local_Nat_As_Open";
const std::wstring LocalNatPolicyDescription = L"Dev scenario rule for NAT network for emulating Open Network.";

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
    vector<LONG> ports,
    uint64 nodeInstanceId)
{
#ifdef PLATFORM_UNIX
    return ConfigurePortFirewallPolicyInternal(policyName, ports, nodeInstanceId, true);
#else
    //Initalize COM before accessing Firewall api
    auto error = FirewallSecurityProviderHelper::InitializeCOM();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = ConfigurePortFirewallPolicyInternal(policyName, ports, nodeInstanceId, true);

    FirewallSecurityProviderHelper::UninitializeCOM();

    return error;
#endif
}

//This method should perform synchronous operations and COM should be initialize before.
ErrorCode FirewallSecurityProvider::ConfigurePortFirewallPolicyInternal(
    wstring const & policyName,
    vector<LONG> ports,
    uint64 nodeInstanceId,
    bool isComInitialized)
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
    auto err = CleanupRulesForAllProfiles(policyName, ports, nodeInstanceId);
    if (!err.IsSuccess())
    {
        WriteInfo(
                TraceFirewallSecurityProvider,
                Root.TraceId,
                "Failed to remove firewall rule, error {0}, rulename", err, policyName);
    }

    wstring ruleDescription = wformatString("Rule for WindowsFabric service package {0}", policyName);
    for (auto const& profile : profilesEnabled_)
    {
        if (SUCCEEDED(hr))
        {
            for (auto const& port : ports)
            {
                for (auto const& protocol : allprotocols_)
                {
                    wstring firewallRuleName = FirewallSecurityProvider::GetFirewallRuleName(policyName, false, profile, protocol, port, nodeInstanceId);
                    WriteInfo(
                        TraceFirewallSecurityProvider,
                        Root.TraceId,
                        "Adding firewall rule for port: {0} RuleName: {1}", port, firewallRuleName);

                    hr = this->AddRule(firewallRuleName,
                        ruleDescription,
                        profile,
                        protocol,
                        port,
                        false);
                    if (SUCCEEDED(hr))
                    {
                        firewallRuleName = FirewallSecurityProvider::GetFirewallRuleName(policyName, true, profile, protocol, port, nodeInstanceId);

                        WriteInfo(
                            TraceFirewallSecurityProvider,
                            Root.TraceId,
                            "Adding firewall rule for port: {0} RuleName: {1}", port, firewallRuleName);

                        hr = this->AddRule(firewallRuleName,
                            ruleDescription,
                            profile,
                            protocol,
                            port,
                            true);
                    }
                    if (FAILED(hr))
                    {
                        WriteWarning(
                            TraceFirewallSecurityProvider,
                            Root.TraceId,
                            "Failed adding firewall rule for port: {0} RuleName: {1}", port, firewallRuleName);
                        break;
                    }
                    else
                    {
                        WriteInfo(
                            TraceFirewallSecurityProvider,
                            Root.TraceId,
                            "Added firewall rule for port: {0} RuleName: {1}", port, firewallRuleName);
                    }
                }
            }
        }
    }
    return ErrorCode::FromHResult(hr);
#else

    ASSERT_IF(!isComInitialized, "COM should be initialized");

    ComPtr<INetFwPolicy2> fwPolicy;
    ComPtr<INetFwRules> pFwRules;
    LONG currentProfilesBitMask = 0;

    auto error = FirewallSecurityProviderHelper::GetRules(&fwPolicy, &pFwRules);
    if (!error.IsSuccess())
    {
        return error;
    }
    else
    {
        error = CleanupRulesForAllProfiles(policyName, pFwRules, ports, nodeInstanceId);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceFirewallSecurityProvider,
                Root.TraceId,
                "Failed to remove firewall rule, error {0}, rulename", error, policyName);
        }
        // Retrieve Current Profiles bitmask
        hr = fwPolicy->get_CurrentProfileTypes(&currentProfilesBitMask);
        if (SUCCEEDED(hr))
        {
            wstring ruleDescription = wformatString("Rule for WindowsFabric service package {0}", policyName);
            bool currentUserProfileEnabled = false;
            for (auto const& profile : profilesEnabled_)
            {
                if (SUCCEEDED(hr))
                {
                    for (auto const& port : ports)
                    {
                        for (auto const& protocol : allprotocols_)
                        {
                            wstring firewallRuleName = FirewallSecurityProvider::GetFirewallRuleName(policyName, false, profile, protocol, port, nodeInstanceId);

                            WriteInfo(
                                TraceFirewallSecurityProvider,
                                Root.TraceId,
                                "Adding firewall rule {0} for port {1}", firewallRuleName, port);

                            hr = this->AddRule(
                                pFwRules,
                                firewallRuleName,
                                ruleDescription,
                                profile,
                                protocol,
                                port,
                                false);

                            if (SUCCEEDED(hr))
                            {
                                WriteInfo(
                                    TraceFirewallSecurityProvider,
                                    Root.TraceId,
                                    "Added firewall rule {0} port:{1}", firewallRuleName, port);

                                firewallRuleName = FirewallSecurityProvider::GetFirewallRuleName(policyName, true, profile, protocol, port, nodeInstanceId);

                                WriteInfo(
                                    TraceFirewallSecurityProvider,
                                    Root.TraceId,
                                    "Adding firewall rule {0} for port {1}", firewallRuleName, port);

                                hr = this->AddRule(
                                    pFwRules,
                                    firewallRuleName,
                                    ruleDescription,
                                    profile,
                                    protocol,
                                    port,
                                    true);
                            }
                            if (FAILED(hr))
                            {
                                WriteInfo(
                                    TraceFirewallSecurityProvider,
                                    Root.TraceId,
                                    "Failed adding firewall rule for port: {0} RuleName: {1}", port, firewallRuleName);

                                break;
                            }
                            else
                            {
                                WriteInfo(
                                    TraceFirewallSecurityProvider,
                                    Root.TraceId,
                                    "Added firewall rule {0} port:{1}", firewallRuleName, port);
                            }
                        }
                    }
                }

                if (profile & currentProfilesBitMask)
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
    }

    return ErrorCode::FromHResult(hr);
#endif    
}

#if defined(PLATFORM_UNIX)
HRESULT FirewallSecurityProvider::AddRule(
    wstring const & policyName,
    wstring const & ruleDescription,
    LONG currentProfilesBitMask,
    LONG protocol,
    LONG port,
    bool outgoing)
{
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
}
#else
HRESULT FirewallSecurityProvider::AddRule(
    ComPtr<INetFwRules> pFwRules,
    wstring const & policyName,
    wstring const & ruleDescription,
    LONG currentProfilesBitMask,
    LONG protocol,
    LONG port,
    bool outgoing)
{
    HRESULT hr = S_OK;
    ComPtr<INetFwRule> pFwRule = nullptr;

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
        pFwRule->put_Grouping((BSTR)FirewallSecurityProviderHelper::firewallGroup_.c_str());
        pFwRule->put_Profiles(currentProfilesBitMask);
        pFwRule->put_Action(NET_FW_ACTION_ALLOW);
        pFwRule->put_Enabled(VARIANT_TRUE);

        // Add the Firewall Rule
        hr = pFwRules->Add(pFwRule.Get());
        if (FAILED(hr))
        {
            WriteWarning(
                TraceFirewallSecurityProvider,
                Root.TraceId,
                "Failed to add firewall rule, hresult {0}", hr);
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
}

ErrorCode FirewallSecurityProvider::AddLocalNatFirewallRule(
    wstring const & localAddress,
    wstring const & remoteAddress)
{
    if (!HostingConfig::GetConfig().LocalNatIpProviderEnabled)
    {
        WriteWarning(TraceFirewallSecurityProvider,
            "LocalNatIpProviderEnabled must be true to AddLocalNatFirewallRule.");
        return ErrorCodeValue::OperationFailed;
    }
    //Add firewall rule for container to host communication
    auto error = FirewallSecurityProviderHelper::InitializeCOM();
    if (error.IsSuccess())
    {
        Microsoft::WRL::ComPtr<INetFwPolicy2> fwPolicy;
        Microsoft::WRL::ComPtr<INetFwRules> pFwRules;
        error = FirewallSecurityProviderHelper::GetRules(&fwPolicy, &pFwRules);
        if (error.IsSuccess())
        {
            auto hr = FirewallSecurityProvider::AddRuleWithLocalAddress(
                                            pFwRules,
                                            LocalNatPolicyName,
                                            LocalNatPolicyDescription,
                                            localAddress,
                                            remoteAddress,
                                            false);
            return ErrorCode::FromHResult(hr);
        }

        FirewallSecurityProviderHelper::UninitializeCOM();
    }
    return error;
}

HRESULT FirewallSecurityProvider::AddRuleWithLocalAddress(
    ComPtr<INetFwRules> pFwRules,
    wstring const & policyName,
    wstring const & ruleDescription,
    wstring const & localAddress,
    wstring const & remoteAddress,
    bool outgoing)
{
    HRESULT hr = S_OK;
    ComPtr<INetFwRule> pFwRule = nullptr;

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
        if (outgoing)
        {
            pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);
        }
        else
        {
            pFwRule->put_Direction(NET_FW_RULE_DIR_IN);
        }
        pFwRule->put_Grouping((BSTR)FirewallSecurityProviderHelper::firewallGroup_.c_str());
        pFwRule->put_Action(NET_FW_ACTION_ALLOW);
        pFwRule->put_Enabled(VARIANT_TRUE);
        pFwRule->put_LocalAddresses((BSTR)localAddress.c_str());
        pFwRule->put_RemoteAddresses((BSTR)remoteAddress.c_str());


        // Add the Firewall Rule
        hr = pFwRules->Add(pFwRule.Get());
        if (FAILED(hr))
        {
            WriteWarning(
                TraceFirewallSecurityProvider,
                "Failed to add firewall rule, hresult {0}", hr);
        }
    }
    else
    {
        WriteWarning(
            TraceFirewallSecurityProvider,
            "Failed to create firewall rule instance, hresult {0}", hr);
    }
    return hr;
}

ErrorCode FirewallSecurityProvider::RemoveLocalNatFirewallRule()
{
    if (!HostingConfig::GetConfig().LocalNatIpProviderEnabled)
    {
        WriteWarning(TraceFirewallSecurityProvider,
            "LocalNatIpProviderEnabled must be true to RemoveLocalNatFirewallRule.");
        return ErrorCodeValue::OperationFailed;
    } 
    auto error = FirewallSecurityProviderHelper::InitializeCOM();
    if (error.IsSuccess())
    {
        Microsoft::WRL::ComPtr<INetFwPolicy2> fwPolicy;
        Microsoft::WRL::ComPtr<INetFwRules> pFwRules;
        error = FirewallSecurityProviderHelper::GetRules(&fwPolicy, &pFwRules);
        if (error.IsSuccess())
        {
            HRESULT hr = S_OK;

            WriteInfo(TraceFirewallSecurityProvider,
                "Removing rule {0}", LocalNatPolicyName);

            hr = pFwRules->Remove((BSTR)LocalNatPolicyName.c_str());
            return ErrorCode::FromHResult(hr);
        }
        FirewallSecurityProviderHelper::UninitializeCOM();
    }
    return error;
}
#endif // defined(PLATFORM_UNIX)

ErrorCode FirewallSecurityProvider::RemoveFirewallRule(
    wstring const & policyName,
    vector<LONG> const& ports,
    uint64 nodeInstanceId)
{
#ifdef PLATFORM_UNIX
    return RemoveFirewallRuleInternal(policyName, ports, nodeInstanceId, true);
#else
    //Initalize COM before accessing Firewall api
    auto error = FirewallSecurityProviderHelper::InitializeCOM();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = RemoveFirewallRuleInternal(policyName, ports, nodeInstanceId, true);

    FirewallSecurityProviderHelper::UninitializeCOM();

    return error;
#endif
}

//This method should perform synchronous operations and COM should be initialize before.
ErrorCode FirewallSecurityProvider::RemoveFirewallRuleInternal(
    wstring const & policyName,
    vector<LONG> const& ports,
    uint64 nodeInstanceId,
    bool isComInitialized)
{
    if (this->State.Value != FabricComponentState::Opened)
    {
        WriteInfo(
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "FirewallSecurityProvider cannot remove rule '{0}' since provider is not open", policyName);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

#if defined(PLATFORM_UNIX)
    return CleanupRulesForAllProfiles(policyName, ports, nodeInstanceId);
#else

    ASSERT_IF(!isComInitialized, "COM should be initialized");

    ComPtr<INetFwPolicy2> fwPolicy;
    ComPtr<INetFwRules> pFwRules;

    auto error = FirewallSecurityProviderHelper::GetRules(&fwPolicy, &pFwRules);
    if (error.IsSuccess())
    {
        error = CleanupRulesForAllProfiles(policyName, pFwRules, ports, nodeInstanceId);
        WriteTrace(
            error.ToLogLevel(),
            TraceFirewallSecurityProvider,
            Root.TraceId,
            "Removing firewall rules for policy {0} returned error {1} nodeInstanceId {2}", policyName, error, nodeInstanceId);
    }

    return error;
#endif    
}

#if defined(PLATFORM_UNIX)
ErrorCode FirewallSecurityProvider::CleanupRulesForAllProfiles(wstring const & policyName, vector<LONG> const& ports, uint64 nodeInstanceId)
{
    HRESULT hr = S_OK;
    HRESULT last_hr = S_OK;
    for (auto const& profile : profilesEnabled_)
    {
        for (auto const& port : ports)
        {
            for (auto const& protocol : allprotocols_)
            {
                for (int dir = 0; dir <= 1; dir++)
                {
                    wstring firewallRuleName = FirewallSecurityProvider::GetFirewallRuleName(policyName, dir, profile, protocol, port, nodeInstanceId);

                    WriteInfo(TraceFirewallSecurityProvider, Root.TraceId,
                        "Removing rule {0}", firewallRuleName);

                    string policyNameA = StringUtility::Utf16ToUtf8(firewallRuleName);
                    hr = RemoveIptablesEntry(policyNameA);
                    if (FAILED(hr))
                    {
                        WriteWarning(TraceFirewallSecurityProvider, Root.TraceId,
                            "Failed to remove incoming firewall rule (ipv4), hresult {0} rule {1}", hr, policyName);
                        last_hr = hr;
                    }

                    hr = RemoveIp6tablesEntry(policyNameA);
                    if (FAILED(hr))
                    {
                        WriteWarning(TraceFirewallSecurityProvider, Root.TraceId,
                            "Failed to remove incoming firewall rule (ipv6), hresult {0} rule {1}", hr, policyName);
                        last_hr = hr;
                    }
                }
            }
        }
    }
    return ErrorCode::FromHResult(last_hr);
}
#else
ErrorCode FirewallSecurityProvider::CleanupRulesForAllProfiles(wstring const & policyName, ComPtr<INetFwRules> pFwRules, vector<LONG> const& ports, uint64 nodeInstanceId)
{
    HRESULT hr = S_OK;
    HRESULT last_hr = S_OK;
    for (auto const& profile : profilesEnabled_)
    {
        for (auto const& port : ports)
        {
            for (auto const& protocol : allprotocols_)
            {
                wstring firewallRuleName = FirewallSecurityProvider::GetFirewallRuleName(policyName, false, profile, protocol, port, nodeInstanceId);

                WriteInfo(TraceFirewallSecurityProvider, Root.TraceId,
                    "Removing rule {0}", firewallRuleName);

                hr = pFwRules->Remove((BSTR)firewallRuleName.c_str());
                if (FAILED(hr))
                {

                    WriteWarning(
                        TraceFirewallSecurityProvider,
                        Root.TraceId,
                        "Failed to remove incoming firewall rule , hresult {0} rule {1}", hr, firewallRuleName);
                    last_hr = hr;
                }

                firewallRuleName = FirewallSecurityProvider::GetFirewallRuleName(policyName, true, profile, protocol, port, nodeInstanceId);

                WriteInfo(TraceFirewallSecurityProvider, Root.TraceId,
                    "Removing rule {0}", firewallRuleName);

                hr = pFwRules->Remove((BSTR)firewallRuleName.c_str());
                if (FAILED(hr))
                {

                    WriteWarning(
                        TraceFirewallSecurityProvider,
                        Root.TraceId,
                        "Failed to remove outgoing firewall rule , hresult {0} rule {1}", hr, policyName);
                    last_hr = hr;
                }
            }
        }
    }
    return ErrorCode::FromHResult(last_hr);
}
#endif

wstring FirewallSecurityProvider::GetFirewallRuleName(
    wstring const & packageId,
    bool outgoing,
    LONG profileType,
    LONG protocol,
    LONG port,
    uint64 nodeInstanceId)
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
        return wformatString("{0}_{1}_{2}_{3}_Out", nodeInstanceId, packageId, port, profileName);
    }
    else
    {
        return wformatString("{0}_{1}_{2}_{3}_IN", nodeInstanceId, packageId, port, profileName);
    }
}
