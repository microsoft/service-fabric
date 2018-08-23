// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType_Dns("ConfigureNodeForDnsService");

#if !defined(PLATFORM_UNIX)
const WCHAR RegistryPath[] = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
const WCHAR NameServerValueName[] = L"NameServer";
const WCHAR ServiceFabricNameServerValueName[] = L"ServiceFabricNameServer";

const WCHAR DnsCacheRegistryKey[] = L"SYSTEM\\CurrentControlSet\\Services\\DNSCache\\Parameters";
const WCHAR DnsNegativeCacheTtlName[] = L"MaxNegativeCacheTtl";

const WCHAR HostFileRelativePath[] = L"\\drivers\\etc\\hosts";

const WCHAR NetShRelativePath[] = L"\\netsh.exe";
#endif

ConfigureNodeForDnsService::ConfigureNodeForDnsService(ComponentRoot const & root)
    : RootedObject(root)
{
}

ConfigureNodeForDnsService::~ConfigureNodeForDnsService()
{
}

Common::ErrorCode ConfigureNodeForDnsService::Configure(
    bool isDnsServiceEnabled,
    bool setAsPreferredDns)
{
    AcquireWriteLock lock(lock_);

    Cleanup();

    if (!isDnsServiceEnabled)
    {
        WriteInfo(TraceType_Dns, Root.TraceId, "Not configuring the node environment for DnsService because the DnsService is disabled.");
        return ErrorCode::Success();
    }

    ErrorCode error;
    if (setAsPreferredDns)
    {
        error = this->ModifyDnsServerList();
        if (!error.IsSuccess())
        {
            Cleanup();
            return error;
        }
    }
    else
    {
        WriteInfo(TraceType_Dns, Root.TraceId, "Not modifying the DNS chain of the node since SetAsPreferredDns flag is set to false.");
    }

    // Adjusting the cache is nice to have but not crucial.
    // It should not prevent DnsService from being the preferred DNS.
    // Trace the error but do not cleanup the environment.
    error = SetHostsFileAcl(false /*remove*/);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "Failed to set ACL on the hosts file, error={0}. DnsService probably won't be able to flush the DNS cache.",
            error);
    }

    error = DisableNegativeCache();
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId, "Failed to disable DNS negative cache, error={0}", error);
    }

    return ErrorCode::Success();
}

void ConfigureNodeForDnsService::Cleanup()
{
    this->RestoreDnsServerList();
    SetHostsFileAcl(true /*remove*/);
}

