// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Globalization;
    using Microsoft.ServiceFabric.Replicator;
    using System.IO;

    internal static class ReliableStateManagerReplicatorSettingsUtil
    {
        private const int SpareMaxStreamSizeInMB = 200 * 1024;

        private const int DefaultMinLogSizeInMBDivider = 2;
        private const int SmallestMinLogSizeInMB = 1;

        /// <summary>
        /// Loads the replicator settings from a section specified in the service settings configuration file - settings.xml
        /// </summary>
        /// <param name="codePackageActivationContext"> Current code package activation context</param>
        /// <param name="configPackageName"> Name of the configuration package</param>
        /// <param name="sectionName">Name of the section within the configuration package xml. If empty or null, the default name("TransactionalReplicator") is presumed</param>
        /// <remarks>
        /// The following are the parameter names that should be provided in the service configuration “settings.xml”, to be recognizable by service fabric to load the transactional replicator settings.
        /// 
        ///     1. BatchAcknowledgementInterval –  <see cref="ReliableStateManagerReplicatorSettings.BatchAcknowledgementInterval"/> value in seconds.
        ///     
        ///     2. InitialCopyQueueSize - <see cref="ReliableStateManagerReplicatorSettings.InitialCopyQueueSize"/> InitialCopyQueueSize value.
        ///     
        ///     3. MaxCopyQueueSize - <see cref="ReliableStateManagerReplicatorSettings.MaxCopyQueueSize"/> value.
        ///     
        ///     4. MaxReplicationMessageSize - <see cref="ReliableStateManagerReplicatorSettings.MaxReplicationMessageSize"/> value in bytes.
        ///     
        ///     5. RetryInterval - <see cref="ReliableStateManagerReplicatorSettings.RetryInterval"/> value in seconds.
        ///     
        ///     6. ReplicatorAddress or ReplicatorEndpoint – <see cref="ReliableStateManagerReplicatorSettings.ReplicatorAddress"/> should be of the form IP:Port. ReplicatorEndpoint must reference a valid service endpoint resource from the service manifest.
        ///     
        ///     7. InitialPrimaryReplicationQueueSize - <see cref="ReliableStateManagerReplicatorSettings.InitialPrimaryReplicationQueueSize"/> value.
        ///     
        ///     8. InitialSecondaryReplicationQueueSize - <see cref="ReliableStateManagerReplicatorSettings.InitialSecondaryReplicationQueueSize"/> value.
        ///     
        ///     9. MaxPrimaryReplicationQueueSize - <see cref="ReliableStateManagerReplicatorSettings.MaxPrimaryReplicationQueueSize"/> value.
        ///     
        ///     10. MaxSecondaryReplicationQueueSize - <see cref="ReliableStateManagerReplicatorSettings.MaxSecondaryReplicationQueueSize"/> value.
        /// 
        ///     11. MaxPrimaryReplicationQueueMemorySize - <see cref="ReliableStateManagerReplicatorSettings.MaxPrimaryReplicationQueueMemorySize"/> value in Bytes.
        ///     
        ///     12. MaxSecondaryReplicationQueueMemorySize - <see cref="ReliableStateManagerReplicatorSettings.MaxSecondaryReplicationQueueMemorySize"/> value in Bytes.
        ///     
        ///     13. OptimizeLogForLowerDiskUsage - <see cref="ReliableStateManagerReplicatorSettings.OptimizeLogForLowerDiskUsage"/> value.
        ///     
        ///     14. MaxMetadataSizeInKB - <see cref="ReliableStateManagerReplicatorSettings.MaxMetadataSizeInKB"/> value in KB.
        ///     
        ///     15. MaxRecordSizeInKB - <see cref="ReliableStateManagerReplicatorSettings.MaxRecordSizeInKB"/> value in KB.
        ///     
        ///     16. MaxWriteQueueDepthInKB - <see cref="ReliableStateManagerReplicatorSettings.MaxWriteQueueDepthInKB"/> value in KB.
        ///     
        ///     17. SharedLogId - <see cref="ReliableStateManagerReplicatorSettings.SharedLogId"/> value.
        ///     
        ///     18. SharedLogPath - <see cref="ReliableStateManagerReplicatorSettings.SharedLogPath"/> value.
        ///     
        ///     19. CheckpointThresholdInMB - <see cref="ReliableStateManagerReplicatorSettings.CheckpointThresholdInMB"/> value.
        ///     
        ///     20. SecondaryClearAcknowledgedOperations - <see cref="ReliableStateManagerReplicatorSettings.SecondaryClearAcknowledgedOperations"/> value.
        /// 
        ///     21. MaxAccumulatedBackupLogSizeInMB - <see cref="ReliableStateManagerReplicatorSettings.MaxAccumulatedBackupLogSizeInMB"/> value.
        ///      
        ///     22. MinLogSizeInMB - <see cref="ReliableStateManagerReplicatorSettings.MinLogSizeInMB"/> value.
        /// 
        ///     23. TruncationThresholdFactor - <see cref="ReliableStateManagerReplicatorSettings.TruncationThresholdFactor"/> value.
        /// 
        ///     24. ThrottlingThresholdFactor - <see cref="ReliableStateManagerReplicatorSettings.ThrottlingThresholdFactor"/> value.
        ///     
        ///     25. ReplicatorListenAddress or ReplicatorEndpoint – <see cref="ReliableStateManagerReplicatorSettings.ReplicatorListenAddress"/> should be of the form IP:Port. ReplicatorEndpoint must reference a valid service endpoint resource from the service manifest.
        ///
        ///     26. ReplicatorPublishAddress or ReplicatorEndpoint – <see cref="ReliableStateManagerReplicatorSettings.ReplicatorPublishAddress"/> should be of the form IP:Port. ReplicatorEndpoint must reference a valid service endpoint resource from the service manifest.
        /// </remarks>
        public static ReliableStateManagerReplicatorSettings LoadFrom(
            CodePackageActivationContext codePackageActivationContext,
            string configPackageName,
            string sectionName)
        {
            var sectionNametoUse = sectionName;

            if (configPackageName == null ||
                codePackageActivationContext == null)
                return null;

            if (string.IsNullOrEmpty(sectionNametoUse))
                sectionNametoUse = ReliableStateManagerReplicatorSettingsConfigurationNames.SectionName;

            var parameters = codePackageActivationContext.GetConfigurationPackageObject(configPackageName).Settings.Sections[sectionNametoUse].Parameters;

            var v1Settings = ReplicatorSettings.LoadFrom(codePackageActivationContext, configPackageName, sectionNametoUse);

            // Read in the V1 replicator settings
            var replicatorSettings = new ReliableStateManagerReplicatorSettings
            {
                RetryInterval = v1Settings.RetryInterval,
                BatchAcknowledgementInterval = v1Settings.BatchAcknowledgementInterval,
                ReplicatorAddress = v1Settings.ReplicatorAddress,
                ReplicatorListenAddress = v1Settings.ReplicatorListenAddress,
                ReplicatorPublishAddress = v1Settings.ReplicatorPublishAddress,
                InitialCopyQueueSize = v1Settings.InitialCopyQueueSize,
                MaxCopyQueueSize = v1Settings.MaxCopyQueueSize,
                MaxReplicationMessageSize = v1Settings.MaxReplicationMessageSize,
                InitialPrimaryReplicationQueueSize = v1Settings.InitialPrimaryReplicationQueueSize,
                MaxPrimaryReplicationQueueSize = v1Settings.MaxPrimaryReplicationQueueSize,
                MaxPrimaryReplicationQueueMemorySize = v1Settings.MaxPrimaryReplicationQueueMemorySize,
                InitialSecondaryReplicationQueueSize = v1Settings.InitialSecondaryReplicationQueueSize,
                MaxSecondaryReplicationQueueSize = v1Settings.MaxSecondaryReplicationQueueSize,
                MaxSecondaryReplicationQueueMemorySize = v1Settings.MaxSecondaryReplicationQueueMemorySize,
                SecondaryClearAcknowledgedOperations = v1Settings.SecondaryClearAcknowledgedOperations,
                MaxStreamSizeInMB = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.MaxStreamSizeInMB.Item1),
                MaxMetadataSizeInKB = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.MaxMetadataSizeInKB.Item1),
                MaxRecordSizeInKB = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.MaxRecordSizeInKB.Item1),
                MaxWriteQueueDepthInKB = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.MaxWriteQueueDepthInKB.Item1),
                OptimizeForLocalSSD = GetPropertyBoolValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.OptimizeForLocalSSD.Item1),
                SharedLogId = GetPropertyStringValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.SharedLogId.Item1),
                SharedLogPath = GetPropertyStringValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.SharedLogPath.Item1),
                OptimizeLogForLowerDiskUsage = GetPropertyBoolValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.OptimizeLogForLowerDiskUsage.Item1),
                CheckpointThresholdInMB = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.CheckpointThresholdInMB.Item1),
                MaxAccumulatedBackupLogSizeInMB = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.MaxAccumulatedBackupLogSizeInMB.Item1),
                SlowApiMonitoringDuration =  GetPropertyTimespanValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.SlowApiMonitoringDuration.Item1),
                MinLogSizeInMB = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.MinLogSizeInMB.Item1),
                TruncationThresholdFactor = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.TruncationThresholdFactor.Item1),
                ThrottlingThresholdFactor = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.ThrottlingThresholdFactor.Item1)
            };

