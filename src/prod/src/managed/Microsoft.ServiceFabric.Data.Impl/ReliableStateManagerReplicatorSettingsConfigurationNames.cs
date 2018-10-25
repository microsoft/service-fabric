// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;

    /// <summary>
    /// NOTE!!! - WARNING- Do not change formatting of this class by moving names of the variables to new lines as it fails parsing in the config validator perl script
    /// </summary>
    internal static class ReliableStateManagerReplicatorSettingsConfigurationNames
    {
        public static readonly string SectionName = "TransactionalReplicator";

        public static readonly Tuple<string, TimeSpan> RetryInterval = new Tuple<string, TimeSpan>("RetryInterval", TimeSpan.FromSeconds(5));

        public static readonly Tuple<string, TimeSpan> BatchAcknowledgementInterval = new Tuple<string, TimeSpan>("BatchAcknowledgementInterval", TimeSpan.FromMilliseconds(15));

        public static readonly Tuple<string, string> ReplicatorAddress = new Tuple<string, string>("ReplicatorAddress", "localhost:0");

        // Intentionally set the default listen and publish address to empty because if the user does not provide anything and does not use the loadfromAPI (e.g FASService)
        // we will end up setting it explicitly to localhost:0 which will override the replicatoraddress provided in the form of IP:Port
        public static readonly Tuple<string, string> ReplicatorListenAddress = new Tuple<string, string>("ReplicatorListenAddress", "");
        public static readonly Tuple<string, string> ReplicatorPublishAddress = new Tuple<string, string>("ReplicatorPublishAddress", "");

        public static readonly Tuple<string, uint> InitialCopyQueueSize = new Tuple<string, uint>("InitialCopyQueueSize", 64);

        public static readonly Tuple<string, uint> MaxCopyQueueSize = new Tuple<string, uint>("MaxCopyQueueSize", 16384);

        public static readonly Tuple<string, uint> MaxReplicationMessageSize = new Tuple<string, uint>("MaxReplicationMessageSize", 52428800);

        public static readonly Tuple<string, uint> InitialPrimaryReplicationQueueSize = new Tuple<string, uint>("InitialPrimaryReplicationQueueSize", 64);

        public static readonly Tuple<string, uint> MaxPrimaryReplicationQueueSize = new Tuple<string, uint>("MaxPrimaryReplicationQueueSize", 8192);

        public static readonly Tuple<string, uint> MaxPrimaryReplicationQueueMemorySize = new Tuple<string, uint>("MaxPrimaryReplicationQueueMemorySize", 0);

        public static readonly Tuple<string, uint> InitialSecondaryReplicationQueueSize = new Tuple<string, uint>("InitialSecondaryReplicationQueueSize", 64);

        public static readonly Tuple<string, uint> MaxSecondaryReplicationQueueSize = new Tuple<string, uint>("MaxSecondaryReplicationQueueSize", 16384);

        public static readonly Tuple<string, uint> MaxSecondaryReplicationQueueMemorySize = new Tuple<string, uint>("MaxSecondaryReplicationQueueMemorySize", 0);

        public static readonly Tuple<string, bool> SecondaryClearAcknowledgedOperations = new Tuple<string, bool>("SecondaryClearAcknowledgedOperations", false);

        public static readonly Tuple<string, int> MaxMetadataSizeInKB = new Tuple<string, int>("MaxMetadataSizeInKB", 4);

        public static readonly Tuple<string, int> MaxRecordSizeInKB = new Tuple<string, int>("MaxRecordSizeInKB", 1024);

        public static readonly Tuple<string, int> MaxWriteQueueDepthInKB = new Tuple<string, int>("MaxWriteQueueDepthInKB", 0);

        public static readonly Tuple<string, string> SharedLogId = new Tuple<string, string>("SharedLogId", "");

        public static readonly Tuple<string, string> SharedLogPath = new Tuple<string, string>("SharedLogPath", "");

        public static readonly Tuple<string, int> MaxStreamSizeInMB = new Tuple<string, int>("MaxStreamSizeInMB", 1024);

        public static readonly Tuple<string, bool> OptimizeLogForLowerDiskUsage = new Tuple<string, bool>("OptimizeLogForLowerDiskUsage", true);

        public static readonly Tuple<string, bool> OptimizeForLocalSSD = new Tuple<string, bool>("OptimizeForLocalSSD", false);

        public static readonly Tuple<string, int> MaxAccumulatedBackupLogSizeInMB = new Tuple<string, int>("MaxAccumulatedBackupLogSizeInMB", 800);

        public static readonly Tuple<string, TimeSpan> SlowApiMonitoringDuration = new Tuple<string, TimeSpan>("SlowApiMonitoringDuration", TimeSpan.FromSeconds(300));

        public static readonly Tuple<string, int> CheckpointThresholdInMB = new Tuple<string, int>("CheckpointThresholdInMB", 50);

        public static readonly Tuple<string, uint> ProgressVectorMaxEntries = new Tuple<string, uint>("ProgressVectorMaxEntries", 0);

        public static readonly Tuple<string, int> MinLogSizeInMB = new Tuple<string, int>("MinLogSizeInMB", 0);

        public static readonly Tuple<string, int> TruncationThresholdFactor = new Tuple<string, int>("TruncationThresholdFactor", 2);

        public static readonly Tuple<string, int> ThrottlingThresholdFactor = new Tuple<string, int>("ThrottlingThresholdFactor", 4);

        public static readonly Tuple<string, string> Test_LoggingEngine = new Tuple<string, string>("Test_LoggingEngine", "ktl");

        public static readonly Tuple<string, int> Test_LogMinDelayIntervalMilliseconds = new Tuple<string, int>("Test_LogMinDelayIntervalMilliseconds", 0);

        public static readonly Tuple<string, int> Test_LogMaxDelayIntervalMilliseconds = new Tuple<string, int>("Test_LogMaxDelayIntervalMilliseconds", 0);

        public static readonly Tuple<string, double> Test_LogDelayRatio = new Tuple<string, double>("Test_LogDelayRatio", 0);

        public static readonly Tuple<string, double> Test_LogDelayProcessExitRatio = new Tuple<string, double>("Test_LogDelayProcessExitRatio", 0);

        /// <summary>
        /// Configuration that enables incremental backups to be chained across primary replicas.
        /// When this flag is turned off, a primary replica can only take an incremental backup if it took the last backup at the same epoch.
        /// When this flag is turned on, a primary replica can take an incremental backup whether or not it was the replica that took the last backup.
        /// </summary>
        public static readonly Tuple<string, bool> EnableIncrementalBackupsAcrossReplicas = new Tuple<string, bool>("EnableIncrementalBackupsAcrossReplicas", false);

        public static readonly Tuple<string, int> LogTruncationIntervalSeconds = new Tuple<string, int>("LogTruncationIntervalSeconds", 0);
    }
}