Common::ErrorCode ConfigureNodeForDnsService::ModifyDnsServerList()
{
#if !defined(PLATFORM_UNIX)
    typedef std::map<std::wstring, std::vector<Common::IPPrefix>> IpMap;
    typedef std::map<std::wstring, std::vector<std::wstring>> DnsMap;

    DnsMap dnsServersPerAdapter;
    ErrorCode error = IpUtility::GetDnsServersPerAdapter(/*out*/dnsServersPerAdapter);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerList failed to retrieve list of DNS servers, error {0}",
            error);
        return error;
    }

    IpMap ipAddressesPerAdapter;
    error = IpUtility::GetIpAddressesPerAdapter(/*out*/ipAddressesPerAdapter);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerList failed to retrieve list of IP addresses, error {0}",
            error);
        return error;
    }

    // For each adapter that has at least one unicast address and at least one dns server
    // modify the list of dns servers to include the first unicast address.
    std::vector<std::wstring> localIpAddresses;
    IpMap dnsMap;
    for (DnsMap::iterator it = dnsServersPerAdapter.begin(); it != dnsServersPerAdapter.end(); ++it)
    {
        const std::wstring & adapterName = it->first;
        const std::vector<std::wstring> & dnsAddresses = it->second;
        if (dnsAddresses.empty())
        {
            WriteInfo(TraceType_Dns, Root.TraceId, "ModifyDnsServerList skipped adapter {0}, DNS list is empty", adapterName);
            continue;
        }

        auto innerIt = ipAddressesPerAdapter.find(adapterName);
        if (innerIt == ipAddressesPerAdapter.end())
        {
            WriteInfo(TraceType_Dns, Root.TraceId, "ModifyDnsServerList skipped adapter {0}, IP address doesn't exist", adapterName);
            continue;
        }

        const std::vector<IPPrefix> & ipAddresses = innerIt->second;
        if (ipAddresses.empty())
        {
            WriteInfo(TraceType_Dns, Root.TraceId, "ModifyDnsServerList skipped adapter {0}, IP address list is empty", adapterName);
            continue;
        }

        // Check if list of dns addresses already contains unicast address
        bool found = false;
        for (int i = 0; i < dnsAddresses.size() && !found; i++)
        {
            const std::wstring & dns = dnsAddresses[i];
            for (int j = 0; j < ipAddresses.size() && !found; j++)
            {
                const IPPrefix & prefix = ipAddresses[j];
                if (Common::StringUtility::AreEqualCaseInsensitive(prefix.GetAddress().GetIpString(), dns))
                {
                    found = true;
                }
            }
        }

        if (found)
        {
            WriteInfo(TraceType_Dns, Root.TraceId, "ModifyDnsServerList skipped adapter {0}, unicast IP address {1} is already in the DNS chain {2}",
                adapterName, ipAddresses, dnsAddresses);
            continue;
        }

        const IPPrefix& ip = ipAddresses[0];
        std::wstring ipStr = ip.GetAddress().GetIpString();
        localIpAddresses.push_back(ipStr);
        error = ModifyDnsServerListForAdapter(adapterName, ipStr, dnsAddresses);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // Check the result by comparing list of DNS servers with changes that we made
    // It is enough to find a single IP address.
    if (localIpAddresses.size() > 0)
    {
        std::vector<std::wstring> newDnsList;
        error = IpUtility::GetDnsServers(/*out*/newDnsList);
        bool failure = true;
        for (int i = 0; i < localIpAddresses.size(); i++)
        {
            const std::wstring & ip = localIpAddresses[i];
            if (std::find(newDnsList.begin(), newDnsList.end(), ip) != newDnsList.end())
            {
                failure = false;
                break;
            }
        }

        if (failure)
        {
            WriteWarning(TraceType_Dns, Root.TraceId,
                "ModifyDnsServerList failed to verify the list of DNS addresses, actual DNS list {0}, expected to contain {1}",
                newDnsList, localIpAddresses);

            return ErrorCode::FromHResult(E_FAIL, "Failed to validate change to DNS server list.");
        }
    }
#endif

    return ErrorCode::Success();
}

