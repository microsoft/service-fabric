// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#if !defined(PLATFORM_UNIX)
#include <dhcpcsdk.h>
#endif

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType_DnsEnvManager("DnsServiceEnvironmentManager");
DnsServiceEnvironmentManager::DnsServiceEnvironmentManager(
    Common::ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    hosting_(hosting)
{
    timer_ = Timer::Create("DnsServiceEnvironmentManager", [this](TimerSPtr const &) { this->OnTimer(); });
}

DnsServiceEnvironmentManager::~DnsServiceEnvironmentManager()
{
    WriteInfo(TraceType_DnsEnvManager, Root.TraceId,
        "DnsServiceEnvironmentManager.destructor");

    StopMonitor();
}

Common::AsyncOperationSPtr DnsServiceEnvironmentManager::BeginSetupDnsEnvAndStartMonitor(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    AcquireWriteLock lock(lock_);

    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "DnsServiceEnvironmentManager::BeginSetupDnsEnvAndStartMonitor called");

#if !defined(PLATFORM_UNIX)
    // DnsService is by default set as a preferred DNS server on the node 
    // which means that the IP address of the node is inserted as the first DNS address in the DNS chain.
    // In case the host's IP address is not in the chain, the warning �FabricDnsService is not preferred DNS server on the node.� is fired.
    // The warning happens when DNS chain is modified while the cluster is running. This occurs mostly on OneBox clusters when:
    // 1. The device connects to a different network (laptop is attached to a new wireless network for example); 
    //    network settings are changed due to DHCP, hence DNS chain needs to be updated. There are 2 subcases here:
    //    - DNS chain is replaced by the new chain supplied by DHCP, the host's IP address will not be in the new chain
    //    - DNS chain is not replaced by the new chain supplied by DHCP, in this case we have to rely on DHCP notification to rebuild the DNS chain since
    //      the old chain still satisfies the condition that host's IP address is in the list, however the public DNS addresses in the chain are incorrect.
    // 2. A node in the OneBox cluster which has DnsService instance is disabled forcing DnsService instance to be moved. Due to multiple Fabric.exe processes running
    //    setup and cleanup of the DNS environment is going to be done by 2 processes at the same time, which can cause the environment to end up in incorrect state.
    //
    // To fix above issues, monitor is introduced that monitors the DNS chain for the presence of the host's IP address and it monitors DHCP for changes.

    adapters_.clear();
    adapterChangedEvents_.clear();

    typedef std::map<std::wstring, std::vector<IPPrefix>> IpMap;
    IpMap ipPerAdapter;
    ErrorCode error = Common::IpUtility::GetIpAddressesPerAdapter(/*out*/ipPerAdapter);
    for (IpMap::iterator it = ipPerAdapter.begin(); it != ipPerAdapter.end(); ++it)
    {
        const std::wstring & adapterName = it->first;
        adapters_.push_back(adapterName);
        adapterChangedEvents_.push_back(INVALID_HANDLE_VALUE);
    }

    for (int i = 0; i < adapters_.size(); i++)
    {
        DHCPCAPI_PARAMS DhcpApiServerNameParams = { 0, OPTION_SERVER_IDENTIFIER, FALSE, NULL, 0 };
        DHCPCAPI_PARAMS params[] = { DhcpApiServerNameParams };
        DHCPCAPI_PARAMS_ARRAY DhcpApiParamsArray = { ARRAYSIZE(params), params };

        DhcpRegisterParamChange(DHCPCAPI_REGISTER_HANDLE_EVENT, NULL, (LPWSTR)adapters_[i].c_str(),
            NULL, DhcpApiParamsArray, &adapterChangedEvents_[i]);
    }
#endif

    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "Started DnsService environment monitor");

    return hosting_.FabricActivatorClientObj->BeginConfigureNodeForDnsService(
        true /*isDnsServiceEnabled*/, timeout, callback, parent);
}

