// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;

    internal static class ReplicatorSettings
    {
        internal static bool ShouldValidateV1SpecificSettings(string SectionName)
        {
            return (SectionName != FabricValidatorConstants.SectionNames.Replication.TransactionalReplicator);
        }

        internal static bool ShouldValidateGlobalSettings(string SectionName)
        {
            return (SectionName == FabricValidatorConstants.SectionNames.Replication.Default);
        }

        public static void Validate(
            string sectionName,
            Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection)
        {
            TimeSpan retryInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.RetryInterval].GetTimeSpanValue();
            TimeSpan batchAckInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.BatchAckInterval].GetTimeSpanValue();

            uint maxReplicationQueueSize = 0;
            uint initialReplicationQueueSize = 0;
            uint maxReplicationQueueMemorySize = 0;
            uint queueHealthWarningAtUsagePercent = 0;
            uint slowIdleRestartAtQueueUsagePercent = 0;
            uint slowActiveSecondaryRestartAtQueueUsagePercent = 0;
            double secondaryProgressRateDecayFactor = 0.0;

            // Check for deprecated settings that are not in the TransactionalReplicator
            if (ShouldValidateV1SpecificSettings(sectionName))
            {
                maxReplicationQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationQueueSize].GetValue<uint>();
                initialReplicationQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.InitialReplicationQueueSize].GetValue<uint>();
                maxReplicationQueueMemorySize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationQueueMemorySize].GetValue<uint>();
            }
            
            // Validate that global settings are only validated for "Replication" section
            if (ShouldValidateGlobalSettings(sectionName))
            {
                queueHealthWarningAtUsagePercent = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.QueueHealthWarningAtUsagePercent].GetValue<uint>();
                slowIdleRestartAtQueueUsagePercent = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.SlowIdleRestartAtQueueUsagePercent].GetValue<uint>();
                slowActiveSecondaryRestartAtQueueUsagePercent = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.SlowActiveSecondaryRestartAtQueueUsagePercent].GetValue<uint>();
                secondaryProgressRateDecayFactor = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.SecondaryProgressRateDecayFactor].GetValue<double>();
            }

            uint maxCopyQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxCopyQueueSize].GetValue<uint>();
            uint initialCopyQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.InitialCopyQueueSize].GetValue<uint>();

            uint maxReplicationMessageSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationMessageSize].GetValue<uint>();

            uint initialSecondaryReplicationQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.InitialSecondaryReplicationQueueSize].GetValue<uint>();
            uint maxSecondaryReplicationQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxSecondaryReplicationQueueSize].GetValue<uint>();
            uint maxSecondaryReplicationQueueMemorySize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxSecondaryReplicationQueueMemorySize].GetValue<uint>();

            uint initialPrimaryReplicationQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.InitialPrimaryReplicationQueueSize].GetValue<uint>();
            uint maxPrimaryReplicationQueueSize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxPrimaryReplicationQueueSize].GetValue<uint>();
            uint maxPrimaryReplicationQueueMemorySize = settingsForThisSection[FabricValidatorConstants.ParameterNames.Replicator.MaxPrimaryReplicationQueueMemorySize].GetValue<uint>();

            if (batchAckInterval >= retryInterval)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Replicator Settings Error: BatchAcknowledgementInterval = {1}, RetryInterval = {2};" +
                    Environment.NewLine +
                    "BatchAcknowledgementInterval should be greater than RetryInterval",
                    sectionName,
                    retryInterval,
                    batchAckInterval));
            }

            if (ShouldValidateV1SpecificSettings(sectionName))
            {
                if (maxReplicationQueueMemorySize == 0 &&
                    maxReplicationQueueSize == 0)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- Replication Queue Size Settings Error: Both MaxReplicationQueueMemorySize and MaxReplicationQueueSize cannot be 0",
                        sectionName));
                }
            }

            if (maxPrimaryReplicationQueueMemorySize == 0 &&
                maxPrimaryReplicationQueueSize == 0)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Primary Replication Queue Size Settings Error: Both MaxPrimaryReplicationQueueMemorySize and MaxPrimaryReplicationQueueSize cannot be 0",
                    sectionName));
            }

            if (maxSecondaryReplicationQueueMemorySize == 0 &&
                maxSecondaryReplicationQueueSize == 0)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Secondary Replication Queue Size Settings Error: Both MaxSecondaryReplicationQueueMemorySize and MaxSecondaryReplicationQueueSize cannot be 0",
                    sectionName));
            }

            if (maxPrimaryReplicationQueueMemorySize > 0 && maxReplicationMessageSize > maxPrimaryReplicationQueueMemorySize)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Primary Replication Queue Size and Message Size Settings Error: MaxReplicationMessageSize cannot be greater than maxPrimaryReplicationQueueMemorySize ",
                    sectionName));
            }

            if (ShouldValidateV1SpecificSettings(sectionName))
            {
                if ((maxReplicationQueueSize > 0 && initialReplicationQueueSize > maxReplicationQueueSize) ||
                    initialReplicationQueueSize <= 1 ||
                    !IsPowerOf2(initialReplicationQueueSize) ||
                    (maxReplicationQueueSize > 0 && !IsPowerOf2(maxReplicationQueueSize)))
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- Replication Queue Size Settings Error: InitialQueueSize = {1}, MaxQueueSize = {2};" +
                        Environment.NewLine +
                        "InitialQueuesize must be less than or equal to MaxQueuesize; " +
                        Environment.NewLine +
                        "InitialQueuesize must be greater than 1" +
                        Environment.NewLine +
                        "InitialQueuesize and MaxQueuesize must be power of 2; ",
                        sectionName,
                        initialSecondaryReplicationQueueSize,
                        maxSecondaryReplicationQueueSize));
                }
            }

            if ((maxSecondaryReplicationQueueSize > 0 && initialSecondaryReplicationQueueSize > maxSecondaryReplicationQueueSize) ||
                initialSecondaryReplicationQueueSize <= 1 ||
                !IsPowerOf2(initialSecondaryReplicationQueueSize) ||
                (maxSecondaryReplicationQueueSize > 0 && !IsPowerOf2(maxSecondaryReplicationQueueSize)))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Secondary Replication Queue Size Settings Error: InitialSecondaryQueueSize = {1}, MaxSecondaryQueueSize = {2};" +
                    Environment.NewLine +
                    "InitialSecondaryQueuesize must be less than or equal to MaxSecondaryQueuesize; " +
                    Environment.NewLine +
                    "InitialSecondaryQueuesize must be greater than 1" +
                    Environment.NewLine +
                    "InitialSecondaryQueuesize and MaxSecondaryQueuesize must be power of 2; ",
                    sectionName,
                    initialSecondaryReplicationQueueSize,
                    maxSecondaryReplicationQueueSize));
            }

            if ((maxPrimaryReplicationQueueSize > 0 && initialPrimaryReplicationQueueSize > maxPrimaryReplicationQueueSize) ||
                initialPrimaryReplicationQueueSize <= 1 ||
                !IsPowerOf2(initialPrimaryReplicationQueueSize) ||
                (maxPrimaryReplicationQueueSize > 0 && !IsPowerOf2(maxPrimaryReplicationQueueSize)))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Primary Replication Queue Size Settings Error: InitialPrimaryQueueSize = {1}, MaxPrimaryQueueSize = {2};" +
                    Environment.NewLine +
                    "InitialPrimaryQueueSize must be less than or equal to MaxPrimaryQueueSize; " +
                    Environment.NewLine +
                    "InitialPrimaryQueueSize must be greater than 1" +
                    Environment.NewLine +
                    "InitialPrimaryQueueSize and MaxPrimaryQueueSize must be power of 2; ",
                    sectionName,
                    initialPrimaryReplicationQueueSize,
                    maxPrimaryReplicationQueueSize));
            }

            if (initialCopyQueueSize > maxCopyQueueSize ||
                initialCopyQueueSize <= 1 ||
                !IsPowerOf2(initialCopyQueueSize) ||
                !IsPowerOf2(maxCopyQueueSize))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Copy Queue Size Settings Error: InitialQueueSize = {1}, MaxQueueSize = {2};" +
                    Environment.NewLine +
                    "InitialQueuesize must be less than or equal to MaxQueuesize; " +
                    Environment.NewLine +
                    "InitialQueuesize must be greater than 1" +
                    Environment.NewLine +
                    "InitialQueuesize and MaxQueuesize must be power of 2; ",
                    sectionName,
                    initialCopyQueueSize,
                    maxCopyQueueSize));
            }

            if (ShouldValidateV1SpecificSettings(sectionName))
            {
                if (maxReplicationQueueMemorySize > 0 &&
                    maxReplicationMessageSize > maxReplicationQueueMemorySize)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- Replication Queue Size and Message Size Settings Error: MaxReplicationMessageSize cannot be greater than MaxReplicationQueueMemorySize",
                        sectionName));
                }
            }

            if (maxSecondaryReplicationQueueMemorySize > 0 &&
                maxReplicationMessageSize > maxSecondaryReplicationQueueMemorySize)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- Secondary Replication Queue Size and Message Size Settings Error: MaxReplicationMessageSize cannot be greater than MaxSecondaryReplicationQueueMemorySize",
                    sectionName));
            }

            if (ShouldValidateGlobalSettings(sectionName))
            {
                if (queueHealthWarningAtUsagePercent < 1 ||
                    queueHealthWarningAtUsagePercent > 100)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- QueueHealthWarningAtUsagePercent Setting Error: Value must be greater than 1 and less than 100. It is {1}",
                        sectionName,
                        queueHealthWarningAtUsagePercent));
                }

                if (slowIdleRestartAtQueueUsagePercent < 1 ||
                    slowIdleRestartAtQueueUsagePercent > 100)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SlowIdleRestartAtQueueUsagePercent Setting Error: Value must be greater than 1 and less than 100. It is {1}",
                        sectionName,
                        slowIdleRestartAtQueueUsagePercent));
                }

                if (slowActiveSecondaryRestartAtQueueUsagePercent < 1 ||
                    slowActiveSecondaryRestartAtQueueUsagePercent > 100)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SlowActiveSecondaryRestartAtQueueUsagePercent Setting Error: Value must be greater than 1 and less than 100. It is {1}",
                        sectionName,
                        slowActiveSecondaryRestartAtQueueUsagePercent));
                }

                if (secondaryProgressRateDecayFactor < 0.0 ||
                    secondaryProgressRateDecayFactor > 1.0)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SecondaryProgressRateDecayFactor Setting Error: Value must be greater than 0.0 and less than 1.0. It is {1}",
                        sectionName,
                        secondaryProgressRateDecayFactor));
                }
            }
        }

        private static bool IsPowerOf2(uint x)
        {
            return (x & (x - 1)) == 0;
        }
    }

    internal static class TransactionalReplicatorSettings
    {
        private const int DefaultMinLogDivider = 2;
        private const int DefaultSparseLogMaxStreamSize = 200 * 1024;
        private const int SmallestMinLogSizeInMB = 1;

        // Validate settings exclusive to the managed V2 replicator
        public static void ValidateManagedSettings(string sectionName,
            Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection)
        {
            string sharedLogId = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogId].GetValue<string>();
            string sharedLogPath = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogPath].GetValue<string>();

            if (!string.IsNullOrEmpty(sharedLogId))
            {
                bool formatOk = true;
                try
                {
                    Guid g = Guid.Parse(sharedLogId);
                }
                catch (FormatException)
                {
                    formatOk = false;
                }

                if (!formatOk)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SharedLogId should be a well-formed guid such as {F67D29E7-DFAB-437e-9F9F-5AC901D3F0BC}",
                        sectionName));
                }

                if (sharedLogId == FabricValidatorConstants.ParameterNames.TransactionalReplicator.DefaultSharedLogContainerGuid)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SharedLogId should not be the default guid {3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}",
                        sectionName));
                }

            }

            if (!string.IsNullOrEmpty(sharedLogPath))
            {
                bool isAbsolutePath;

                isAbsolutePath = System.IO.Path.IsPathRooted(sharedLogPath);

                if (!isAbsolutePath)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Section '{0}'- SharedLogPath should be a well-formed absolute pathname",
                        sectionName));
                }
            }

            if ((string.IsNullOrEmpty(sharedLogId)) && (!string.IsNullOrEmpty(sharedLogPath)))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- if shared log id is empty then shared log path should be empty",
                    sectionName));
            }

            if ((!string.IsNullOrEmpty(sharedLogId)) && (string.IsNullOrEmpty(sharedLogPath)))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- if shared log id is not empty then shared log path should be set",
                    sectionName));
            }

            int logTruncationIntervalSeconds = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.LogTruncationIntervalSeconds].GetValue<int>();

            if (logTruncationIntervalSeconds < 0)
            {
                throw new ArgumentException("LogTruncationIntervalSeconds must be greater than 0");
            }
        }

        // Validate settings exclusive to the Native Transactional Replicator
        public static void ValidateNativeSettings(string sectionName,
            Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection)
        {
            // Transacation Replicator setting validation
            int serializationVersion = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion].GetValue<int>();
            if (serializationVersion > 1 || serializationVersion < 0)
            {
                throw new ArgumentException(
                    string.Format(
                    "SerializationVersion = {0}. SerializationVersion must be either 0 or 1.",
                    serializationVersion));
            }
        }

        // Validate settings exclusive to the managed V2 replicator and the Native Transactional Replicator
        public static void ValidateCommonSettings(string sectionName,
            Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection)
        {
            // Replicator setting validation
            int checkpointThresholdInMB = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB].GetValue<int>();
            int maxAccumulatedBackupLogSizeInMB = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB].GetValue<int>();
            int minLogSizeInMB = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB].GetValue<int>();
            int truncationThresholdFactor = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor].GetValue<int>();
            int throttlingThresholdFactor = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor].GetValue<int>();

            //
            // Log settings validation
            //
            int maxStreamSizeInMB = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB].GetValue<int>();
            int maxMetadataSizeInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxMetadataSizeInKB].GetValue<int>();
            int maxRecordSizeInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB].GetValue<int>();
            int maxWriteQueueDepthInKB = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxWriteQueueDepthInKB].GetValue<int>();

            bool OptimizeLogForLowerDiskUsage = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage].GetValue<bool>();
            TimeSpan slowApiMonitoringDuration = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.SlowApiMonitoringDuration].GetValue<TimeSpan>();

            //
            // Internal settings validation
            //
            string test_LoggingEngine = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LoggingEngine].GetValue<string>();
            int test_LogMinDelayIntervalMilliseconds = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogMinDelayIntervalMilliseconds].GetValue<int>();
            int test_LogMaxDelayIntervalMilliseconds = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogMaxDelayIntervalMilliseconds].GetValue<int>();
            double test_LogDelayRatio = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogDelayRatio].GetValue<double>();
            double test_LogDelayProcessExitRatio = settingsForThisSection[FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogDelayProcessExitRatio].GetValue<double>();

            if (!((maxWriteQueueDepthInKB == 0) || (IsMultipleOf4(maxWriteQueueDepthInKB) && maxWriteQueueDepthInKB > 0)))
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- MaxWriteQueueDepthInKB must be zero or a positive multiple of 4",
                    sectionName));
            }

            if (!IsMultipleOf4(maxMetadataSizeInKB) || maxMetadataSizeInKB < 4)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- MaxMetadataSizeInKB must be a positive multiple of 4",
                    sectionName));
            }

            if (!IsMultipleOf4(maxRecordSizeInKB) || maxRecordSizeInKB < 1)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- MaxRecordSizeInKB must be a positive multiple of 4",
                    sectionName));
            }

            if (maxRecordSizeInKB < 128)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- MaxRecordSizeInKB must be greater than or equal to 128",
                    sectionName));
            }

            if (OptimizeLogForLowerDiskUsage == false)
            {
                int logSizeinKB = maxStreamSizeInMB * 1024;
                if (logSizeinKB < 16 * maxRecordSizeInKB)
                {
                    throw new ArgumentException(
                        string.Format(
                            "Section '{0}'- MaxStreamSizeInMB * 1024 must be larger or equal to MaxRecordSizeInKB * 16",
                            sectionName));
                }
            }

            if (maxStreamSizeInMB < 1)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- MaxStreamSizeInMb must be greater than 0. It is {1}",
                    sectionName,
                    maxStreamSizeInMB));
            }

            if (checkpointThresholdInMB < 1)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- CheckpointThresholdInMB must be greater than 0. It is {1}",
                    sectionName,
                    checkpointThresholdInMB));
            }

            if (OptimizeLogForLowerDiskUsage == false &&
                maxStreamSizeInMB < checkpointThresholdInMB)
            {
                throw new ArgumentException(
                    string.Format(
                    "Section '{0}'- MaxStreamSizeInMB: {1} must be greater than CheckpointThresholdInMB: {2} when OptimizeLogForLowerDiskUsage is disabled",
                    sectionName,
                    maxStreamSizeInMB,
                    checkpointThresholdInMB));
            }

            if (maxAccumulatedBackupLogSizeInMB < 1)
            {
                throw new ArgumentOutOfRangeException(
                    string.Format("Section '{0}' - MaxAccumulatedBackupLogSizeInMB: {1} must be greated than 0", sectionName, maxAccumulatedBackupLogSizeInMB));
            }

            if (maxAccumulatedBackupLogSizeInMB >= maxStreamSizeInMB)
            {
                throw new ArgumentOutOfRangeException(
                    string.Format("Section '{0}' - MaxAccumulatedBackupLogSizeInMB: {1} must be less than MaxStreamSizeInMB {2}", sectionName, maxAccumulatedBackupLogSizeInMB, maxStreamSizeInMB));
            }

            if (slowApiMonitoringDuration < TimeSpan.Zero ||
                slowApiMonitoringDuration.Equals(TimeSpan.MaxValue))
            {
                throw new ArgumentException(
                    string.Format(
                    "SlowApiMonitoringDuration = {0}. SlowApiMonitoring must be greater than zero and less than TimeSpan.MaxValue. A value of zero disables api monitoring.",
                    slowApiMonitoringDuration));
            }

            minLogSizeInMB = GetMinLogSizeInMB(
                sectionName,
                minLogSizeInMB,
                checkpointThresholdInMB);

            int throttlingThresholdInMB = GetThrottleThresholdInMB(throttlingThresholdFactor, checkpointThresholdInMB, minLogSizeInMB);

            ValidateMinLogSize(
                sectionName,
                minLogSizeInMB,
                OptimizeLogForLowerDiskUsage,
                maxStreamSizeInMB,
                SmallestMinLogSizeInMB,
                DefaultSparseLogMaxStreamSize);

            ValidateTruncationAndThrottlingThreshold(
                sectionName,
                truncationThresholdFactor,
                minLogSizeInMB,
                checkpointThresholdInMB,
                throttlingThresholdInMB,
                OptimizeLogForLowerDiskUsage,
                maxStreamSizeInMB,
                DefaultSparseLogMaxStreamSize);

            ValidateInternalSettings(
                test_LogMinDelayIntervalMilliseconds,
                test_LogMaxDelayIntervalMilliseconds,
                test_LogDelayRatio,
                test_LogDelayProcessExitRatio,
                GetLoggingEngine(test_LoggingEngine));
        }

        private static void ValidateTruncationAndThrottlingThreshold(
            string sectionName,
            int truncationThresholdFactor,
            int minLogSizeInMB,
            int checkpointThresholdInMB,
            int throttlingThresholdInMB,
            bool isOptimizeLogForLowerDiskUsage,
            int maxStreamSizeInMB,
            int sparseLogMaxStreamSize)
        {
            if (truncationThresholdFactor <= 1)
            {
                string message = string.Format(
                    "Section '{0}' - TruncationThresholdFactor: {1} must be greater than 1",
                    sectionName,
                    truncationThresholdFactor);
                throw new ArgumentOutOfRangeException(message);
            }

            int truncationThresholdInMB = truncationThresholdFactor * minLogSizeInMB;
            if (isOptimizeLogForLowerDiskUsage == false)
            {
                if (truncationThresholdInMB >= maxStreamSizeInMB)
                {
                    string message =
                        string.Format(
                            "Section '{0}' - MinLogSizeInMB: MinLogSizeInMB ({1}) * TruncationThresholdFactor ({2}) must be less than MaxStreamSizeInMB ({3})",
                            sectionName,
                            minLogSizeInMB,
                            truncationThresholdFactor,
                            maxStreamSizeInMB);
                    throw new ArgumentOutOfRangeException(message);
                }
            }
            else
            {
                if (truncationThresholdInMB >= sparseLogMaxStreamSize)
                {
                    string message = string.Format(
                        "Section '{0}' - MinLogSizeInMB: {1} must be less than {2}",
                        sectionName,
                        minLogSizeInMB,
                        sparseLogMaxStreamSize);
                    throw new ArgumentOutOfRangeException(message);
                }
            }

            if (isOptimizeLogForLowerDiskUsage == false)
            {
                if (throttlingThresholdInMB >= maxStreamSizeInMB)
                {
                    string message =
                        string.Format(
                            "Section '{0}' - ThrottlingThresholdFactor: ThrottleThresholdInMB {1} must be less than MaxStreamSizeInMB ({2})",
                            sectionName,
                            throttlingThresholdInMB,
                            maxStreamSizeInMB);
                    throw new ArgumentOutOfRangeException(message);
                }
            }
            else
            {
                if (throttlingThresholdInMB >= sparseLogMaxStreamSize)
                {
                    string message = string.Format(
                        "Section '{0}' - MinLogSizeInMB: {1} must be less than {2}",
                        sectionName,
                        minLogSizeInMB,
                        sparseLogMaxStreamSize);
                    throw new ArgumentOutOfRangeException(message);
                }
            }

            if (throttlingThresholdInMB <= truncationThresholdInMB)
            {
                string message = string.Format(
                    "Section '{0}' - ThrottlingThresholdInMB: {1} must be less than TruncationThresholdInMB {2}",
                    sectionName,
                    throttlingThresholdInMB,
                    truncationThresholdInMB);
                throw new ArgumentOutOfRangeException(message);
            }

            if (throttlingThresholdInMB <= checkpointThresholdInMB)
            {
                string message = string.Format(
                    "Section '{0}' - ThrottlingThresholdInMB: {1} must be less than CheckpointThresholdInMb {2}",
                    sectionName,
                    throttlingThresholdInMB,
                    checkpointThresholdInMB);
                throw new ArgumentOutOfRangeException(message);
            }
        }

        /// <summary>
        /// Validate settings intended for internal use only
        /// </summary>
        /// <param name="rawLoggingEngine"></param>
        /// <param name="testFileLogMinDelayInMS"></param>
        /// <param name="testFileLogMaxDelayInMs"></param>
        /// <param name="testFileLogDelayRatio"></param>
        private static void ValidateInternalSettings(
            int testLogMinDelayInMS,
            int testLogMaxDelayInMs,
            double testLogDelayRatio,
            double testLogDelayProcessExitRatio,
            string loggingEngine)
        {
            if (!loggingEngine.Equals("ktl") && !loggingEngine.Equals("file") && !loggingEngine.Equals("memory"))
            {
                throw new ArgumentException(string.Format("Test_LoggingEngine must be one of ktl/memory/file - provided {0}", loggingEngine));
            }

            if (testLogMinDelayInMS != 0)
            {
                if (testLogMaxDelayInMs < testLogMinDelayInMS ||
                    testLogMinDelayInMS < 0)
                {
                    throw new ArgumentOutOfRangeException(
                        "Test_LogMaxDelayIntervalMilliseconds must be non-zero and greater than Test_LogMinDelayIntervalMilliseconds. Test_LoggingEngine must be set to file");
                }
            }

            if (testLogMaxDelayInMs != 0)
            {
                if (testLogMinDelayInMS > testLogMaxDelayInMs ||
                    testLogMaxDelayInMs < 0)
                {
                    throw new ArgumentOutOfRangeException(
                        "Test_LogMaxDelayIntervalMilliseconds must be non-zero and greater than Test_LogMinDelayIntervalMilliseconds. Test_LoggingEngine must be set to file");
                }
            }

            if (testLogDelayRatio != 0)
            {
                if (Math.Abs(testLogDelayRatio) > 1 ||
                    testLogDelayRatio < 0)
                {
                    throw new ArgumentException("Test_LogDelayRatio must be greater than zero and less than 1");
                }
            }

            if (testLogDelayProcessExitRatio != 0)
            {
                if (Math.Abs(testLogDelayProcessExitRatio) > 1 ||
                    testLogDelayProcessExitRatio < 0)
                {
                    throw new ArgumentException("test_LogDelayProcessExitRatio must be greater than zero and less than 1");
                }
            }
        }

        private static void ValidateMinLogSize(
            string sectionName,
            int minLogSizeInMB,
            bool isOptimizeLogForLowerDiskUsage,
            int maxStreamSizeInMB,
            int smallestMinLogSizeInMB,
            int sparseLogMaxStreamSize)
        {
            // Cannot be 0, size GetMinLogSizeInMB sets the correct default if it is 0.
            if (minLogSizeInMB < smallestMinLogSizeInMB)
            {
                string message = string.Format(
                        "Section '{0}' - MinLogSizeInMB: {1} must be greater than 0",
                        sectionName,
                        minLogSizeInMB);
                throw new ArgumentOutOfRangeException(message);
            }

            if (isOptimizeLogForLowerDiskUsage == false)
            {
                if (minLogSizeInMB >= maxStreamSizeInMB)
                {
                    string message = string.Format(
                            "Section '{0}' - MinLogSizeInMB: {1} must be less than MaxStreamSizeInMB {2}",
                            sectionName,
                            minLogSizeInMB,
                            maxStreamSizeInMB);
                    throw new ArgumentOutOfRangeException(message);
                }
            }
            else
            {
                if (minLogSizeInMB >= sparseLogMaxStreamSize)
                {
                    string message = string.Format(
                        "Section '{0}' - MinLogSizeInMB: {1} must be less than {2}",
                        sectionName,
                        minLogSizeInMB,
                        sparseLogMaxStreamSize);
                    throw new ArgumentOutOfRangeException(message);
                }
            }
        }

        /// <summary>
        /// Gets the min log size in MB.
        /// 0 MinLogSizeInMB indicates that CheckpointThresholdInMB / DefaultMinLogDivider needs to be used.
        /// </summary>
        /// <param name="sectionName">Section being validated</param>
        /// <param name="minLogSizeInMB">Minimum log size.</param>
        /// <param name="checkpointThresholdInMB">Checkpoint Threshold In MB</param>
        /// <returns></returns>
        private static int GetMinLogSizeInMB(
            string sectionName,
            int minLogSizeInMB, 
            int checkpointThresholdInMB)
        {
            if (minLogSizeInMB < 0)
            {
                throw new ArgumentOutOfRangeException(string.Format("Section '{0}' - MinLogSizeInMB: {1} must be greater than or equal to 0", sectionName, minLogSizeInMB));
            }

            if (checkpointThresholdInMB <= 0)
            {
                throw new InvalidOperationException(string.Format("CheckpointThresholdInMB: {0} must be greater than 0", checkpointThresholdInMB));
            }

            if (minLogSizeInMB == 0)
            {
                var defaultSize = checkpointThresholdInMB / DefaultMinLogDivider;
                if (defaultSize < SmallestMinLogSizeInMB)
                {
                    return SmallestMinLogSizeInMB;
                }

                return defaultSize;
            }

            return minLogSizeInMB;
        }

        private static int GetThrottleThresholdInMB(
            int throttleThreholdFactor,
            int checkpointThresholdInMB,
            int minLogSizeInMB)
        {
            int throttleThresholdInMBFromCheckpointInMB = checkpointThresholdInMB * throttleThreholdFactor;
            int throttleThresholdInMBFromMinLogSizeInMB = minLogSizeInMB * throttleThreholdFactor;

            return Math.Max(throttleThresholdInMBFromCheckpointInMB, throttleThresholdInMBFromMinLogSizeInMB);
        }

        private static bool IsMultipleOf4(int value)
        {
            return ((value % 4) == 0);
        }

        /// <summary>
        /// Remove escape characters from logging engine string.
        /// First Replace statement removes backslashes.
        /// Second Replace statement removes extraneous quotation marks.
        /// </summary>
        /// <param name="rawLoggingEngineString"></param>
        /// <returns></returns>
        private static string GetLoggingEngine(string rawLoggingEngineString)
        {
            return rawLoggingEngineString.Replace(@"\""", "").Replace("\"", "");
        }
    }

    /// <summary>
    /// Configuration validator for the TransactionalReplicator2 section
    /// Corresponds to the native Transactional Replicator
    /// </summary>
    class NativeTransactionalReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.TransactionalReplicator2.Default; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            TransactionalReplicatorSettings.ValidateNativeSettings(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateCommonSettings(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }

    /// <summary>
    /// Configuration validator for the ClusterManager/TransactionalReplicator2 section
    /// </summary>
    class ClusterManagerNativeTransactionalReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.TransactionalReplicator2.ClusterManager; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            TransactionalReplicatorSettings.ValidateNativeSettings(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateCommonSettings(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }

    /// <summary>
    /// Configuration validator for the FileStoreService/TransactionalReplicator2 section
    /// </summary>
    class FileStoreServiceNativeTransactionalReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.TransactionalReplicator2.FileStoreService; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            TransactionalReplicatorSettings.ValidateNativeSettings(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateCommonSettings(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }

    /// <summary>
    /// Configuration validator for the Naming/TransactionalReplicator2 section
    /// </summary>
    class NamingNativeTransactionalReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.TransactionalReplicator2.Naming; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            TransactionalReplicatorSettings.ValidateNativeSettings(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateCommonSettings(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }

    /// <summary>
    /// Configuration validator for the Failover/TransactionalReplicator2 section
    /// </summary>
    class FailoverNativeTransactionalReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.TransactionalReplicator2.Failover; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            TransactionalReplicatorSettings.ValidateNativeSettings(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateCommonSettings(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }

    /// <summary>
    /// Configuration validator for the RepairManager/TransactionalReplicator2 section
    /// </summary>
    class RepairManagerNativeTransactionalReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.TransactionalReplicator2.RepairManager; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            TransactionalReplicatorSettings.ValidateNativeSettings(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateCommonSettings(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }

    /// <summary>
    /// Configuration validator for the TransactionalReplicator section
    /// Corresponds to the V2 Replicator in managed code
    /// </summary>
    class TransactionalReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Replication.TransactionalReplicator; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ReplicatorSettings.Validate(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateManagedSettings(SectionName, settingsForThisSection);
            TransactionalReplicatorSettings.ValidateCommonSettings(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the Replication section.
    /// </summary>
    class ReplicatorSettingsValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Replication.Default; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ReplicatorSettings.Validate(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the ClusterManager Replication section.
    /// </summary>
    class ReplicatorSettingsValidatorClusterManager : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Replication.ClusterManager; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ReplicatorSettings.Validate(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the FileStoreService Replication section.
    /// </summary>
    class ReplicatorSettingsValidatorFileStoreService : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Replication.FileStoreService; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ReplicatorSettings.Validate(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
 
    /// <summary>
    /// This is the class handles the validation of configuration for the Naming Replication section.
    /// </summary>
    class ReplicatorSettingsValidatorNaming : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Replication.Naming; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ReplicatorSettings.Validate(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the Failover Replication section.
    /// </summary>
    class ReplicatorSettingsValidatorFailover : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Replication.Failover; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ReplicatorSettings.Validate(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the Failover Replication section.
    /// </summary>
    class ReplicatorSettingsValidatorRepairManager : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.Replication.RepairManager; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            ReplicatorSettings.Validate(SectionName, settingsForThisSection);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

}