// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Runtime
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Globalization;

    /// <summary>
    ///Settings that configures the  FabricTransport Listener.
    /// </summary>
    internal class FabricTransportListenerSettings : FabricTransportSettings
    {
        internal const string DefaultEndpointResourceName = "ServiceEndpoint";
        private static readonly string Tracetype = "FabricTransportListenerSettings";
        private static readonly string DefaultPackageName = "Config";

        /// <summary>
        /// Creates a new instance of FabricTransportListenerSettings and initializes properties with default Values.
        /// </summary>
        public FabricTransportListenerSettings()
        {
            this.EndpointResourceName = DefaultEndpointResourceName;
        }

        /// <summary>
        /// EndpointResourceName is name of the endpoint resource specified in ServiceManifest .This is used to obtain the port number on which to
        /// service will listen. 
        /// </summary>
        /// <value>
        /// EndpointResourceName is  name of the  endpoint resource defined in the service manifest.
        /// </value>
        /// <remarks>
        /// Default value of EndpointResourceName  is "ServiceEndpoint" </remarks>
        public string EndpointResourceName { get; set; }

        /// <summary>
        /// Loads the FabricTransport settings from a section specified in the service settings configuration file - settings.xml 
        /// </summary>
        /// <param name="sectionName">Name of the section within the configuration file. if not found , it throws ArgumentException.</param>
        /// <param name="configPackageName"> Name of the configuration package. if not found Settings.xml in the configuration package path, it throws ArgumentException. 
        /// If not specified, default name is "Config"</param>
        /// <returns>FabricTransportListenerSettings</returns>
        /// <remarks>
        /// The following are the parameter names that should be provided in the configuration file,to be recognizable by service fabric to load the transport settings.
        ///     
        ///     1. MaxQueueSize - <see cref="FabricTransportSettings.MaxQueueSize"/>value in long.
        ///     2. MaxMessageSize - <see cref="FabricTransportSettings.MaxMessageSize"/>value in bytes.
        ///     3. MaxConcurrentCalls - <see cref="FabricTransportSettings.MaxConcurrentCalls"/>value in long.
        ///     4. SecurityCredentials - <see cref="FabricTransportSettings.SecurityCredentials"/> value.
        ///     5. OperationTimeoutInSeconds - <see cref="FabricTransportSettings.OperationTimeout"/> value in seconds.
        ///     6. KeepAliveTimeoutInSeconds - <see cref="FabricTransportSettings.KeepAliveTimeout"/> value in seconds.
        /// </remarks>
        public static FabricTransportListenerSettings LoadFrom(string sectionName, string configPackageName = null)
        {
            var settings = new FabricTransportListenerSettings();
            var packageName = configPackageName ?? DefaultPackageName;
            var isInitialized = settings.InitializeConfigFileFromConfigPackage(packageName);

            if (!isInitialized)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, SR.ErrorConfigPackageNotFound,
                    configPackageName));
            }

            isInitialized = settings.InitializeSettingsFromConfig(sectionName);

            if (!isInitialized)
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, SR.ErrorSectionNameNotFound,
                    sectionName));
            }

            AppTrace.TraceSource.WriteInfo(
                Tracetype,
                "MaxMessageSize: {0} , MaxConcurrentCalls: {1} , MaxQueueSize: {2} , OperationTimeoutInSeconds: {3} KeepAliveTimeoutInSeconds : {4} , SecurityCredentials {5}",
                settings.MaxMessageSize,
                settings.MaxConcurrentCalls,
                settings.MaxQueueSize,
                settings.OperationTimeout.TotalSeconds,
                settings.KeepAliveTimeout.TotalSeconds,
                settings.SecurityCredentials.CredentialType);

            return settings;
        }


        /// <summary>
        /// Try to load the FabricTransport settings from a section specified in the service settings configuration file - settings.xml 
        /// </summary>
        /// <param name="sectionName">Name of the section within the configuration file. if not found , it return false</param>
        /// <param name="configPackageName"> Name of the configuration package. if not found Settings.xml in the configuration package path, it return false. 
        /// If not specified, default name is "Config"</param>
        /// <param name="listenerSettings">When this method returns it sets the <see cref="FabricTransportListenerSettings"/> listenersettings if load from Config succeeded. If fails ,its sets listenerSettings to null/> </param>
        /// <returns> <see cref="bool"/> specifies whether the settings get loaded successfully from Config.
        /// It returns true when load from Config succeeded, else return false.</returns>
        /// <remarks>
        /// The following are the parameter names that should be provided in the configuration file,to be recognizable by service fabric to load the transport settings.
        ///     
        ///     1. MaxQueueSize - <see cref="FabricTransportSettings.MaxQueueSize"/>value in long.
        ///     2. MaxMessageSize - <see cref="FabricTransportSettings.MaxMessageSize"/>value in bytes.
        ///     3. MaxConcurrentCalls - <see cref="FabricTransportSettings.MaxConcurrentCalls"/>value in long.
        ///     4. SecurityCredentials - <see cref="FabricTransportSettings.SecurityCredentials"/> value.
        ///     5. OperationTimeoutInSeconds - <see cref="FabricTransportSettings.OperationTimeout"/> value in seconds.
        ///     6. KeepAliveTimeoutInSeconds - <see cref="FabricTransportSettings.KeepAliveTimeout"/> value in seconds.
        /// </remarks>
        public static bool TryLoadFrom(string sectionName, out FabricTransportListenerSettings listenerSettings,
            string configPackageName = null)
        {
            try
            {
                var settings = new FabricTransportListenerSettings();
                var packageName = configPackageName ?? DefaultPackageName;
                var isInitialized = settings.InitializeConfigFileFromConfigPackage(packageName);

                if (!isInitialized)
                {
                    listenerSettings = null;
                    return false;
                }

                isInitialized = settings.InitializeSettingsFromConfig(sectionName);

                if (!isInitialized)
                {
                    listenerSettings = null;
                    return false;
                }

                listenerSettings = settings;

                AppTrace.TraceSource.WriteWarning(
                    Tracetype,
                    "MaxMessageSize: {0} , MaxConcurrentCalls: {1} , MaxQueueSize: {2} , OperationTimeoutInSeconds: {3} KeepAliveTimeoutInSeconds : {4} , SecurityCredentials {5}",
                    settings.MaxMessageSize,
                    settings.MaxConcurrentCalls,
                    settings.MaxQueueSize,
                    settings.OperationTimeout.TotalSeconds,
                    settings.KeepAliveTimeout.TotalSeconds,
                    settings.SecurityCredentials.CredentialType);

                return true;
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteInfo(Tracetype, "Exception thrown while loading from Config {0}", ex);
                listenerSettings = null;
                return false;
            }
        }

        /// <summary>
        /// FabricTransportListenerSettings returns the default Settings .Loads the configuration file from default Config Package"Config" .
        ///</summary>
        /// <param name="sectionName">Name of the section within the configuration file. If not found section in configuration file, it will return the default Settings</param>
        /// <returns></returns>
        internal new static FabricTransportListenerSettings GetDefault(string sectionName = DefaultSectionName)
        {
            FabricTransportListenerSettings listenerSettings = null;
            if (!TryLoadFrom(sectionName, out listenerSettings))
            {
                listenerSettings = new FabricTransportListenerSettings();

                AppTrace.TraceSource.WriteInfo(
                    Tracetype,
                    "Loading Default Settings , MaxMessageSize: {0} , MaxConcurrentCalls: {1} , MaxQueueSize: {2} , OperationTimeoutInSeconds: {3} KeepAliveTimeoutInSeconds : {4} , SecurityCredentials {5}",
                    listenerSettings.MaxMessageSize,
                    listenerSettings.MaxConcurrentCalls,
                    listenerSettings.MaxQueueSize,
                    listenerSettings.OperationTimeout.TotalSeconds,
                    listenerSettings.KeepAliveTimeout.TotalSeconds,
                    listenerSettings.SecurityCredentials.CredentialType);
            }
            return listenerSettings;
        }

        internal FabricTransportListenerAddress GetListenerAddress(ServiceContext serviceContext)
        {
            var replicaId = serviceContext.ReplicaOrInstanceId;
            var partitionId = serviceContext.PartitionId;
            var path = string.Format(CultureInfo.InvariantCulture, "{0}-{1}-{2}", partitionId, replicaId, Guid.NewGuid());
            var port = Helper.GetEndpointPort(serviceContext.CodePackageActivationContext, this.EndpointResourceName);
            return new FabricTransportListenerAddress(serviceContext.ListenAddress, port, path);
        }
    }
}