#if !DotNetCoreClr
            // 12529905 - Disable new configuration for LogTruncationIntervalSeconds in CoreCLR
            replicatorSettings.LogTruncationIntervalSeconds = GetPropertyIntValue(parameters, ReliableStateManagerReplicatorSettingsConfigurationNames.LogTruncationIntervalSeconds.Item1);

            replicatorSettings.EnableIncrementalBackupsAcrossReplicas = GetPropertyBoolValue(
                parameters, 
                ReliableStateManagerReplicatorSettingsConfigurationNames.EnableIncrementalBackupsAcrossReplicas.Item1);
#endif
            // LoadFrom Should throw if invalid configurations are being loaded
            ValidatePublicSettings(replicatorSettings);
            return replicatorSettings;
        }

        internal static void ValidatePublicSettings(ReliableStateManagerReplicatorSettings txReplicatorSettings)
        {
            VaidateReplicatorSettings(txReplicatorSettings);
            ValidateLoggerSettings(txReplicatorSettings);
        }

        internal static void ValidateSettings(TransactionalReplicatorSettings txReplicatorSettings)
        {
            ValidatePublicSettings(txReplicatorSettings.PublicSettings);
            ValidateInternalSettings(txReplicatorSettings);
        }

        private static void ValidateInternalSettings(TransactionalReplicatorSettings txReplicatorSettings)
        {
            if (!string.IsNullOrEmpty(txReplicatorSettings.Test_LoggingEngine))
            {
                LoggerEngine engine;
                bool validLoggingEngine = Enum.TryParse(txReplicatorSettings.Test_LoggingEngine, true, out engine);

                if (!validLoggingEngine || engine == LoggerEngine.Invalid)
                {
                    throw new ArgumentOutOfRangeException(
                        "TransactionalReplicatorSettings.Test_LoggingEngine",
                        string.Format(
                            CultureInfo.CurrentCulture,
                            SR.Error_ReliableSMSettings_InvalidLoggingEngine,
                            txReplicatorSettings.Test_LoggingEngine,
                            "ktl/memory/file"));
                }
            }

            if (txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMinDelayIntervalMilliseconds != 0)
            {
                if ((txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMaxDelayIntervalMilliseconds < txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMinDelayIntervalMilliseconds) ||
                    txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMaxDelayIntervalMilliseconds < 0)
                {
                    throw new ArgumentOutOfRangeException(
                        "Test_LogMaxDelayIntervalMilliseconds must be non-zero and greater than Test_LogMinDelayIntervalMilliseconds");
                }
            }

            if (txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMaxDelayIntervalMilliseconds != 0)
            {
                if ((txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMinDelayIntervalMilliseconds > txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMaxDelayIntervalMilliseconds) ||
                    txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogMinDelayIntervalMilliseconds < 0)
                {
                    throw new ArgumentOutOfRangeException(
                        "Test_LogMaxDelayIntervalMilliseconds must be non-zero and greater than Test_LogMinDelayIntervalMilliseconds");
                }
            } 

            if (Math.Abs(txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogDelayRatio) > 1 || txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogDelayRatio < 0)
            {
                throw new ArgumentException("Test_LogDelayRatio must be greater than zero and less than 1");
            }

            if (Math.Abs(txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogDelayProcessExitRatio) > 1 || txReplicatorSettings.LogWriteFaultInjectionParameters.Test_LogDelayProcessExitRatio < 0)
            {
                throw new ArgumentException("Test_LogDelayProcessExitRatio must be greater than zero and less than 1");
            }
        }

        private static void VaidateReplicatorSettings(ReliableStateManagerReplicatorSettings txReplicatorSettings)
        {
            if (txReplicatorSettings.CheckpointThresholdInMB.HasValue)
            {
                if (txReplicatorSettings.CheckpointThresholdInMB < 1)
                {
                    throw new ArgumentOutOfRangeException(
                        "ReplicatorSettings.CheckpointThresholdInMB",
                        string.Format(CultureInfo.CurrentCulture, SR.Error_ReliableSMSettings_CheckpointNegative, txReplicatorSettings.CheckpointThresholdInMB));
                }
            }

            if (txReplicatorSettings.MaxAccumulatedBackupLogSizeInMB.HasValue)
            {
                if (txReplicatorSettings.MaxAccumulatedBackupLogSizeInMB < 1)
                {
                    throw new ArgumentOutOfRangeException(
                        "ReplicatorSettings.MaxAccumulatedBackupLogSizeInMB",
                        string.Format(CultureInfo.CurrentCulture, SR.Error_ReliableSMSettings_BackupLogSizeNegative, txReplicatorSettings.MaxAccumulatedBackupLogSizeInMB));
                }

                if (txReplicatorSettings.MaxStreamSizeInMB.HasValue &&
                    txReplicatorSettings.MaxAccumulatedBackupLogSizeInMB >= txReplicatorSettings.MaxStreamSizeInMB.Value)
                {
                    throw new ArgumentOutOfRangeException(
                        "ReplicatorSettings.MaxAccumulatedBackupLogSizeInMB",
                            string.Format(CultureInfo.CurrentCulture, SR.Error_ReliableSMSettings_BackupLogSizeGreaterThanMax, 
                            txReplicatorSettings.MaxAccumulatedBackupLogSizeInMB, 
                            txReplicatorSettings.MaxStreamSizeInMB.Value));
                }
            }

            if (txReplicatorSettings.SlowApiMonitoringDuration.HasValue)
            {
                if (txReplicatorSettings.SlowApiMonitoringDuration < TimeSpan.Zero || 
                    txReplicatorSettings.SlowApiMonitoringDuration.Equals(TimeSpan.MaxValue))
                {
                    throw new ArgumentOutOfRangeException(
                        "txReplicatorSettings.SlowApiMonitoringDuration",
                        string.Format(CultureInfo.CurrentCulture, SR.Error_Invalid_SlowApiMonitoringDuration,
                        txReplicatorSettings.SlowApiMonitoringDuration,
                        ReliableStateManagerReplicatorSettingsConfigurationNames.SlowApiMonitoringDuration.Item2));
                }
            }

            ValidateMinLogSize(txReplicatorSettings);

#if !DotNetCoreClr
            if (txReplicatorSettings.LogTruncationIntervalSeconds.HasValue)
            {
                if (txReplicatorSettings.LogTruncationIntervalSeconds < 0)
                {
                    throw new ArgumentOutOfRangeException(
                        "txReplicatorSettings.LogTruncationIntervalSeconds",
                        SR.Error_Invalid_LogTruncationInterval);
                }
            }
#endif
        }

        private static void ValidateMinLogSize(ReliableStateManagerReplicatorSettings txReplicatorSettings)
        {
            var sectionName = ReliableStateManagerReplicatorSettingsConfigurationNames.SectionName;

            ConfigStoreUtility.InitializeConfigStore();

            int checkpointThresholdInMB = txReplicatorSettings.CheckpointThresholdInMB.HasValue
                ? txReplicatorSettings.CheckpointThresholdInMB.Value
                : ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.CheckpointThresholdInMB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.CheckpointThresholdInMB.Item2).Value;

            int minLogSizeInMB = txReplicatorSettings.MinLogSizeInMB.HasValue
                ? txReplicatorSettings.MinLogSizeInMB.Value
                : ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MinLogSizeInMB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MinLogSizeInMB.Item2).Value;

            minLogSizeInMB = GetMinLogSizeInMB(minLogSizeInMB, checkpointThresholdInMB);

            int truncationThresholdFactor = txReplicatorSettings.TruncationThresholdFactor.HasValue
                ? txReplicatorSettings.TruncationThresholdFactor.Value
                : ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.TruncationThresholdFactor.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.TruncationThresholdFactor.Item2).Value;

            int throttlingThresholdFactor = txReplicatorSettings.ThrottlingThresholdFactor.HasValue
                ? txReplicatorSettings.ThrottlingThresholdFactor.Value
                : ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ThrottlingThresholdFactor.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ThrottlingThresholdFactor.Item2).Value;

            int maxStreamSizeInMB = txReplicatorSettings.MaxStreamSizeInMB.HasValue
                ? txReplicatorSettings.MaxStreamSizeInMB.Value
                : ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxStreamSizeInMB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxStreamSizeInMB.Item2).Value;

            bool isSparseFileOff = txReplicatorSettings.OptimizeLogForLowerDiskUsage.HasValue && (txReplicatorSettings.OptimizeLogForLowerDiskUsage.Value == false);
            maxStreamSizeInMB = isSparseFileOff ? maxStreamSizeInMB : SpareMaxStreamSizeInMB;

            // MinLogSizeInMB checks,
            if (minLogSizeInMB < 1)
            {
                throw new ArgumentOutOfRangeException(
                    "ReplicatorSettings.MinLogSizeInMB",
                    string.Format(CultureInfo.CurrentCulture,
                    SR.Error_ReliableSMSettings_MinLogSizeInMB_LessThanOne,
                    minLogSizeInMB));
            }

            if (minLogSizeInMB >= maxStreamSizeInMB)
            {
                throw new ArgumentOutOfRangeException(
                    "ReplicatorSettings.MinLogSizeInMB",
                    string.Format(CultureInfo.CurrentCulture,
                    "MinLogSizeInMB {0} must be smaller than MaxStreamSizeInMB {1}",
                    minLogSizeInMB,
                    maxStreamSizeInMB));
            }

            // TruncationFactor checks
            if (truncationThresholdFactor < 2)
            {
                throw new ArgumentOutOfRangeException(
                    "ReplicatorSettings.TruncationThresholdFactor",
                    string.Format(CultureInfo.CurrentCulture,
                    "TruncationThresholdFactor {0} must be greater than 1.",
                    truncationThresholdFactor));
            }

            int truncationThresholdInMB = minLogSizeInMB * truncationThresholdFactor;
            if (truncationThresholdInMB >= maxStreamSizeInMB)
            {
                // Parameter name is MinLogSizeInMB since its public, others are not.
                throw new ArgumentOutOfRangeException(
                    "ReplicatorSettings.MinLogSizeInMB",
                    string.Format(CultureInfo.CurrentCulture,
                    "MinLogSizeInMB {0} * TruncationThresholdFactor {1} must be smaller than MaxStreamSizeInMB {2}",
                    minLogSizeInMB,
                    truncationThresholdFactor,
                    maxStreamSizeInMB));
            }

            // ThrottlingThresholdFactor checks.
            // throttling threshold factor must be at least three since truncation must at least be 2.
            if (throttlingThresholdFactor < 3)
            {
                throw new ArgumentOutOfRangeException(
                    "ReplicatorSettings.ThrottlingThresholdFactor",
                    string.Format(CultureInfo.CurrentCulture,
                    "ThrottlingThresholdFactor {0} must be greater than 1.",
                    throttlingThresholdFactor));
            }

            if (throttlingThresholdFactor <= truncationThresholdFactor)
            {
                throw new ArgumentOutOfRangeException(
                    "ReplicatorSettings.ThrottlingThresholdFactor",
                    string.Format(CultureInfo.CurrentCulture,
                    "ThrottlingThresholdFactor {0} must be greater than TruncationThresholdFactor {1}.",
                    throttlingThresholdFactor,
                    truncationThresholdFactor));
            }

            int throttlingThresholdInMB = GetThrottleThresholdInMB(throttlingThresholdFactor, checkpointThresholdInMB, minLogSizeInMB);
            if (throttlingThresholdInMB >= maxStreamSizeInMB)
            {
                // Parameter name is MinLogSizeInMB since its public, others are not.
                throw new ArgumentOutOfRangeException(
                    "ReplicatorSettings.MinLogSizeInMB",
                    string.Format(CultureInfo.CurrentCulture,
                    "Max(MinLogSizeInMB {0} * ThrottlingThresholdFactor {1}, CheckpointThresholdInMB {2} * ThrottlingThresholdFactor {1}) must be smaller than MaxStreamSizeInMB {3}",
                    minLogSizeInMB,
                    throttlingThresholdFactor,
                    checkpointThresholdInMB,
                    maxStreamSizeInMB));
            }
        }

        private static bool IsMultipleOf4(int value)
        {
            return ((value%4) == 0);
        }

        private static void ValidateLoggerSettings(ReliableStateManagerReplicatorSettings txReplicatorSettings)
        {
            if (!string.IsNullOrEmpty(txReplicatorSettings.SharedLogId))
            {
                var formatOk = true;
                try
                {
                    Guid.Parse(txReplicatorSettings.SharedLogId);
                }
                catch (FormatException)
                {
                    formatOk = false;
                }

                if (!formatOk)
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_SharedLogId_Format);
                }
            }

            if (!string.IsNullOrEmpty(txReplicatorSettings.SharedLogPath))
            {
                bool isAbsolutePath;

                isAbsolutePath = System.IO.Path.IsPathRooted(txReplicatorSettings.SharedLogPath);

                if (!isAbsolutePath)
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_SharedLogPath_Format);
                }
            }

            if ((string.IsNullOrEmpty(txReplicatorSettings.SharedLogId)) &&
                (!string.IsNullOrEmpty(txReplicatorSettings.SharedLogPath)))
            {
                throw new ArgumentException(
                    SR.Error_ReliableSMSettings_SharedLogId_Empy);
            }

            if ((!string.IsNullOrEmpty(txReplicatorSettings.SharedLogId)) &&
                (string.IsNullOrEmpty(txReplicatorSettings.SharedLogPath)))
            {
                throw new ArgumentException(
                    SR.Error_ReliableSMSettings_SharedLogPath_Empty);
            }

            if (txReplicatorSettings.MaxWriteQueueDepthInKB.HasValue)
            {
                if (txReplicatorSettings.MaxWriteQueueDepthInKB < 0)
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxWriteQueueDepth_Positive);
                }

                if (!((txReplicatorSettings.MaxWriteQueueDepthInKB == 0) ||
                      (IsMultipleOf4(txReplicatorSettings.MaxWriteQueueDepthInKB.Value))))
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxWriteQueueDepth_Conditions);
                }
            }

            if (txReplicatorSettings.MaxMetadataSizeInKB.HasValue)
            {
                if (txReplicatorSettings.MaxMetadataSizeInKB < 0)
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxMetadataSize_Positive);
                }

                if (!IsMultipleOf4(txReplicatorSettings.MaxMetadataSizeInKB.Value))
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxMetadataSize_Condition);
                }
            }

            if (txReplicatorSettings.MaxRecordSizeInKB.HasValue)
            {
                if (!IsMultipleOf4(txReplicatorSettings.MaxRecordSizeInKB.Value))
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxRecordSize_Condition_Multiple);
                }

                if (txReplicatorSettings.MaxRecordSizeInKB < 128)
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxRecordSize_GreaterThan_128);
                }
            }

            if (txReplicatorSettings.OptimizeLogForLowerDiskUsage.HasValue &&
                txReplicatorSettings.OptimizeLogForLowerDiskUsage.Value == false &&
                txReplicatorSettings.MaxStreamSizeInMB.HasValue &&
                txReplicatorSettings.MaxRecordSizeInKB.HasValue)
            {
                var logSizeinKB = txReplicatorSettings.MaxStreamSizeInMB.Value*1024;
                if (logSizeinKB < 16*txReplicatorSettings.MaxRecordSizeInKB)
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxStreamSize_GreaterThan_MaxRecordSize);
                }
            }

            // If the log is not optimized for low disk, it means it is NOT a sparse log.
            // Hence validate that the checkpoint threshold falls within the log size limit
            if (txReplicatorSettings.OptimizeLogForLowerDiskUsage.HasValue &&
                txReplicatorSettings.OptimizeLogForLowerDiskUsage.Value == false &&
                txReplicatorSettings.MaxStreamSizeInMB.HasValue &&
                txReplicatorSettings.CheckpointThresholdInMB.HasValue)
            {
                if (txReplicatorSettings.MaxStreamSizeInMB.Value < txReplicatorSettings.CheckpointThresholdInMB.Value)
                {
                    throw new ArgumentException(
                        SR.Error_ReliableSMSettings_MaxRecordSize_GreaterThan_CheckpointThreshold);
                }
            }

            return;
        }

        /// <summary>
        /// The get property long value.
        /// </summary>
        /// <param name="parameters">
        /// The parameters.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// The <see cref="long"/> value
        /// </returns>
        internal static long? GetPropertyLongValue(
            KeyedCollection<string, ConfigurationProperty> parameters,
            string name)
        {
            long? t = null;

            if (parameters == null || (parameters.Contains(name) == false))
            {
                return null;
            }

            var property = parameters[name];

            try
            {
                t = long.Parse(property.Value);
            }
            catch (Exception e)
            {
                var type = string.Format(CultureInfo.InvariantCulture, "PersistedReplicatorSettings@{0}", name);
                AppTrace.TraceSource.WriteExceptionAsWarning(type, e);
            }

            return (t);
        }

        /// <summary>
        /// The get property boolean value.
        /// </summary>
        /// <param name="parameters">
        /// The parameters.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/> value
        /// </returns>
        internal static bool? GetPropertyBoolValue(
            KeyedCollection<string, ConfigurationProperty> parameters,
            string name)
        {
            bool? t = null;

            if (parameters == null || (parameters.Contains(name) == false))
            {
                return null;
            }

            var property = parameters[name];

            try
            {
                t = bool.Parse(property.Value);
            }
            catch (Exception e)
            {
                var type = string.Format(CultureInfo.InvariantCulture, "PersistedReplicatorSettings@{0}", name);
                AppTrace.TraceSource.WriteExceptionAsWarning(type, e);
            }

            return (t);
        }

        /// <summary>
        /// The get property timespan value.
        /// </summary>
        /// <param name="parameters">
        /// The parameters.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// The <see cref="TimeSpan"/> value
        /// </returns>
        internal static TimeSpan? GetPropertyTimespanValue(
            KeyedCollection<string, ConfigurationProperty> parameters,
            string name)
        {
            TimeSpan? t = null;

            if (parameters == null || (parameters.Contains(name) == false))
            {
                return null;
            }

            var property = parameters[name];

            try
            {
                t = TimeSpan.FromSeconds(double.Parse(property.Value));
            }
            catch (Exception e)
            {
                var type = string.Format(CultureInfo.InvariantCulture, "PersistedReplicatorSettings@{0}", name);
                AppTrace.TraceSource.WriteExceptionAsWarning(type, e);
            }

            return (t);
        }

        /// <summary>
        /// The get property integer value.
        /// </summary>
        /// <param name="parameters">
        /// The parameters.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// The <see cref="uint"/> value
        /// </returns>
        internal static uint? GetPropertyUIntValue(
            KeyedCollection<string, ConfigurationProperty> parameters,
            string name)
        {
            uint? t = null;

            if (parameters == null || (parameters.Contains(name) == false))
            {
                return null;
            }

            var property = parameters[name];

            try
            {
                t = uint.Parse(property.Value);
            }
            catch (Exception e)
            {
                var type = string.Format(CultureInfo.InvariantCulture, "PersistedReplicatorSettings@{0}", name);
                AppTrace.TraceSource.WriteExceptionAsWarning(type, e);
            }

            return (t);
        }

        /// <summary>
        /// The get property integer value.
        /// </summary>
        /// <param name="parameters">
        /// The parameters.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// The <see cref="uint"/> value
        /// </returns>
        internal static int? GetPropertyIntValue(
            KeyedCollection<string, ConfigurationProperty> parameters,
            string name)
        {
            int? t = null;

            if (parameters == null || (parameters.Contains(name) == false))
            {
                return null;
            }

            var property = parameters[name];

            try
            {
                t = int.Parse(property.Value);
            }
            catch (Exception e)
            {
                var type = string.Format(CultureInfo.InvariantCulture, "PersistedReplicatorSettings@{0}", name);
                AppTrace.TraceSource.WriteExceptionAsWarning(type, e);
            }

            return (t);
        }

        /// <summary>
        /// The get property String value.
        /// </summary>
        /// <param name="parameters">
        /// The parameters.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// The <see cref="Guid"/> value
        /// </returns>
        internal static string GetPropertyStringValue(
            KeyedCollection<string, ConfigurationProperty> parameters,
            string name)
        {
            string t = null;

            if (parameters == null || (parameters.Contains(name) == false))
            {
                return null;
            }

            var property = parameters[name];

            try
            {
                t = property.Value;
            }
            catch (Exception e)
            {
                var type = string.Format(CultureInfo.InvariantCulture, "PersistedReplicatorSettings@{0}", name);
                AppTrace.TraceSource.WriteExceptionAsWarning(type, e);
            }

            return (t);
        }

        /// <summary>
        /// Loads the replicator settings from the cluster manifest section 'TransactionalReplicator' only if the value 
        /// is already not set on the property
        /// </summary>
        /// <param name="replicatorSettings">Existing replicator settings object</param>
        internal static void LoadDefaultsIfNotSet(ref ReliableStateManagerReplicatorSettings replicatorSettings)
        {
            var sectionName = ReliableStateManagerReplicatorSettingsConfigurationNames.SectionName;

            ConfigStoreUtility.InitializeConfigStore();

            if (!replicatorSettings.RetryInterval.HasValue)
                replicatorSettings.RetryInterval = ConfigStoreUtility.GetUnencryptedTimespanConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.RetryInterval.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.RetryInterval.Item2);

            if (!replicatorSettings.BatchAcknowledgementInterval.HasValue)
                replicatorSettings.BatchAcknowledgementInterval = ConfigStoreUtility.GetUnencryptedTimespanConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.BatchAcknowledgementInterval.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.BatchAcknowledgementInterval.Item2);

            if (string.IsNullOrEmpty(replicatorSettings.ReplicatorAddress))
                replicatorSettings.ReplicatorAddress = ConfigStoreUtility.GetUnencryptedConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ReplicatorAddress.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ReplicatorAddress.Item2);

            if (string.IsNullOrEmpty(replicatorSettings.ReplicatorListenAddress))
                replicatorSettings.ReplicatorListenAddress = ConfigStoreUtility.GetUnencryptedConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ReplicatorListenAddress.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ReplicatorListenAddress.Item2);

            if (string.IsNullOrEmpty(replicatorSettings.ReplicatorPublishAddress))
                replicatorSettings.ReplicatorPublishAddress = ConfigStoreUtility.GetUnencryptedConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ReplicatorPublishAddress.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ReplicatorPublishAddress.Item2);

            if (!replicatorSettings.InitialCopyQueueSize.HasValue)
                replicatorSettings.InitialCopyQueueSize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.InitialCopyQueueSize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.InitialCopyQueueSize.Item2);

            if (!replicatorSettings.MaxCopyQueueSize.HasValue)
                replicatorSettings.MaxCopyQueueSize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxCopyQueueSize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxCopyQueueSize.Item2);

            if (!replicatorSettings.MaxReplicationMessageSize.HasValue)
                replicatorSettings.MaxReplicationMessageSize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxReplicationMessageSize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxReplicationMessageSize.Item2);

            if (!replicatorSettings.InitialPrimaryReplicationQueueSize.HasValue)
                replicatorSettings.InitialPrimaryReplicationQueueSize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.InitialPrimaryReplicationQueueSize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.InitialPrimaryReplicationQueueSize.Item2);

            if (!replicatorSettings.MaxPrimaryReplicationQueueSize.HasValue)
                replicatorSettings.MaxPrimaryReplicationQueueSize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxPrimaryReplicationQueueSize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxPrimaryReplicationQueueSize.Item2);

            if (!replicatorSettings.MaxPrimaryReplicationQueueMemorySize.HasValue)
                replicatorSettings.MaxPrimaryReplicationQueueMemorySize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxPrimaryReplicationQueueMemorySize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxPrimaryReplicationQueueMemorySize.Item2);

            if (!replicatorSettings.InitialSecondaryReplicationQueueSize.HasValue)
                replicatorSettings.InitialSecondaryReplicationQueueSize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.InitialSecondaryReplicationQueueSize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.InitialSecondaryReplicationQueueSize.Item2);

            if (!replicatorSettings.MaxSecondaryReplicationQueueSize.HasValue)
                replicatorSettings.MaxSecondaryReplicationQueueSize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxSecondaryReplicationQueueSize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxSecondaryReplicationQueueSize.Item2);

            if (!replicatorSettings.MaxSecondaryReplicationQueueMemorySize.HasValue)
                replicatorSettings.MaxSecondaryReplicationQueueMemorySize = ConfigStoreUtility.GetUnencryptedLongConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxSecondaryReplicationQueueMemorySize.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxSecondaryReplicationQueueMemorySize.Item2);

            // Now read the persisted replicator specific settings
            if (!replicatorSettings.MaxStreamSizeInMB.HasValue)
                replicatorSettings.MaxStreamSizeInMB = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxStreamSizeInMB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxStreamSizeInMB.Item2);

            if (!replicatorSettings.MaxMetadataSizeInKB.HasValue)
                replicatorSettings.MaxMetadataSizeInKB = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxMetadataSizeInKB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxMetadataSizeInKB.Item2);

            if (!replicatorSettings.MaxRecordSizeInKB.HasValue)
                replicatorSettings.MaxRecordSizeInKB = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxRecordSizeInKB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxRecordSizeInKB.Item2);

            if (!replicatorSettings.CheckpointThresholdInMB.HasValue)
                replicatorSettings.CheckpointThresholdInMB = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.CheckpointThresholdInMB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.CheckpointThresholdInMB.Item2);

            if (!replicatorSettings.MaxAccumulatedBackupLogSizeInMB.HasValue)
                replicatorSettings.MaxAccumulatedBackupLogSizeInMB = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxAccumulatedBackupLogSizeInMB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxAccumulatedBackupLogSizeInMB.Item2);

            if (!replicatorSettings.OptimizeForLocalSSD.HasValue)
                replicatorSettings.OptimizeForLocalSSD = ConfigStoreUtility.GetUnencryptedBoolConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.OptimizeForLocalSSD.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.OptimizeForLocalSSD.Item2);

            if (!replicatorSettings.SecondaryClearAcknowledgedOperations.HasValue)
                replicatorSettings.SecondaryClearAcknowledgedOperations = ConfigStoreUtility.GetUnencryptedBoolConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SecondaryClearAcknowledgedOperations.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SecondaryClearAcknowledgedOperations.Item2);

            if (!replicatorSettings.OptimizeLogForLowerDiskUsage.HasValue)
                replicatorSettings.OptimizeLogForLowerDiskUsage = ConfigStoreUtility.GetUnencryptedBoolConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.OptimizeLogForLowerDiskUsage.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.OptimizeLogForLowerDiskUsage.Item2);

            if (!replicatorSettings.MaxWriteQueueDepthInKB.HasValue)
                replicatorSettings.MaxWriteQueueDepthInKB = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxWriteQueueDepthInKB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxWriteQueueDepthInKB.Item2);

            if (string.IsNullOrEmpty(replicatorSettings.SharedLogId))
                replicatorSettings.SharedLogId = ConfigStoreUtility.GetUnencryptedStringConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SharedLogId.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SharedLogId.Item2);

            if (string.IsNullOrEmpty(replicatorSettings.SharedLogPath))
                replicatorSettings.SharedLogPath = ConfigStoreUtility.GetUnencryptedStringConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SharedLogPath.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SharedLogPath.Item2);

            if (!replicatorSettings.SlowApiMonitoringDuration.HasValue)
                replicatorSettings.SlowApiMonitoringDuration = ConfigStoreUtility.GetUnencryptedTimespanConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SlowApiMonitoringDuration.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.SlowApiMonitoringDuration.Item2);

            // TODO: Workaround if coalescing is not enabled and old driver is checked in for the release
            if (replicatorSettings.OptimizeLogForLowerDiskUsage == true)
            {
                // 200GB Sparse Log size
                replicatorSettings.MaxStreamSizeInMB = SpareMaxStreamSizeInMB;
            }

            if (!replicatorSettings.MinLogSizeInMB.HasValue)
                replicatorSettings.MinLogSizeInMB = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MinLogSizeInMB.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MinLogSizeInMB.Item2);

            if (replicatorSettings.MinLogSizeInMB == 0)
            {
                replicatorSettings.MinLogSizeInMB = GetMinLogSizeInMB(replicatorSettings.MinLogSizeInMB.Value, replicatorSettings.CheckpointThresholdInMB.Value);
            }

            if (!replicatorSettings.TruncationThresholdFactor.HasValue)
                replicatorSettings.TruncationThresholdFactor = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.TruncationThresholdFactor.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.TruncationThresholdFactor.Item2);

            if (!replicatorSettings.ThrottlingThresholdFactor.HasValue)
                replicatorSettings.ThrottlingThresholdFactor = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ThrottlingThresholdFactor.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.ThrottlingThresholdFactor.Item2);

