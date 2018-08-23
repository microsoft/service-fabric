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
    using System.IO;
    using System.Linq;

    // This class manages the service package table
    internal class ServicePackageTableManager : IDisposable
    {
        // Constants
        private const string TraceType = "ServicePackageTableManager";
        private const string AppInstanceDataDirName = "AppInstanceData";
        private const string AppInstanceTableDirName = "Table";
        private const string AppInstanceDataEtlDirName = "Etl";
        private const string HostingSectionName = "Hosting";
        private const string ServicePackageNotificationIntervalInSecondsParamName = "DcaNotificationIntervalInSeconds";
        private const int DefaultServicePackageNotificationIntervalInSeconds = 180;
        private const int MaxServicePackageInactiveTimeMultiplier = 5;
        private const int DefaultServicePackageDeactivationTimeInSeconds = 
                            DefaultServicePackageNotificationIntervalInSeconds * MaxServicePackageInactiveTimeMultiplier;

        private const string InactiveServicePackageScanTimerId = "InactiveServicePackageScanTimer";

        // Application instance manager
        private readonly AppInstanceManager applicationInstanceManager;

        // Service package table
        private readonly ServicePackageTable servicePackageTable;

        // Object that manages backing of up service package information in a file on disk
        private readonly ServicePackageTableBackup tableBackup;

#if !DotNetCoreClrLinux
        // Object that manages reading of application instance related events from ETL files
        private readonly AppInstanceEtlFileDataReader etlFileReader;
#endif
        // Timer used to scan for inactive service packages
        private readonly DcaTimer inactiveServicePackageScanTimer;

        // Maximum time for which a service package is allowed to remain inactive 
        // before it is deactivated
        private int maxServicePackageInactiveTimeSeconds;

        // Whether or not the object has been disposed
        private bool disposed;

        internal ServicePackageTableManager(AppInstanceManager appInstanceMgr)
        {
            this.applicationInstanceManager = appInstanceMgr;
            this.servicePackageTable = new ServicePackageTable();

            // Get the interval at which we need to read ETL files
            int servicePackageNotificationIntervalSeconds = Utility.GetUnencryptedConfigValue(
                                                               HostingSectionName,
                                                               ServicePackageNotificationIntervalInSecondsParamName,
                                                               DefaultServicePackageNotificationIntervalInSeconds);
            this.maxServicePackageInactiveTimeSeconds = servicePackageNotificationIntervalSeconds * MaxServicePackageInactiveTimeMultiplier;
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Interval at which we check for inactive service packages: {0} seconds.",
                servicePackageNotificationIntervalSeconds);
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Maximum time for which a service package can remain inactive before being deleted: {0} seconds.",
                this.maxServicePackageInactiveTimeSeconds);

            // Create the folder where the backup files are stored
            string tableBackupDirectory = Path.Combine(
                                              Utility.LogDirectory,
                                              AppInstanceDataDirName,
                                              AppInstanceTableDirName);

            FabricDirectory.CreateDirectory(tableBackupDirectory);

            // Initialize the application activation table
            AppActivationTable.Initialize(tableBackupDirectory);

            // Retrieve the file where we last saved the service package table
            this.tableBackup = new ServicePackageTableBackup(
                                       tableBackupDirectory,
                                       this.AddOrUpdateServicePackage);

            // Initialize the service package table from the file that we last saved.
            if (false == this.tableBackup.Read())
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Unable to initialize service package table from backup file on disk.");
            }

            // Initialize the timestamp up to which ETW events can be read
            Utility.ApplicationEtwTracesEndTime = this.tableBackup.LatestBackupTime.Timestamp;

            // Compute the directory containing ETL files containing information about
            // application instances
            string etlFileDirectory = Path.Combine(
                                          Utility.LogDirectory,
                                          AppInstanceDataDirName,
                                          AppInstanceDataEtlDirName);
            FabricDirectory.CreateDirectory(etlFileDirectory);

#if !DotNetCoreClrLinux
            // Create the object that reads events from ETL files.
            this.etlFileReader = new AppInstanceEtlFileDataReader(
                                         etlFileDirectory,
                                         this.tableBackup,
                                         this.AddOrUpdateServicePackage,
                                         this.RemoveServicePackage);
#endif

            long inactiveServicePackageScanIntervalMillisec = ((long)servicePackageNotificationIntervalSeconds) * 1000;
            this.inactiveServicePackageScanTimer = new DcaTimer(
                                                        InactiveServicePackageScanTimerId,
                                                        this.MarkInactiveServicePackagesForDeletion,
                                                        inactiveServicePackageScanIntervalMillisec);
            this.inactiveServicePackageScanTimer.Start();
        }

        // Methods invoked when changes need to be made to the service 
        // package table
        internal delegate void AddOrUpdateHandler(
                                   string nodeName,
                                   string applicationInstanceId,
                                   DateTime activationTime,
                                   string servicePackageName,
                                   ServicePackageTableRecord serviceRecord,
                                   EtwEventTimestamp dataTimestamp,
                                   bool updateBackupFile);

        internal delegate void RemoveHandler(
                                   string nodeName,
                                   string applicationInstanceId,
                                   string servicePackageName,
                                   EtwEventTimestamp dataTimestamp,
                                   bool updateBackupFile,
                                   bool removeDueToInactivity);

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            
            this.disposed = true;