Common::ErrorCode  DnsServiceEnvironmentManager::EndSetupDnsEnvAndStartMonitor(
    AsyncOperationSPtr const & operation
)
{
    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "DnsServiceEnvironmentManager::EndSetupDnsEnvAndStartMonitor called");

    Common::ErrorCode error = hosting_.FabricActivatorClientObj->EndConfigureNodeForDnsService(operation);

    WriteInfo(TraceType_DnsEnvManager, Root.TraceId,
        "DnsServiceEnvironmentManager::EndSetupDnsEnvAndStartMonitor ConfigureNodeForDnsService result {0}", error);

    TimeSpan interval = DNS::DnsServiceConfig::GetConfig().NodeDnsEnvironmentHealthCheckInterval;
    if (interval == TimeSpan::Zero)
    {
        WriteWarning(TraceType_DnsEnvManager, Root.TraceId,
            "DnsServiceEnvironmentManager::EndSetupDnsEnvAndStartMonitor, NodeDnsEnvironmentHealthCheckInterval is zero, not starting the timer.");
    }
    else
    {
        timer_->Change(interval, TimeSpan::Zero);
    }

    return error;
}

Common::AsyncOperationSPtr DnsServiceEnvironmentManager::BeginRemoveDnsFromEnvAndStopMonitor(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    AcquireWriteLock lock(lock_);

    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "DnsServiceEnvironmentManager::BeginRemoveDnsFromEnvAndStopMonitor called");

    StopMonitorUnsafe();

    return hosting_.FabricActivatorClientObj->BeginConfigureNodeForDnsService(
        false /*isDnsServiceEnabled*/, timeout, callback, parent);
}

Common::ErrorCode DnsServiceEnvironmentManager::EndRemoveDnsFromEnvAndStopMonitor(
    AsyncOperationSPtr const & operation
)
{
    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "DnsServiceEnvironmentManager::EndRemoveDnsFromEnvAndStopMonitor called");

    return hosting_.FabricActivatorClientObj->EndConfigureNodeForDnsService(operation);
}

Common::ErrorCode DnsServiceEnvironmentManager::RemoveDnsFromEnvAndStopMonitorAndWait()
{
    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "DnsServiceEnvironmentManager::RemoveDnsFromEnvAndStopMonitorAndWait called");

    TimeSpan timeout = DNS::DnsServiceConfig::GetConfig().ConfigureDnsEnvironmentTimeoutInterval;

    AsyncOperationWaiter waiter;
    Common::AsyncOperationSPtr operation = this->BeginRemoveDnsFromEnvAndStopMonitor(
        timeout,
        [this, &waiter](AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            waiter.SetError(this->EndRemoveDnsFromEnvAndStopMonitor(operation));
            waiter.Set();
        }
    },
        AsyncOperationSPtr());

    if (operation->CompletedSynchronously)
    {
        waiter.SetError(this->EndRemoveDnsFromEnvAndStopMonitor(operation));
        waiter.Set();
    }

    waiter.WaitOne();

    Common::ErrorCode error = waiter.GetError();
    WriteInfo(TraceType_DnsEnvManager, Root.TraceId,
        "DnsServiceEnvironmentManager::RemoveDnsFromEnvAndStopMonitorAndWait completed, CompletedSynchronously {0}, error {1}",
        operation->CompletedSynchronously, error);

    return error;
}

void DnsServiceEnvironmentManager::OnTimer()
{
    AcquireWriteLock lock(lock_);

    bool change = false;
    bool setAsPreferredDns = DNS::DnsServiceConfig::GetConfig().SetAsPreferredDns;

    if (setAsPreferredDns)
    {
        if (!IsDnsEnvironmentHealthy())
        {
            WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "DnsService environment is unhealthy");

            change = true;
        }
#if !defined(PLATFORM_UNIX)
        else
        {
            DWORD index = WaitForMultipleObjects((DWORD)adapterChangedEvents_.size(), adapterChangedEvents_.data(), FALSE/*waitAll*/, 0/*ms*/);
            if (index < WAIT_OBJECT_0 + adapterChangedEvents_.size())
            {
                // Reset all events
                for (int i = 0; i < adapterChangedEvents_.size(); i++)
                {
                    ResetEvent(adapterChangedEvents_[i]);
                }

                WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "Encountered DHCP change for adapter {0}", adapters_[index]);

                change = true;
            }
        }