Common::ErrorCode ConfigureNodeForDnsService::ModifyDnsServerListForAdapter(
    const std::wstring & adapterName,
    const std::wstring & ipAddress,
    const std::vector<std::wstring> & dnsAddresses)
{
#if !defined(PLATFORM_UNIX)
    std::wstring keyPath = RegistryPath + adapterName;

    Common::RegistryKey key(keyPath, false/*readonly*/, true/*openExisting*/);
    ErrorCode error = ErrorCode::FromWin32Error(key.Error);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerListForAdapter failed to open registry key {0}, error {1}",
            keyPath, error);

        return error;
    }

    std::wstring currentNameServerValue;
    for (int i = 0; i < dnsAddresses.size(); i++)
    {
        if (i > 0)
        {
            currentNameServerValue += L",";
        }
        currentNameServerValue += dnsAddresses[i];
    }

    //
    // Get current value of NameServer registry key and store it in special ServiceFabricNameServer registry value
    // for backup purposes. That way, once DNS service is removed from the system, we can restore the initial value.
    //
    std::wstring nameServerOldValue;
    if (!key.GetValue(NameServerValueName, nameServerOldValue))
    {
        error = ErrorCode::FromWin32Error(key.Error);

        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerListForAdapter failed to get registry value {0}, error {1}",
            NameServerValueName, error);

        return error;
    }

    if (!key.SetValue(ServiceFabricNameServerValueName, nameServerOldValue, true/*typeRegSz*/))
    {
        error = ErrorCode::FromWin32Error(key.Error);

        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerListForAdapter failed to set registry value {0} to {1}, error {2}",
            ServiceFabricNameServerValueName, nameServerOldValue, error);

        return error;
    }

    //
    // Set the new value to NameServer. 
    //
    error = SetStaticDnsChainRegistryValue(key, currentNameServerValue);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Windows OS keeps in-memory DNS chain that is automatically refreshed every 15min approx.
    // InsertIntoStaticDnsChainAndRefresh inserts into the DNS chain using netsh.exe in order to force immediate refresh of the DNS chain.
    // Netsh.exe uses private Windows API to achieve this.
    error = InsertIntoStaticDnsChainAndRefresh(adapterName, ipAddress);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerListForAdapter failed to insert IP address {0} at the first position in the DNS chain, error {1}",
            ipAddress, error);

        // If insert failed then do the next best thing, which is to change the registry key.
        std::wstring newNameServerValue(ipAddress);
        if (!currentNameServerValue.empty())
        {
            newNameServerValue += L",";
            newNameServerValue += currentNameServerValue;
        }

        error = SetStaticDnsChainRegistryValue(key, newNameServerValue);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    std::wstring nameServerNewValue;
    if (!key.GetValue(NameServerValueName, nameServerNewValue))
    {
        error = ErrorCode::FromWin32Error(key.Error);

        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerListForAdapter failed to get registry value {0}, error {1}",
            NameServerValueName, error);

        return error;
    }

    WriteInfo(TraceType_Dns, Root.TraceId,
        "ModifyDnsServerListForAdapter successfully changed registry key {0} value {1} to {2}",
        keyPath, NameServerValueName, nameServerNewValue);
#endif

    return ErrorCode::Success();
}

Common::ErrorCode ConfigureNodeForDnsService::RestoreDnsServerList()
{
#if !defined(PLATFORM_UNIX)
    typedef std::map<std::wstring, std::vector<std::wstring>> IpMap;

    IpMap dnsServersPerAdapter;
    ErrorCode error = IpUtility::GetDnsServersPerAdapter(dnsServersPerAdapter);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "RestoreDnsServerList failed to retrieve list of DNS servers, error {0}",
            error);
        return error;
    }

    for (IpMap::iterator it = dnsServersPerAdapter.begin(); it != dnsServersPerAdapter.end(); ++it)
    {
        const std::wstring & adapterName = it->first;
        error = RestoreDnsServerListForAdapter(adapterName);
        if (!error.IsSuccess())
        {
            return error;
        }
    }
#endif

    return ErrorCode::Success();
}