#if !DotNetCoreClr
            if (!replicatorSettings.LogTruncationIntervalSeconds.HasValue)
                replicatorSettings.LogTruncationIntervalSeconds = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                    sectionName,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.LogTruncationIntervalSeconds.Item1,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.LogTruncationIntervalSeconds.Item2);
#endif
        }

        internal static void LoadInternalSettingsDefault(ref TransactionalReplicatorSettings replicatorSettings)
        {
            var sectionName = ReliableStateManagerReplicatorSettingsConfigurationNames.SectionName;
            ConfigStoreUtility.InitializeConfigStore();

            replicatorSettings.Test_LoggingEngine = ConfigStoreUtility.GetUnencryptedStringConfigValue(
                sectionName,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LoggingEngine.Item1,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LoggingEngine.Item2);

            var manifestFilelogMinDelayInterval = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                sectionName,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogMinDelayIntervalMilliseconds.Item1,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogMinDelayIntervalMilliseconds.Item2);

            var manifestFilelogMaxDelayInterval = ConfigStoreUtility.GetUnencryptedIntConfigValue(
                sectionName,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogMaxDelayIntervalMilliseconds.Item1,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogMaxDelayIntervalMilliseconds.Item2);

            var manifestFileLogDelayRatio = ConfigStoreUtility.GetUnencryptedDoubleConfigValue(
                sectionName,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogDelayRatio.Item1,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogDelayRatio.Item2);

            var manifestFileLogDelayProcessExitRatio = ConfigStoreUtility.GetUnencryptedDoubleConfigValue(
                sectionName,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogDelayProcessExitRatio.Item1,
                ReliableStateManagerReplicatorSettingsConfigurationNames.Test_LogDelayProcessExitRatio.Item2);

            var manifestEnableIncrementalBackupsAcrossReplicas = ConfigStoreUtility.GetUnencryptedBoolConfigValue(
                sectionName,
                ReliableStateManagerReplicatorSettingsConfigurationNames.EnableIncrementalBackupsAcrossReplicas.Item1,
                ReliableStateManagerReplicatorSettingsConfigurationNames.EnableIncrementalBackupsAcrossReplicas.Item2);

            var progressVectorMaxEntries = ConfigStoreUtility.GetUnencryptedUIntConfigValue(
                sectionName,
                ReliableStateManagerReplicatorSettingsConfigurationNames.ProgressVectorMaxEntries.Item1,
                ReliableStateManagerReplicatorSettingsConfigurationNames.ProgressVectorMaxEntries.Item2);

            if (manifestFilelogMinDelayInterval.HasValue)
            {
                replicatorSettings.LogWriteFaultInjectionParameters.Test_LogMinDelayIntervalMilliseconds = manifestFilelogMinDelayInterval.Value;
            }

            if (manifestFilelogMaxDelayInterval.HasValue)
            {
                replicatorSettings.LogWriteFaultInjectionParameters.Test_LogMaxDelayIntervalMilliseconds = manifestFilelogMaxDelayInterval.Value;
            }

            if (manifestFileLogDelayRatio.HasValue)
            {
                replicatorSettings.LogWriteFaultInjectionParameters.Test_LogDelayRatio = manifestFileLogDelayRatio.Value;
            }

            if (manifestFileLogDelayProcessExitRatio.HasValue)
            {
                replicatorSettings.LogWriteFaultInjectionParameters.Test_LogDelayProcessExitRatio = manifestFileLogDelayProcessExitRatio.Value;
            }

            if (manifestEnableIncrementalBackupsAcrossReplicas.HasValue)
            {
                replicatorSettings.EnableIncrementalBackupsAcrossReplicas = manifestEnableIncrementalBackupsAcrossReplicas.Value;
            }