#endif
    }

    if (change)
    {
        TimeSpan timeout = DNS::DnsServiceConfig::GetConfig().ConfigureDnsEnvironmentTimeoutInterval;
        auto operation = hosting_.FabricActivatorClientObj->BeginConfigureNodeForDnsService(
            true /*isDnsServiceEnabled*/,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnConfigureNodeForDnsServiceComplete(operation, false /*expectedCompletedSynchronously*/); },
            nullptr);

        this->OnConfigureNodeForDnsServiceComplete(operation, true /*expectedCompletedSynchronously*/);
    }
    else
    {
        TimeSpan interval = DNS::DnsServiceConfig::GetConfig().NodeDnsEnvironmentHealthCheckInterval;
        if (interval == TimeSpan::Zero)
        {
            WriteWarning(TraceType_DnsEnvManager, Root.TraceId,
                "DnsServiceEnvironmentManager::OnTimer, NodeDnsEnvironmentHealthCheckInterval is zero, not starting the timer.");
        }
        else
        {
            timer_->Change(interval, TimeSpan::Zero);
        }
    }
}

void DnsServiceEnvironmentManager::OnConfigureNodeForDnsServiceComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = hosting_.FabricActivatorClientObj->EndConfigureNodeForDnsService(operation);

    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_DnsEnvManager, Root.TraceId, "Failed to configure node environment for DnsService, error {0}", error);
    }
    else
    {
        WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "Successfully configured node environment for DnsService");
    }

    timer_->Change(DNS::DnsServiceConfig::GetConfig().NodeDnsEnvironmentHealthCheckInterval, TimeSpan::Zero);
}

void DnsServiceEnvironmentManager::StopMonitor()
{
    AcquireWriteLock lock(lock_);
    StopMonitorUnsafe();
}

void DnsServiceEnvironmentManager::StopMonitorUnsafe()
{
    timer_->Cancel();
    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "Stopped DnsService environment monitor timer.");

#if !defined(PLATFORM_UNIX)
    for (int i = 0; i < adapters_.size(); i++)
    {
        DhcpDeRegisterParamChange(0 /*flags must be zero*/, NULL /*Reserved MUST BE NULL */, &adapterChangedEvents_[i]);
    }
    adapters_.clear();
    adapterChangedEvents_.clear();

    WriteInfo(TraceType_DnsEnvManager, Root.TraceId, "Unregistered DnsService environment monitor DHCP params.");
#endif
}

bool DnsServiceEnvironmentManager::IsDnsEnvironmentHealthy()
{
    bool setAsPreferredDns = DNS::DnsServiceConfig::GetConfig().SetAsPreferredDns;
    if (!setAsPreferredDns)
    {
        return true;
    }

    bool isHealthy = false;

#if !defined(PLATFORM_UNIX)
    std::vector<std::wstring> dnsServers;
    typedef std::map<std::wstring, std::vector<Common::IPPrefix>> IpMap;
    IpMap map;

    Common::IpUtility::GetDnsServers(/*out*/dnsServers);
    Common::IpUtility::GetIpAddressesPerAdapter(/*out*/map);

    for (IpMap::iterator it = map.begin(); it != map.end() && !isHealthy; ++it)
    {
        const std::vector<Common::IPPrefix> & ipAddresses = it->second;

        for (int i = 0; i < dnsServers.size() && !isHealthy; i++)
        {
            const std::wstring & dns = dnsServers[i];
            for (int j = 0; j < ipAddresses.size() && !isHealthy; j++)
            {
                const Common::IPPrefix& prefix = ipAddresses[j];
                if (Common::StringUtility::AreEqualCaseInsensitive(dns, prefix.GetAddress().GetIpString()))
                {
                    isHealthy = true;
                }
            }
        }
    }
#else
    // Linux is always healthy
    isHealthy = true;
#endif

    return isHealthy;
}