Common::ErrorCode ConfigureNodeForDnsService::RestoreDnsServerListForAdapter(
    const std::wstring & adapterName
)
{
#if !defined(PLATFORM_UNIX)
    std::wstring keyPath = RegistryPath + adapterName;

    Common::RegistryKey key(keyPath, false/*readonly*/, true/*openExisting*/);
    ErrorCode error = ErrorCode::FromWin32Error(key.Error);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "RestoreDnsServerListForAdapter failed to open registry key {0}, error {1}",
            keyPath, error);

        return error;
    }

    std::wstring nameServerOldValue;
    if (!key.GetValue(ServiceFabricNameServerValueName, nameServerOldValue))
    {
        error = ErrorCode::FromWin32Error(key.Error);

        if (!error.IsWin32Error(ERROR_FILE_NOT_FOUND))
        {
            WriteWarning(TraceType_Dns, Root.TraceId,
                "RestoreDnsServerListForAdapter failed to get registry value {0}, registry key {1}, error {2}",
                ServiceFabricNameServerValueName, keyPath, error);

            return error;
        }

        //
        // Registry value ServiceFabricNameServer is not found, which means we never changed NameServer
        // for this adapter, hence there is nothing to restore. Just move on.
        //
        WriteInfo(TraceType_Dns, Root.TraceId,
            "RestoreDnsServerListForAdapter didn't find the registry value {0}, registry key {1}, nothing to restore.",
            ServiceFabricNameServerValueName, keyPath);
        return ErrorCode::Success();
    }

    //
    // Restore NameServer to it's initial state.
    //
    if (!key.SetValue(NameServerValueName, nameServerOldValue, true/*typeRegSz*/))
    {
        error = ErrorCode::FromWin32Error(key.Error);

        WriteWarning(TraceType_Dns, Root.TraceId,
            "RestoreDnsServerListForAdapter failed to set registry value {0} to {1}, registry key {2}, error {3}",
            NameServerValueName, nameServerOldValue, keyPath, error);

        return error;
    }

    //
    // Delete ServiceFabricNameServer registry value to stop tracking this adapter.
    //
    if (!key.DeleteValue(ServiceFabricNameServerValueName))
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "RestoreDnsServerListForAdapter failed to delete registry value {0}, error {1}",
            ServiceFabricNameServerValueName, error);

        return error;
    }

    WriteInfo(TraceType_Dns, Root.TraceId,
        "RestoreDnsServerListForAdapter successfully restored registry key {0} value {1} to {2}",
        keyPath, NameServerValueName, nameServerOldValue);
#endif

    return ErrorCode::Success();
}

Common::ErrorCode ConfigureNodeForDnsService::DisableNegativeCache()
{
#if !defined(PLATFORM_UNIX)
    Common::RegistryKey key(DnsCacheRegistryKey, false/*readonly*/, true/*openExisting*/);
    Common::ErrorCode error = Common::ErrorCode::FromWin32Error(key.Error);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "DisableNegativeCache failed to open registry key, error {3}",
            DnsCacheRegistryKey, error);

        return error;
    }

    if (!key.SetValue(DnsNegativeCacheTtlName, 0))
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "DisableNegativeCache failed to set registry key value {0} {1}, error {2}",
            DnsCacheRegistryKey, DnsNegativeCacheTtlName, error);

        return error;
    }

    WriteInfo(TraceType_Dns, Root.TraceId,
        "DisableNegativeCache successfully disabled negative cache, reg key {0} {1}",
        DnsCacheRegistryKey, DnsNegativeCacheTtlName);
#endif

    return ErrorCode::Success();
}

Common::ErrorCode ConfigureNodeForDnsService::SetHostsFileAcl(bool remove)
{
#if !defined(PLATFORM_UNIX)
    Common::ErrorCode error;

    WCHAR wszSystem[MAX_PATH];
    ULONG length = GetSystemDirectory(wszSystem, ARRAYSIZE(wszSystem));
    if (0 == length)
    {
        error = Common::ErrorCode::FromWin32Error();

        WriteWarning(TraceType_Dns, Root.TraceId,
            "SetHostsFileAcl failed to get path of the system directory, error {0}",
            error);

        return error;
    }

    std::wstring hostsFilePath(wszSystem);
    hostsFilePath += HostFileRelativePath;
    Common::SidSPtr sidsptr;
    
    error = BufferedSid::CreateSPtr(WinNetworkServiceSid, sidsptr);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "SetHostsFileAcl failed to create BufferedSid for WinNetworkService user, error {0}", error);
        return error;
    }

    std::vector<Common::SidSPtr> sids;
    sids.push_back(sidsptr);

    if (!remove)
    {
        error = Common::SecurityUtility::UpdateFileAcl(sids, hostsFilePath, GENERIC_READ | FILE_WRITE_ATTRIBUTES, TimeSpan::MaxValue);
    }
    else
    {
        error = Common::SecurityUtility::RemoveFileAcl(sidsptr, hostsFilePath, false, TimeSpan::MaxValue);
    }

    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "SetHostsFileAcl failed to update file {0} ACL, remove {1}, error {2}",
            hostsFilePath, remove, error);

        return error;
    }

    WriteInfo(TraceType_Dns, Root.TraceId,
        "SetHostsFileAcl successfully set write ACL for WinNetworkService user to file {0}", hostsFilePath);
