// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric;
    using System.Text;

    /// <summary>
    /// Settings that configure the replicator
    /// </summary>
    public class ReliableStateManagerReplicatorSettings
    {
        /// <summary>
        /// Gets or sets how long the replicator waits after it transmits a message from the primary to the secondary for the secondary to acknowledge that it has received the message.
        /// The default value is 5 seconds.
        /// </summary>
        /// <returns>The retry interval.</returns>
        public TimeSpan? RetryInterval { get; set; }
        
        /// <summary>
        /// Gets or sets the amount of time that the replicator waits after receiving an operation before sending back an acknowledgment.
        /// The default value is 5 milliseconds.
        /// </summary>
        /// <returns>The batch acknowledgment interval.</returns>
        public TimeSpan? BatchAcknowledgementInterval { get; set; }

        /// <summary>
        /// Gets or sets the address in {ip}:{port} format that this replicator will use when communicating with other replicators.
        /// The default value is "localhost:0", which picks a dynamic port number in runtime.
        /// If replicator is running inside a container, you should try setting up <see cref="ReliableStateManagerReplicatorSettings.ReplicatorListenAddress" /> and <see cref="ReliableStateManagerReplicatorSettings.ReplicatorPublishAddress" />.
        /// </summary>
        /// <returns>The replicator address.</returns>
        public string ReplicatorAddress { get; set; }

        /// <summary>
        /// Gets or sets the address in {ip}:{port} format that this replicator will use to receive information from other replicators.
        /// The default value is "localhost:0", which picks a dynamic port number in runtime.
        /// {ip} part of the listen address can be obtained from <see cref="System.Fabric.CodePackageActivationContext.ServiceListenAddress" />.
        /// </summary>
        /// <returns>The replicator address.</returns>
        public string ReplicatorListenAddress { get; set; }

        /// <summary>
        /// Gets or sets the address in {ip}:{port} format that this replicator will use to send information to other replicators.
        /// The default value is "localhost:0", which picks a dynamic port number in runtime.
        /// {ip} part of the publish address can be obtained from <see cref="System.Fabric.CodePackageActivationContext.ServicePublishAddress" />.
        /// </summary>
        /// <returns>The replicator address.</returns>
        public string ReplicatorPublishAddress { get; set; }

        /// <summary>
        /// Gets or sets the security credentials for securing the traffic between replicators.
        /// </summary>
        /// <returns>The security credentials.</returns>
        public SecurityCredentials SecurityCredentials { get; set; }

        /// <summary>
        /// Gets or sets the initial size of the copy operation queue inside the replicator, which contains copy operations.
        /// Default value is 64.
        /// The value is the number of operations in the copy operation queue. Must be a power of 2.
        /// </summary>
        /// <returns>The initial copy queue size.</returns>
        public long? InitialCopyQueueSize { get; set; }
        
        /// <summary>
        /// Gets or sets the maximum size of the copy operation queue inside replicator, which contains copy operations.
        /// Default value is 1024.
        /// The value is the max number of operations in the copy operation queue. Must be a power of 2.
        /// </summary>
        /// <returns>The max copy queue size.</returns>
        public long? MaxCopyQueueSize { get; set; }
        
        /// <summary>
        /// Gets or sets the max replication message size.
        /// Default value is 50MB.
        /// The unit is Bytes.
        /// </summary>
        /// <returns>The max replication message size.</returns>
        public long? MaxReplicationMessageSize { get; set; }

        /// <summary>
        /// Gets or sets the initial primary replication queue size.
        /// Default value is 64.
        /// The value is the number of operations in the primary replication queue. Must be a power of 2.
        /// </summary>
        /// <returns>The initial primary replication queue size.</returns>
        public long? InitialPrimaryReplicationQueueSize { get; set; }

        /// <summary>
        /// Gets or sets the max primary replication queue size.
        /// Default value is 1024.
        /// The value is the max number of operations in the primary replication queue. Must be a power of 2.
        /// </summary>
        /// <returns>The max primary replication queue size.</returns>
        public long? MaxPrimaryReplicationQueueSize { get; set; }

        /// <summary>
        /// Gets or sets the max primary replication queue memory size.
        /// Default value is 0, which implies there is no memory limitation.
        /// The unit is Bytes.
        /// </summary>
        /// <returns>The max primary replication queue memory size.</returns>
        public long? MaxPrimaryReplicationQueueMemorySize { get; set; }

        /// <summary>
        /// Gets or sets the initial secondary replication queue size.
        /// Default value is 64.
        /// The value is the number of operations in the secondary replication queue. Must be a power of 2.
        /// </summary>
        /// <returns>The initial secondary replication queue size.</returns>
        public long? InitialSecondaryReplicationQueueSize { get; set; }

        /// <summary>
        /// Gets or sets the max secondary replication queue size.
        /// Default value is 2048.
        /// The value is the max number of operations in the secondary replication queue. Must be a power of 2.
        /// </summary>
        /// <returns>The max secondary replication queue size.</returns>
        public long? MaxSecondaryReplicationQueueSize { get; set; }

        /// <summary>
        /// Gets or sets the max secondary replication queue memory size.
        /// Default value is 0, which implies there is no memory limitation.
        /// The unit is Bytes.
        /// </summary>
        /// <returns>The max secondary replication queue size.</returns>
        public long? MaxSecondaryReplicationQueueMemorySize { get; set; }

        /// <summary>
        /// Gets or sets the GUID identifier for the log container that is shared by a number of replicas on the windows fabric node including this one.
        /// Default value is "" which causes the replicator to use the global shared log for the node.
        /// </summary>
        /// <returns>The shared log id.</returns>
        public string SharedLogId { get; set; }
        
        /// <summary>
        /// Gets or sets the full pathname to the log container that is shared by a number of replicas on the node including this one.
        /// Default value is "" which causes the replicator to use the global shared log for the node.
        /// </summary>
        /// <returns>The shared log path.</returns>
        public string SharedLogPath { get; set; } 

        /// <summary>
        /// Deprecated
        /// </summary>
        /// <returns>The max stream size.</returns>
        public int? MaxStreamSizeInMB { get; set; }

        /// <summary>
        /// Gets or sets the amount of extra persistent storage space reserved for the replicator specified in kilobytes that is associated with this replica. This 
        /// value must be a multiple of 4.
        /// The default value is 4. 
        /// The unit is KB.
        /// </summary>
        /// <returns>The max metadata size.</returns>
        public int? MaxMetadataSizeInKB { get; set; }

        /// <summary>
        /// Gets or sets the largest record size which the replicator may write specified in kilobytes for the log that is associated with this replica. This 
        /// value must be a multiple of 4 and greater than or equal to 128.
        /// The default value is 1024. 
        /// The unit is KB.
        /// </summary>
        /// <returns>The max record size.</returns>
        public int? MaxRecordSizeInKB { get; set; }

        /// <summary>
        /// Gets or sets the maximum write queue depth that the core logger can use as specified in kilobytes for the log that is associated with this replica. This 
        /// value is the maximum number of bytes that can be outstanding during core logger updates. It may be 0 for the core logger
        /// to compute an appropriate value or a multiple of 4.
        /// The default value is 0.
        /// The unit is KB.
        /// </summary>
        /// <returns>The max write queue depth.</returns>
        public int? MaxWriteQueueDepthInKB { get; set; }

        /// <summary>
        /// Gets or sets the checkpoint threshold. A checkpoint will be initiated when the log usage exceeds this value.
        /// Default value is 50.
        /// The unit is MB.
        /// </summary>
        /// <returns>The checkpoint threshold.</returns>
        public int? CheckpointThresholdInMB { get; set; }

        /// <summary>
        /// Gets or sets the max size for an accumulated backup log across backups. 
        /// An incremental backup requests will fail if the backup logs generated by the request causes the total amount of logs accumulated including the last full backup to be greater than MaxAccumulatedBackupLogSizeInMB.
        /// In such cases, user is required to take a full backup.
        /// Default value is 800.
        /// The unit is MB.
        /// </summary>
        /// <returns>The max accumulated backup log size in MB.</returns>
        public int? MaxAccumulatedBackupLogSizeInMB { get; set; }

        /// <summary>
        /// Deprecated
        /// </summary>
        /// <returns>If the OptimizeForLocalSSD option is enabled.</returns>
        public bool? OptimizeForLocalSSD { get; set; }
        
        /// <summary>
        /// Gets or sets a flag, when true indicates the log should optimize in a way where less disk space is used for the log at the cost of IO performance. If false, the log will use more disk space but have better IO performance.
        /// Default value is true.
        /// </summary>
        /// <returns>If the OptimizeLogForLowerDiskUsage option is enabled.</returns>
        public bool? OptimizeLogForLowerDiskUsage { get; set; }

        /// <summary>
        /// Gets or sets a flag, when true indicates the secondary replicator should clear the in-memory queue after acknowledging the operations to the primary (After the operations are flushed to disk).
        /// Default value is false.
        /// Settings this to "TRUE" can result in additional disk reads on the new primary, while catching up replicas after a failover.
        /// </summary>
        /// <returns>If the SecondaryClearAcknowledgedOperations option is enabled.</returns>
        public bool? SecondaryClearAcknowledgedOperations { get; set; }
        
        /// <summary>
        /// Sets the interval after which the replicator sends a warning health report that the API is slow and is taking longer than expected duration.
        /// Default value is 5 minutes.
        /// </summary>
        public TimeSpan? SlowApiMonitoringDuration { get; set; }

        /// <summary>
        /// Gets or sets the minimum log size. A truncation will not be initiated if it would reduce the size of the log to below this value.
        /// Default value is 0.
        /// </summary>
        /// <returns>The minimum log size.</returns>
        public int? MinLogSizeInMB { get; set; }

        /// <summary>
        /// Gets or sets the truncation threshold factor. A truncation will be initiated when the log usage exceeds this value times MinLogSizeInMB.
        /// Default value is 2.
        /// </summary>
        /// <returns>The truncation threshold.</returns>
        public int? TruncationThresholdFactor { get; set; }

        /// <summary>
        /// Gets or sets the throttling threshold factor. Throttling will be initiated when the log usage exceeds this value times MinLogSizeInMB.
        /// Default value is 3.
        /// </summary>
        /// <returns>The throttling threshold.</returns>
        public int? ThrottlingThresholdFactor { get; set; }

#if !DotNetCoreClr
        // 12529905 - Disable new configuration for LogTruncationIntervalSeconds in CoreCLR
        /// <summary>
        /// Gets or sets a time interval at which log truncation will be initiated
        /// </summary>
        public int? LogTruncationIntervalSeconds { get; set; }

        /// <summary>
        /// Configuration that enables incremental backups to be chained across primary replicas.
        /// When this flag is turned off, a primary replica can only take an incremental backup if it took the last backup at the same epoch.
        /// When this flag is turned on, a primary replica can take an incremental backup whether or not it was the replica that took the last backup with the same dataloss number.
        /// </summary>
        internal bool? EnableIncrementalBackupsAcrossReplicas { get; set; }
#endif

        /// <summary>
        /// Determines whether the specified ReplicatorSettings is equal to the current object.
        /// </summary>
        /// <param name="obj">
        /// Object to check against.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public override bool Equals(object obj)
        {
            if (obj == null || obj.GetType() != GetType())
            {
                return false;
            }

            var arg = (ReliableStateManagerReplicatorSettings)obj;
            return InternalEquals(this, arg);
        }

        /// <summary>
        /// Serves as a hash function for this type.
        /// </summary>
        /// <returns>
        /// The <see cref="int"/> representing the hash code.
        /// </returns>
        public override int GetHashCode()
        {
            // ReSharper disable once BaseObjectGetHashCodeCallInGetHashCode
            return base.GetHashCode();
        }

        /// <summary>
        /// Returns a string that represents the current object.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public override string ToString()
        {
            var builder = new StringBuilder();
            builder.AppendFormat(Environment.NewLine);
            if (this.SharedLogId != null)
            {
                builder.AppendFormat("SharedLogId = {0}" + Environment.NewLine, this.SharedLogId);
            }

            if (this.SharedLogPath != null)
            {
                builder.AppendFormat("SharedLogPath = {0}" + Environment.NewLine, this.SharedLogPath);
            }

            if (this.MaxStreamSizeInMB.HasValue)
            {
                builder.AppendFormat("MaxStreamSizeInMB = {0}" + Environment.NewLine, this.MaxStreamSizeInMB);
            }

            if (this.MaxRecordSizeInKB.HasValue)
            {
                builder.AppendFormat("MaxRecordSizeInKB = {0}" + Environment.NewLine, this.MaxRecordSizeInKB);
            }

            if (this.MaxMetadataSizeInKB.HasValue)
            {
                builder.AppendFormat("MaxMetadataSizeInKB = {0}" + Environment.NewLine, this.MaxMetadataSizeInKB);
            }

            if (this.OptimizeForLocalSSD.HasValue)
            {
                builder.AppendFormat("OptimizeForLocalSSD = {0}" + Environment.NewLine, this.OptimizeForLocalSSD);
            }

            if (this.OptimizeLogForLowerDiskUsage.HasValue)
            {
                builder.AppendFormat("OptimizeLogForLowerDiskUsage = {0}" + Environment.NewLine, this.OptimizeLogForLowerDiskUsage);
            }

            if (this.CheckpointThresholdInMB.HasValue)
            {
                builder.AppendFormat("CheckpointThresholdInMB = {0}" + Environment.NewLine, this.CheckpointThresholdInMB);
            }

            if (this.MaxAccumulatedBackupLogSizeInMB.HasValue)
            {
                builder.AppendFormat("MaxAccumulatedBackupLogSizeInMB = {0}" + Environment.NewLine, this.MaxAccumulatedBackupLogSizeInMB);
            }

            if (this.MinLogSizeInMB.HasValue)
            {
                builder.AppendFormat("MinLogSizeInMB = {0}" + Environment.NewLine, this.MinLogSizeInMB);
            }

            if (this.TruncationThresholdFactor.HasValue)
            {
                builder.AppendFormat("TruncationThresholdFactor = {0}" + Environment.NewLine, this.TruncationThresholdFactor);
            }

            if (this.ThrottlingThresholdFactor.HasValue)
            {
                builder.AppendFormat("ThrottlingThresholdFactor = {0}" + Environment.NewLine, this.ThrottlingThresholdFactor);
            }

            if (this.SlowApiMonitoringDuration.HasValue)
            {
                builder.AppendFormat("SlowApiMonitoringDuration = {0}" + Environment.NewLine, this.SlowApiMonitoringDuration);
            }

#if !DotNetCoreClr
            // 12529905 - Disable new configuration for LogTruncationIntervalSeconds in CoreCLR
            if (this.LogTruncationIntervalSeconds.HasValue)
            {
                builder.AppendFormat("LogTruncationIntervalSeconds = {0}" + Environment.NewLine, this.LogTruncationIntervalSeconds);
            }

            if (this.EnableIncrementalBackupsAcrossReplicas.HasValue)
            {
                builder.AppendFormat("EnableIncrementalBackupsAcrossReplicas = {0}" + Environment.NewLine, this.EnableIncrementalBackupsAcrossReplicas);
            }
#endif
            return builder.ToString();
        }

        /// <summary>
        /// Checks for equality of setting values.
        /// </summary>
        /// <param name="old">Old settings.</param>
        /// <param name="updated">Updated settings.</param>
        /// <returns>
        /// TRUE if the settings are equivalent.
        /// </returns>
        private static bool InternalEquals(ReliableStateManagerReplicatorSettings old, ReliableStateManagerReplicatorSettings updated)
        {
            // compare only the V2 settings.
            var areEqual = true;

            if (!string.IsNullOrEmpty(updated.SharedLogId))
            {
                areEqual = !string.IsNullOrEmpty(old.SharedLogId) && (old.SharedLogId == updated.SharedLogId);
            }

            if (areEqual && !string.IsNullOrEmpty(updated.SharedLogPath))
            {
                areEqual = !string.IsNullOrEmpty(old.SharedLogPath) && (old.SharedLogPath == updated.SharedLogPath);
            }

            if (areEqual && updated.MaxStreamSizeInMB.HasValue)
            {
                areEqual = old.MaxStreamSizeInMB.HasValue && (old.MaxStreamSizeInMB.Value == updated.MaxStreamSizeInMB.Value);
            }

            if (areEqual && updated.MaxRecordSizeInKB.HasValue)
            {
                areEqual = old.MaxRecordSizeInKB.HasValue && (old.MaxRecordSizeInKB.Value == updated.MaxRecordSizeInKB.Value);
            }

            if (areEqual && updated.MaxMetadataSizeInKB.HasValue)
            {
                areEqual = old.MaxMetadataSizeInKB.HasValue && (old.MaxMetadataSizeInKB.Value == updated.MaxMetadataSizeInKB.Value);
            }

            if (areEqual && updated.OptimizeForLocalSSD.HasValue)
            {
                areEqual = old.OptimizeForLocalSSD.HasValue && (old.OptimizeForLocalSSD.Value == updated.OptimizeForLocalSSD.Value);
            }

            if (areEqual && updated.OptimizeLogForLowerDiskUsage.HasValue)
            {
                areEqual = old.OptimizeLogForLowerDiskUsage.HasValue && (old.OptimizeLogForLowerDiskUsage.Value == updated.OptimizeLogForLowerDiskUsage.Value);
            }

            if (areEqual && updated.CheckpointThresholdInMB.HasValue)
            {
                areEqual = old.CheckpointThresholdInMB.HasValue && (old.CheckpointThresholdInMB.Value == updated.CheckpointThresholdInMB.Value);
            }

            if (areEqual && updated.MaxAccumulatedBackupLogSizeInMB.HasValue)
            {
                areEqual = old.MaxAccumulatedBackupLogSizeInMB.HasValue && (old.MaxAccumulatedBackupLogSizeInMB.Value == updated.MaxAccumulatedBackupLogSizeInMB.Value);
            }

            if (areEqual && updated.MinLogSizeInMB.HasValue)
            {
                areEqual = old.MinLogSizeInMB.HasValue && (old.MinLogSizeInMB.Value == updated.MinLogSizeInMB.Value);
            }

            if (areEqual && updated.TruncationThresholdFactor.HasValue)
            {
                areEqual = old.TruncationThresholdFactor.HasValue && (old.TruncationThresholdFactor.Value == updated.TruncationThresholdFactor.Value);
            }

            if (areEqual && updated.ThrottlingThresholdFactor.HasValue)
            {
                areEqual = old.ThrottlingThresholdFactor.HasValue && (old.ThrottlingThresholdFactor.Value == updated.ThrottlingThresholdFactor.Value);
            }

            if (areEqual && updated.SlowApiMonitoringDuration.HasValue)
            {
                areEqual = old.SlowApiMonitoringDuration.HasValue && (old.SlowApiMonitoringDuration == updated.SlowApiMonitoringDuration);
            }

#if !DotNetCoreClr
            // 12529905 - Disable new configuration for LogTruncationIntervalSeconds in CoreCLR
            if (areEqual && updated.LogTruncationIntervalSeconds.HasValue)
            {
                areEqual = old.LogTruncationIntervalSeconds.HasValue && (old.LogTruncationIntervalSeconds == updated.LogTruncationIntervalSeconds);
            }

            if (areEqual && updated.EnableIncrementalBackupsAcrossReplicas.HasValue)
            {
                areEqual = old.EnableIncrementalBackupsAcrossReplicas.HasValue && (old.EnableIncrementalBackupsAcrossReplicas == updated.EnableIncrementalBackupsAcrossReplicas);
            }
#endif
            return areEqual;
        }
    }
}