// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    internal class ConfigReader
    {
        private const string ConfigKeyPrefix = "Autopilot.";

        private readonly IConfigSection configSection;

        public ConfigReader(IConfigSection configSection)
        {
            this.configSection = configSection.Validate("configSection");
        }

        public TimeSpan JobPollingInterval
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("JobPollingIntervalInSeconds", 1 * 60));
            }
        }

        /// <summary>
        /// The amount of time to observe coordinator errors before reporting a health warning.
        /// <seealso cref="CoordinatorFailureRetryDuration"/>
        /// </summary>
        public TimeSpan CoordinatorFailureWarningThreshold
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("CoordinatorFailureWarningThresholdInSeconds", 3 * 60));
            }
        }

        /// <summary>
        /// Max. amount of time to retry on coordinator errors beyond which the service fails over.
        /// </summary>
        public TimeSpan CoordinatorFailureRetryDuration
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("CoordinatorFailureRetryDurationInSeconds", 15 * 60));
            }
        }

        public TimeSpan CoordinatorStatusHealthEventTTL
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("CoordinatorStatusHealthEventTTLInSeconds", 30 * 60));
            }
        }

        public TimeSpan ExtendRepairDelayValue
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("ExtendRepairDelayValueInSeconds", 72 * 60 * 60));
            }
        }

        public TimeSpan ExtendRepairDelayThreshold
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("ExtendRepairDelayThresholdInSeconds", 71 * 60 * 60));
            }
        }

        public TimeSpan ApprovedNonDestructiveRepairDelayValue
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("ApprovedNonDestructiveRepairDelayValueInSeconds", 5 * 60));
            }
        }

        public TimeSpan ApprovedDestructiveRepairDelayValue
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("ApprovedDestructiveRepairDelayValueInSeconds", 0 * 60));
            }
        }

        public TimeSpan OverdueRepairTaskPreparingThreshold
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("OverdueRepairTaskPreparingThresholdInSeconds", 30 * 60));
            }
        }

        public TimeSpan OverdueRepairTaskRestoringThreshold
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("OverdueRepairTaskRestoringThresholdInSeconds", 5 * 60));
            }
        }

        public TimeSpan PostApprovalExecutionDelay
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("PostApprovalExecutionDelayInSeconds", 1 * 60));
            }
        }

        public TimeSpan MinimumRepairExecutionTime
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("MinimumRepairExecutionTimeInSeconds", 5 * 60));
            }
        }

        public bool EnableNodeHealthReports
        {
            get
            {
                return this.ReadConfigValue("EnableNodeHealthReports", true);
            }
        }

        public TimeSpan NodeHealthEventTTL
        {
            get
            {
                return TimeSpan.FromSeconds(this.ReadConfigValue("NodeHealthEventTTLInSeconds", 5 * 60));
            }
        }

        public bool AutoApprove
        {
            get
            {
                return this.ReadConfigValue("AutoApprove", false);
            }
        }

        public double CoordinatorDutyCycleWarningThreshold
        {
            get
            {
                return Math.Max(0.0, Math.Min(1.0, this.ReadConfigValue("CoordinatorDutyCycleWarningThreshold", 0.5)));
            }
        }

        public string AllowRepairTaskCompletionInMachineStates
        {
            get
            {
                return this.ReadConfigValue("AllowRepairTaskCompletionInMachineStates", "HP");
            }
        }

        public bool LimitRepairTaskImpactLevel
        {
            get
            {
                return this.ReadConfigValue("LimitRepairTaskImpactLevel", false);
            }
        }

        private T ReadConfigValue<T>(
            string keyName,
            T defaultValue = default(T))
        {
            return this.configSection.ReadConfigValue<T>(
                ConfigKeyPrefix + keyName,
                defaultValue);
        }
    }
}