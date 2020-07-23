// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PLBConfig : Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(PLBConfig, "PLB")

        public:
            typedef Common::ConfigCollection<std::wstring, uint> KeyIntegerValueMap;

            class KeyDoubleValueMap : public std::map<std::wstring, double>
            {
            public:
                static KeyDoubleValueMap Parse(Common::StringMap const & entries);
                void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
            };

            class KeyBoolValueMap : public std::map<std::wstring, bool>
            {
            public:
                static KeyBoolValueMap Parse(Common::StringMap const & entries);
                void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
            };

            //TODO: add validation for all configurations
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", DummyPLBEnabled, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //If true, uses "Cluster Resource Manager" instead of "Load Balancer" or "PLB" in strings and health reporting codes, i.e. System.CRM instead of System.PLB
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseCRMPublicName, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies whether to trace reasons for CRM issued movements to the operational events channel
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", TraceCRMReasons, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies whether or not the PlacementConstraint expression for a service is validated when a service's ServiceDescription is updated
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", ValidatePlacementConstraint, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Limits the size of the table used for quick validation and caching of Placement Constraint Expressions
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PlacementConstraintValidationCacheSize, 10000, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines whether health warnings are reported for Constraint Violations. 0: Report for All Constraint Violations, 1: Report only for Hard Constraint Violations, 2: Ignore Soft Domain Violations 3: Smart Reporting, negative: Ignore All
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", ConstraintViolationReportingPolicy, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of times a replica has to go unplaced before a health warning is reported for it (if verbose health reporting is enabled)
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", VerboseHealthReportLimit, 20, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of times constraint violating replica has to be persistently unfixed before diagnostics are conducted and health reports are emitted
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", ConstraintViolationHealthReportLimit, 50, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of times constraint violating replica has to be persistently unfixed before diagnostics are conducted and detailed health reports are emitted
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", DetailedConstraintViolationHealthReportLimit, 200, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of times an unplaced replica has to be persistently unpalced before detailed health reports are emitted
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", DetailedVerboseHealthReportLimit, 200, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of consecutive times that ResourceBalancer-issued Movements are dropped before diagnostics are conducted and health warnings are emitted. Negative: No Warnings Emitted under this condition
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", ConsecutiveDroppedMovementsHealthReportLimit, 20, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of nodes per constraint to include before truncation in the Unplaced Replica reports
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", DetailedNodeListLimit, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of partitions per diganostic entry for a constraint to include before truncation in  Diagnostics
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", DetailedPartitionListLimit, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of diagnostic entries (with detailed information) per constraint to include before truncation in  Diagnostics
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", DetailedDiagnosticsInfoListLimit, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the number of metrics to include before truncation in the Unsuccessful Balancing reports
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", DetailedMetricListLimit, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines how long PLB Health Events survive before they expire
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", PLBHealthEventTTL, Common::TimeSpan::FromSeconds(65.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated, please use PLBRefreshGap instead
            //Defines how frequently the PLB background thread scans its state to determine if any actions need to be taken
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", PLBRefreshInterval, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the minimum amount of time that must pass before PLB refreshes state again
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", PLBRefreshGap, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the minimum amount of time that must pass before two consecutive placement rounds
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", MinPlacementInterval, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the minimum amount of time that must pass before two consecutive constraint check rounds
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", MinConstraintCheckInterval, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the minimum amount of time that must pass before two consecutive balancing rounds
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", MinLoadBalancingInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Do not start balancing activities within this period after a node down event
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", BalancingDelayAfterNodeDown, Common::TimeSpan::FromSeconds(120.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Do not start balancing activities within this period after adding a new node
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", BalancingDelayAfterNewNode, Common::TimeSpan::FromSeconds(120.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Do not Fix FaultDomain and UpgradeDomain constraint violations within this period after a node down event
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", ConstraintFixPartialDelayAfterNodeDown, Common::TimeSpan::FromSeconds(120.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //DDo not Fix FaultDomain and UpgradeDomain constraint violations within this period after adding a new node
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", ConstraintFixPartialDelayAfterNewNode, Common::TimeSpan::FromSeconds(120.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Rewind the state machine and retry placement or constraintcheck or balancing again when we are at NoActionNeeded for a while
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", PLBRewindInterval, Common::TimeSpan::FromSeconds(300.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //The times of retry for each PLB action
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PLBActionRetryTimes, 3, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the maximum amount of time PLB background thread can wait if actions from the last scan is not submitted yet
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", MaxMovementHoldingTime, Common::TimeSpan::FromSeconds(3600.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Defines the maximum amount of time PLB background thread can wait if actions from the last scan is submitted but not executed by FM yet
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", MaxMovementExecutionTime, Common::TimeSpan::FromSeconds(10.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Balancing activities will be reduced if (current AvgStdDev) <= (AvgStdDev after last balancing) * (1 + AvgStdDevDeltaThrottleThreshold)
            //This type of throttling can be disabled by providing a negative value for this parameter.
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", AvgStdDevDeltaThrottleThreshold, -1, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Maximum number of total movements allowed in Balancing and Placement phases (absolute number, does not depend on cluster size)
            // in the past interval indicated by GlobalMovementThrottleCountingInterval. 0 indicates no limit.
            // If both this and GlobalMovementThrottleThresholdPercentage are specified, then more conservative limit is used.
            PUBLIC_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", GlobalMovementThrottleThreshold, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Maximum number of total movements allowed in Balancing and Placement phases (expressed as percentage of total number of replicas in the cluster)
            // in the past interval indicated by GlobalMovementThrottleCountingInterval. 0 indicates no limit.
            // If both this and GlobalMovementThrottleThreshold are specified, then more conservative limit is used.
            PUBLIC_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", GlobalMovementThrottleThresholdPercentage, 0.0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Maximum number of movements allowed in Placement Phase (absolute number, does not depend on cluster size)
            // in the past interval indicated by GlobalMovementThrottleCountingInterval.0 indicates no limit.
            // If both this and GlobalMovementThrottleThresholdPercentageForPlacement are specified, then more conservative limit is used.
            PUBLIC_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", GlobalMovementThrottleThresholdForPlacement, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Maximum number of movements allowed in Placement Phase (expressed as percentage of total number of replicas in PLB)
            // in the past interval indicated by GlobalMovementThrottleCountingInterval. 0 indicates no limit.
            // If both this and GlobalMovementThrottleThresholdForPlacement are specified, then more conservative limit is used.
            PUBLIC_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", GlobalMovementThrottleThresholdPercentageForPlacement, 0.0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Maximum number of movements allowed in Balancing Phase (absolute number, does not depend on cluster size)
            // in the past interval indicated by GlobalMovementThrottleCountingInterval. 0 indicates no limit.
            // If both this and GlobalMovementThrottleThresholdPercentageForBalancing are specified, then more conservative limit is used.
            PUBLIC_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", GlobalMovementThrottleThresholdForBalancing, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Maximum number of movements allowed in Balancing Phase (expressed as percentage of total number of replicas in PLB)
            // in the past interval indicated by GlobalMovementThrottleCountingInterval. 0 indicates no limit.
            // If both this and GlobalMovementThrottleThresholdForBalancing are specified, then more conservative limit is used.
            PUBLIC_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", GlobalMovementThrottleThresholdPercentageForBalancing, 0.0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Indicate the length of the past interval for which to track per domain replica movements (used along with GlobalMovementThrottleThreshold). Can be set to 0 to ignore global throttling altogether.
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", GlobalMovementThrottleCountingInterval, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Static);

            //No balancing related movement will occur for a partition if the number of balancing related movements for replicas of that partition has reached or exceeded MovementPerFailoverUnitThrottleThreshold
            //in the past interval indicated by MovementPerPartitionThrottleCountingInterval
            PUBLIC_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", MovementPerPartitionThrottleThreshold, 50, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Indicate the length of the past interval for which to track replica movements for each partition (used along with MovementPerPartitionThrottleThreshold)
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", MovementPerPartitionThrottleCountingInterval, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Static);

            //No balancing will be performed if (the number of currently throttled partitions) > (the number of imbalanced partitions) * MovementThrottledPartitionsPercentageThreshold
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", MovementThrottledPartitionsPercentageThreshold, 0.5, Common::ConfigEntryUpgradePolicy::Dynamic);

            //When placing services, search for at most this long before returning a result
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", PlacementSearchTimeout, Common::TimeSpan::FromSeconds(0.5), Common::ConfigEntryUpgradePolicy::Dynamic);

            //When placing services, search for at most this long before returning a result
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", ConstraintCheckSearchTimeout, Common::TimeSpan::FromSeconds(0.5), Common::ConfigEntryUpgradePolicy::Dynamic);

            //When performing fast balancing, search for at most this long before returning a result
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", FastBalancingSearchTimeout, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

            //When performing slow balancing, search for at most this long before returning a result
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", SlowBalancingSearchTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);

            //Enable or disable balancing and constraint check movements
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", LoadBalancingEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Enable or disable constraint check movements independently of the LoadBalancingEnabled config
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", ConstraintCheckEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Enable or disable the logic of split of service domain logic
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", SplitDomainEnabled, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which determines if the LB is in test mode, which results in additional tracing and validity checking
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", IsTestMode, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The percentage of difference in score when doing load balancing
            TEST_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", AllowedBalancingScoreDifference, 0.1, Common::ConfigEntryUpgradePolicy::Dynamic);

            TEST_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PerfTestNumberOfNodes, 3000, Common::ConfigEntryUpgradePolicy::Dynamic);

            TEST_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PerfTestNumberOfPartitions, 100000, Common::ConfigEntryUpgradePolicy::Dynamic);

            TEST_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PerfTestNumberOfInstances, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", PrintRefreshTimers, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Enable or disable fault domain in PLB
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", FaultDomainEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Enable or disable upgrade domain in PLB
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UpgradeDomainEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Set the balancing threshold for local domains. Balancing phase will balance and take into account stdDev
            //from local PLB domain entries if maxNodeLoad/minNodeLoad is greater than LocalBalancingThreshold.
            //Defragmentation will work if maxNodeLoad/minNodeLoad in at least one FD or UD is smaller than LocalBalancingThreshold.
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", LocalBalancingThreshold, 0.0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Weight for local domains when calculating score. Standard deviation from all local domains will be combined together and
            //each of them will have weight of 1/n if there are n local domains. This number is then multiplied with this weight and
            //combined with deviation from global domain (weight of global domain is 1 - LocalDomainWeight)
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", LocalDomainWeight, 0.25, Common::ConfigEntryUpgradePolicy::Dynamic);

            //When searching for a balanced solution, every 10ms the LB search thread will sleep for this amount of time
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", YieldDurationPer10ms, 7, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Initial random seed
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", InitialRandomSeed, -1, Common::ConfigEntryUpgradePolicy::Static);

            //Maximum number of simulated annealing iterations.  The default value of -1 specifies no limit within the specified timeout
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MaxSimulatedAnnealingIterations, -1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Maximum percentage of the service objects in the cluster to move at any time
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", MaxPercentageToMove, 0.3, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Maximum percentage of the service objects in the cluster to move during placement
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", MaxPercentageToMoveForPlacement, 0.1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The number of potential simulated annealing solutions to keep in fast load balancing: not used any more
            DEPRECATED_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", FastBalancingPopulationSize, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The number of potential simulated annealing solutions to keep in slow load balancing
            DEPRECATED_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", SlowBalancingPopulationSize, 2, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The rate at which the simulated annealing algorithm changes the tradeoff from random search to score improvement in fast load balancing
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", FastBalancingTemperatureDecayRate, 0.8, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The rate at which the simulated annealing algorithm changes the tradeoff from random search to score improvement in slow load balancing
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", SlowBalancingTemperatureDecayRate, 0.98, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Whether to use old (98%) or new (based on #replicas and deviation) method for calculating initial temperature
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", EnableClusterSpecificInitialTemperature, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines if we want to use score calculation in Constraint Check Fixing
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseScoreInConstraintCheck, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Turns on and off logic which prefers moving to already upgraded UDs
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", PreferUpgradedUDs, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Increasing parameter will lead to more random moves, lowering leads to more heuristic moves
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", ClusterSpecificTemperatureCoefficient, 100.0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Instructs the LB to ignore the cost element of the scoring function, resulting potentially large number of moves for better balanced placement
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", IgnoreCostInScoring, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Instructs the LB to ignore the cost element of the scoring function, resulting potentially large number of moves for better balanced placement
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseMoveCostReports, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Offset for calculating move cost
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MoveCostOffset, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Value for move cost Zero used in score calculation
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MoveCostZeroValue, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Value for move cost Zero used in score calculation
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MoveCostLowValue, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Value for move cost Medium used in score calculation
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MoveCostMediumValue, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Value for move cost High used in score calculation
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MoveCostHighValue, 40, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Value for move cost for swaps that is used during calculation of energy if UseMoveCostReports is set to true.
            // Before changing this setting, consult values for Zero, Low, Medium and High cost in order to understand
            // how heavy do you want swap cost to be.
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", SwapCost, 0.1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which indicates that there must be a particular amount of improvement in the score between two solutions in order for the LB to accept the new solution
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", ScoreImprovementThreshold, 0.0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The interval with which to trace node loads for each service domain
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", NodeLoadsTracingInterval, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Dynamic);

            //The interval with which to trace Application loads for each service domain
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", ApplicationLoadsTracingInterval, Common::TimeSpan::FromSeconds(40), Common::ConfigEntryUpgradePolicy::Dynamic);

            //The interval with which to periodically trace information of PLB
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", PLBPeriodicalTraceInterval, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Dynamic);

            //The max number of violated items to be printed to trace
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MaxViolatedItemsToTrace, 100, Common::ConfigEntryUpgradePolicy::Dynamic, Common::GreaterThan(0));

            //The max number of replicas to trace that cannot be placed or that are violating constraints and cannot be corrected
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", MaxInvalidReplicasToTrace, 10, Common::ConfigEntryUpgradePolicy::Dynamic, Common::GreaterThan(0));

            //Determines the set of MetricActivityThresholds for the metrics in the cluster.
            //Balancing will work if maxNodeLoad is greater than MetricActivityThresholds.
            //For deferag metrics it defines the amount of load equal to or below which Service Fabric will consider the node empty
            PUBLIC_CONFIG_GROUP(KeyIntegerValueMap, L"MetricActivityThresholds", MetricActivityThresholds, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the set of MetricBalancingThresholds for the metrics in the cluster.
            //Balancing will work if maxNodeLoad/minNodeLoad is greater than MetricBalancingThresholds.
            //Defragmentation will work if maxNodeLoad/minNodeLoad in at least one FD or UD is smaller than MetricBalancingThresholds.
            PUBLIC_CONFIG_GROUP(KeyDoubleValueMap, L"MetricBalancingThresholds", MetricBalancingThresholds, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated, please use ReservedLoadPerNode instead
            //Determines the set of MetricEmptyNodeThresholds for the metrics in the cluster.
            //Determines how much load on a node can exist to be free for that node to be considered empty.
            INTERNAL_CONFIG_GROUP(KeyIntegerValueMap, L"MetricEmptyNodeThresholds", MetricEmptyNodeThresholds, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the set of ReservedLoadPerNode for the metrics in the cluster.
            //Determines how much load should be reserved on a node.
            INTERNAL_CONFIG_GROUP(KeyIntegerValueMap, L"ReservedLoadPerNode", ReservedLoadPerNode, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the set of GlobalMetricWeights for the metrics in the cluster.
            PUBLIC_CONFIG_GROUP(KeyDoubleValueMap, L"GlobalMetricWeights", GlobalMetricWeights, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the set of metrices where balancing is done by percentage of load on node, instead of absolute load
            //Available if every node has defined and limited capacity
            INTERNAL_CONFIG_GROUP(KeyBoolValueMap, L"BalancingByPercentage", BalancingByPercentage, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the set of metrices that should be used for defragmentation and not for load balancing.
            PUBLIC_CONFIG_GROUP(KeyBoolValueMap, L"DefragmentationMetrics", DefragmentationMetrics, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the number of free nodes which are needed to consider cluster defragmented by specifying either percent in range [0.0 - 1.0) or number of empty nodes as number >= 1.0
            PUBLIC_CONFIG_GROUP(KeyDoubleValueMap, L"DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold", DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies the policy defragmentation follows when emptying nodes. For a given metric 0 indicates that SF should try to defragment nodes evenly across UDs and FDs, 1 indicates only that the nodes must be defragmented
            PUBLIC_CONFIG_GROUP(KeyIntegerValueMap, L"DefragmentationEmptyNodeDistributionPolicy", DefragmentationEmptyNodeDistributionPolicy, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies whether the scoped defragmentation logic should be used for this metric
            INTERNAL_CONFIG_GROUP(KeyBoolValueMap, L"DefragmentationScopedAlgorithmEnabled", DefragmentationScopedAlgorithmEnabled, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Map: MetricName to Strategy. Specify strategy for each metric:
            //     0 - Balancing                      / balance cluster
            //     1 - ReservationWithBalancing	      / replicas will be placed so cluster load is balanced across the cluster, if possible avoiding reserved nodes
            //     2 - Reservation                    / replicas will be placed on a random node in cluster, if possible avoiding reserved nodes
            //     3 - ReservationWithPacking         / replicas will be placed so cluster load is packed as dense as possible, if possible avoiding reserved nodes
            //     4 - Defragmentation                / defragmentation of cluster load
            // Default value for this config is: 0 - Balancing cluster
            INTERNAL_CONFIG_GROUP(KeyIntegerValueMap, L"PlacementStrategy", PlacementStrategy, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies whether the restricted defragmentation heuristic should be used (as additional solution)
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RestrictedDefragmentationHeuristicEnabled, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies the ratio of node emptying used in the calculations of score, for specified metric
            //0.0 - don't do any extra moves to free up space on nodes, reservation of capacity doesn't improve score
            //> 0.0 - allow certain moves if the score will be improved during emptying nodes, emptying load is included in score
            //1.0 - Max value
            INTERNAL_CONFIG_GROUP(KeyDoubleValueMap, L"DefragmentationEmptyNodeWeight", DefragmentationEmptyNodeWeight, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies the ratio of balancing used in the calculations of score, for specified metric
            //0.0 - don't do any extra moves to improve balancing phase
            //> 0.0 - allow certain moves if the score will be improved during balancing
            //1.0 - Max value
            INTERNAL_CONFIG_GROUP(KeyDoubleValueMap, L"DefragmentationNonEmptyNodeWeight", DefragmentationNonEmptyNodeWeight, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Specifies whether empty nodes for multiple scoped defrag metrics have to overlap
            //If set to false, empty nodes can overlap, but it's not required
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", NodesWithReservedLoadOverlap, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines factor for standard deviation of load across all nodes during calculating stdDev.
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", DefragmentationNodesStdDevFactor, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines factor for standard deviation of load across all fault domains during calculating stdDev.
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", DefragmentationFdsStdDevFactor, 0.01, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines factor for standard deviation of load across all upgrade domains during calculating stdDev.
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", DefragmentationUdsStdDevFactor, 0.01, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Node capacity percentage per metric name, used as a buffer in order to keep some free place on a node for the failover case.
            PUBLIC_CONFIG_GROUP(KeyDoubleValueMap, L"NodeBufferPercentage", NodeBufferPercentage, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated, please use PreventTransientOvercommit instead
            //Determines should PLB immediately count on resources that will be freed up by the initiated moves.
            //By default, PLB can initiate move out and move in on the same node which can create intermediate overcommit.
            //Setting this parameter to true will prevent those kind of overcommits and on-demand defrag (aka placementWithMove) will do nothing.
            DEPRECATED_CONFIG_ENTRY(bool, L"PreventIntermediateOvercommit", PreventIntermediateOvercommit, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines should PLB immediately count on resources that will be freed up by the initiated moves.
            //By default, PLB can initiate move out and move in on the same node which can create transient overcommit.
            //Setting this parameter to true will prevent those kind of overcommits and on-demand defrag (aka placementWithMove) will be disabled.
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", PreventTransientOvercommit, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // In case when PreventTransientOvercommit is true, this config determines if PLB can use the load that is occupied by disappearing replicas.
            // If the config is set to false, then PLB will detect the load of such replicas and will not allow Simulated Annealing to use it.
            // Regardless of this config, PLB will not make transient overcommit during the single run.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", CountDisappearingLoadForSimulatedAnnealing, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determine whether the in-build throttling is enabled
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", InBuildThrottlingEnabled, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The associated metric name for this throttling
            PUBLIC_CONFIG_ENTRY(std::wstring, L"PlacementAndLoadBalancing", InBuildThrottlingAssociatedMetric, L"", Common::ConfigEntryUpgradePolicy::Static);

            //The maximal number of in-build replicas allowed globally
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", InBuildThrottlingGlobalMaxValue, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determine whether the swap-primary throttling is enabled
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", SwapPrimaryThrottlingEnabled, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The associated metric name for this throttling
            PUBLIC_CONFIG_ENTRY(std::wstring, L"PlacementAndLoadBalancing", SwapPrimaryThrottlingAssociatedMetric, L"", Common::ConfigEntryUpgradePolicy::Static);

            //The maximal number of swap-primary replicas allowed globally
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", SwapPrimaryThrottlingGlobalMaxValue, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Map of node type name to maximum number of total parallel builds on each node of that type.
            // If this number is greater than zero, and throttling is enabled, then this is the hard limit for the number of 
            // InBuild replicas that Cluster Resource Manager (CRM) can create on the node. Prior to making moves, CRM will consider
            // the number of existing InBuild replicas on the node, and if that number is below the limit then CRM will be able to make
            // new moves into the node respecting the limit. It is possible that the number of InBuild replicas exceeds this limit for reasons 
            // other than moves initiated by CRM. For example, if several replicas on the node restart they may go into InBuild state
            // and number of buils on the node may be exceeded.
            // Throttling is enabled if:
            //  - ThrottlingConstraintPriority is set to value >= 0.
            //  - Throttling of the current phase in CRM is enabled by setting corresponding switch to true:
            //      - ThrottlePlacementPhase for placement or placement with move phases.
            //      - ThrottleConstraintCheckPhase for constraint check.
            //      - ThrottleBalancingPhase for LoadBalancing or QuickLoadBalancing.
            INTERNAL_CONFIG_GROUP(KeyIntegerValueMap, L"MaximumInBuildReplicasPerNode", MaximumInBuildReplicasPerNode, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Map of node type name to maximum number of total parallel builds on the node during balancing phase.
            // If this number is greater than zero, and throttling is enabled, then Balancing phase is not allowed to move replicas to the node
            // if that would cause the total number of InBuild replicas (existing + new builds) to go over this limit.
            // If both this value and MaximumInBuildReplicasPerNode are specified, then the lower of the two is used for balancing phase.
            // Throttling of balancing phase is enabled if:
            //  - ThrottlingConstraintPriority is set to value >= 0.
            //  - ThrottleBalancingPhase is set to true.
            INTERNAL_CONFIG_GROUP(KeyIntegerValueMap, L"MaximumInBuildReplicasPerNodeBalancingThrottle", MaximumInBuildReplicasPerNodeBalancingThrottle, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Map of node type name to maximum number of total parallel builds on the node during constraint check phase.
            // If this number is greater than zero, and throttling is enabled, then Constraint Check phase is not allowed to move replicas to the node
            // if that would cause the total number of InBuild replicas (existing + new builds) to go over this limit.
            // If both this value and MaximumInBuildReplicasPerNode are specified, then the lower of the two is used for Constraint Check phase.
            // Throttling of constraint check phase is enabled if:
            //  - ThrottlingConstraintPriority is set to value >= 0.
            //  - ThrottleConstraintCheckPhase is set to true.
            INTERNAL_CONFIG_GROUP(KeyIntegerValueMap, L"MaximumInBuildReplicasPerNodeConstraintCheckThrottle", MaximumInBuildReplicasPerNodeConstraintCheckThrottle, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Map of node type name to maximum number of total parallel builds on the node during Placement phase.
            // If this number is greater than zero, and throttling is enabled, then Placement phase is not allowed to move or add replicas to the node
            // if that would cause the total number of InBuild replicas (existing + new builds) to go over this limit.
            // If both this value and MaximumInBuildReplicasPerNode are specified, then the lower of the two is used for placement phase.
            // Throttling of placement phase is enabled if:
            //  - ThrottlingConstraintPriority is set to value >= 0.
            //  - ThrottlePlacementPhase is set to true.
            INTERNAL_CONFIG_GROUP(KeyIntegerValueMap, L"MaximumInBuildReplicasPerNodePlacementThrottle", MaximumInBuildReplicasPerNodePlacementThrottle, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Priority of the throttling constraint. If it is greater then zero, then throttling will be used in all phases.
            // There are separate switches for each phase in ClusterResourceManager that can be used to disable throttling:
            //  - ThrottlePlacementPhase to disable throttling during placement.
            //  - ThrottleBalancingPhase to disable throttling during balancing.
            //  - ThrottleConstraintCheckPhase to disable throttling during constraint check.
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", ThrottlingConstraintPriority, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // If true, ThrottlingConstraint is enabled in placement phase, and placement of new replicas is throttled.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", ThrottlePlacementPhase, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // If true, ThrottlingConstraint is enabled during balancing phase, and movement of replicas is throttled.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", ThrottleBalancingPhase, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // If true, ThrottlingConstraint is enabled during constraint check runs, and fixing violations is throttled.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", ThrottleConstraintCheckPhase, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // In tests, PLB will assert if number of InBuild replicas per node exceeds this value. Negative is ignored.
            TEST_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PerNodeThrottlingCheck, -1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines whether to trace simulated annealing statistics
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", TraceSimulatedAnnealingStatistics, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the interval at which to trace simulated annealing statistics
            TEST_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", SimulatedAnnealingStatisticsInterval, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

            //How many random movements to probe for determining the initial temperature
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", InitialTemperatureProbeCount, 50, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Number of iterations per round during simulated annealing
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", SimulatedAnnealingIterationsPerRound, 1000, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Number of iterations per round during placement search
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PlacementSearchIterationsPerRound, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Number of iterations per round during constraint checking
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", ConstraintCheckIterationsPerRound, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Interval with which to process pending updates from FM.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", ProcessPendingUpdatesInterval, Common::TimeSpan::FromSeconds(0.3), Common::ConfigEntryUpgradePolicy::Dynamic);

            //The probability to generate a swap primary movement during load balancing
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", SwapPrimaryProbability, 0.3, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The interval of that the loads in the history decay for the given factor measured in second
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", LoadDecayInterval, Common::TimeSpan::FromSeconds(60.0), Common::ConfigEntryUpgradePolicy::Static);

            //The factor that the loads in the history decay, it should be greater than or equal to 0 and less than or equal to 1
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", LoadDecayFactor, 0, Common::ConfigEntryUpgradePolicy::Static);

            //Determines the priority of placement constraint: 0: Hard, 1: Soft, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PlacementConstraintPriority, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the priority of prefered location constraint: 0: Hard, 1: Soft, 2: Optimization, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PreferredLocationConstraintPriority, 2, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the priority of capacity constraint: 0: Hard, 1: Soft, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", CapacityConstraintPriority, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the priority of affinity constraint: 0: Hard, 1: Soft, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", AffinityConstraintPriority, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the priority of fault domain constraint: 0: Hard, 1: Soft, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", FaultDomainConstraintPriority, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the priority of upgrade domain constraint: 0: Hard, 1: Soft, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", UpgradeDomainConstraintPriority, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the priority of scaleout count constraint: 0: Hard, 1: Soft, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", ScaleoutCountConstraintPriority, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines the priority of capacity constraint: 0: Hard, 1: Soft, negative: Ignore
            PUBLIC_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", ApplicationCapacityConstraintPriority, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated as of v5.2 - please use MoveParentToFixAffinityViolation instead
            //Setting which determines if the affinity is bi-directional
            DEPRECATED_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", IsAffinityBidirectional, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which determines if parent replicas can be moved to fix affinity constraints
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", MoveParentToFixAffinityViolation, false, Common::ConfigEntryUpgradePolicy::Dynamic)

                //Setting which determines after which transition we will try to find solution by moving parent replicas if MoveParentToFixAffinityViolation is true
                //If MoveParentToFixAffinityViolation is false this flag does nothing
                INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", MoveParentToFixAffinityViolationTransitionPercentage, 0.2, Common::ConfigEntryUpgradePolicy::Dynamic)

                //Setting which determines if to move existing replica during placement
                PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", MoveExistingReplicaForPlacement, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which determines if use different secondary load
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseSeparateSecondaryLoad, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which determines if child service replica can be placed if no parent replica is up
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", PlaceChildWithoutParent, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //The probability to move parent instead of child replica probability for affinity constraint check
            DEPRECATED_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", AffinityMoveParentReplicaProbability, 0.5, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated and no longer consumed
            //Determines whether we should relax constraints (including Affinity, FD/UD, and Capacity) during placement if we cannot find the placement honoring all constraints
            DEPRECATED_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxConstraintsForPlacementEnabled, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated and no longer consumed
            //Determines whether we should relax FD/UD constraint during placement if we cannot find the placement honoring all constraints
            DEPRECATED_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxFaultDomainConstraintsForPlacement, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated and no longer consumed
            //Determines whether we should relax Affinity constraints during placement if we cannot find the placement honoring all constraints
            DEPRECATED_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxAffinityConstraintsForPlacement, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines whether we should relax Affinity constraint during cluster upgrade if we cannot find the placement honoring all constraints
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxAffinityConstraintDuringUpgrade, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Deprecated and no longer consumed
            //Determines whether we should relax Capacity constraints during placement if we cannot find the placement honoring all constraints
            DEPRECATED_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxCapacityConstraintsForPlacement, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines whether we should allow PLB operations that are fixing constraint violations of the partitions belongs to application which is currently in application upgrade.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", AllowConstraintCheckFixesDuringApplicationUpgrade, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //How many nodes to be periodically traced
            INTERNAL_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", PLBPeriodicalTraceApplicationCount, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

            //How many nodes to be periodically traced
            INTERNAL_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", PLBPeriodicalTraceNodeCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);

            //How many services to be periodically traced
            INTERNAL_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", PLBPeriodicalTraceServiceCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);

            //How many load entry to be traced in the node load trace
            INTERNAL_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", PLBNodeLoadTraceEntryCount, 500, Common::ConfigEntryUpgradePolicy::Dynamic);

            //How many load entry per service domain to be traced in the application load trace interval
            INTERNAL_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", PLBApplicationLoadTraceMaxSize, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

            //How many load entry to be traced in a single application load trace
            INTERNAL_CONFIG_ENTRY(uint, L"PlacementAndLoadBalancing", PLBApplicationLoadTraceBatchSize, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines if all service replicas in cluster will be placed "all or nothing" given limited suitable nodes for them
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", PartiallyPlaceServices, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines if any type of failover unit update should interrupt fast or slow balancing run.
            //With specified "false" balancing run will be interrupted if FailoverUnit:  is created/deleted, has missing replicas, changed primary replica location or changed number of replicas.
            //Balancing run will NOT be interrupted in other cases - if FailoverUnit:  has extra replicas, changed any replica flag, changed only partition version or any other case.
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", InterruptBalancingForAllFailoverUnitUpdates, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Move or swap primary replicas for aligned affinity together if it is true.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", CheckAlignedAffinityForUpgrade, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Add new replicas together for affinity correlated partitions during upgrade
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", CheckAffinityForUpgradePlacement, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Should capacity be relaxed if there is no enough capacity for the swaps of replicas during upgrade.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxCapacityConstraintForUpgrade, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines whether we should relax Scaleout constraint during upgrade for applications which maximal node count (i.e. scaleout) is set to 1
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxScaleoutConstraintDuringUpgrade, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines whether we should relax ALIGNED affinity during singleton replica upgrade
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxAlignAffinityConstraintDuringSingletonUpgrade, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines if boost tests will be executed with default application for all services that do not have one.
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseAppGroupsInBoost, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines if boost tests will be executed with default RG settings.
            TEST_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseRGInBoost, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines whether we should trace metric info before and after balancing phase
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", TraceMetricInfoForBalancingRun, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Config that specifies whether we should use default load for -1 services
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseDefaultLoadForServiceOnEveryNode, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines how should PLB choose target nodes for replica placement based on incoming replicas load per defrag metric.
            //This parameter should target first N most occupied nodes whose accumulated empty space in total is greater than incoming replicas
            //load multiplied by value of this parameter, if N is greater than number of nodes calculated by parameter PlacementHeuristicEmptySpacePercent.
            INTERNAL_CONFIG_GROUP(KeyDoubleValueMap, L"PlacementHeuristicIncomingLoadFactor", PlacementHeuristicIncomingLoadFactor, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Determines how should PLB choose target nodes for replica placement based on remaining free capacity per defrag metric.
            //This parameter should target first M most occupied nodes whose accumulated empty space in total is greater than total cluster empty
            //space multiplied by value of this parameter, if M is greater than number of nodes calculated by parameter PlacementHeuristicIncomingLoadFactor.
            INTERNAL_CONFIG_GROUP(KeyDoubleValueMap, L"PlacementHeuristicEmptySpacePercent", PlacementHeuristicEmptySpacePercent, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which determines if higher child target replica count than parent will be considered as a valid state by affinity constraint
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", AllowHigherChildTargetReplicaCountForAffinity, true, Common::ConfigEntryUpgradePolicy::Dynamic)

            //Configuration that specifies whether replica distribution among upgrade domains (of services with packing policy) should be based on a partition write quorum or
            //PLB should keep max replica difference between number of replicas per upgrade domains up to 1.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", QuorumBasedReplicaDistributionPerUpgradeDomains, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Configuration that specifies whether replica distribution among fault domains (of services with packing policy) should be based on a partition write quorum or
            //PLB should keep max replica difference between number of replicas per fault domains up to 1.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", QuorumBasedReplicaDistributionPerFaultDomains, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Configuration that specifies whether replica distribution among fault and upgrade domains should automatically (when necessary) switch to a distribution
            // based on a partition write quorum instead of keeping max replica difference between number of replicas per domains up to 1, which is default distribution.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", QuorumBasedLogicAutoSwitch, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which determines whether balancing should be run during application upgrade
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", AllowBalancingDuringApplicationUpgrade, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // This config will trigger auto detection of available resources on node (CPU and Memory)
            // When this config is set to true - we will read real capacities and correct them if user specified bad node capacities or didn't define them at all
            // If this config is set to false - we will trace a warning that user specified bad node capacities, but we will not correct them, meaning that
            // user wants to have the capacities specified as > than the node really has or if capacities are undefined, it will assume unlimited capacity
            PUBLIC_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", AutoDetectAvailableResources, true, Common::ConfigEntryUpgradePolicy::Static);

            // These configs are used when calculating available node capacity for RG metrics (CPU and Memory)
            // Since we need to leave buffered capacity for system services and OS on node, we're calculating it as
            // apiNumCores * CpuPercentageNodeCapacity (where 1.0 means 100%)
            // apiAvailableMemoryMB * MemoryPercentageNodeCapacity (where 1.0 means 100%)
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", CpuPercentageNodeCapacity, 0.8, Common::ConfigEntryUpgradePolicy::Static);
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", MemoryPercentageNodeCapacity, 0.8, Common::ConfigEntryUpgradePolicy::Static);

            // Metric weight for resource governance metric - CpuCores and MemoryInMB
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", CpuCoresMetricWeight, 1.0, Common::ConfigEntryUpgradePolicy::Dynamic);
            INTERNAL_CONFIG_ENTRY(double, L"PlacementAndLoadBalancing", MemoryInMBMetricWeight, 1.0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Include load for None replicas for exclusive services
            // This is needed so that there is no PLB-LRM mismatch during upgrades and balancing
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", IncludeResourceGovernanceNoneReplicaLoad, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //This parameter controls whether we should have swap preferred solutions during CC fixes
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", EnablePreferredSwapSolutionInConstraintCheck, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //A boolean flag indicating if batch placement would be used
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", UseBatchPlacement, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Number of new replicas would be placed for each batch
            INTERNAL_CONFIG_ENTRY(int, L"PlacementAndLoadBalancing", PlacementReplicaCountPerBatch, 100000, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Setting which determines whether soft constraints should be relaxed for placement
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", RelaxConstraintForPlacement, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Determines how often statistics are traced out.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", StatisticsTracingInterval, Common::TimeSpan::FromSeconds(60.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            // When service is auto scaled by adding or removing partitions, there is a call to client to perform the actual scaling operation.
            // This setting determines how long is the timeout for waiting on the client to reply.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"PlacementAndLoadBalancing", ServiceRepartitionForAutoScalingTimeout, Common::TimeSpan::FromSeconds(60.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            // Prefer exisint replica locations during PreferredLocation check if it is true.
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", PreferExistingReplicaLocations, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Prefer nodes that have some required images when placing containers if the config is enabled
            INTERNAL_CONFIG_ENTRY(bool, L"PlacementAndLoadBalancing", PreferNodesForContainerPlacement, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        };
    }
}
