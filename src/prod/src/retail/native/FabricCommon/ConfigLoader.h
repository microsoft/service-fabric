// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ConfigLoader
    {
        DENY_COPY(ConfigLoader)

    public:
        ConfigLoader();
        ~ConfigLoader();

        static ConfigLoader & GetConfigLoader();

        static HRESULT FabricGetConfigStore(
            HMODULE dllModule,
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ __RPC__in_opt IFabricConfigStoreUpdateHandler *updateHandler,
            /* [out, retval] */ void ** configStore);

        static HRESULT FabricGetConfigStoreEnvironmentVariable( 
            /* [out] */ __RPC__deref_out_opt IFabricStringResult **envVariableName,
            /* [out] */ __RPC__deref_out_opt IFabricStringResult **envVariableValue);

    private:
        ConfigStoreSPtr FabricGetConfigStore(HMODULE dllModule);
        ErrorCode CreateConfigStore_CallerHoldsLock(HMODULE dllModule);
        void FabricGetConfigStoreEnvironmentVariable(__out std::wstring & envVariableName, __out std::wstring & envVariableValue);

        static Common::ErrorCode GetStoreTypeAndLocation(HMODULE dllModule,
            __out ConfigStoreType::Enum & storeType, 
            __out std::wstring & storeLocation);
        static void MakeAbsolute(std::wstring & storeLocation);
        
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

    private:
        Common::ExclusiveLock lock_;
        Common::ConfigStoreSPtr store_;
        std::wstring storeEnvironmentVariableName_;
        std::wstring storeEnvironmentVariableValue_;

        static ConfigLoader * singleton_;
        static INIT_ONCE initOnceConfigLoader_;
        static BOOL CALLBACK InitConfigLoaderFunction(PINIT_ONCE, PVOID, PVOID *);
    };
}
