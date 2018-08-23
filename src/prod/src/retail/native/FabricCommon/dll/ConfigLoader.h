// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ConfigLoader
    {
    public:
        static ConfigStoreDescriptionUPtr CreateConfigStore(HMODULE dllModule);

        static HRESULT FabricGetConfigStore(
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ __RPC__in_opt IFabricConfigStoreUpdateHandler *updateHandler,
            /* [out, retval] */ void ** configStore);

    private:
        static void ProcessFabricConfigSettings(Common::ConfigSettings & settings);
        static void GetNodeIdGeneratorConfig(Common::ConfigSettings & settings, std::wstring & rolesForWhichToUseV1NodeIdGenerator, bool & useV2NodeIdGenerator, std::wstring & nodeIdGeneratorVersion);
        static Common::ErrorCode GenerateNodeIds(Common::ConfigSettings & configSettings);
        static std::wstring GenerateNodeId(std::wstring const & instanceName);
        static Common::ErrorCode ReplaceIPAddresses(Common::ConfigSettings & configSettings);
        static Common::ErrorCode ReplaceIPAddress(
            __inout std::wstring & ipv4Address, 
            __inout std::wstring & ipv6Address,
            std::wstring const & input, 
            __out std::wstring & output);
        static void ReplaceSpecifiedAddressOrFQDN(
            std::wstring const & ipAddressOrFQDN,
            std::wstring const & input, 
            __out std::wstring & output);
        static Common::ErrorCode GetIPv4Address(__out std::wstring & ipv4Address);
        static Common::ErrorCode GetIPv6Address(__out std::wstring & ipv6Address);
        static std::wstring GetIPAddressOrFQDN(Common::ConfigSettings & configSettings);

    };
}
