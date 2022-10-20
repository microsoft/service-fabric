// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{

    class FabricEnvironment
    {

    public:
        static Common::GlobalWString FabricDataRootEnvironmentVariable;
        static Common::GlobalWString FabricRootEnvironmentVariable;
        static Common::GlobalWString FabricBinRootEnvironmentVariable;
        static Common::GlobalWString FabricLogRootEnvironmentVariable;
        static Common::GlobalWString EnableCircularTraceSessionEnvironmentVariable;
        static Common::GlobalWString FabricCodePathEnvironmentVariable;
        static Common::GlobalWString HostedServiceNameEnvironmentVariable;
        static Common::GlobalWString FileConfigStoreEnvironmentVariable;
        static Common::GlobalWString PackageConfigStoreEnvironmentVariable;
        static Common::GlobalWString SettingsConfigStoreEnvironmentVariable;
        static Common::GlobalWString RemoveNodeConfigurationEnvironmentVariable;

        static Common::GlobalWString LogDirectoryRelativeToDataRoot;
        static Common::GlobalWString WindowsFabricRemoveNodeConfigurationRegValue;

        static Common::GlobalWString FabricDnsServerIPAddressEnvironmentVariable;
        static Common::GlobalWString FabricIsolatedNetworkInterfaceNameEnvironmentVariable;

        static Common::GlobalWString FabricHostServicePathEnvironmentVariable;
        static Common::GlobalWString UpdaterServicePathEnvironmentVariable;

        static Common::ErrorCode GetStoreTypeAndLocation(HMODULE dllModule,
            __out ConfigStoreType::Enum & storeType,
            __out std::wstring & storeLocation);

        static Common::ErrorCode GetInfrastructureManifest(
            std::wstring const &,
            __out std::wstring &);

        static Common::ErrorCode GetFabricRoot(
            __out std::wstring &);

        static Common::ErrorCode GetFabricRoot(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetFabricBinRoot(
            __out std::wstring &);

        static Common::ErrorCode GetFabricBinRoot(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetFabricCodePath(
            __out std::wstring &);

        static Common::ErrorCode GetFabricCodePath(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetFabricCodePath(
            HMODULE dllModule, __out std::wstring & fabricCodePath);

        static Common::ErrorCode GetFabricCodePath(
            HMODULE dllModule, LPCWSTR machineName, __out std::wstring & fabricCodePath);

        static Common::ErrorCode GetFabricDataRoot(
            __out std::wstring &);

        static Common::ErrorCode GetFabricDataRoot(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetFabricLogRoot(
            __out std::wstring &);

        static Common::ErrorCode GetFabricLogRoot(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetEnableCircularTraceSession(
            __out BOOLEAN &);

        static Common::ErrorCode GetEnableCircularTraceSession(
            LPCWSTR machineName, __out BOOLEAN &);

        static Common::ErrorCode GetFabricDnsServerIPAddress(
            __out std::wstring &);

        static Common::ErrorCode GetFabricDnsServerIPAddress(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetFabricIsolatedNetworkInterfaceName(
            __out std::wstring &);

        static Common::ErrorCode GetFabricIsolatedNetworkInterfaceName(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetFabricHostServicePath(
            __out std::wstring &);

        static Common::ErrorCode GetFabricHostServicePath(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode GetUpdaterServicePath(
            __out std::wstring &);

        static Common::ErrorCode GetUpdaterServicePath(
            LPCWSTR machineName, __out std::wstring &);

        static Common::ErrorCode SetFabricRoot(
            const std::wstring &);

        static Common::ErrorCode SetFabricRoot(
            const std::wstring &, LPCWSTR machineName);

        static Common::ErrorCode SetFabricBinRoot(
            const std::wstring &);

        static Common::ErrorCode SetFabricBinRoot(
            const std::wstring &, LPCWSTR machineName);

        static Common::ErrorCode SetFabricCodePath(
            const std::wstring &);

        static Common::ErrorCode SetFabricCodePath(
            const std::wstring &, LPCWSTR machineName);

        static Common::ErrorCode SetFabricDataRoot(
            const std::wstring &);

        static Common::ErrorCode SetFabricDataRoot(
            const std::wstring &, LPCWSTR machineName);

        static Common::ErrorCode SetFabricLogRoot(
            const std::wstring &);

        static Common::ErrorCode SetFabricLogRoot(
            const std::wstring &, LPCWSTR machineName);

        static Common::ErrorCode SetEnableCircularTraceSession(
            BOOLEAN, LPCWSTR machineName);

        static Common::ErrorCode SetEnableUnsupportedPreviewFeatures(
            BOOLEAN, LPCWSTR machineName);

        static Common::ErrorCode GetEnableUnsupportedPreviewFeatures(
            __out bool &);

        static Common::ErrorCode SetIsSFVolumeDiskServiceEnabled(
            BOOLEAN, LPCWSTR machineName);

        static Common::ErrorCode GetIsSFVolumeDiskServiceEnabled(
            __out bool &);

        static void GetFabricTracesTestKeyword(
            __out std::wstring &);

        static Common::ErrorCode GetRemoveNodeConfigurationValue(__out std::wstring &);

        static Common::ErrorCode GetFabricVersion(__out std::wstring &);

#if defined(PLATFORM_UNIX)
        static Common::ErrorCode GetLinuxPackageManagerType(__out Common::LinuxPackageManagerType::Enum &);

        static Common::ErrorCode SetSfInstalledMoby(
            LPCWSTR fileContents);
#endif

    private:
        static Common::ErrorCode GetRegistryKeyHelper(
            const std::wstring &, const std::wstring &, ErrorCode, LPCWSTR, __out std::wstring &);

        static Common::ErrorCode GetRegistryKeyHelper(
            const std::wstring &, const std::wstring &, ErrorCode, LPCWSTR, __out DWORD &);

        static Common::ErrorCode GetRegistryKeyHelper(
            RegistryKey &, const std::wstring &, ErrorCode, __out std::wstring &);

        static Common::ErrorCode GetRegistryKeyHelper(
            RegistryKey &, const std::wstring &, ErrorCode, __out DWORD &);

        static Common::ErrorCode SetRegistryKeyHelper(
            const std::wstring &, const std::wstring &, LPCWSTR);

        static Common::ErrorCode SetRegistryKeyHelper(
            const std::wstring &, DWORD, LPCWSTR);

        static Common::ErrorCode SetRegistryKeyHelper(
            RegistryKey &, const std::wstring &, const std::wstring &);

        static Common::ErrorCode SetRegistryKeyHelper(
            RegistryKey &, const std::wstring &, DWORD);

        static Common::ErrorCode GetEtcConfigValue(
            const std::wstring &, ErrorCode, __out std::wstring &);

        static Common::ErrorCode GetEtcConfigValue(
            const std::wstring &, ErrorCode, __out DWORD &);

        static Common::ErrorCode SetEtcConfigValue(
            const std::wstring &, const std::wstring &);

        static Common::ErrorCode SetEtcConfigValue(
            const std::wstring &, DWORD);

    private:
#if defined(PLATFORM_UNIX)
        static Common::LinuxPackageManagerType::Enum packageManagerType_;
#endif
    };
}
