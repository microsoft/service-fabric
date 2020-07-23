// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;

    internal class HostingSettings
    {
        internal string ContainerPackageRootFolder { get; private set; }

        internal string ContainerFabricBinRootFolder { get; private set; }

        internal string ContainerFabricLogRootFolder { get; private set; }

        internal string ContainerAppDeploymentRootFolder { get; private set; }

        internal string DefaultContainerNetwork { get; private set; }

        internal string DefaultNatNetwork { get; private set; }

        internal TimeSpan DockerRequestTimeout { get; private set; }

        internal TimeSpan ContainerImageDownloadTimeout { get; private set; }

        internal TimeSpan ContainerDeactivationTimeout { get; private set; }

        internal int ContainerDeactivationRetryDelayInSec { get; private set; }

        internal int ContainerTerminationMaxRetryCount { get; private set; }

        internal int ContainerEventManagerMaxContinuousFailure { get; private set; }

        internal int ContainerEventManagerFailureBackoffInSec { get; private set; }

        internal bool EnableDockerHealthCheckIntegration { get; private set; }

        internal string ContainerServiceNamedPipeOrUnixSocketAddress { get; private set; }

        internal int DeadContainerCleanupUntilInMinutes { get; private set; }

        internal int ContainerCleanupScanIntervalInMinutes { get; private set; }

        internal string DefaultContainerRepositoryAccountName { get; private set; }

        internal string DefaultContainerRepositoryPassword { get; private set; }

        internal bool IsDefaultContainerRepositoryPasswordEncrypted { get; private set; }
        
        internal string ContainerRepositoryCredentialTokenEndPoint { get; private set; }

        internal string DefaultMSIEndpointForTokenAuthentication { get; private set; }

        internal string DefaultContainerRepositoryPasswordType { get; private set; }

        internal bool DisableDockerRequestRetry { get; private set; }

        internal bool LocalNatIpProviderEnabled { get; private set; }

        internal string LocalNatIpProviderNetworkName { get; private set; }

        internal static unsafe HostingSettings LoadHostingSettings()
        {
            var nativeRes = NativeContainerActivatorService.LoadHostingSettings();
            var uncastedSettings = nativeRes.get_Result();

            var nativeSettings = (NativeTypes.FABRIC_HOSTING_SETTINGS*)uncastedSettings;

            var nativeSettingMap = NativeTypes.FromNativeStringPairList(nativeSettings->SettingsMap);

            var settings = new HostingSettings();

            return new HostingSettings
            {
                ContainerPackageRootFolder = nativeSettingMap["ContainerPackageRootFolder"],
                ContainerFabricBinRootFolder = nativeSettingMap["ContainerFabricBinRootFolder"],
                ContainerFabricLogRootFolder = nativeSettingMap["ContainerFabricLogRootFolder"],
                ContainerAppDeploymentRootFolder = nativeSettingMap["ContainerAppDeploymentRootFolder"],
                DockerRequestTimeout = TimeSpan.FromSeconds(int.Parse(nativeSettingMap["DockerRequestTimeout"])),
                ContainerImageDownloadTimeout = TimeSpan.FromSeconds(int.Parse(nativeSettingMap["ContainerImageDownloadTimeout"])),
                ContainerDeactivationTimeout = TimeSpan.FromSeconds(int.Parse(nativeSettingMap["ContainerDeactivationTimeout"])),
                ContainerDeactivationRetryDelayInSec = int.Parse(nativeSettingMap["ContainerDeactivationRetryDelayInSec"]),
                ContainerTerminationMaxRetryCount = int.Parse(nativeSettingMap["ContainerTerminationMaxRetryCount"]),
                ContainerEventManagerMaxContinuousFailure = int.Parse(nativeSettingMap["ContainerEventManagerMaxContinuousFailure"]),
                ContainerEventManagerFailureBackoffInSec = int.Parse(nativeSettingMap["ContainerEventManagerFailureBackoffInSec"]),
                EnableDockerHealthCheckIntegration = bool.Parse(nativeSettingMap["EnableDockerHealthCheckIntegration"]),
                ContainerServiceNamedPipeOrUnixSocketAddress = nativeSettingMap["ContainerServiceNamedPipeOrUnixSocketAddress"],
                DeadContainerCleanupUntilInMinutes = int.Parse(nativeSettingMap["DeadContainerCleanupUntilInMinutes"]),
                ContainerCleanupScanIntervalInMinutes = int.Parse(nativeSettingMap["ContainerCleanupScanIntervalInMinutes"]),
                DefaultContainerRepositoryAccountName = nativeSettingMap["DefaultContainerRepositoryAccountName"],
                DefaultContainerRepositoryPassword = nativeSettingMap["DefaultContainerRepositoryPassword"],
                IsDefaultContainerRepositoryPasswordEncrypted = bool.Parse(nativeSettingMap["IsDefaultContainerRepositoryPasswordEncrypted"]),
                DefaultContainerNetwork = nativeSettingMap["DefaultContainerNetwork"],
                DefaultNatNetwork = nativeSettingMap["DefaultNatNetwork"],
                ContainerRepositoryCredentialTokenEndPoint = nativeSettingMap["ContainerRepositoryCredentialTokenEndPoint"],
                DefaultMSIEndpointForTokenAuthentication = nativeSettingMap["DefaultMSIEndpointForTokenAuthentication"],
                DefaultContainerRepositoryPasswordType = nativeSettingMap["DefaultContainerRepositoryPasswordType"],
                DisableDockerRequestRetry = bool.Parse(nativeSettingMap["DisableDockerRequestRetry"]),
                LocalNatIpProviderEnabled = bool.Parse(nativeSettingMap["LocalNatIpProviderEnabled"]),
                LocalNatIpProviderNetworkName = nativeSettingMap["LocalNatIpProviderNetworkName"]
            };
        }
    }
}