#endif

    return ErrorCode::Success();
}

Common::ErrorCode ConfigureNodeForDnsService::SetStaticDnsChainRegistryValue(
    Common::RegistryKey & key,
    const std::wstring & nameServerValue)
{
    Common::ErrorCode error;

#if !defined(PLATFORM_UNIX)
    if (!key.SetValue(NameServerValueName, nameServerValue, true/*typeRegSz*/))
    {
        error = ErrorCode::FromWin32Error(key.Error);

        WriteWarning(TraceType_Dns, Root.TraceId,
            "ModifyDnsServerListForAdapter failed to set registry value {0} to {1}, error {2}",
            NameServerValueName, nameServerValue, error);

        //
        // In case of failure, remove ServiceFabricNameServer registry value
        // to make it clear that no change was made.
        //
        if (!key.DeleteValue(ServiceFabricNameServerValueName))
        {
            WriteWarning(TraceType_Dns, Root.TraceId,
                "ModifyDnsServerListForAdapter failed to delete registry value {0}, error {1}",
                ServiceFabricNameServerValueName, error);
        }
    }
#endif

    return error;
}

Common::ErrorCode ConfigureNodeForDnsService::InsertIntoStaticDnsChainAndRefresh(
    const std::wstring & adapterName,
    const std::wstring & ipStr
)
{
    Common::ErrorCode error;
#if !defined(PLATFORM_UNIX)

    std::wstring adapterDescription;
    error = GetAdapterFriendlyName(adapterName, adapterDescription);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType_Dns, Root.TraceId,
            "InsertIntoStaticDnsChainAndRefresh failed to find the adapter description for the adapter {0}, error {1}",
            adapterName, error);
        return error;
    }

    WCHAR wszSystem[MAX_PATH];
    ULONG length = GetSystemDirectory(wszSystem, ARRAYSIZE(wszSystem));
    if (0 == length)
    {
        error = Common::ErrorCode::FromWin32Error();

        WriteWarning(TraceType_Dns, Root.TraceId,
            "InsertIntoStaticDnsChainAndRefresh failed to get path of the system directory, error {0}",
            error);

        return error;
    }

    std::wstring netshStr(wszSystem);
    netshStr += NetShRelativePath;

    std::wstring command = wformatString("{0}{1} interface ipv4 add dnsservers name=\"{2}\" address={3} validate=no index=1",
        wszSystem, NetShRelativePath, adapterDescription, ipStr);

    error = ProcessUtility::ExecuteCommandLine(command);

    WriteInfo(TraceType_Dns, Root.TraceId, "InsertIntoStaticDnsChainAndRefresh executed command line {0}, error {1}", command, error);
#endif

    return error;
}

Common::ErrorCode ConfigureNodeForDnsService::GetAdapterFriendlyName(
    const std::wstring & adapterName,
    std::wstring & adapterDescription
)
{
#if !defined(PLATFORM_UNIX)
    const DWORD flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_UNICAST;

    ByteBuffer buffer;
    ULONG bufferSize = 0;
    DWORD error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    if (error == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufferSize);
        error = GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buffer.data(), &bufferSize);
    }

    if (error != ERROR_SUCCESS)
    {
        return ErrorCode::FromWin32Error(error);
    }

    IP_ADAPTER_ADDRESSES* pCurrAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    while (pCurrAddresses != NULL)
    {
        std::wstring name = StringUtility::Utf8ToUtf16(pCurrAddresses->AdapterName);
        if (StringUtility::AreEqualCaseInsensitive(name, adapterName))
        {
            adapterDescription = pCurrAddresses->FriendlyName;
            return ErrorCode::Success();
        }

        pCurrAddresses = pCurrAddresses->Next;
    }
#endif

    return ErrorCodeValue::NotFound;
}
