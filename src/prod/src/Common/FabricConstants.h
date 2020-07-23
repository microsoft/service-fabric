// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FabricConstants
    {
    public:
        static Common::GlobalWString WindowsFabricAllowedUsersGroupName;
        static Common::GlobalWString WindowsFabricAllowedUsersGroupComment;
        static Common::GlobalWString WindowsFabricAdministratorsGroupName;
        static Common::GlobalWString WindowsFabricAdministratorsGroupComment;
        static Common::GlobalWString CurrentClusterManifestFileName;
        static Common::GlobalWString InfrastructureManifestFileName;
        static Common::GlobalWString TargetInformationFileName;
        static Common::GlobalWString FabricFolderName;
        static Common::GlobalWString FabricDataFolderName;
        static Common::GlobalWString DefaultFileConfigStoreLocation;
        static Common::GlobalWString DefaultPackageConfigStoreLocation;
        static Common::GlobalWString DefaultSettingsConfigStoreLocation;
        static Common::GlobalWString DefaultClientSettingsConfigStoreLocation;
        static Common::GlobalWString FabricGatewayName;
        static Common::GlobalWString FabricApplicationGatewayName;
        static Common::GlobalWString AppsFolderName;

        static Common::GlobalWString FabricRegistryKeyPath;
        static Common::GlobalWString FabricEtcConfigPath;
        static Common::GlobalWString FabricRootRegKeyName;
        static Common::GlobalWString FabricBinRootRegKeyName;
        static Common::GlobalWString FabricCodePathRegKeyName;
        static Common::GlobalWString FabricDataRootRegKeyName;
        static Common::GlobalWString FabricLogRootRegKeyName;
        static Common::GlobalWString EnableCircularTraceSessionRegKeyName;
        static Common::GlobalWString EnableUnsupportedPreviewFeaturesRegKeyName;
        static Common::GlobalWString IsSFVolumeDiskServiceEnabledRegKeyName;
        static Common::GlobalWString UseFabricInstallerSvcKeyName;
        static Common::GlobalWString FabricHostServicePathRegKeyName;
        static Common::GlobalWString UpdaterServicePathRegKeyName;
#if defined(PLATFORM_UNIX)
        static Common::GlobalWString SfInstalledMobyRegKeyName;
#endif

        static Common::GlobalWString FabricDnsServerIPAddressRegKeyName;
        static Common::GlobalWString FabricIsolatedNetworkInterfaceRegKeyName;

        static DWORD MaxFileSize;
    };
}
