// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;

    /// <summary>
    /// This is the class handles the validation of configuration for the MetricActivityThresholds section.
    /// </summary>
    ///

    class PlacementAndLoadBalancingSettingsConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);

            int yieldDurationPer10ms = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.YieldDurationPer10ms].GetValue<int>();

            double SwapPrimaryProbability = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.SwapPrimaryProbability].GetValue<double>();
            double LoadDecayFactor = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.LoadDecayFactor].GetValue<double>();
            double AffinityMoveParentReplicaProbability = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.AffinityMoveParentReplicaProbability].GetValue<double>();

            double PLBRefreshInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.PLBRefreshInterval].GetValue<double>();
            double MinLoadBalancingInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MinLoadBalancingInterval].GetValue<double>();
            double LocalBalancingThreshold = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.LocalBalancingThreshold].GetValue<double>();
            double LocalDomainWeight = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.LocalDomainWeight].GetValue<double>();
            double MinPlacementInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MinPlacementInterval].GetValue<double>();
            double MinConstraintCheckInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MinConstraintCheckInterval].GetValue<double>();
            double PLBRefreshGap = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.PLBRefreshGap].GetValue<double>();
            double BalancingDelayAfterNewNode = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.BalancingDelayAfterNewNode].GetValue<double>();
            double BalancingDelayAfterNodeDown = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.BalancingDelayAfterNodeDown].GetValue<double>();
            double GlobalMovementThrottleCountingInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.GlobalMovementThrottleCountingInterval].GetValue<double>();
            double MovementPerPartitionThrottleCountingInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MovementPerPartitionThrottleCountingInterval].GetValue<double>();
            double PlacementSearchTimeout = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.PlacementSearchTimeout].GetValue<double>();
            double ConstraintCheckSearchTimeout = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.ConstraintCheckSearchTimeout].GetValue<double>();
            double FastBalancingSearchTimeout = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.FastBalancingSearchTimeout].GetValue<double>();
            double SlowBalancingSearchTimeout = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.SlowBalancingSearchTimeout].GetValue<double>();
            bool UseScoreInConstraintCheck = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.UseScoreInConstraintCheck].GetValue<bool>();
            double MaxPercentageToMove = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MaxPercentageToMove].GetValue<double>();
            double MaxPercentageToMoveForPlacement = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MaxPercentageToMoveForPlacement].GetValue<double>();
            double MoveParentToFixAffinityViolationTransitionPercentage =
                   settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MoveParentToFixAffinityViolationTransitionPercentage].GetValue<double>();

            int PlacementConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.PlacementConstraintPriority].GetValue<int>();
            int PreferredLocationConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.PreferredLocationConstraintPriority].GetValue<int>();
            int ScaleoutCountConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.ScaleoutCountConstraintPriority].GetValue<int>();
            int ApplicationCapacityConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.ApplicationCapacityConstraintPriority].GetValue<int>();

            int CapacityConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.CapacityConstraintPriority].GetValue<int>();
            int AffinityConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.AffinityConstraintPriority].GetValue<int>();
            int FaultDomainConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.FaultDomainConstraintPriority].GetValue<int>();
            int UpgradeDomainConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.UpgradeDomainConstraintPriority].GetValue<int>();
            int ThrottlingConstraintPriority = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.ThrottlingConstraintPriority].GetValue<int>();

            bool isPlacementTimerDefined = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MinPlacementInterval].IsFromClusterManifest;
            bool isCCTimerDefined = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MinConstraintCheckInterval].IsFromClusterManifest;
            bool isRefreshTimerDefined = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.PLBRefreshInterval].IsFromClusterManifest;
            bool MoveParentToFixAffinityViolation = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.MoveParentToFixAffinityViolation].GetValue<bool>();

            bool QuorumBasedLogicAutoSwitch = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.QuorumBasedLogicAutoSwitch].GetValue<bool>();

            double StatisticsTracingInterval = settingsForThisSection[FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval].GetValue<double>();

            if (isRefreshTimerDefined && (isPlacementTimerDefined || isCCTimerDefined))
            {
                throw (new ArgumentException(StringResources.Error_Fabric_Validator_LoadBalancingMultipleTimersError));
            }

            if (MinPlacementInterval < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.MinPlacementInterval, MinPlacementInterval, 0.0));
            }

            if (MinConstraintCheckInterval < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.MinConstraintCheckInterval, MinConstraintCheckInterval, 0.0));
            }

            if (PLBRefreshGap < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.PLBRefreshGap, PLBRefreshGap, 0.0));
            }

            if (BalancingDelayAfterNewNode < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.BalancingDelayAfterNewNode, BalancingDelayAfterNewNode, 0.0));
            }

            if (BalancingDelayAfterNodeDown < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.BalancingDelayAfterNodeDown, BalancingDelayAfterNodeDown, 0.0));
            }

            if (StatisticsTracingInterval < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThanOrEqualTo, FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval, StatisticsTracingInterval, 0.0));
            }

            if (GlobalMovementThrottleCountingInterval < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.GlobalMovementThrottleCountingInterval, GlobalMovementThrottleCountingInterval, 0.0));
            }

            if (MovementPerPartitionThrottleCountingInterval < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.MovementPerPartitionThrottleCountingInterval, MovementPerPartitionThrottleCountingInterval, 0.0));
            }

            if (PlacementSearchTimeout < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.PlacementSearchTimeout, PlacementSearchTimeout, 0.0));
            }

            if (ConstraintCheckSearchTimeout < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.ConstraintCheckSearchTimeout, ConstraintCheckSearchTimeout, 0.0));
            }

            if (FastBalancingSearchTimeout < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.FastBalancingSearchTimeout, FastBalancingSearchTimeout, 0.0));
            }

            if (SlowBalancingSearchTimeout < 0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueGreaterThan, FabricValidatorConstants.ParameterNames.PLB.SlowBalancingSearchTimeout, SlowBalancingSearchTimeout, 0.0));
            }

            if (LocalBalancingThreshold != 0.0 && LocalBalancingThreshold < 1.0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_DoubleValueEqualToInputOrGreaterThanEqualToInput,
                    0.0, 1.0));
            }

            if (LocalDomainWeight < 0.0 || LocalDomainWeight > 1.0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueInRange, FabricValidatorConstants.ParameterNames.PLB.LocalDomainWeight, LocalDomainWeight, 0.0, 1.0));
            }

            if (MaxPercentageToMove < 0.0 || MaxPercentageToMove > 1.0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueInRange, FabricValidatorConstants.ParameterNames.PLB.MaxPercentageToMove, MaxPercentageToMove, 0.0, 1.0));
            }

            if (MaxPercentageToMoveForPlacement < 0.0 || MaxPercentageToMoveForPlacement > 1.0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueInRange, FabricValidatorConstants.ParameterNames.PLB.MaxPercentageToMoveForPlacement, MaxPercentageToMoveForPlacement, 0.0, 1.0));
            }

            if (yieldDurationPer10ms < 0 || yieldDurationPer10ms >= 10)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedIntValueInRangeExcludingRight,
                    FabricValidatorConstants.ParameterNames.PLB.YieldDurationPer10ms, yieldDurationPer10ms, 0, 10));
            }

            if ((SwapPrimaryProbability < 0.0) || (SwapPrimaryProbability > 1.0))
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueInRange,FabricValidatorConstants.ParameterNames.PLB.SwapPrimaryProbability, SwapPrimaryProbability, 0.0, 1.0));
            }

            if ((LoadDecayFactor < 0.0) || (LoadDecayFactor > 1.0))
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueInRange, FabricValidatorConstants.ParameterNames.PLB.LoadDecayFactor, LoadDecayFactor, 0.0, 1.0));
            }

            if ((AffinityMoveParentReplicaProbability < 0.0) || (AffinityMoveParentReplicaProbability > 1.0))
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueInRange, FabricValidatorConstants.ParameterNames.PLB.AffinityMoveParentReplicaProbability, AffinityMoveParentReplicaProbability, 0.0, 1.0));
            }

            if (PlacementConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.PlacementConstraintPriority + " = " + PlacementConstraintPriority, -1));
            }

            if (PreferredLocationConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.PreferredLocationConstraintPriority + " = " + PreferredLocationConstraintPriority, -1));
            }

            if (ScaleoutCountConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.ScaleoutCountConstraintPriority + " = " + ScaleoutCountConstraintPriority, -1));
            }

            if (ApplicationCapacityConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.ApplicationCapacityConstraintPriority + " = " + ApplicationCapacityConstraintPriority, -1));
            }

            if (CapacityConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.CapacityConstraintPriority + " = " + CapacityConstraintPriority, -1));
            }

            if (AffinityConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.AffinityConstraintPriority + " = " + AffinityConstraintPriority, -1));
            }

            if (FaultDomainConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.FaultDomainConstraintPriority + " = " + FaultDomainConstraintPriority, -1));
            }

            if (UpgradeDomainConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.UpgradeDomainConstraintPriority + " = " + UpgradeDomainConstraintPriority, -1));
            }

            if (ThrottlingConstraintPriority < -1)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_FirstIntParameterGreaterThanOrEqualToSecondIntParameter, FabricValidatorConstants.ParameterNames.PLB.ThrottlingConstraintPriority + " = " + ThrottlingConstraintPriority, -1));
            }

            if (MoveParentToFixAffinityViolationTransitionPercentage < 0.0 || MoveParentToFixAffinityViolationTransitionPercentage > 1.0)
            {
                throw new ArgumentException(string.Format(StringResources.Error_FabricValidator_NamedDoubleValueInRange, FabricValidatorConstants.ParameterNames.PLB.MoveParentToFixAffinityViolationTransitionPercentage, MoveParentToFixAffinityViolationTransitionPercentage, 0.0, 1.0));
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    class MetricActivityThresholdsConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.MetricActivityThresholds; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, key, 0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the MetricBalancingThresholds section.
    /// </summary>
    class MetricBalancingThresholdsConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.MetricBalancingThresholds; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueGreaterThanEqualToInput(settingsForThisSection, key, 1.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the GlobalMetricWeights section.
    /// </summary>
    class GlobalMetricWeightsConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.GlobalMetricWeights; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueGreaterThanEqualToInput(settingsForThisSection, key, 0.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the MaximumInBuildReplicasPerNode section.
    /// </summary>
    class MaximumInBuildReplicasPerNodeConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNode; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, key, 1, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the MaximumInBuildReplicasPerNodeBalancingThrottle section.
    /// </summary>
    class MaximumInBuildReplicasPerNodeBalancingThrottleConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeBalancingThrottle; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, key, 1, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the MaximumInBuildReplicasPerNodeConstraintCheckThrottle section.
    /// </summary>
    class MaximumInBuildReplicasPerNodeConstraintCheckThrottleConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeConstraintCheckThrottle; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, key, 1, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the MaximumInBuildReplicasPerNodePlacementThrottle section.
    /// </summary>
    class MaximumInBuildReplicasPerNodePlacementThrottleConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodePlacementThrottle; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, key, 1, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold section.
    /// </summary>
    class DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueGreaterThanEqualToInput(settingsForThisSection, key, 0.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the DefragmentationEmptyNodeDistributionPolicy section.
    /// </summary>
    class DefragmentationEmptyNodeDistributionPolicyConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueInRangeExcludingRight(settingsForThisSection, key, 0, 2, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the PlacementStrategy section.
    /// There are 3 placement strategies defined in file "Metric.h", with enum PlacementStrategy.
    /// If the number of strategies changes, it should also be updated here. 
    /// </summary>
    class PlacementStrategyConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.PlacementStrategy; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueInRangeExcludingRight(settingsForThisSection, key, 0, 5, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the PlacementHeuristicFactorPerMetric section.
    /// </summary>
    class PlacementHeuristicIncomingLoadFactorConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueGreaterThanEqualToInput(settingsForThisSection, key, 0.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the PlacementHeuristicEmptyLoadFactorPerMetric section.
    /// </summary>
    class PlacementHeuristicEmptySpacePercentConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueInRange(settingsForThisSection, key, 0.0, 1.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the DefragmentationMetrics section.
    /// </summary>
    class DefragmentationMetricsConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.DefragmentationMetrics; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                bool value = settingsForThisSection[key].GetValue<bool>();
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the NodeBufferPercentage section.
    /// </summary>
    class NodeBufferPercentageConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.NodeBufferPercentage; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueInRange(settingsForThisSection, key, 0.0, 1.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the DefragmentationScopedAlgorithmEnabled section.
    /// </summary>
    class DefragmentationScopedAlgorithmEnabledConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                bool value = settingsForThisSection[key].GetValue<bool>();
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the DefragmentationEmptyNodeWeight section.
    /// </summary>
    class DefragmentationEmptyNodeWeightConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueInRange(settingsForThisSection, key, 0.0, 1.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the DefragmentationNonEmptyNodeWeight section.
    /// </summary>
    class DefragmentationNonEmptyNodeWeightConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.DefragmentationNonEmptyNodeWeight; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyDoubleValueInRange(settingsForThisSection, key, 0.0, 1.0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the MetricEmptyNodeThresholds section.
    /// </summary>
    class MetricEmptyNodeThresholdsConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, key, 0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the ReservedLoadPerNode section.
    /// </summary>
    class ReservedLoadPerNodeConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.ReservedLoadPerNode; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                ValueValidator.VerifyIntValueGreaterThanEqualToInput(settingsForThisSection, key, 0, SectionName);
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }

    /// <summary>
    /// This is the class handles the validation of configuration for the BalancingByPercentage section.
    /// </summary>
    class BalancingByPercentageConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.BalancingByPercentage; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            var settingsForThisSection = windowsFabricSettings.GetSection(this.SectionName);
            foreach (string key in settingsForThisSection.Keys)
            {
                bool value = settingsForThisSection[key].GetValue<bool>();
            }
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {

        }
    }
}
