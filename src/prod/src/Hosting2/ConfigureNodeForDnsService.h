// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureNodeForDnsService :
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ConfigureNodeForDnsService)

    public:
        ConfigureNodeForDnsService(
            Common::ComponentRoot const & root);
        virtual ~ConfigureNodeForDnsService();

        Common::ErrorCode Configure(
            bool isDnsServiceEnabled,
            bool setAsPreferredDns);

    private:
        Common::ErrorCode ModifyDnsServerList();

        Common::ErrorCode ModifyDnsServerListForAdapter(
            const std::wstring & adapterName,
            const std::wstring & ipAddress,
            const std::vector<std::wstring> & dnsAddresses);

        Common::ErrorCode RestoreDnsServerList();

        Common::ErrorCode RestoreDnsServerListForAdapter(
            const std::wstring & adapterName
        );

        Common::ErrorCode DisableNegativeCache();

        Common::ErrorCode SetHostsFileAcl(bool remove);

        Common::ErrorCode SetStaticDnsChainRegistryValue(
            Common::RegistryKey & key,
            const std::wstring & nameServerValue
        );

        Common::ErrorCode InsertIntoStaticDnsChainAndRefresh(
            const std::wstring & adapterName,
            const std::wstring & ipAddress);

        Common::ErrorCode GetAdapterFriendlyName(
            const std::wstring & adapterName,
            std::wstring & adapterDescription);

        void Cleanup();

    private:
        Common::RwLock lock_;
    };
}