#if !DotNetCoreClr
            // The EnableIncrementalBackupsAcrossReplicas config is not in CoreClr. Thus it needs to set to default value first then load from settings.
            // Note: consider set this config enabled as default.
            if (replicatorSettings.PublicSettings.EnableIncrementalBackupsAcrossReplicas.HasValue)
            {
                replicatorSettings.EnableIncrementalBackupsAcrossReplicas = replicatorSettings.PublicSettings.EnableIncrementalBackupsAcrossReplicas.Value;
            }
#endif

            if (progressVectorMaxEntries.HasValue)
            {
                replicatorSettings.ProgressVectorMaxEntries = progressVectorMaxEntries.Value;
            }
        }

        /// <summary>
        /// Downgrade this object to a ReplicatorSettings struct
        /// </summary>
        /// <returns>
        /// The <see cref="ReplicatorSettings"/>.
        /// </returns>
        internal static ReplicatorSettings ToReplicatorSettings(ReliableStateManagerReplicatorSettings settings)
        {
            var replicatorSettings = new System.Fabric.ReplicatorSettings
            {
                RetryInterval = settings.RetryInterval,
                BatchAcknowledgementInterval = settings.BatchAcknowledgementInterval,
                ReplicatorAddress = settings.ReplicatorAddress,
                ReplicatorListenAddress = settings.ReplicatorListenAddress,
                ReplicatorPublishAddress = settings.ReplicatorPublishAddress,
                SecurityCredentials = settings.SecurityCredentials,
                InitialCopyQueueSize = settings.InitialCopyQueueSize,
                MaxCopyQueueSize = settings.MaxCopyQueueSize,
                MaxReplicationMessageSize = settings.MaxReplicationMessageSize,
                InitialPrimaryReplicationQueueSize = settings.InitialPrimaryReplicationQueueSize,
                MaxPrimaryReplicationQueueSize = settings.MaxPrimaryReplicationQueueSize,
                MaxPrimaryReplicationQueueMemorySize = settings.MaxPrimaryReplicationQueueMemorySize,
                InitialSecondaryReplicationQueueSize = settings.InitialSecondaryReplicationQueueSize,
                MaxSecondaryReplicationQueueSize = settings.MaxSecondaryReplicationQueueSize,
                MaxSecondaryReplicationQueueMemorySize = settings.MaxSecondaryReplicationQueueMemorySize,
                SecondaryClearAcknowledgedOperations = settings.SecondaryClearAcknowledgedOperations
            };
          
            // The following are Ex1 Settings

            // The following are Ex3 Settings
            return (replicatorSettings);
        }

        /// <summary>
        /// Gets the ming log size in MB.
        /// 0 MinLogSizeInMB indicates that CheckpointThresholdInMB / 2 needs to be used.
        /// </summary>
        /// <param name="minLogSizeInMB">Minimum log size.</param>
        /// <param name="checkpointThresholdInMB"></param>
        /// <returns></returns>
        private static int GetMinLogSizeInMB(int minLogSizeInMB, int checkpointThresholdInMB)
        {
            if (minLogSizeInMB < 0)
            {
                throw new ArgumentOutOfRangeException(string.Format(" MinLogSizeInMB: {0} must be greater than or equal to 0", minLogSizeInMB));
            }

            if (checkpointThresholdInMB <= 0)
            {
                throw new InvalidOperationException(string.Format("CheckpointThresholdInMB: {0} must be greater than 0", checkpointThresholdInMB));
            }

            if (minLogSizeInMB == 0)
            {
                var defaultSize = checkpointThresholdInMB / DefaultMinLogSizeInMBDivider;
                if (defaultSize < SmallestMinLogSizeInMB)
                {
                    return SmallestMinLogSizeInMB;
                }

                return defaultSize;
            }

            return minLogSizeInMB;
        }

        /// <summary>
        /// Gets the ming log size in MB.
        /// 0 MinLogSizeInMB indicates that CheckpointThresholdInMB / DefaultMinLogDivider needs to be used.
        /// </summary>
        /// <param name="throttleThresholdFactor">Throttle Threshold factor.</param>
        /// <param name="checkpointThresholdInMB">Checkpoint Threshold In MB</param>
        /// <param name="minLogSizeInMB">Minimum log size.</param>
        /// <returns></returns>
        private static int GetThrottleThresholdInMB(int throttleThresholdFactor, int checkpointThresholdInMB, int minLogSizeInMB)
        {
            int throttleThresholdInMBFromCheckpointInMB = checkpointThresholdInMB * throttleThresholdFactor;
            int throttleThresholdInMBFromMinLogSizeInMB = minLogSizeInMB * throttleThresholdFactor;

            return Math.Max(throttleThresholdInMBFromCheckpointInMB, throttleThresholdInMBFromMinLogSizeInMB);
        }
    }
}