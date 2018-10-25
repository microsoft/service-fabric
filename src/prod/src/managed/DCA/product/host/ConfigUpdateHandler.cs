// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Dca;
    using System.Threading;

    // Configuration update handler class
    internal class ConfigUpdateHandler : IDisposable, IConfigStoreUpdateHandler
    {
        // Constants
        private const string TraceType = "ConfigUpdate";
        private const string TimerId = "ClusterManifestChangeProcessingTimer";
        private const string ConfigurationChangeReactionTimeInSeconds = "ConfigurationChangeReactionTimeInSeconds";
        private static readonly TimeSpan DefaultConfigChangeReactionTime = TimeSpan.FromSeconds(5);

        // Updates that can be ignored because of one of the following reasons:
        // - we always read the current value instead of caching it
        // - we do not support update of a particular value
        private static readonly Dictionary<string, HashSet<string>> UpdatesToIgnore = new Dictionary<string, HashSet<string>>()
        {
            {
                ConfigReader.DiagnosticsSectionName,
                new HashSet<string>(StringComparer.Ordinal)
                {
                    ConfigurationChangeReactionTimeInSeconds, // reason: no caching
#if !DotNetCoreClrLinux
                    AppInstanceEtlFileDataReader.EtlFileReadIntervalInMinutesParamName, // reason: no caching
                    AppInstanceEtlFileDataReader.TestOnlyEtlFileReadIntervalInSecondsParamName, // reason: no caching
                    AppInstanceEtlFileDataReader.EtlFileFlushIntervalInSecondsParamName, // reason: no caching
                    AppInstanceEtlFileDataReader.TestOnlyAppDataDeletionIntervalSecondsParamName, // reason: not supported
#endif
                    Utility.AppDiagnosticStoreAccessRequiresImpersonationValueName, // reason: not supported
                }
            }
        };

        // Event that is set when the Windows Fabric application has been created
        private readonly ManualResetEvent windowsFabricApplicationCreated;

        // Set of sections whose configuration has changed
        private readonly HashSet<string> changedSections;

        // Timer to delivery changed sections to application instance manager
        private readonly DcaTimer changeDeliveryTimer;

        // Lock used to serialize timer callbacks
        private readonly object changeDeliveryLock;

        // The application instance manager
        private AppInstanceManager appInstanceMgr;

        // Whether or not configuration updates should be processed
        private bool ignoreConfigurationUpdates;

        // Time after which we react to configuration changes
        private TimeSpan currentConfigChangeReactionTime;

        // Whether or not the object has been disposed
        private bool disposed;

        internal ConfigUpdateHandler()
        {
            this.ignoreConfigurationUpdates = true;
            this.currentConfigChangeReactionTime = TimeSpan.FromDays(1);
            this.windowsFabricApplicationCreated = new ManualResetEvent(false);
            this.changedSections = new HashSet<string>();
            this.changeDeliveryLock = new object();

            // Create the timer that will handle the configuration change after the
            // reaction time has elapsed. Using a reaction time helps in collapsing
            // multiple changes that might occur close together in time. This avoids
            // repeated updating of the data collector in response to each of those
            // changes.
            this.changeDeliveryTimer = new DcaTimer(
                                               TimerId,
                                               this.DeliverChangedSections,
                                               DefaultConfigChangeReactionTime);
        }

        public bool OnUpdate(string sectionName, string keyName)
        {
            if (this.ignoreConfigurationUpdates)
            {
                // The objects that we access as part of config update processing have
                // not yet been initialized. Ignore the config update.
                return true;
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Configuration change in section {0}, key {1}.",
                sectionName,
                keyName);

            if (UpdatesToIgnore.ContainsKey(sectionName) &&
                UpdatesToIgnore[sectionName].Contains(keyName))
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Section {0}, key {1} was found in the ignore-change list, so the configuration change will be ignored.",
                    sectionName,
                    keyName);
                return true;
            }

            // Add the changed section to the list of changed sections
            bool startTimer = false;
            lock (this.changedSections)
            {
                if (0 == this.changedSections.Count)
                {
                    // Currently, this is the only section that has changed.
                    // Deliver it to application instance manager after the 
                    // reaction time period has elapsed, so that multiple 
                    // changes occurring close together in time may be collapsed.
                    startTimer = true;
                }

                this.changedSections.Add(sectionName);
            }

            // If necessary start the timer to deliver the changes
            // to the application instance manager.
            if (startTimer)
            {
                this.changeDeliveryTimer.Start(this.GetReactionTime());
            }
            else
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Configuration change processing timer is already running, so this change will be processed when the timer is signaled.");
            }

            return true;
        }

        public bool CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Start ignoring configuration changes
            this.ignoreConfigurationUpdates = true;

            // Stop and dispose the change delivery timer
            this.changeDeliveryTimer.StopAndDispose();
            this.changeDeliveryTimer.DisposedEvent.WaitOne();

            this.windowsFabricApplicationCreated.Dispose();

            GC.SuppressFinalize(this);
        }

        internal void StartProcessingConfigUpdates()
        {
            // Set the flag to indicate that we should start processing 
            // configuration changes.
            this.ignoreConfigurationUpdates = false;
        }

        internal void StartConfigUpdateDeliveryToAppInstanceMgr(AppInstanceManager appInstanceMgr)
        {
            this.appInstanceMgr = appInstanceMgr;
            bool result = this.windowsFabricApplicationCreated.Set();
            if (false == result)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Error setting event to indicate that config updates can be delivered to app instance manager.");
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Configuration change delivery to application instance manager is enabled.");
        }

        private TimeSpan GetReactionTime()
        {
            // Figure out the reaction time for configuration changes
            // NOTE: We DO NOT cache this value. Instead, we read it 
            // from the config store each time. This is because the 
            // config can change and we always want to the current value.
            var configChangeReactionTime = TimeSpan.FromSeconds(Utility.GetUnencryptedConfigValue(
                                                  ConfigReader.DiagnosticsSectionName,
                                                  ConfigurationChangeReactionTimeInSeconds,
                                                  (int)DefaultConfigChangeReactionTime.TotalSeconds));
            if (this.currentConfigChangeReactionTime != configChangeReactionTime)
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Reaction time for cluster manifest configuration changes: {0} seconds",
                    configChangeReactionTime);
                this.currentConfigChangeReactionTime = configChangeReactionTime;
            }

            return configChangeReactionTime;
        }

        private void DeliverChangedSections(object state)
        {
            lock (this.changeDeliveryLock)
            {
                // Wait until the Windows Fabric application has been created,
                // because these changes are meant for it.
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Waiting until configuration changes can be delivered to application instance manager.");
                this.windowsFabricApplicationCreated.WaitOne();

                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Wait satisfied. Identifying configuration changes to be delivered to application instance manager ...");

                // Get the sections that have changed
                HashSet<string> sectionsToDeliver;
                lock (this.changedSections)
                {
                    sectionsToDeliver = new HashSet<string>(this.changedSections);
                    this.changedSections.Clear();
                }

                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Found {0} changed sections to deliver to application instance manager.",
                    sectionsToDeliver.Count);

                // If no sections have changed, then return immediately.
                if (0 == sectionsToDeliver.Count)
                {
                    return;
                }

                // Deliver the changed sections to the application instance manager
                this.appInstanceMgr.UpdateApplicationInstance(
                    Utility.WindowsFabricApplicationInstanceId,
                    null,
                    sectionsToDeliver);
            }
        }
    }
}