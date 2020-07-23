// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service
{
    using System.Fabric.Common;

    internal sealed class GRMSettings
    {
        private const string LinuxProxyImageNameKey = "LinuxProxyImageName";
        private const string WindowsProxyImageNameKey = "WindowsProxyImageName";
        private const string ProxyReplicaCountKey = "ProxyReplicaCount";
        private const string ProxyCPUCoresKey = "ProxyCPUCores";
        private const string ProxyCreateToReadyTimeoutInMinutesKey = "ProxyCreateToReadyTimeoutInMinutes";
        private const string ImageStoreConnectionStringKey = "ImageStoreConnectionString";
        private const string IPProviderEnabledKey = "IPProviderEnabled";
        private const string LocalNatIPProviderEnabledKey = "LocalNatIpProviderEnabled";
        private const string IsolatedNetworkSetupKey = "IsolatedNetworkSetup";
        private const string ContainerNetworkSetupKey = "ContainerNetworkSetup";
        private const string ClientCertThumbprintsKey = "ClientCertThumbprints";
        private const string ClusterCertThumbprintsKey = "ClusterCertThumbprints";
        private const string ProxyCPUCoresDefault = "1";

        private static readonly TimeSpan ProxyCreateToReadyTimeoutInMinutesDefault = TimeSpan.FromMinutes(30);

        public GRMSettings()
        {
            this.GRMSectionName = "GatewayResourceManager";
            this.ProxyCreateToReadyTimeoutInMinutes = ProxyCreateToReadyTimeoutInMinutesDefault;
        }

        public string GRMSectionName { get; private set; }

        public string LinuxProxyImageName { get; private set; }

        public string WindowsProxyImageName { get; private set; }

        public string ProxyReplicaCount { get; private set; }

        public string ProxyCPUCores { get; private set; }

        public TimeSpan ProxyCreateToReadyTimeoutInMinutes { get; private set; }

        public string ImageStoreConnectionString { get; private set; }

        public bool IPProviderEnabled { get; private set; }

        public bool LocalNatIPProviderEnabled { get; set; }

        public bool ContainerNetworkSetup { get; private set; }

        public bool IsolatedNetworkSetup { get; private set; }

        public string ClientCertThumbprints { get; set; }

        public string ClusterCertThumbprints { get; set; }

        internal void LoadSettings(NativeConfigStore nativeConfig)
        {
            this.LinuxProxyImageName = nativeConfig.ReadUnencryptedString(this.GRMSectionName, LinuxProxyImageNameKey);
            this.WindowsProxyImageName = nativeConfig.ReadUnencryptedString(this.GRMSectionName, WindowsProxyImageNameKey);
            this.ProxyReplicaCount = nativeConfig.ReadUnencryptedString(this.GRMSectionName, ProxyReplicaCountKey);
            this.ImageStoreConnectionString = nativeConfig.ReadUnencryptedString("Management", ImageStoreConnectionStringKey);
            this.IPProviderEnabled = nativeConfig.ReadUnencryptedBool("Hosting", IPProviderEnabledKey, GatewayResourceManagerTrace.TraceSource, Program.TraceType, false);
            this.LocalNatIPProviderEnabled = nativeConfig.ReadUnencryptedBool("Hosting", LocalNatIPProviderEnabledKey, GatewayResourceManagerTrace.TraceSource, Program.TraceType, false);
            this.IsolatedNetworkSetup = nativeConfig.ReadUnencryptedBool("Setup", IsolatedNetworkSetupKey, GatewayResourceManagerTrace.TraceSource, Program.TraceType, false);
            this.ContainerNetworkSetup = nativeConfig.ReadUnencryptedBool("Setup", ContainerNetworkSetupKey, GatewayResourceManagerTrace.TraceSource, Program.TraceType, false);

            var proxyCPUCores = nativeConfig.ReadUnencryptedString(this.GRMSectionName, ProxyCPUCoresKey);
            this.ProxyCPUCores = string.IsNullOrEmpty(proxyCPUCores) ? ProxyCPUCoresDefault : proxyCPUCores;

            var proxyCreateToReadyTimeoutInMinutes = nativeConfig.ReadUnencryptedString(this.GRMSectionName, ProxyCreateToReadyTimeoutInMinutesKey);
            this.ProxyCreateToReadyTimeoutInMinutes = string.IsNullOrEmpty(proxyCreateToReadyTimeoutInMinutes) ? ProxyCreateToReadyTimeoutInMinutesDefault : TimeSpan.FromMinutes(int.Parse(proxyCreateToReadyTimeoutInMinutes));

            try
            {
                this.ClientCertThumbprints = nativeConfig.ReadUnencryptedString("Security", ClientCertThumbprintsKey);
            }
            catch (Exception)
            {
                this.ClientCertThumbprints = string.Empty;
            }

            try
            {
                this.ClusterCertThumbprints = nativeConfig.ReadUnencryptedString("Security", ClusterCertThumbprintsKey);
            }
            catch (Exception)
            {
                this.ClusterCertThumbprints = string.Empty;
            }
        }
    }
}
