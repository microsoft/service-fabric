// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Strings;

    internal class HostingConfig : IConfigStoreUpdateHandler
    {
        private readonly string HostingSectionName = "Hosting";

        // Used to receive update notification.
        private readonly IConfigStore configStore;
        private readonly RwLock configLock;

        private HostingSettings hostingSettings;

        internal static void Initialize()
        {
            Config = new HostingConfig();
        }

        internal static HostingConfig Config { get; private set; }

        private HostingConfig()
        {
            this.hostingSettings = HostingSettings.LoadHostingSettings();
            this.configLock = new RwLock();
            this.configStore = NativeConfigStore.FabricGetConfigStore(this);
        }

        #region Hosting Configs

        internal string ContainerPackageRootFolder
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerPackageRootFolder;
                }
            }
        }

        internal string ContainerFabricBinRootFolder
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerFabricBinRootFolder;
                }
            }
        }

        internal string ContainerFabricLogRootFolder
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerFabricLogRootFolder;
                }
            }
        }

        internal string ContainerAppDeploymentRootFolder
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerAppDeploymentRootFolder;
                }
            }
        }

        internal TimeSpan DockerRequestTimeout
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DockerRequestTimeout;
                }
            }
        }

        internal TimeSpan ContainerImageDownloadTimeout
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerImageDownloadTimeout;
                }
            }
        }

        internal TimeSpan ContainerDeactivationTimeout
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerDeactivationTimeout;
                }
            }
        }

        internal int ContainerDeactivationRetryDelayInSec
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerDeactivationRetryDelayInSec;
                }
            }
        }

        internal int ContainerTerminationMaxRetryCount
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerTerminationMaxRetryCount;
                }
            }
        }

        internal int ContainerEventManagerMaxContinuousFailure
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerEventManagerMaxContinuousFailure;
                }
            }
        }

        internal int ContainerEventManagerFailureBackoffInSec
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerEventManagerFailureBackoffInSec;
                }
            }
        }

        internal bool EnableDockerHealthCheckIntegration
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.EnableDockerHealthCheckIntegration;
                }
            }
        }

        internal string ContainerServiceNamedPipeOrUnixSocketAddress
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerServiceNamedPipeOrUnixSocketAddress;
                }
            }
        }

        internal int DeadContainerCleanupUntilInMinutes
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DeadContainerCleanupUntilInMinutes;
                }
            }
        }

        internal int ContainerCleanupScanIntervalInMinutes
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerCleanupScanIntervalInMinutes;
                }
            }
        }

        internal string DefaultContainerRepositoryAccountName
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DefaultContainerRepositoryAccountName;
                }
            }
        }


        internal string DefaultContainerRepositoryPassword
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {

                    return hostingSettings.DefaultContainerRepositoryPassword;
                }
            }
        }
 
        internal bool IsDefaultContainerRepositoryPasswordEncrypted
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.IsDefaultContainerRepositoryPasswordEncrypted;
                }
            }
        }
        internal string DefaultContainerRepositoryPasswordType
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DefaultContainerRepositoryPasswordType;
                }
            }
        }

        internal string ContainerDefaultNetwork
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DefaultContainerNetwork;
                }
            }
        }


        internal string ContainerRepositoryCredentialTokenEndPoint
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.ContainerRepositoryCredentialTokenEndPoint;
                }
            }
        }

        internal string DefaultNatNetwork
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DefaultNatNetwork;
                }
            }
        }

        internal string DefaultMSIEndpointForTokenAuthentication
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DefaultMSIEndpointForTokenAuthentication;
                }
            }
        }

        internal bool DisableDockerRequestRetry
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.DisableDockerRequestRetry;
                }
            }
        }

        internal bool LocalNatIpProviderEnabled
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.LocalNatIpProviderEnabled;
                }
            }
        }

        internal string LocalNatIpProviderNetworkName
        {
            get
            {
                using (this.configLock.AcquireReadLock())
                {
                    return hostingSettings.LocalNatIpProviderNetworkName;
                }
            }
        }

        #endregion

        #region Update Handlers

        bool IConfigStoreUpdateHandler.CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(StringResources.Error_InvalidOperation);
        }

        bool IConfigStoreUpdateHandler.OnUpdate(string sectionName, string keyName)
        {
            if(sectionName.Equals(HostingSectionName, StringComparison.OrdinalIgnoreCase))
            {
                using (this.configLock.AcquireWriteLock())
                {
                    this.hostingSettings = HostingSettings.LoadHostingSettings();
                }
            }

            return true;
        }

        #endregion
    }
}