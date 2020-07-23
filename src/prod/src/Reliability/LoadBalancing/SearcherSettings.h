// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "PLBConfig.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        // This class contains all dynamic settings that are accessed multiple times during a searcher run or during placement creation.
        // In case that these settings are changed during the run, we want to be able to complete the current run without changing them.
        // Settings are refreshed when PLB is constructed, or when Refresh() begins.
        // This class is not thread safe therefore any time you add a field that does not have atomic access you need to synchronize with a lock!
        class SearcherSettings
        {
        public:

            DENY_COPY(SearcherSettings);

            SearcherSettings()
            {
                RefreshSettings();
                UseSeparateSecondaryLoad = PLBConfig::GetConfig().UseSeparateSecondaryLoad;
            }

            void RefreshSettings()
            {
                auto & config = PLBConfig::GetConfig();
                IgnoreCostInScoring = config.IgnoreCostInScoring;
                MoveCostOffset = config.MoveCostOffset;
                LocalDomainWeight = config.LocalDomainWeight;
                DefragmentationNodesStdDevFactor = config.DefragmentationNodesStdDevFactor;
                DefragmentationFdsStdDevFactor = config.DefragmentationFdsStdDevFactor;
                DefragmentationUdsStdDevFactor = config.DefragmentationUdsStdDevFactor;
                PlaceChildWithoutParent = config.PlaceChildWithoutParent;
                MoveParentToFixAffinityViolation = config.MoveParentToFixAffinityViolation;
                IsAffinityBidirectional = config.IsAffinityBidirectional;
                PreventTransientOvercommit = config.PreventIntermediateOvercommit || config.PreventTransientOvercommit;
                CountDisappearingLoadForSimulatedAnnealing = config.CountDisappearingLoadForSimulatedAnnealing;
                CheckAlignedAffinityForUpgrade = config.CheckAlignedAffinityForUpgrade;
                SwapPrimaryProbability = config.SwapPrimaryProbability;
                UseMoveCostReports = config.UseMoveCostReports;
                MoveCostZeroValue = config.MoveCostZeroValue;
                MoveCostLowValue = config.MoveCostLowValue;
                MoveCostMediumValue = config.MoveCostMediumValue;
                MoveCostHighValue = config.MoveCostHighValue;
                SwapCost = config.SwapCost;
                CheckAffinityForUpgradePlacement = config.CheckAffinityForUpgradePlacement;
                RelaxScaleoutConstraintDuringUpgrade = config.RelaxScaleoutConstraintDuringUpgrade;
                RelaxAlignAffinityConstraintDuringSingletonUpgrade = config.RelaxAlignAffinityConstraintDuringSingletonUpgrade;
                AllowHigherChildTargetReplicaCountForAffinity = config.AllowHigherChildTargetReplicaCountForAffinity;
                DummyPLBEnabled = config.DummyPLBEnabled;
                IsTestMode = config.IsTestMode;
                NodeBufferPercentage = config.NodeBufferPercentage;
                UseDefaultLoadForServiceOnEveryNode = config.UseDefaultLoadForServiceOnEveryNode;
                PlacementHeuristicIncomingLoadFactor = config.PlacementHeuristicIncomingLoadFactor;
                PlacementHeuristicEmptySpacePercent = config.PlacementHeuristicEmptySpacePercent;
                QuorumBasedReplicaDistributionPerUpgradeDomains = config.QuorumBasedReplicaDistributionPerUpgradeDomains;
                QuorumBasedReplicaDistributionPerFaultDomains = config.QuorumBasedReplicaDistributionPerFaultDomains;
                QuorumBasedLogicAutoSwitch = config.QuorumBasedLogicAutoSwitch;
                UpgradeDomainEnabled = config.UpgradeDomainEnabled;
                FaultDomainEnabled = config.FaultDomainEnabled;
                UpgradeDomainConstraintPriority = config.UpgradeDomainConstraintPriority;
                FaultDomainConstraintPriority = config.FaultDomainConstraintPriority;
                UseScoreInConstraintCheck = config.UseScoreInConstraintCheck;
                MoveParentToFixAffinityViolationTransitionPercentage = config.MoveParentToFixAffinityViolationTransitionPercentage;
                EnablePreferredSwapSolutionInConstraintCheck = config.EnablePreferredSwapSolutionInConstraintCheck;
                UseBatchPlacement = config.UseBatchPlacement;
                PlacementReplicaCountPerBatch = config.PlacementReplicaCountPerBatch;
                RelaxConstraintForPlacement = config.RelaxConstraintForPlacement;
                DefragmentationMetrics = config.DefragmentationMetrics;
                MetricActivityThresholds = config.MetricActivityThresholds;
                MetricBalancingThresholds = config.MetricBalancingThresholds;
                LocalBalancingThreshold = config.LocalBalancingThreshold;
                GlobalMetricWeights = config.GlobalMetricWeights;
                DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold = config.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
                MetricEmptyNodeThresholds = config.MetricEmptyNodeThresholds;
                ReservedLoadPerNode = config.ReservedLoadPerNode;
                DefragmentationEmptyNodeDistributionPolicy = config.DefragmentationEmptyNodeDistributionPolicy;
                DefragmentationScopedAlgorithmEnabled = config.DefragmentationScopedAlgorithmEnabled;
                DefragmentationEmptyNodeWeight = config.DefragmentationEmptyNodeWeight;
                DefragmentationNonEmptyNodeWeight = config.DefragmentationNonEmptyNodeWeight;
                PlacementStrategy = config.PlacementStrategy;
                NodesWithReservedLoadOverlap = config.NodesWithReservedLoadOverlap;
                PreferExistingReplicaLocations = config.PreferExistingReplicaLocations;
                BalancingByPercent = config.BalancingByPercentage;
                PreferNodesForContainerPlacement = config.PreferNodesForContainerPlacement;
                PreferUpgradedUDs = config.PreferUpgradedUDs;
                MaximumInBuildReplicasPerNode = config.MaximumInBuildReplicasPerNode;
                MaximumInBuildReplicasPerNodeBalancingThrottle = config.MaximumInBuildReplicasPerNodeBalancingThrottle;
                MaximumInBuildReplicasPerNodeConstraintCheckThrottle = config.MaximumInBuildReplicasPerNodeConstraintCheckThrottle;
                MaximumInBuildReplicasPerNodePlacementThrottle = config.MaximumInBuildReplicasPerNodePlacementThrottle;
            }

            bool IgnoreCostInScoring;
            int MoveCostOffset;
            double LocalDomainWeight;
            double DefragmentationNodesStdDevFactor;
            double DefragmentationFdsStdDevFactor;
            double DefragmentationUdsStdDevFactor;
            bool PlaceChildWithoutParent;
            bool MoveParentToFixAffinityViolation;
            bool IsAffinityBidirectional;
            bool PreventTransientOvercommit;
            bool CountDisappearingLoadForSimulatedAnnealing;
            bool CheckAlignedAffinityForUpgrade;
            double SwapPrimaryProbability;
            bool UseMoveCostReports;
            int MoveCostZeroValue;
            int MoveCostLowValue;
            int MoveCostMediumValue;
            int MoveCostHighValue;
            double SwapCost;
            bool CheckAffinityForUpgradePlacement;
            bool RelaxScaleoutConstraintDuringUpgrade;
            bool RelaxAlignAffinityConstraintDuringSingletonUpgrade;
            bool AllowHigherChildTargetReplicaCountForAffinity;
            bool DummyPLBEnabled;
            bool IsTestMode;
            PLBConfig::KeyDoubleValueMap NodeBufferPercentage;
            bool UpgradeDomainEnabled;
            bool FaultDomainEnabled;
            int UpgradeDomainConstraintPriority;
            int FaultDomainConstraintPriority;
            bool UseScoreInConstraintCheck;
            double MoveParentToFixAffinityViolationTransitionPercentage;
            bool NodesWithReservedLoadOverlap;
            bool PreferUpgradedUDs;

            //we need to treat this one separately as this also affects frontend behavior
            bool UseSeparateSecondaryLoad;

            bool UseDefaultLoadForServiceOnEveryNode;
            PLBConfig::KeyDoubleValueMap PlacementHeuristicIncomingLoadFactor;
            PLBConfig::KeyDoubleValueMap PlacementHeuristicEmptySpacePercent;

            PLBConfig::KeyBoolValueMap DefragmentationMetrics;
            PLBConfig::KeyIntegerValueMap MetricActivityThresholds;
            PLBConfig::KeyDoubleValueMap MetricBalancingThresholds;
            double LocalBalancingThreshold;
            PLBConfig::KeyDoubleValueMap GlobalMetricWeights;
            PLBConfig::KeyDoubleValueMap DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;
            PLBConfig::KeyIntegerValueMap MetricEmptyNodeThresholds;
            PLBConfig::KeyIntegerValueMap ReservedLoadPerNode;
            PLBConfig::KeyIntegerValueMap DefragmentationEmptyNodeDistributionPolicy;
            PLBConfig::KeyBoolValueMap DefragmentationScopedAlgorithmEnabled;
            PLBConfig::KeyDoubleValueMap DefragmentationEmptyNodeWeight;
            PLBConfig::KeyDoubleValueMap DefragmentationNonEmptyNodeWeight;
            PLBConfig::KeyIntegerValueMap PlacementStrategy;

            PLBConfig::KeyBoolValueMap BalancingByPercent;

            bool QuorumBasedReplicaDistributionPerUpgradeDomains;
            bool QuorumBasedReplicaDistributionPerFaultDomains;

            bool QuorumBasedLogicAutoSwitch;

            bool EnablePreferredSwapSolutionInConstraintCheck;

            bool UseBatchPlacement;
            int PlacementReplicaCountPerBatch;

            bool RelaxConstraintForPlacement;

            bool PreferExistingReplicaLocations;

            bool PreferNodesForContainerPlacement;
            PLBConfig::KeyIntegerValueMap MaximumInBuildReplicasPerNode;
            PLBConfig::KeyIntegerValueMap MaximumInBuildReplicasPerNodeBalancingThrottle;
            PLBConfig::KeyIntegerValueMap MaximumInBuildReplicasPerNodeConstraintCheckThrottle;
            PLBConfig::KeyIntegerValueMap MaximumInBuildReplicasPerNodePlacementThrottle;
        };

    }
}
