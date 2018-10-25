// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using Tools.EtlReader;

    // This class reads events from the ETL files that contain information about
    // the application instances on the node
    internal class AppInstanceEtlFileDataReader : AppInstanceEtwDataReader, IDisposable
    {
        // Constants
        internal const string EtlFileReadIntervalInMinutesParamName = "AppInstanceDataEtlReadIntervalInMinutes";
        internal const string TestOnlyEtlFileReadIntervalInSecondsParamName = "TestOnlyAppInstanceDataEtlReadIntervalInSeconds";
        internal const string EtlFileFlushIntervalInSecondsParamName = "AppInstanceDataEtlFlushIntervalInSeconds";
        internal const string TestOnlyAppDataDeletionIntervalSecondsParamName = "TestOnlyAppDataDeletionIntervalInSeconds";
        private const string TraceType = "AppInstanceEtlFileDataReader";
        private const string EtlFilePattern = "*.etl";
        private const string OldDataDeletionTimerId = "OldAppInstanceDataDeletionTimer";
        private const string AppInstanceEtlReadTimerId = "AppInstanceEtlReadTimer";
        private const string AppEtwTraceDeletionAgeParamName = "AppEtwTraceDeletionAgeInDays";
        private const string TestAppEtwTraceDeletionAgeParamName = "TestOnlyAppEtwTraceDeletionAgeInMinutes";

        private static readonly TimeSpan DefaultOldDataDeletionInterval = TimeSpan.FromHours(3); // 3 hours
        private static readonly TimeSpan DefaultEtlFileReadIntervalInMinutes = TimeSpan.FromMinutes(3);
        private static readonly TimeSpan DefaultEtlFileFlushInterval = TimeSpan.FromMinutes(1);
        private static readonly TimeSpan DefaultAppEtwTraceDeletionAgeDays = TimeSpan.FromDays(3);
        private static readonly TimeSpan DefaultHostingAppActivityEtwTraceDeletionAgeDays = TimeSpan.FromHours(1);
        private static readonly TimeSpan MaxDataDeletionAge = TimeSpan.FromDays(1000);

        // Directory containing the ETL files
        private readonly string etlDirectory;

        // Reference to object that backs up the service package table on disk
        private readonly ServicePackageTableBackup tableBackup;

        // Timer used to read ETL files periodically
        private readonly DcaTimer etlReadTimer;

        // Timer used to schedule old data deletion
        private readonly DcaTimer oldDataDeletionTimer;

        // Interval at which ETL files are read
        private TimeSpan currentEtlReadIntervalMillisec;

        // Interval at which the ETL file session is flushed
        private TimeSpan currentEtlFileFlushInterval;

        // Time after which the ETL files containing ETW traces written by applications
        // should be deleted.
        // NOTE: Do not confuse these ETL files with the ones containing traces written
        // by the hosting subsystem to convey service package activity.
        private TimeSpan currentAppEtwTracesDeletionAgeMinutes;

        // Timestamp of the ETW event that we received most recently
        private EtwEventTimestamp mostRecentEtwEventTimestampBeforeCurrentEtl;

        // Whether or not the object has been disposed
        private bool disposed;

        internal AppInstanceEtlFileDataReader(
                     string directory,
                     ServicePackageTableBackup tableBackup,
                     ServicePackageTableManager.AddOrUpdateHandler addOrUpdateServicePackageHandler,
                     ServicePackageTableManager.RemoveHandler removeServicePackageHandler)
                 : base(addOrUpdateServicePackageHandler, removeServicePackageHandler)
        {
            this.etlDirectory = directory;
            this.tableBackup = tableBackup;
            this.currentEtlReadIntervalMillisec = TimeSpan.MaxValue;
            this.currentEtlFileFlushInterval = TimeSpan.MaxValue;
            this.currentAppEtwTracesDeletionAgeMinutes = TimeSpan.MaxValue;

            // Start reading events from the ETL file.
            this.etlReadTimer = new DcaTimer(
                                        AppInstanceEtlReadTimerId,
                                        this.Read,
                                        this.GetEtlReadInterval());
            this.etlReadTimer.Start();

            // Create a timer to periodically delete old ETL files
            var oldDataDeletionInterval = TimeSpan.FromSeconds(Utility.GetUnencryptedConfigValue(
                                                      ConfigReader.DiagnosticsSectionName,
                                                      TestOnlyAppDataDeletionIntervalSecondsParamName,
                                                      (int)DefaultOldDataDeletionInterval.TotalSeconds));
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Interval at which we check for old data to delete: {0} seconds.",
                oldDataDeletionInterval);
            this.oldDataDeletionTimer = new DcaTimer(
                                                OldDataDeletionTimerId,
                                                this.DeleteOldEtlFiles,
                                                oldDataDeletionInterval);
            this.oldDataDeletionTimer.Start();
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Set the flag that indicates that the DCA is being stopped
            this.Stopping = true;

            // Stop the timer that triggers periodic read of ETW events
            if (null != this.etlReadTimer)
            {
                this.etlReadTimer.StopAndDispose();
                this.etlReadTimer.DisposedEvent.WaitOne();
            }

            // Stop the old data deletion timer and wait for the outstanding
            // callback (if any) to finish running
            if (null != this.oldDataDeletionTimer)
            {
                this.oldDataDeletionTimer.StopAndDispose();
                this.oldDataDeletionTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
        }

        private TimeSpan GetEtlReadInterval()
        {
            // Get the interval at which we need to read ETL files
            // NOTE: We DO NOT cache this value. Instead, we read it from the 
            // config store each time. This is because the config can change 
            // and we always want to the current value.
            var etlFileReadInterval = TimeSpan.FromMinutes(Utility.GetUnencryptedConfigValue(
                                                   ConfigReader.DiagnosticsSectionName,
                                                   EtlFileReadIntervalInMinutesParamName,
                                                   (int)DefaultEtlFileReadIntervalInMinutes.TotalMinutes));
            var testOnlyEtlFileReadInterval = TimeSpan.FromSeconds(Utility.GetUnencryptedConfigValue(
                                                         ConfigReader.DiagnosticsSectionName,
                                                         TestOnlyEtlFileReadIntervalInSecondsParamName,
                                                         0));

            // If the test setting is present, it takes higher priority.
            TimeSpan etlReadInterval;
            if (TimeSpan.Zero != testOnlyEtlFileReadInterval)
            {
                etlReadInterval = testOnlyEtlFileReadInterval;
            }
            else
            {
                etlReadInterval = etlFileReadInterval;
            }

            // If the ETL read interval has changed since the last time we read
            // the config, then write a log message to indicate the change.
            if (this.currentEtlReadIntervalMillisec != etlReadInterval)
            {
                this.currentEtlReadIntervalMillisec = etlReadInterval;
                if (TimeSpan.Zero != testOnlyEtlFileReadInterval)
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Interval at which we read ETL files containing application instance data: {0} seconds.",
                        testOnlyEtlFileReadInterval);
                }
                else
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Interval at which we read ETL files containing application instance data: {0} minutes.",
                        etlFileReadInterval);
                }
            }

            return etlReadInterval;
        }

        private TimeSpan GetEtlFileFlushInterval()
        {
            // Get the interval at which the ETW session is flushed
            // NOTE: We DO NOT cache this value. Instead, we read it from the 
            // config store each time. This is because the config can change 
            // and we always want to the current value.
            var etlFileFlushInterval = TimeSpan.FromSeconds(Utility.GetUnencryptedConfigValue(
                                                  ConfigReader.DiagnosticsSectionName,
                                                  EtlFileFlushIntervalInSecondsParamName,
                                                  (int)DefaultEtlFileFlushInterval.TotalSeconds));
            if (this.currentEtlFileFlushInterval != etlFileFlushInterval)
            {
                this.currentEtlFileFlushInterval = etlFileFlushInterval;
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Interval at which we flush the ETW session that produces ETL files: {0} seconds.",
                    etlFileFlushInterval);
            }

            return etlFileFlushInterval;
        }

        private TimeSpan GetApplicationEtwTracesDeletionAge()
        {
            // Get the deletion age for ETL files containing ETW traces written by applications
            // NOTE: We DO NOT cache this value. Instead, we read it from the 
            // config store each time. This is because the config can change 
            // and we always want to the current value.
            var etlDeletionAge = TimeSpan.FromDays(Utility.GetUnencryptedConfigValue(
                                         ConfigReader.DiagnosticsSectionName,
                                         AppEtwTraceDeletionAgeParamName,
                                         (int)DefaultAppEtwTraceDeletionAgeDays.TotalDays));
            if (etlDeletionAge > MaxDataDeletionAge)
            {
                Utility.TraceSource.WriteWarning(
                    TraceType,
                    "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                    etlDeletionAge,
                    ConfigReader.DiagnosticsSectionName,
                    AppEtwTraceDeletionAgeParamName,
                    MaxDataDeletionAge);
                etlDeletionAge = MaxDataDeletionAge;
            }

            var appEtwTracesDeletionAge = etlDeletionAge;

            // Check for test settings
            var logDeletionAgeTestValue = TimeSpan.FromMinutes(Utility.GetUnencryptedConfigValue(
                                              ConfigReader.DiagnosticsSectionName,
                                              TestAppEtwTraceDeletionAgeParamName,
                                              0));
            if (logDeletionAgeTestValue != TimeSpan.Zero)
            {
                if (logDeletionAgeTestValue > MaxDataDeletionAge)
                {
                    Utility.TraceSource.WriteWarning(
                        TraceType,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        logDeletionAgeTestValue,
                        ConfigReader.DiagnosticsSectionName,
                        TestAppEtwTraceDeletionAgeParamName,
                        MaxDataDeletionAge);
                    logDeletionAgeTestValue = MaxDataDeletionAge;
                }

                appEtwTracesDeletionAge = logDeletionAgeTestValue;
            }

            if (this.currentAppEtwTracesDeletionAgeMinutes != appEtwTracesDeletionAge)
            {
                this.currentAppEtwTracesDeletionAgeMinutes = appEtwTracesDeletionAge;
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Deletion age for application ETW traces: {0} minutes.",
                        logDeletionAgeTestValue);
                }
                else
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Deletion age for application ETW traces: {0} days.",
                        etlDeletionAge);
                }
            }

            return appEtwTracesDeletionAge;
        }

        private int CompareFileCreationTimesOldFirst(FileInfo file1, FileInfo file2)
        {
            // We want the file with the oldest creation time to come first
            return DateTime.Compare(file1.CreationTime, file2.CreationTime);
        }

        private void Read(object state)
        {
            this.LatestProcessedEtwEventTimestamp = this.tableBackup.LatestBackupTime;
            this.ResetMostRecentEtwEventTimestamp();

            // Get all files in the ETL directory that match the pattern
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Searching for ETL files whose names match '{0}' in directory '{1}'.",
                EtlFilePattern,
                this.etlDirectory);

            DirectoryInfo dirInfo = new DirectoryInfo(this.etlDirectory);
            FileInfo[] etlFiles = dirInfo.GetFiles(EtlFilePattern);

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Found {0} ETL files whose names match '{1}' in directory '{2}'.",
                etlFiles.Length,
                EtlFilePattern,
                this.etlDirectory);

            // Sort the files such that the file with the oldest creation
            // time comes first. 
            Array.Sort(etlFiles, this.CompareFileCreationTimesOldFirst);

            // We read events that are old enough to have been flushed to disk
            var etlFileFlushInterval = this.GetEtlFileFlushInterval();
            DateTime endTime = DateTime.UtcNow.AddSeconds(-2 * etlFileFlushInterval.TotalSeconds);

            foreach (FileInfo etlFile in etlFiles)
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Processing ETL file: {0}. Events with timestamp between {1} ({2}) and {3} ({4}) will be read from it.",
                    etlFile.Name,
                    this.LatestProcessedEtwEventTimestamp.Timestamp,
                    this.LatestProcessedEtwEventTimestamp.Timestamp.Ticks,
                    endTime,
                    endTime.Ticks);

                // Note the timestamp of the event we received most recently before we start
                // processing this ETL file. In case we retry the processing of this ETL file,
                // we need to revert back to this timestamp.
                this.mostRecentEtwEventTimestampBeforeCurrentEtl = this.MostRecentEtwEventTimestamp;

                try
                {
                    Utility.PerformWithRetries(
                        ctx =>
                        {
                            // Create a ETL file reader
                            using (var eventReader = new TraceFileEventReader(etlFile.FullName))
                            {
                                var traceSessionMetadata = eventReader.ReadTraceSessionMetadata();
                                FabricEvents.Events.TraceSessionStats(
                                    traceSessionMetadata.TraceSessionName,
                                    traceSessionMetadata.EventsLostCount,
                                    traceSessionMetadata.StartTime,
                                    traceSessionMetadata.EndTime);

                                // Register our ETW event processing callback
                                eventReader.EventRead += this.OnEtwEventReceived;

                                // Read ETW events from the ETL file
                                eventReader.ReadEvents(
                                    this.LatestProcessedEtwEventTimestamp.Timestamp,
                                    endTime);
                            }
                        },
                        (object)null,
                        new RetriableOperationExceptionHandler(this.ReadEventsExceptionHandler));
                    
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Finished processing ETL file: {0}.",
                        etlFile.Name);
                }
                catch (Exception e)
                {
                    Utility.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        "Failed to read some or all of the events from ETL file {0}.",
                        etlFile.FullName);
                }

                if (this.Stopping)
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "The DCA is stopping, so no more ETL files will be processed.");
                    break;
                }
            }

            // Update the timestamp up to which ETW events can be read
            Utility.ApplicationEtwTracesEndTime = this.LatestProcessedEtwEventTimestamp.Timestamp;

            // Schedule the next pass, unless the DCA is being stopped
            this.etlReadTimer.Start(this.GetEtlReadInterval());
        }

        private RetriableOperationExceptionHandler.Response ReadEventsExceptionHandler(Exception e)
        {
            Utility.TraceSource.WriteWarning(
                TraceType,
                "Exception encountered while reading ETL file. We will retry if retry limit has not been reached. Exception information: {0}.",
                e);

            // Update the timestamp of the event we last saw to the timestamp of the event we
            // last saw before we started processing the current ETL file.
            this.MostRecentEtwEventTimestamp = this.mostRecentEtwEventTimestampBeforeCurrentEtl;

            return RetriableOperationExceptionHandler.Response.Retry;
        }

        private void DeleteOldEtlFiles(object state)
        {
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Starting old data deletion pass ...");

            // Delete ETL files containing data older than our most recent backup time
            // Exclude the file with the most recent creation time, because that is an
            // active ETL file. We do not want to try to delete active ETL files.
            DateTime latestBackupTime = this.tableBackup.LatestBackupTime.Timestamp;

            // Setting cutofftime to latest timestamp between latestBackupTime and (now - DefaultHostingAppActivityEtwTraceDeletionAgeDays)
            // To prevent the appInfo folder of never get deleted in case latestBackupTime never gets updated for some reason (RDBug 10997761).
            DateTime cutoffTime = DateTime.UtcNow.AddMinutes(-AppInstanceEtlFileDataReader.DefaultHostingAppActivityEtwTraceDeletionAgeDays.TotalMinutes);

            if (cutoffTime < latestBackupTime)
            {
                cutoffTime = latestBackupTime;
            }
            else
            {
                Utility.TraceSource.WriteWarning(
                    TraceType,
                    "Using trace deletion age policy of {0} to try deleting old etl files since last table backup time at {1} is older.",
                    cutoffTime,
                    latestBackupTime);
            }

            Utility.DeleteOldFilesFromFolder(
                TraceType,
                this.etlDirectory,
                EtlFilePattern,
                cutoffTime,
                () => { return this.Stopping; },
                true);

            // Delete ETL files containing application ETW traces
            cutoffTime = DateTime.UtcNow.AddMinutes(-this.GetApplicationEtwTracesDeletionAge().TotalMinutes);
            List<string> appEtwTraceFolders = AppEtwTraceFolders.GetAll();
            foreach (string appEtwTraceFolder in appEtwTraceFolders)
            {
                Utility.DeleteOldFilesFromFolder(
                    TraceType,
                    appEtwTraceFolder,
                    EtlFilePattern,
                    cutoffTime,
                    () => { return this.Stopping; },
                    true);
            }

            // Delete marker files used in application ETW trace collection
            List<string> markerFilesDirsForApps = EtlProducer.MarkerFileDirectoriesForApps;
            foreach (string markerFileDir in markerFilesDirsForApps)
            {
                Utility.DeleteOldFilesFromFolder(
                    TraceType,
                    markerFileDir,
                    EtlFilePattern,
                    cutoffTime,
                    () => { return this.Stopping; },
                    false);
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Finished old data deletion pass.");

            // Schedule the next pass, unless the DCA is being stopped
            this.oldDataDeletionTimer.Start();
        }
    }
}