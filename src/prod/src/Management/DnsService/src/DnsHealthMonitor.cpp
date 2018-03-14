// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsHealthMonitor.h"

DWORD TimeoutInSeconds = 60;
WCHAR SourceId[] = L"System.FabricDnsService";
WCHAR PropertyEnvironment[] = L"Environment";

WCHAR DescriptionEnvironmentSuccess[] = L"FabricDnsService environment is setup correctly.";
WCHAR DescriptionEnvironmentFailure[] = L"FabricDnsService is not preferred DNS server on the node.";

/*static*/
void DnsHealthMonitor::Create(
    __out DnsHealthMonitor::SPtr& spHealth,
    __in KAllocator& allocator,
    __in const DnsServiceParams& params,
    __in IFabricStatelessServicePartition2& fabricHealth
)
{
    spHealth = _new(TAG, allocator) DnsHealthMonitor(params, fabricHealth);
    KInvariant(spHealth != nullptr);
}

DnsHealthMonitor::DnsHealthMonitor(
    __in const DnsServiceParams& params,
    __in IFabricStatelessServicePartition2& fabricHealth
) : _params(params),
_fabricHealth(fabricHealth),
_lastStateEnvironment(FABRIC_HEALTH_STATE_UNKNOWN)
{
    KTimer::Create(/*out*/_spTimer, GetThisAllocator(), KTL_TAG_TIMER);
    if (_spTimer == nullptr)
    {
        KInvariant(false);
    }
}

DnsHealthMonitor::~DnsHealthMonitor()
{
}

bool DnsHealthMonitor::StartMonitor(
    __in_opt KAsyncContextBase* const parent
)
{
    Start(parent, nullptr/*callback*/);
    return true;
}

void DnsHealthMonitor::OnStart()
{
    StartTimer();
}

void DnsHealthMonitor::OnCancel()
{
    _spTimer->Cancel();
    Complete(STATUS_CANCELLED);
}

void DnsHealthMonitor::StartTimer()
{
    _spTimer->Reuse();
    KAsyncContextBase::CompletionCallback timerCallback(this, &DnsHealthMonitor::OnTimer);
    _spTimer->StartTimer(TimeoutInSeconds * 1000, this/*parent*/, timerCallback);
}

void DnsHealthMonitor::OnTimer(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& context
)
{
    if (context.Status() == STATUS_SUCCESS)
    {
        CheckEnvironment();

        StartTimer();
    }
    else
    {
        // Timer was cancelled
        KInvariant(context.Status() == STATUS_CANCELLED);
    }
}

void DnsHealthMonitor::CheckEnvironment()
{
    // Make sure local IP address is the preferred DNS
    if (!_params.SetAsPreferredDns)
    {
        return;
    }

    bool found = false;

    std::vector<std::wstring> dnsServers;
    Common::ErrorCode error = Common::IpUtility::GetDnsServers(/*out*/dnsServers);
    if (!error.IsSuccess())
    {
        return;
    }

    typedef std::map<std::wstring, std::vector<Common::IPPrefix>> IpMap;
    IpMap map;
    error = Common::IpUtility::GetIpAddressesPerAdapter(/*out*/map);
    if (!error.IsSuccess())
    {
        return;
    }

    for (IpMap::iterator it = map.begin(); it != map.end() && !found; ++it)
    {
        const std::vector<Common::IPPrefix> & ipAddresses = it->second;

        for (int i = 0; i < dnsServers.size() && !found; i++)
        {
            const std::wstring & dns = dnsServers[i];
            for (int j = 0; j < ipAddresses.size() && !found; j++)
            {
                const Common::IPPrefix& prefix = ipAddresses[j];
                if (Common::StringUtility::AreEqualCaseInsensitive(dns, prefix.GetAddress().GetIpString()))
                {
                    found = true;
                }
            }
        }
    }

#if defined(PLATFORM_UNIX)
    if (!found)
    {
        // Linux is always healthy for now
        found = true;
    }
#endif

    FABRIC_HEALTH_INFORMATION info;
    info.SourceId = SourceId;
    info.Property = PropertyEnvironment;
    info.RemoveWhenExpired = FALSE;
    info.TimeToLiveSeconds = FABRIC_HEALTH_REPORT_INFINITE_TTL;
    info.SequenceNumber = FABRIC_AUTO_SEQUENCE_NUMBER;

    if (found)
    {
        info.State = FABRIC_HEALTH_STATE_OK;
        info.Description = DescriptionEnvironmentSuccess;
    }
    else
    {
        info.State = FABRIC_HEALTH_STATE_WARNING;
        info.Description = DescriptionEnvironmentFailure;
    }

    if (_lastStateEnvironment != info.State)
    {
        _lastStateEnvironment = info.State;
        _fabricHealth.ReportInstanceHealth(&info);
    }
}