#if !DotNetCoreClrLinux
            // Stop the reader that reads from ETL files
            if (null != this.etlFileReader)
            {
                this.etlFileReader.Dispose();
            }
#endif

            // Stop the inactive service scan timer and wait for the outstanding
            // callback (if any) to finish running
            if (null != this.inactiveServicePackageScanTimer)
            {
                this.inactiveServicePackageScanTimer.StopAndDispose();
                this.inactiveServicePackageScanTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
        }

        private void AddOrUpdateServicePackage(
                         string nodeName,
                         string applicationInstanceId,
                         DateTime servicePackageActivationTime,
                         string servicePackageName,
                         ServicePackageTableRecord serviceRecord,
                         EtwEventTimestamp dataTimestamp,
                         bool updateBackupFile)
        {
            try
            {
                // Create the object that wraps the application configuration
                AppConfig appConfig;
                try
                {
                    appConfig = new AppConfig(
                        nodeName,
                        serviceRecord.RunLayoutRoot,
                        applicationInstanceId,
                        serviceRecord.ApplicationRolloutVersion);
                }
                catch (InvalidOperationException)
                {
                    return;
                }

                // Create the object that wraps the service configuration
                ServiceConfig serviceConfig;
                try
                {
                    serviceConfig = new ServiceConfig(
                        serviceRecord.RunLayoutRoot,
                        applicationInstanceId,
                        servicePackageName,
                        serviceRecord.ServiceRolloutVersion);
                }
                catch (InvalidOperationException)
                {
                    return;
                }

                // Check if the service package already exists in the service package table
                string uniqueAppId = string.Concat(nodeName, applicationInstanceId);
                string uniqueServiceId = string.Concat(uniqueAppId, servicePackageName);
                List<ServicePackageTableRecord> records = this.servicePackageTable.GetRecordsForAppInstance(
                                                                nodeName,
                                                                applicationInstanceId);
                ServicePackageTableRecord record = (null == records) ?
                                                        null :
                                                        records.FirstOrDefault(s => s.ServicePackageName.Equals(
                                                                                        servicePackageName,
                                                                                        StringComparison.Ordinal));
                if (null != record)
                {
                    // Service package already exists. Check if needs to be updated. 
                    if ((false == record.ApplicationRolloutVersion.Equals(serviceRecord.ApplicationRolloutVersion)) ||
                        (false == record.ServiceRolloutVersion.Equals(serviceRecord.ServiceRolloutVersion)))
                    {
                        this.applicationInstanceManager.AddServiceToApplicationInstance(
                            uniqueAppId,
                            appConfig,
                            servicePackageName,
                            serviceConfig);
                    }
                    else
                    {
                        Utility.TraceSource.WriteInfo(
                            TraceType,
                            "Service package activation did not require the data collector to be restarted. Node name: {0}, application instance ID: {1}, app rollout version: {2}, service package name: {3}, service rollout version: {4}.",
                            nodeName,
                            applicationInstanceId,
                            serviceRecord.ApplicationRolloutVersion,
                            servicePackageName,
                            serviceRecord.ServiceRolloutVersion);
                    }
                }
                else
                {
                    // Service package does not exist yet. Check whether any other service packages
                    // exist for the same app instance
                    if (records.Count > 0)
                    {
                        // Other service packages exist for this application instance.
                        this.applicationInstanceManager.AddServiceToApplicationInstance(
                            uniqueAppId,
                            appConfig,
                            servicePackageName,
                            serviceConfig);
                    }
                    else
                    {
                        // No service packages exist for this application instance.
                        // Create a new data collector for this application instance.
                        this.applicationInstanceManager.CreateApplicationInstance(
                            uniqueAppId,
                            servicePackageActivationTime,
                            appConfig,
                            servicePackageName,
                            serviceConfig);
                    }
                }

                // Store information about this service package in our table
                this.servicePackageTable.AddOrUpdateRecord(uniqueServiceId, serviceRecord);
            }
            finally
            {
                // If necessary, update the backup file on disk to reflect this change
                if (updateBackupFile)
                {
                    if (false == this.tableBackup.Update(
                                     this.servicePackageTable.GetAllRecords(),
                                     dataTimestamp))
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Unable to make backup of service package table to a file on disk.");
                    }
                }
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Entry added to/updated in service package table: Node name: {0}, application instance ID: {1}, app rollout version: {2}, service package name: {3}, service rollout version: {4}.",
                nodeName,
                applicationInstanceId,
                serviceRecord.ApplicationRolloutVersion,
                servicePackageName,
                serviceRecord.ServiceRolloutVersion);
        }

        private void RemoveServicePackage(
                        string nodeName, 
                        string applicationInstanceId, 
                        string servicePackageName, 
                        EtwEventTimestamp dataTimestamp, 
                        bool updateBackupFile,
                        bool removeDueToInactivity)
        {
            try
            {
                // Check if the service package already exists in the service package table
                string uniqueAppId = string.Concat(nodeName, applicationInstanceId);
                string uniqueServiceId = string.Concat(uniqueAppId, servicePackageName);
                List<ServicePackageTableRecord> records = this.servicePackageTable.GetRecordsForAppInstance(
                                                                nodeName,
                                                                applicationInstanceId);
                ServicePackageTableRecord record = (null == records) ?
                                                        null :
                                                        records.FirstOrDefault(s => s.ServicePackageName.Equals(
                                                                                        servicePackageName,
                                                                                        StringComparison.Ordinal));
                if (null != record)
                {
                    // Service package exists in the service package table
                    bool skipRemove = false;

                    if (removeDueToInactivity)
                    {
                        // We are removing this service package not because it was explicitly
                        // deactivated by hosting, but because we recently did not receive any
                        // notification from hosting that this service package is still active.
                        // However, if some activity occurred on the service package since our
                        // last check, then don't remove this service package.
                        DateTime cutoffTime = DateTime.UtcNow.AddSeconds(-1 * this.maxServicePackageInactiveTimeSeconds);
                        if (record.LatestNotificationTimestamp.CompareTo(cutoffTime) >= 0)
                        {
                            Utility.TraceSource.WriteInfo(
                                TraceType,
                                "Skipping removal of entry from service package table because recent activity was detected. Node name: {0}, application instance ID: {1}, service package name: {2}.",
                                nodeName,
                                applicationInstanceId,
                                servicePackageName);
                            skipRemove = true;
                        }
                    }

                    if (false == skipRemove)
                    {
                        if (records.Count > 1)
                        {
                            // There are other service packages for this application instance
                            this.applicationInstanceManager.RemoveServiceFromApplicationInstance(uniqueAppId, servicePackageName);
                        }
                        else
                        {
                            // This is the last service package for its application instance
                            // Delete the data collector for this application instance
                            this.applicationInstanceManager.DeleteApplicationInstance(
                                uniqueAppId);
                        }

                        // Delete the service package information from our table
                        this.servicePackageTable.RemoveRecord(uniqueServiceId);

                        Utility.TraceSource.WriteInfo(
                            TraceType,
                            "Entry removed from service package table: Node name: {0}, application instance ID: {1}, service package name: {2}.",
                            nodeName,
                            applicationInstanceId,
                            servicePackageName);
                    }
                }
                else
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Deactivation of unknown service package ignored. Node name: {0}, application instance ID: {1}, service package name: {2}.",
                        nodeName,
                        applicationInstanceId,
                        servicePackageName);
                }
            }
            finally
            {
                // If necessary, update the backup file on disk to reflect this change
                if (updateBackupFile)
                {
                    if (false == this.tableBackup.Update(
                                     this.servicePackageTable.GetAllRecords(),
                                     dataTimestamp))
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Unable to make backup of service package table to a file on disk.");
                    }
                }
            }
        }

        private void RestartInactiveServicePackageScanTimer()
        {
            int servicePackageNotificationIntervalSeconds = Utility.GetUnencryptedConfigValue(
                                                               HostingSectionName,
                                                               ServicePackageNotificationIntervalInSecondsParamName,
                                                               DefaultServicePackageNotificationIntervalInSeconds);
            int currentMaxServicePackageInactiveTimeSeconds = servicePackageNotificationIntervalSeconds * MaxServicePackageInactiveTimeMultiplier;
            if (this.maxServicePackageInactiveTimeSeconds != currentMaxServicePackageInactiveTimeSeconds)
            {
                this.maxServicePackageInactiveTimeSeconds = currentMaxServicePackageInactiveTimeSeconds;

                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Interval at which we check for inactive service packages has changed. New interval: {0} seconds.",
                    servicePackageNotificationIntervalSeconds);
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Maximum time for which a service package can remain inactive before being deleted: {0} seconds.",
                    this.maxServicePackageInactiveTimeSeconds);

                long inactiveServicePackageScanIntervalMillisec = ((long)servicePackageNotificationIntervalSeconds) * 1000;
                this.inactiveServicePackageScanTimer.Start(inactiveServicePackageScanIntervalMillisec);
            }
            else
            {
                this.inactiveServicePackageScanTimer.Start();
            }
        }

        private void MarkInactiveServicePackagesForDeletion(object context)
        {
            // Compute the oldest timestamp that we consider active.
            DateTime cutoffTime = DateTime.UtcNow.AddSeconds(-1 * this.maxServicePackageInactiveTimeSeconds);

            // Figure out which service packages have a last activity timestamp
            // older than the above timestamp and mark those service packages for
            // deletion.
            this.servicePackageTable.MarkInactiveRecordsForDeletion(cutoffTime);

            // Schedule the next pass, unless the DCA is being stopped
            this.RestartInactiveServicePackageScanTimer();
        }
    }
}