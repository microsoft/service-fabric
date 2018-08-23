// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using System.Linq;

    // This class manages the data collector objects for an application instance
    internal class AppInstance : IDisposable
    {
        // Constants
        private const string TraceType = "AppInstance";

        private readonly string applicationInstanceId;
        private readonly DiskSpaceManager diskSpaceManager;

        // Data collector
        private FabricDCA dataCollector;

        // Whether or not the object has been disposed
        private bool disposed;

        internal AppInstance(string applicationInstanceId, AppConfig appConfig, string servicePackageName, ServiceConfig serviceConfig, DiskSpaceManager diskSpaceManager)
        {
            this.applicationInstanceId = applicationInstanceId;
            this.diskSpaceManager = diskSpaceManager;

            // Make the configuration available to the application
            ConfigReader.AddAppConfig(this.applicationInstanceId, appConfig);

            if (null != servicePackageName)
            {
                ConfigReader.AddServiceConfig(this.applicationInstanceId, servicePackageName, serviceConfig);
            }

            // Create the data collector for the application instance
            this.dataCollector = new FabricDCA(this.applicationInstanceId, diskSpaceManager);
        }

        public void FlushData()
        {
            this.dataCollector.FlushData();
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (GetTestOnlyFlushDataOnDispose())
            {
                this.dataCollector.FlushData();
            }

            // Dispose the data collector
            this.dataCollector.Dispose();

            // Remove the current configuration object
            ConfigReader.RemoveAppConfig(this.applicationInstanceId);
            ConfigReader.RemoveServiceConfigsForApp(this.applicationInstanceId);
            GC.SuppressFinalize(this);
        }

        internal void Update(AppConfig appConfig, HashSet<string> changedSections)
        {
            // Figure out whether the data collector is interested in any of the
            // changed sections. If so, it needs to be restarted.
            IEnumerable<string> relevantChanges = Enumerable.Intersect(
                                                        this.dataCollector.RegisteredAppConfigSections,
                                                        changedSections);
            if (0 == relevantChanges.Count())
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Configuration change did not require the data collector for application instance {0} to be restarted.",
                    this.applicationInstanceId);
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Data collector for application instance {0} will be restarted because the application configuration has changed.",
                this.applicationInstanceId);

            // Dispose the current data collector
            this.dataCollector.Dispose();

            // Update the configuration
            ConfigReader.AddAppConfig(this.applicationInstanceId, appConfig);

            // Create a new data collector
            this.dataCollector = new FabricDCA(this.applicationInstanceId, this.diskSpaceManager);
        }

        internal void AddService(AppConfig appConfig, string servicePackageName, ServiceConfig serviceConfig)
        {
            bool restartNeeded = false;
            if (this.ServiceConfigHasChanged(servicePackageName, serviceConfig))
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Data collector for application instance {0} will be restarted because service package {1} was activated.",
                    this.applicationInstanceId,
                    servicePackageName);
                restartNeeded = true;
            }
            else if (this.AppConfigHasChanged(appConfig))
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Data collector for application instance {0} will be restarted because the application configuration has changed.",
                    this.applicationInstanceId);
                restartNeeded = true;
            }
            else
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Activation of service package {0} does not require the data collector for application instance {1} to be restarted.",
                    servicePackageName,
                    this.applicationInstanceId);
            }

            if (restartNeeded)
            {
                // Dispose the current data collector
                this.dataCollector.Dispose();

                // Update the configuration
                ConfigReader.AddAppConfig(this.applicationInstanceId, appConfig);
                ConfigReader.AddServiceConfig(this.applicationInstanceId, servicePackageName, serviceConfig);

                // Create a new data collector
                this.dataCollector = new FabricDCA(this.applicationInstanceId, this.diskSpaceManager);
            }
        }

        internal void RemoveService(string servicePackageName)
        {
            ServiceConfig serviceConfig = ConfigReader.GetServiceConfig(
                                              this.applicationInstanceId, 
                                              servicePackageName);
            if (null == serviceConfig)
            {
                // We are not interested in this service, so there's nothing
                // to do here.
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Deactivation of service package {0} does not require the data collector for application instance {1} to be restarted.",
                    servicePackageName,
                    this.applicationInstanceId);
                return;
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Data collector for application instance {0} will be restarted because service package {1} was deactivated.",
                this.applicationInstanceId,
                servicePackageName);

            // Dispose the current data collector
            this.dataCollector.Dispose();

            // Update the configuration
            ConfigReader.RemoveServiceConfig(this.applicationInstanceId, servicePackageName);

            // Create a new data collector
            this.dataCollector = new FabricDCA(this.applicationInstanceId, this.diskSpaceManager);
        }

        private static bool GetTestOnlyFlushDataOnDispose()
        {
            return Utility.GetUnencryptedConfigValue("Diagnostics", "TestOnlyFlushDataOnDispose", false);
        }

        private bool AppConfigHasChanged(AppConfig appConfig)
        {
            AppConfig currentAppConfig = ConfigReader.GetAppConfig(this.applicationInstanceId);
            HashSet<string> changedSections = AppConfig.GetChangedSections(
                                                  currentAppConfig,
                                                  appConfig);

            // Figure out whether the data collector is interested in any of the
            // changed sections. If so, it needs to be restarted.
            IEnumerable<string> relevantChanges = Enumerable.Intersect(
                                                        this.dataCollector.RegisteredAppConfigSections,
                                                        changedSections);
            return 0 != relevantChanges.Count();
        }

        private bool ServiceConfigHasChanged(string servicePackageName, ServiceConfig serviceConfig)
        {
            ServiceConfig currentServiceConfig = ConfigReader.GetServiceConfig(
                                                     this.applicationInstanceId, 
                                                     servicePackageName);

            HashSet<string> changedSections = (null == currentServiceConfig) ?
                                                serviceConfig.GetDiagnosticSections() :
                                                ServiceConfig.GetChangedSections(
                                                  currentServiceConfig,
                                                  serviceConfig);

            // Figure out whether the data collector is interested in any of the
            // changed sections. If so, it needs to be restarted.
            IEnumerable<string> relevantChanges = Enumerable.Intersect(
                                                        this.dataCollector.RegisteredServiceConfigSections,
                                                        changedSections);
            return relevantChanges.Any();
        }
    }
}