// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Placement.h"
#include "PLBEventSource.h"
#include "Searcher.h"
#include "PLBDiagnostics.h"
#include "PLBConfig.h"
#include "TempSolution.h"
#include "ViolationList.h"
#include "SearchInsight.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

size_t Searcher::GetMoveNumber(double maxPercentageToMove)
{
    double maxMove = maxPercentageToMove * placement_->ExistingReplicaCount;

    size_t result = static_cast<size_t>(ceil(maxMove));
    result = min(result, placement_->ExistingReplicaCount);

    // Account for throttling (max number of slots based on per node in-builds)
    result = min(result, checker_->MaxMovementSlots);

    return min(result, movementsAllowedGlobally_);
}

Searcher::Searcher(
    PLBEventSource const& trace,
    Common::atomic_bool const& toStop,
    Common::atomic_bool const& balancingEnabled,
    PLBDiagnosticsSPtr const& plbDiagnosticsSPtr,
    size_t sleepTimePer10ms,
    int randomSeed
    ) :
    trace_(trace),
    toStop_(toStop),
    balancingEnabled_(balancingEnabled),
    plbDiagnosticsSPtr_(plbDiagnosticsSPtr),
    sleepTimePer10ms_(sleepTimePer10ms),
    randomSeed_(randomSeed),
    random_(randomSeed),
    batchIndex_(0)
{
    ASSERT_IF(sleepTimePer10ms >= 10, "Sleep time should be less than 10 ms");
}

CandidateSolution Searcher::SearchForSolution(
    PLBSchedulerAction const & searchType,
    Placement const & placement,
    Checker & checker,
    DomainId const & domainId,
    bool containsDefragMetric,
    size_t allowedMovements)
{
    movementsAllowedGlobally_ = allowedMovements;
    placement_ = &placement;
    checker_ = &checker;
    checker_->BatchIndex = batchIndex_;

    domainId_ = domainId;
    random_.Reseed(randomSeed_);

    PLBConfig const& config = PLBConfig::GetConfig();
    if (searchType.Action == PLBSchedulerActionType::NewReplicaPlacement ||
        searchType.Action == PLBSchedulerActionType::NewReplicaPlacementWithMove)
    {
        bool creationWithMove = (searchType.Action == PLBSchedulerActionType::NewReplicaPlacement) ? false : true;
        return SearchForPlacement(
            config.MaxPercentageToMoveForPlacement,
            config.PlacementSearchTimeout,
            static_cast<uint64>(config.MaxSimulatedAnnealingIterations),
            config.PlacementSearchIterationsPerRound,
            config.SimulatedAnnealingIterationsPerRound,
            0.8,
            creationWithMove,
            searchType.Action);
    }
    else if (searchType.Action == PLBSchedulerActionType::ConstraintCheck)
    {
        return SearchForConstraintCheck(
            config.ConstraintCheckSearchTimeout,
            static_cast<uint64>(config.MaxSimulatedAnnealingIterations),
            config.ConstraintCheckIterationsPerRound,
            config.SimulatedAnnealingIterationsPerRound,
            0.8,
            searchType);
    }
    else if (searchType.Action == PLBSchedulerActionType::QuickLoadBalancing)
    {
        return SearchForBalancing(
            true,
            config.MaxPercentageToMove,
            config.FastBalancingSearchTimeout,
            static_cast<uint64>(config.MaxSimulatedAnnealingIterations),
            config.SimulatedAnnealingIterationsPerRound,
            config.FastBalancingTemperatureDecayRate,
            containsDefragMetric,
            searchType.Action);
    }
    else if (searchType.Action == PLBSchedulerActionType::LoadBalancing)
    {
        return SearchForBalancing(
            false,
            config.MaxPercentageToMove,
            config.SlowBalancingSearchTimeout,
            static_cast<uint64>(config.MaxSimulatedAnnealingIterations),
            config.SimulatedAnnealingIterationsPerRound,
            config.SlowBalancingTemperatureDecayRate,
            containsDefragMetric,
            searchType.Action);
    }
    else
    {
        Assert::CodingError("Invalid search type");
    }
}

bool Searcher::IsInterrupted()
{
    return toStop_.load();
}

void Searcher::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    PLBConfig const& config = PLBConfig::GetConfig();

    writer.WriteLine("YieldDurationPer10ms:{0}", sleepTimePer10ms_);
    writer.WriteLine("PLBRefreshInterval:{0}", config.PLBRefreshInterval);
    writer.WriteLine("PlacementSearchTimeout:{0}", config.PlacementSearchTimeout);
    writer.WriteLine("FastBalancingSearchTimeout:{0}", config.FastBalancingSearchTimeout);
    writer.WriteLine("SlowBalancingSearchTimeout:{0}", config.SlowBalancingSearchTimeout);
    writer.WriteLine("LoadBalancingEnabled:{0}", config.LoadBalancingEnabled);
    writer.WriteLine("MaxSimulatedAnnealingIterations:{0}", config.MaxSimulatedAnnealingIterations);
    writer.WriteLine("MaxPercentageToMove:{0}", config.MaxPercentageToMove);
    writer.WriteLine("FastBalancingTemperatureDecayRate:{0}", config.FastBalancingTemperatureDecayRate);
    writer.WriteLine("SlowBalancingTemperatureDecayRate:{0}", config.SlowBalancingTemperatureDecayRate);
    writer.WriteLine("IgnoreCostInScoring:{0}", config.IgnoreCostInScoring);
    writer.WriteLine("ScoreImprovementThreshold:{0}", config.ScoreImprovementThreshold);
    writer.WriteLine("IsTestMode:{0}", config.IsTestMode);
    writer.WriteLine("MetricActivityThresholds:{0}", config.MetricActivityThresholds);
    writer.WriteLine("MetricBalancingThresholds:{0}", config.MetricBalancingThresholds);
    writer.WriteLine("GlobalMetricWeights:{0}", config.GlobalMetricWeights);
    writer.WriteLine("DefragmentationMetrics:{0}", config.DefragmentationMetrics);
    writer.Write("{0}", *placement_);
}

wstring Searcher::ToString() const
{
    wstring result;
    StringWriter writer(result);
    writer.Write("{0}", *this);
    return result;
}

//-----------------------------
// private functions
struct Searcher::PlacementSolution
{
    DENY_COPY(PlacementSolution);

public:
    TempSolution solution_;
    int maxConstraintPriority_;

    PlacementSolution(TempSolution && solution, int maxConstraintPriority)
        : solution_(move(solution)), maxConstraintPriority_(maxConstraintPriority)
    {
    }

    PlacementSolution(PlacementSolution && other)
        : solution_(move(other.solution_)), maxConstraintPriority_(other.maxConstraintPriority_)
    {
    }

    PlacementSolution& operator=(PlacementSolution && other)
    {
        if (this != &other)
        {
            solution_ = move(other.solution_);
            maxConstraintPriority_ = other.maxConstraintPriority_;
        }

        return *this;
    }
};

void Searcher::SearchForUpgrade(CandidateSolution& solution)
{
    std::vector<PartitionEntry const*> upgradePartitionsToBePlaced;
    std::vector<PartitionEntry const*> upgradePartitionsSwapPrimary;
    int countLimit = ((countLimit = static_cast<int>(placement_->PartitionCount)) < 0) ? -1 : countLimit;

    BoundedSet<Common::Guid> notInUpgradePartitions(countLimit);
    BoundedSet<Common::Guid> noUpgradeFlagsFoundPartitions(countLimit);

    for (size_t i = 0; i < placement_->PartitionCount; ++i)
    {
        PartitionEntry const& partition = placement_->SelectPartition(i);
        if (!partition.IsInUpgrade ||
            partition.SingletonReplicaUpgradeOptimization != PartitionEntry::SingletonReplicaUpgradeOptimizationType::None)
        {
            notInUpgradePartitions.Add(partition.PartitionId);
            continue;
        }

        // For partition in upgrade:
        // IsPrimaryToBeSwappedOut - I
        if (partition.ExistsSwapOutLocation)
        {
            upgradePartitionsSwapPrimary.push_back(&partition);
        }
        // IsPrimaryToBePlaced - J, or IsReplicaToBePlaced - K:
        else if (partition.ExistsUpgradeLocation)
        {
            upgradePartitionsToBePlaced.push_back(&partition);
        }
        else
        {
            noUpgradeFlagsFoundPartitions.Add(partition.PartitionId);
        }
    }

    if (notInUpgradePartitions.Size())
    {
        trace_.SearchForUpgradePartitionIgnored(notInUpgradePartitions, L" they are not in upgrade.");
        notInUpgradePartitions.Clear();
    }
    if (noUpgradeFlagsFoundPartitions.Size())
    {
        trace_.SearchForUpgradePartitionIgnored(noUpgradeFlagsFoundPartitions, L" no upgrade flags were found.");
        noUpgradeFlagsFoundPartitions.Clear();
    }

    TempSolution tempSolution(solution);

    int maxPriority = checker_->GetMaxPriority();
    if (maxPriority < 0)
    {
        maxPriority = 0;
    }

    std::vector<PartitionEntry const*> unplacedPartitions;
    if (!upgradePartitionsToBePlaced.empty())
    {
        trace_.Searcher(L"Start searching for upgrade ToBePlaced");

        for (int priority = maxPriority; priority >= 0; --priority)
        {
            checker_->CheckUpgradePartitionsToBePlaced(tempSolution, upgradePartitionsToBePlaced, random_, priority, unplacedPartitions);
            if (unplacedPartitions.empty())
            {
                break;
            }

            upgradePartitionsToBePlaced.clear();
            upgradePartitionsToBePlaced.swap(unplacedPartitions);
        }

        trace_.Searcher(wformatString("End searching of upgrade ToBePlaced with {0} moves generated", tempSolution.ValidMoveCount));

        if (tempSolution.ValidMoveCount > 0)
        {
            solution.ApplyChange(tempSolution);
            tempSolution.Clear();
        }
        else
        {
            trace_.SearchForUpgradeUnsucess(upgradePartitionsToBePlaced[0]->PartitionId, L"ToBePlaced");
        }
    }

    if (!upgradePartitionsSwapPrimary.empty())
    {
        size_t swapPartitionCount = upgradePartitionsSwapPrimary.size();
        trace_.Searcher(L"Start searching for upgrade SwapPrimary");

        // First run the check with node capacity
        bool relaxCapacity = false;
        bool retry = false;
        bool trace = false;

        //Processing Unswapped Primary Partitions
        auto ProcessUnswappedPrimaryUpgradePartitions = [&]() -> void
        {
            if (retry)
            {
                upgradePartitionsSwapPrimary.clear();
                upgradePartitionsSwapPrimary.swap(unplacedPartitions);
            }

            if (trace)
            {
                checker_->TraceCheckSwapPrimaryUpgradePartition(
                    tempSolution,
                    upgradePartitionsSwapPrimary,
                    random_,
                    unplacedPartitions,
                    relaxCapacity);
            }
            else
            {
                checker_->CheckSwapPrimaryUpgradePartition(
                    tempSolution,
                    upgradePartitionsSwapPrimary,
                    random_,
                    unplacedPartitions,
                    relaxCapacity);
            }
        };

        ProcessUnswappedPrimaryUpgradePartitions();

        if (!unplacedPartitions.empty())
        {
            TraceUpgradeSwapPrimary(unplacedPartitions, false);

            if (PLBConfig::GetConfig().RelaxCapacityConstraintForUpgrade)
            {
                relaxCapacity = true;
                retry = true;
                trace = false;

                ProcessUnswappedPrimaryUpgradePartitions();

                if (!unplacedPartitions.empty())
                {
                    TraceUpgradeSwapPrimary(unplacedPartitions, true);
                }
            }
        }

        trace_.Searcher(wformatString("End searching of upgrade SwapPrimary for {0} partitions; Can't find swap solution for {1} partitions.",
            swapPartitionCount, unplacedPartitions.size()));

        if (tempSolution.ValidMoveCount > 0)
        {
            solution.ApplyChange(tempSolution);
            tempSolution.Clear();
        }

        if (!unplacedPartitions.empty())
        {
            relaxCapacity = true;
            retry = true;
            trace = true;
            plbDiagnosticsSPtr_->ExecuteUnderUSPDTLock(ProcessUnswappedPrimaryUpgradePartitions);
        }
    }
}

void Searcher::TraceUpgradeSwapPrimary(vector<PartitionEntry const*> const& unplacedPartitions, bool relaxCapacity) const
{
    // Trace out the partitions that a swapPrimary solution couldn't be found
    BoundedSet<PlacementReplica const*> invalidPartitions(PLBConfig::GetConfig().MaxInvalidReplicasToTrace);

    for (size_t i = 0; i < unplacedPartitions.size() && invalidPartitions.CanAdd(); ++i)
    {
        PlacementReplica const* p = unplacedPartitions[i]->PrimaryReplica;
        if (p)
        {
            invalidPartitions.Add(p);
        }
    }

    if (!invalidPartitions.IsEmpty())
    {
        trace_.SearcherUnsuccessfulSwapPrimary(invalidPartitions, relaxCapacity);
    }
}

void Searcher::SearchForExtraReplicas(CandidateSolution &solution)
{
    int maxPriority = checker_->GetMaxPriority();
    if (maxPriority < 0)
    {
        maxPriority = 0;
    }

    trace_.Searcher(wformatString("Start searching for removal of {0} extra replicas", placement_->ExtraReplicasCount));
    TempSolution tempSolution(solution);

    for (int priority = maxPriority; priority >= 0; priority--)
    {
        checker_->DropExtraReplicas(tempSolution, random_, priority);
        if (tempSolution.ValidMoveCount > 0)
        {
            // Do not relax constraints any more if we can find partial solution.
            break;
        }
    }

    solution.ApplyChange(tempSolution);

    trace_.Searcher(wformatString("Search for dropping extra replicas completed with {0} drops found", tempSolution.ValidMoveCount));
}

bool Searcher::UseBatchPlacement() const
{
    if (placement_->Settings.UseBatchPlacement && placement_->NewReplicaCount > placement_->Settings.PlacementReplicaCountPerBatch)
    {
        return true;
    }

    return false;
}

CandidateSolution Searcher::SearchForPlacement(
    double maxPercentageToMove,
    TimeSpan timeout,
    uint64 maxRound,
    size_t placementSearchIterationsPerRound,
    size_t simuAnnealingTransitionsPerRound,
    double temperatureDecayRatio,
    bool creationWithMove,
    PLBSchedulerActionType::Enum currentSchedulerAction,
    size_t noChangeRoundToExit,
    double diffEachRound)
{
    // search for the to be created replicas, to be dropped replicas or to be swapped replicas
    // there can be constraint violations initially

    trace_.Searcher(L"Start searching for initial creation actions");
    StopwatchTime now = Stopwatch::Now();
    StopwatchTime placementEnd = now + timeout;

    // 1/5 of time for random placement
    TimeSpan RandomWalkTimeout = timeout / 5;

    size_t numberOfMigrations = placement_->PartitionsInUpgradeCount + placement_->ExtraReplicasCount;
    if (placement_->PartitionsInUpgradePlacementCount > 0)
    {
        numberOfMigrations += placement_->PartitionCount;
    }

    CandidateSolution solution(
        &(*placement_),
        vector<Movement>(placement_->NewReplicaCount),
        vector<Movement>(numberOfMigrations),
        currentSchedulerAction);

    if (placement_->NewReplicaCount == 0 && placement_->PartitionsInUpgradeCount == 0 && placement_->ExtraReplicasCount == 0)
    {
        return solution;
    }

    // Special logic for large number of placement
    int maxConstraintPriority;

    if (UseBatchPlacement())
    {
        trace_.SearcherBatchPlacement(placement_->NewReplicaCount, placement_->Settings.PlacementReplicaCountPerBatch);

        RandomWalkPlacement(solution, maxConstraintPriority, now + timeout, maxRound, placementSearchIterationsPerRound);

        return solution;
    }

    if (placement_->ExtraReplicasCount != 0)
    {
        SearchForExtraReplicas(solution);
    }

    auto ProcessDiagnosticsForUnplacedReplicas = [&]() -> void
    {
        BoundedSet<PlacementReplica const*> unplacedReplicas = solution.GetUnplacedReplicas();
        if (!unplacedReplicas.IsEmpty())
        {
            //Generates the diagnostics reasons/tracing for the inability to place the replica
            TraceWalkPlacement(solution);
            trace_.SearcherUnsuccessfulPlacement(placement_->NewReplicaCount - solution.ValidAddCount, placement_->NewReplicaCount, unplacedReplicas);
        }
    };

    // This check is to make sure that when placement is occurring after Checker selects the highest node ID,
    // the placement is not then scrambled by optimization
    if (placement_->Settings.DummyPLBEnabled)
    {
        DummyWalkPlacement(solution, maxConstraintPriority);
        if (placement_->PartitionsInUpgradeCount != 0)
        {
            // Search solution for upgrade
            SearchForUpgrade(solution);
        }

        plbDiagnosticsSPtr_->ExecuteUnderPDTLock(ProcessDiagnosticsForUnplacedReplicas);

        return solution;
    }

    uint64 placementRound = maxRound;
    if (PLBConfig::GetConfig().MaxSimulatedAnnealingIterations != -1)
    {
        placementRound = maxRound / 5 + 1;
    }

    RandomWalkPlacement(solution, maxConstraintPriority, now + RandomWalkTimeout, placementRound, placementSearchIterationsPerRound);

    // If couldn't place the new replica in the regular run
    // If there is partition in upgrade, PlacementWithMove would casue error in the movement index
    if (solution.ValidMoveCount == 0 &&
        creationWithMove &&
        placement_->CanNewReplicasBePlaced() &&
        placement_->PartitionsInUpgradeCount == 0 &&
        placement_->PartitionsInUpgradePlacementCount == 0)
    {
        // use 1/5 of the time to try the placement by moving the existing replica (partial closure first)
        // if nothing placed, use 1/5 of the time to try the placement by moving the existing replica (full closure)
        size_t maxNumberOfMigrations = GetMoveNumber(maxPercentageToMove);
        if (maxNumberOfMigrations == 0)
        {
            //if we can't make any movements we skip this phase..
            trace_.Searcher(L"NewReplicaPlacementWithMove was skipped due to movement threshold being met.");
        }
        else
        {
            trace_.Searcher(L"Start searching for initial creation actions with existing replica movement (Partial closure)");

            CandidateSolution solutionWithRandomMove(
                &(*placement_),
                vector<Movement>(placement_->NewReplicaCount),
                vector<Movement>(maxNumberOfMigrations),
                currentSchedulerAction);

            // try NewReplicaPlacementWithMove with partial closure
            StopwatchTime randomPlaceEnd = now + RandomWalkTimeout;
            RandomWalkPlacementWithReplicaMove(
                solutionWithRandomMove,
                maxConstraintPriority,
                randomPlaceEnd + RandomWalkTimeout,
                placementRound,
                placementSearchIterationsPerRound,
                false,
                true);

            // try NewReplicaPlacementWithMove with full closure without totalRandom
            // totalRandom false means: only the random move that can improve the placement would be adopted
            if (solutionWithRandomMove.ValidMoveCount == 0)
            {
                trace_.Searcher(L"Start searching for initial creation actions with existing replica movement (Full closure)");
                randomPlaceEnd += RandomWalkTimeout;
                RandomWalkPlacementWithReplicaMove(
                    solutionWithRandomMove,
                    maxConstraintPriority,
                    randomPlaceEnd + RandomWalkTimeout,
                    placementRound,
                    placementSearchIterationsPerRound,
                    false,
                    false);
            }

            // try NewReplicaPlacementWithMove with full closure and use total random to see if we can get better result
            if (solutionWithRandomMove.ValidMoveCount == 0)
            {
                randomPlaceEnd += RandomWalkTimeout;
                RandomWalkPlacementWithReplicaMove(
                    solutionWithRandomMove,
                    maxConstraintPriority,
                    randomPlaceEnd + RandomWalkTimeout,
                    placementRound,
                    placementSearchIterationsPerRound,
                    true,
                    false);
            }

            if (solutionWithRandomMove.IsAnyReplicaPlaced())
            {
                solution = move(solutionWithRandomMove);
            }
        }
    }
    else if (solution.ValidAddCount > 0)
    {
        StopwatchTime balancingStart = Stopwatch::Now();
        if (balancingStart >= placementEnd)
        {
            trace_.Searcher(L"Skip balancing of creation with no time left");
        }
        else
        {
            // if the valid move count is > 0, do an LB for the placed replicas
        // use 4/5 of the time to balance the initial placement
            trace_.Searcher(L"Start balancing the creation actions");

            vector<SimulatedAnnealingSolution> solutions;

            AddSimulatedAnnealingSolution(solution, solutions, false, true, maxConstraintPriority, false);

            solution = SimulatedAnnealing(
                move(solutions),
                placementEnd,
                maxRound,
                simuAnnealingTransitionsPerRound,
                temperatureDecayRatio,
                noChangeRoundToExit,
                diffEachRound);
        }
    }

    bool isAffinityRelaxed = PLBConfig::GetConfig().RelaxAffinityConstraintDuringUpgrade;

    if (!balancingEnabled_.load() && !solution.IsAnyReplicaPlaced() && isAffinityRelaxed)
    {
        // If balancing is disabled (e.g. cluster upgrade is in progress) and nothing is placed
        // try placement with affinity relaxed
        placementEnd += RandomWalkTimeout;

        RandomWalkPlacement(
            solution,
            maxConstraintPriority,
            placementEnd,
            placementRound,
            placementSearchIterationsPerRound,
            true);
    }

    // If maxConstraintPriority == 0, all soft constraint has been relaxed, should skip
    // If there is still replica can't be placed, soft constraint should be relaxed to see if they can be placed.
    if (placement_->Settings.RelaxConstraintForPlacement && maxConstraintPriority > 0 && !solution.AllReplicasPlaced())
    {
        for (int maxPriorityToUse = maxConstraintPriority - 1; maxPriorityToUse >= 0; maxPriorityToUse--)
        {
            checker_->SetMaxPriorityToUse(maxPriorityToUse);

            placementEnd += RandomWalkTimeout;

            RandomWalkPlacement(
                solution,
                maxPriorityToUse,
                placementEnd,
                placementRound,
                placementSearchIterationsPerRound,
                false);

            if (solution.AllReplicasPlaced())
            {
                break;
            }
        }

    }

    // Diagnostics bookkeeping for uplaced replicas
    plbDiagnosticsSPtr_->ExecuteUnderPDTLock(ProcessDiagnosticsForUnplacedReplicas);

    for (auto it = solution.Creations.begin(); it != solution.Creations.end(); ++it)
    {
        //Nullptr checks
        if ((it->TargetToBeAddedReplica != nullptr))
        {
            //Diagnostic Bookkeeping
            plbDiagnosticsSPtr_->TrackPlacedReplica(it->TargetToBeAddedReplica);
        }
    }

    if (placement_->PartitionsInUpgradeCount != 0)
    {
        // Search solution for upgrade
        SearchForUpgrade(solution);
    }

    // Trace the solution search insight if exists
    if (solution.HasSearchInsight())
    {
        trace_.PLBSearchInsight(solution.SolutionSearchInsight);
    }

    return solution;
}

void Searcher::RandomWalkPlacementWithReplicaMove(
    CandidateSolution & solution,
    int & maxConstraintPriority,
    StopwatchTime endTime,
    uint64 maxRound,
    size_t transitionPerRound,
    bool totalRandom,
    bool usePartialClosure)
{
    maxConstraintPriority = 0;
    size_t currentBestPlacementCount = 0;
    size_t placementCount = 0;
    uint64 totalIterations = 0;
    uint64 totalTransitions = 0;

    StopwatchTime lastStartTime = Stopwatch::Now();

    TempSolution tempSolution(solution);
    for (uint64 round = 0; round < maxRound && !toStop_.load() && currentBestPlacementCount < solution.OriginalPlacement->NewReplicaCount; ++round)
    {
        StopwatchTime now = Stopwatch::Now();
        if (round != 0 && now >= endTime)
        {
            break;
        }

        TimeSpan duration = now - lastStartTime;
        if (duration >= TimeSpan::FromMilliseconds(static_cast<double>(10 - sleepTimePer10ms_)))
        {
            Sleep(static_cast<DWORD>(sleepTimePer10ms_));
            lastStartTime = now;
        }

        for (size_t transition = 0; transition < transitionPerRound; ++transition)
        {
            bool ret = checker_->MoveSolutionRandomly(tempSolution, false, false, maxConstraintPriority, false, random_, true, usePartialClosure);
            if (!ret)
            {
                if (!tempSolution.IsEmpty)
                {
                    tempSolution.Clear();
                }
                continue;
            }

            placementCount = CountPlaceNewReplica(tempSolution, maxConstraintPriority);
            // If there is no placement found yet, the random move should be saved in totalRandom mode
            if (placementCount > currentBestPlacementCount || (currentBestPlacementCount == 0 && totalRandom))
            {
                currentBestPlacementCount = placementCount;
                solution.ApplyMoveChange(tempSolution);
                ++totalTransitions;
            }

            tempSolution.Clear();
            ++totalIterations;
        }

    }

    if (currentBestPlacementCount > 0)
    {
        bool allPlaced = false;
        for (size_t placeRound = 0; placeRound < transitionPerRound; ++placeRound)
        {
            tempSolution.Clear();
            placementCount = CountPlaceNewReplica(tempSolution, maxConstraintPriority);
            if (placementCount >= currentBestPlacementCount)
            {
                // Find the solution
                allPlaced = true;
                solution.ApplyChange(tempSolution);
                break;
            }
        }

        if (!allPlaced && placementCount > 0)
        {
            solution.ApplyChange(tempSolution);
        }

    }

    if (totalRandom)
    {
        trace_.Searcher(wformatString("Search of placement with total random replica move completed with {0} total iterations in total round {1}", totalIterations, totalTransitions));
    }
    else
    {
        trace_.Searcher(wformatString("Search of placement with replica move completed with {0} total iterations in total round {1}", totalIterations, totalTransitions));
    }

}

size_t Searcher::CountPlaceNewReplica(TempSolution & solution, int maxConstraintPriority)
{
    size_t moveCountBeforePlace = solution.ValidMoveCount;
    checker_->PlaceNewReplicas(solution, random_, maxConstraintPriority);

    return (solution.ValidMoveCount - moveCountBeforePlace);
}

void Searcher::InitialWalk(
    CandidateSolution & solution,
    vector<PlacementSolution> & solutions,
    int & maxPriority,
    bool relaxAffinity  /*= false */,
    bool enableVerboseTracing /* = false */,
    bool modifySolution /* = true */)
{
    maxPriority = checker_->GetMaxPriority();

    if (maxPriority < 0 || relaxAffinity || ((!modifySolution) && enableVerboseTracing))
    {
        maxPriority = 0;
    }

    bool anyPlaced = false;
    for (int i = maxPriority; i >= 0 && !anyPlaced; --i)
    {
        TempSolution bestTempSolution(solution);
        checker_->PlaceNewReplicas(bestTempSolution, random_, i, enableVerboseTracing, relaxAffinity, modifySolution);

        anyPlaced = (bestTempSolution.ValidAddCount > 0);

        solutions.push_back(PlacementSolution(move(bestTempSolution), i));
    }

}

void Searcher::RandomWalking(
    CandidateSolution & solution,
    std::vector<PlacementSolution> & solutions,
    Common::StopwatchTime endTime,
    uint64 maxRound,
    size_t transitionPerRound,
    bool relaxAffinity /* = false */)
{
    StopwatchTime lastStartTime = Stopwatch::Now();

    TempSolution tempSolution(solution);

    uint64 totalIterations = 0;
    uint64 totalTransitions = 0;
    for (uint64 round = 0; round < maxRound && !toStop_.load() && IsRunning(solutions); ++round)
    {
        StopwatchTime now = Stopwatch::Now();
        if (round != 0 && now >= endTime)
        {
            break;
        }

        TimeSpan duration = now - lastStartTime;
        if (duration >= TimeSpan::FromMilliseconds(static_cast<double>(10 - sleepTimePer10ms_)))
        {
            Sleep(static_cast<DWORD>(sleepTimePer10ms_));
            lastStartTime = now;
        }

        for (size_t solutionIndex = 0; solutionIndex < solutions.size(); ++solutionIndex)
        {
            PlacementSolution & currentPlacementSolution = solutions[solutionIndex];

            for (size_t transition = 0; transition < transitionPerRound; ++transition)
            {
                checker_->PlaceNewReplicas(tempSolution, random_, currentPlacementSolution.maxConstraintPriority_, false, relaxAffinity, true);
                if (tempSolution.ValidAddCount > currentPlacementSolution.solution_.ValidAddCount)
                {
                    currentPlacementSolution.solution_ = move(tempSolution);
                    ++totalTransitions;
                }

                tempSolution.Clear();
                ++totalIterations;
            }

            if (currentPlacementSolution.solution_.ValidMoveCount > 0)
            {
                break;
            }
        }
    }

    trace_.Searcher(wformatString("Search of placement completed with {0} total iterations and {1} total transitions", totalIterations, totalTransitions));

}

void Searcher::ChooseWalk(
    CandidateSolution & solution,
    std::vector<PlacementSolution> & solutions,
    int & maxConstraintPriority,
    int maxPriority,
    bool modifySolution /* = true */)
{
    auto itBest = solutions.begin();
    for (; itBest != solutions.end(); ++itBest)
    {
        if (itBest->solution_.ValidAddCount > 0)
        {
            break;
        }
    }

    if (itBest != solutions.end())
    {
        maxConstraintPriority = itBest->maxConstraintPriority_;

        if (modifySolution)
        {
            solution.ApplyChange(itBest->solution_);
        }
    }
    else
    {
        maxConstraintPriority = maxPriority;
    }

    trace_.Searcher(wformatString("Search of placement completed with maxConstraintPriority={0}", maxConstraintPriority));
}

void Searcher::TraceWalkPlacement(
    CandidateSolution & solution)
{
    int maxPriority, traceConstraintPriority;
    vector<PlacementSolution> solutions;

    InitialWalk(solution, solutions, maxPriority, false, true, false);
    ChooseWalk(solution, solutions, traceConstraintPriority, maxPriority, false);
}

void Searcher::DummyWalkPlacement(
    CandidateSolution & solution,
    int & maxConstraintPriority)
{
    int maxPriority;
    vector<PlacementSolution> solutions;

    InitialWalk(solution, solutions, maxPriority, false, true);

    trace_.Searcher(wformatString("Search of dummy placement completed"));

    ChooseWalk(solution, solutions, maxConstraintPriority, maxPriority);
}

void Searcher::RandomWalkPlacement(
    CandidateSolution & solution,
    int & maxConstraintPriority,
    StopwatchTime endTime,
    uint64 maxRound,
    size_t transitionPerRound,
    bool relaxAffinity /* = false */)
{
    int maxPriority;
    vector<PlacementSolution> solutions;

    InitialWalk(solution, solutions, maxPriority, relaxAffinity);

    if (!UseBatchPlacement())
    {
        // No random walking for batch placement
        RandomWalking(solution, solutions, endTime, maxRound, transitionPerRound, relaxAffinity);
    }

    ChooseWalk(solution, solutions, maxConstraintPriority, maxPriority);
}

bool Searcher::IsRunning(vector<PlacementSolution> const & solutions)
{
    ASSERT_IF(solutions.empty(), "Empty solution");

    PlacementSolution const& solution = solutions.front();

    return (solution.solution_.ValidMoveCount < solution.solution_.OriginalPlacement->NewReplicaCount);
}

struct Searcher::ConstraintCheckSolution
{
    DENY_COPY(ConstraintCheckSolution);

public:
    TempSolution solution_;
    ViolationList violations_;
    int maxConstraintPriority_;
    double bestEnergy_;
    bool preferSwap_;

    ConstraintCheckSolution(TempSolution && solution, ViolationList violations, int maxConstraintPriority, double bestEnergy, bool preferSwap)
        : solution_(move(solution)), violations_(move(violations)), maxConstraintPriority_(maxConstraintPriority), bestEnergy_(bestEnergy), preferSwap_(preferSwap)
    {
    }

    ConstraintCheckSolution(ConstraintCheckSolution && other)
        : solution_(move(other.solution_)), violations_(move(other.violations_)), maxConstraintPriority_(other.maxConstraintPriority_), bestEnergy_(other.bestEnergy_), preferSwap_(other.preferSwap_)
    {
    }

    ConstraintCheckSolution& operator=(ConstraintCheckSolution && other)
    {
        if (this != &other)
        {
            solution_ = move(other.solution_);
            violations_ = move(other.violations_);
            maxConstraintPriority_ = other.maxConstraintPriority_;
            bestEnergy_ = other.bestEnergy_;
            preferSwap_ = other.preferSwap_;
        }

        return *this;
    }
};

CandidateSolution Searcher::SearchForConstraintCheck(
    TimeSpan timeout,
    uint64 maxRound,
    size_t constraintCheckIterationsPerRound,
    size_t simuAnnealingTransitionsPerRound,
    double temperatureDecayRatio,
    PLBSchedulerAction const & searchType,
    size_t noChangeRoundToExit,
    double diffEachRound)
{
    // search for constraint violations and try to correct them
    // there can be to be created replicas

    StopwatchTime now = Stopwatch::Now();

    CandidateSolution solution(
        &(*placement_),
        vector<Movement>(),
        vector<Movement>(placement_->ExistingReplicaCount),
        searchType.Action);

    // use 1/5 of the time to do constraint check
    int maxConstraintPriority;
    RandomWalkConstraintCheck(solution, maxConstraintPriority, now + timeout / 5, maxRound / 5, constraintCheckIterationsPerRound, searchType.IsConstraintCheckLight);
    maxRound -= maxRound / 5;

    auto ProcessDiagnosticsForUnfixedConstraintViolations = [&]() -> void
    {
        TempSolution tempSolution(solution);
        ViolationList remainingViolations = checker_->CheckSolution(tempSolution, maxConstraintPriority, false, random_);

        if (!remainingViolations.IsEmpty())
        {
            //For diagnostics we'll actually trace the behavior of {Inner]Movesolution instead of [Inner]Correct Solution, because
            //The initial bit of [Inner]Correct Solution that runs before it calls MoveSolution
            //does not act in a reliably repro-able way to conduct diagnostics on, as it is a
            //random local heuristic rather than a global filter, which would give some info about the
            //global state of the cluster, as it impacts the attempted constraint fixing here.

            //For diagnostic method name consistency and public scope accessibility, however, the method Name
            //We will add tracing for is CorrectSolution

            //Calling this method with this parameter picks out the particular partitions that need fixing
            std::vector<std::shared_ptr<IConstraintDiagnosticsData>> constraintsDiagnosticsData;
            set<Guid> invalidPartitions = checker_->CheckSolutionForInvalidPartitions(tempSolution, Checker::DiagnosticsOption::ConstraintFixUnsuccessful, constraintsDiagnosticsData, random_);

            if (!invalidPartitions.empty())
            {
                    //Perform Diagnostics Methods
                    //Add to Health Reporting Queue if necessary

                    bool diagnosticsSucceeded = !checker_->DiagnoseCorrectSolution(tempSolution, random_);

                    if (!diagnosticsSucceeded)
                    {
                        for (auto const q : invalidPartitions)
                        {
                            trace_.ConstraintFixDiagnosticsFailed(q);
                        }
                    }
            }

            //Move the table data out of the cache into the proper table to clean out old fixed constraint violations
            //Also cleans up the replica pointers that we were storing to have fast access to diagnostics
            plbDiagnosticsSPtr_->CleanupConstraintDiagnosticsCaches();
        }
    };

    ProcessDiagnosticsForUnfixedConstraintViolations();

    //temperatureDecayRatio;
    //noChangeRoundToExit;
    //diffEachRound;
    if (solution.ValidMoveCount > 0)
    {
        // use 4/5 of the time to balance the constraint check
        trace_.Searcher(L"Start balancing the constraint check actions");
        vector<SimulatedAnnealingSolution> solutions;
        AddSimulatedAnnealingSolution(solution, solutions, false, true, maxConstraintPriority, false);
        CandidateSolution balancedSolution = SimulatedAnnealing(
            move(solutions),
            now + timeout,
            maxRound,
            simuAnnealingTransitionsPerRound,
            temperatureDecayRatio,
            noChangeRoundToExit,
            diffEachRound);

        if (balancedSolution.ValidMoveCount <= solution.ValidMoveCount && balancedSolution.AvgStdDev <= solution.AvgStdDev)
        {
            solution = move(balancedSolution);
        }
    }

    return solution;
}

void Searcher::RandomWalkConstraintCheck(CandidateSolution & solution, int & maxConstraintPriority, StopwatchTime endTime, uint64 maxRound, size_t transitionPerRound, bool constraintFixLight)
{

    int maxPriority = checker_->GetMaxPriority();
    if (maxPriority < 0)
    {
        maxPriority = 0;
    }

    trace_.Searcher(wformatString("Start searching for constraint violations with max priority {0}", maxPriority));
    TempSolution tempSolution(solution);
    ViolationList remainingViolations = checker_->CheckSolution(tempSolution, maxPriority, false, random_);
    if (remainingViolations.IsEmpty())
    {
        trace_.Searcher(L"No violation found");
        maxConstraintPriority = maxPriority;
        return;
    }

    trace_.SearcherConstraintViolationFound(remainingViolations);

    vector<ConstraintCheckSolution> solutions;
    bool anyCorrected = false;
    bool useScoreInConstraintCheck = placement_->Settings.UseScoreInConstraintCheck;
    int swappingLimit = placement_->Settings.EnablePreferredSwapSolutionInConstraintCheck ? 1 : 0;

    for (int i = maxPriority; i >= 0 && !anyCorrected; --i)
    {
        for (int swapping = 0; swapping <= swappingLimit; ++swapping)
        {
            bool shouldSwap = swapping != 0;

            // TODO: no need to get initial violation list if we are sure CorrectSolution won't make a solution worse
            TempSolution bestTempSolution(solution);
            ViolationList bestRemainingViolations = checker_->CheckSolution(bestTempSolution, i, false, random_);
            double bestEnergy = solution.Energy;

            if (bestRemainingViolations.IsEmpty())
            {
                break;
            }

            tempSolution.Clear();
            tempSolution.IsSwapPreferred = shouldSwap;

            if (checker_->CorrectSolution(tempSolution, random_, i, constraintFixLight, 0, transitionPerRound) && tempSolution.ValidMoveCount > 0)
            {
                // TODO: directly return ViolationList from CorrectSolution?
                ViolationList currentRemainingViolations = checker_->CheckSolution(tempSolution, i, false, random_);
                int compareResult = currentRemainingViolations.CompareViolation(bestRemainingViolations);

                CandidateSolution const& baseSolution = tempSolution.BaseSolution;

                if (compareResult <= 0)
                {
                    Score score = baseSolution.TryChange(tempSolution);

                    bool improvedResult = useScoreInConstraintCheck
                        ? (compareResult < 0 || score.Energy < bestEnergy)
                        : (compareResult < 0 || tempSolution.ValidMoveCount < bestTempSolution.ValidMoveCount);

                    if (improvedResult)
                    {
                        bestTempSolution = move(tempSolution);
                        bestRemainingViolations = move(currentRemainingViolations);
                        bestEnergy = score.Energy;

                        if (bestRemainingViolations.CompareViolation(remainingViolations) < 0)
                        {
                            anyCorrected = true;
                        }
                    }

                    baseSolution.UndoChange(tempSolution);
                }
            }

            solutions.push_back(ConstraintCheckSolution(move(bestTempSolution), move(bestRemainingViolations), i, bestEnergy, shouldSwap));
        }
    }

    StopwatchTime lastStartTime = Stopwatch::Now();
    uint64 totalIterations = 0;
    uint64 totalTransitions = 0;
    for (uint64 round = 0; round < maxRound && !toStop_.load() && IsRunning(solutions); ++round)
    {
        int bestCurrentPriority = INT_MIN;

        StopwatchTime now = Stopwatch::Now();
        if (now >= endTime)
        {
            break;
        }

        TimeSpan duration = now - lastStartTime;
        if (duration >= TimeSpan::FromMilliseconds(static_cast<double>(10 - sleepTimePer10ms_)))
        {
            Sleep(static_cast<DWORD>(sleepTimePer10ms_));
            lastStartTime = now;
        }

        for (size_t solutionIndex = 0; solutionIndex < solutions.size(); ++solutionIndex)
        {
            ConstraintCheckSolution & currentConstraintCheckSolution = solutions[solutionIndex];
            CandidateSolution const& baseSolution = tempSolution.BaseSolution;

            //solutions are ordered by priority so we can break here
            if (currentConstraintCheckSolution.maxConstraintPriority_ < bestCurrentPriority)
            {
                break;
            }

            for (size_t transition = 0; transition < transitionPerRound; ++transition)
            {
                tempSolution.Clear();
                tempSolution.IsSwapPreferred = currentConstraintCheckSolution.preferSwap_;

                if (checker_->CorrectSolution(tempSolution,
                                              random_,
                                              currentConstraintCheckSolution.maxConstraintPriority_,
                                              constraintFixLight,
                                              transition,
                                              transitionPerRound)
                    && tempSolution.ValidMoveCount > 0)
                {
                    ViolationList currentRemainingViolations = checker_->CheckSolution(
                        tempSolution,
                        currentConstraintCheckSolution.maxConstraintPriority_,
                        false,
                        random_);

                    int compareResult = currentRemainingViolations.CompareViolation(currentConstraintCheckSolution.violations_);
                    if (compareResult <= 0)
                    {
                        Score score = baseSolution.TryChange(tempSolution);

                        bool improvedResult = useScoreInConstraintCheck
                            ? (compareResult < 0 || score.Energy < currentConstraintCheckSolution.bestEnergy_)
                            : (compareResult < 0 || tempSolution.ValidMoveCount < currentConstraintCheckSolution.solution_.ValidMoveCount);

                        if (improvedResult)
                        {
                            currentConstraintCheckSolution.solution_ = move(tempSolution);
                            currentConstraintCheckSolution.violations_ = move(currentRemainingViolations);
                            currentConstraintCheckSolution.bestEnergy_ = score.Energy;
                            ++totalTransitions;
                        }

                        baseSolution.UndoChange(tempSolution);
                    }
                }

                ++totalIterations;
            }

            // We choose the solution with the highest priority that solves violations,
            // so if this solution decreases the amount of violations
            // there's no point in spending iterations on a solution with a smaller priority
            if (currentConstraintCheckSolution.violations_.CompareViolation(remainingViolations) < 0)
            {
                if (currentConstraintCheckSolution.maxConstraintPriority_ > bestCurrentPriority)
                {
                    bestCurrentPriority = currentConstraintCheckSolution.maxConstraintPriority_;
                }
            }
        }

    }

    trace_.Searcher(wformatString("Search of constraint check completed with {0} total iterations and {1} total transitions", totalIterations, totalTransitions));

    auto itBest = solutions.end();
    for (auto itCurrent = solutions.begin(); itCurrent != solutions.end(); ++itCurrent)
    {
        if (itCurrent->violations_.CompareViolation(remainingViolations) < 0)
        {
            //we do not have a best solution up to now
            if (itBest == solutions.end())
            {
                itBest = itCurrent;
            }
            //solutions are ordered by priority so we can break here
            //we always try to fix with the highest priority
            else if (itBest->maxConstraintPriority_ > itCurrent->maxConstraintPriority_)
            {
                break;
            }
            //let us check if more violations are fixed or energy/move count is better
            //there can be two solutions with the same priority - one which prefers swap and one which does not
            else
            {
                int compareResult = itCurrent->violations_.CompareViolation(itBest->violations_);
                bool improvedResult = useScoreInConstraintCheck
                    ? (compareResult < 0 || (compareResult == 0 && itCurrent->bestEnergy_ < itBest->bestEnergy_))
                    : (compareResult < 0 || (compareResult == 0 && itCurrent->solution_.ValidMoveCount < itBest->solution_.ValidMoveCount));

                if (improvedResult)
                {
                    itBest = itCurrent;
                }
            }
        }
    }

    if (itBest != solutions.end())
    {
        trace_.Searcher(wformatString("Constraint check best solution has priority={0} and preferSwap={1}", itBest->maxConstraintPriority_, itBest->preferSwap_));

        maxConstraintPriority = itBest->maxConstraintPriority_;
        solution.ApplyChange(itBest->solution_);
        if (itBest->violations_.IsEmpty())
        {
            trace_.Searcher(L"Constraint violation corrected");
        }
        else
        {
            trace_.SearcherConstraintViolationPartiallyCorrected(itBest->violations_);
        }
    }
    else
    {
        maxConstraintPriority = maxPriority;
        trace_.SearcherConstraintViolationNotCorrected(remainingViolations);
    }
}

bool Searcher::IsRunning(vector<ConstraintCheckSolution> const & solutions)
{
    ASSERT_IF(solutions.empty(), "Empty solution");

    ConstraintCheckSolution const& solution = solutions.front();

    return !(solution.violations_.IsEmpty() && solution.solution_.ValidMoveCount <= 1);
}

CandidateSolution Searcher::SearchForBalancing(
    bool useNodeLoadAsHeuristic,
    double maxPercentageToMove,
    TimeSpan timeout,
    uint64 maxRound,
    size_t transitionPerRound,
    double temperatureDecayRatio,
    bool containsDefragMetric,
    PLBSchedulerActionType::Enum currentSchedulerAction,
    size_t noChangeRoundToExit,
    double diffEachRound)
{
    // search for load balancing actions, there can be to be created replicas
    // there can be constraint violations too

    StopwatchTime endTime = Stopwatch::Now() + timeout;

    size_t maxNumberOfMigrations = GetMoveNumber(maxPercentageToMove);

    if (maxNumberOfMigrations == 0)
    {
        //if we can't make any movements we skip this round of simulated annealing
        trace_.Searcher(L"PLB run was skipped due to movement threshold being met..");
        return CandidateSolution(&(*placement_), vector<Movement>(), vector<Movement>(), currentSchedulerAction);
    }
    CandidateSolution solution(&(*placement_), vector<Movement>(), vector<Movement>(maxNumberOfMigrations), currentSchedulerAction);

    vector<SimulatedAnnealingSolution> solutions;
    int maxConstraintPriority = checker_->GetMaxPriority();

    // Add solutions with different search strategies, first solution is swap only, second solution is to enable soft constraints,
    // third solution is to skip soft constraints: priority >= 2
    // 4th solution is advanced heuristic for defrag

    //------------------------

    AddSimulatedAnnealingSolution(solution, solutions, true, useNodeLoadAsHeuristic, maxConstraintPriority, false);
    AddSimulatedAnnealingSolution(solution, solutions, false, useNodeLoadAsHeuristic, maxConstraintPriority, false);
        
    if (maxConstraintPriority > 1)
    {
        AddSimulatedAnnealingSolution(solution, solutions, false, useNodeLoadAsHeuristic, 1, false);

        if (containsDefragMetric && PLBConfig::GetConfig().RestrictedDefragmentationHeuristicEnabled)
        {
            AddSimulatedAnnealingSolution(solution, solutions, false, useNodeLoadAsHeuristic, 1, true);
        }
    }
    //------------------------

    CandidateSolution newSolution = SimulatedAnnealing(
        move(solutions),
        endTime,
        maxRound,
        transitionPerRound,
        temperatureDecayRatio,
        noChangeRoundToExit,
        diffEachRound);

    if (!containsDefragMetric)
    {
        ASSERT_IFNOT(newSolution.AvgStdDev >= 0.0 && solution.AvgStdDev >= 0.0, "Invalid stddev values {0} {1}", newSolution.AvgStdDev, solution.AvgStdDev);
    }

    double scoreImprovement = solution.AvgStdDev - newSolution.AvgStdDev;

    PLBConfig const& config = PLBConfig::GetConfig();
    if (scoreImprovement >= config.ScoreImprovementThreshold)
    {
        return newSolution;
    }
    else
    {
        plbDiagnosticsSPtr_->TrackBalancingFailure(solution, newSolution, domainId_);
        trace_.SearcherScoreImprovementNotAccepted(scoreImprovement, solution.AvgStdDev, newSolution.AvgStdDev, config.ScoreImprovementThreshold);
        return solution;
    }
}

struct Searcher::SimulatedAnnealingSolution
{
    DENY_COPY(SimulatedAnnealingSolution);

public:
    CandidateSolution solution_;
    bool swapOnly_;
    bool useNodeLoadAsHeuristic_;
    int maxConstraintPriority_;
    double temperature_;
    bool running_;
    size_t noBestRound_;
    double initialEnergy_;
    size_t noChangeRound_;
    bool useRestrictedDefrag_;

    SimulatedAnnealingSolution(CandidateSolution && solution, bool swapOnly, bool useNodeLoadAsHeuristic, int maxConstraintPriority, double temperature, bool useRestrictedDefrag)
        : solution_(std::move(solution)),
        swapOnly_(swapOnly),
        useNodeLoadAsHeuristic_(useNodeLoadAsHeuristic),
        maxConstraintPriority_(maxConstraintPriority),
        temperature_(temperature),
        running_(true),
        noBestRound_(0),
        initialEnergy_(0.0),
        noChangeRound_(0),
        useRestrictedDefrag_(useRestrictedDefrag)
    {
    }

    SimulatedAnnealingSolution(SimulatedAnnealingSolution && other)
        : solution_(std::move(other.solution_)),
        swapOnly_(other.swapOnly_),
        useNodeLoadAsHeuristic_(other.useNodeLoadAsHeuristic_),
        maxConstraintPriority_(other.maxConstraintPriority_),
        temperature_(other.temperature_),
        running_(other.running_),
        noBestRound_(other.noBestRound_),
        initialEnergy_(other.initialEnergy_),
        noChangeRound_(other.noChangeRound_),
        useRestrictedDefrag_(other.useRestrictedDefrag_)
    {
    }

    SimulatedAnnealingSolution& operator=(SimulatedAnnealingSolution && other)
    {
        if (this != &other)
        {
            solution_ = std::move(other.solution_);
            swapOnly_ = other.swapOnly_;
            useNodeLoadAsHeuristic_ = other.useNodeLoadAsHeuristic_;
            maxConstraintPriority_ = other.maxConstraintPriority_;
            temperature_ = other.temperature_;
            running_ = other.running_;
            noBestRound_ = other.noBestRound_;
            initialEnergy_ = other.initialEnergy_;
            noChangeRound_ = other.noChangeRound_;
            useRestrictedDefrag_ = other.useRestrictedDefrag_;
        }

        return *this;
    }
};

CandidateSolution Searcher::SimulatedAnnealing(
    vector<SimulatedAnnealingSolution> && solutions,
    StopwatchTime endTime,
    uint64 maxRound,
    size_t transitionPerRound,
    double temperatureDecayRatio,
    size_t noChangeRoundToExit,
    double diffEachRound)
{
    ASSERT_IF(solutions.empty(), "Empty solution list");

    size_t bestSolutionIndex = SIZE_MAX;
    vector<size_t> successfulTriesPerSolution(solutions.size(), 0);

    PLBConfig const& config = PLBConfig::GetConfig();
    bool traceSAStat = config.TraceSimulatedAnnealingStatistics;
    int statInterval = config.SimulatedAnnealingStatisticsInterval;

    vector<Guid> saIds = TraceSimulatedAnnealingStarted(solutions);

    CandidateSolution const& solution = solutions[0].solution_;

    double currentBestEnergy = solution.Energy;
    size_t currentBestValidMoveCount = solution.ValidMoveCount;
    vector<Movement> currentBestCreations(solution.Creations);
    vector<Movement> currentBestMovements(solution.Migrations);

    for (size_t solutionIndex = 1; solutionIndex < solutions.size(); ++solutionIndex)
    {
        CandidateSolution & currentSolution = solutions[solutionIndex].solution_;
        if (currentSolution.Energy < currentBestEnergy || (currentBestEnergy == currentSolution.Energy && currentSolution.ValidMoveCount < currentBestValidMoveCount))
        {
            currentBestEnergy = currentSolution.Energy;
            currentBestValidMoveCount = currentSolution.ValidMoveCount;
            currentBestCreations = currentSolution.Creations;
            currentBestMovements = currentSolution.Migrations;
        }
    }

    StopwatchTime lastStartTime = Stopwatch::Now();

    vector<bool> previousBest;
    previousBest.resize(solutions.size(), false);

    uint64 totalIterations = 0;
    uint64 totalTransitions = 0;
    uint64 totalPositiveTransitions = 0;
    uint64 successfulMoves = 0;

    for (uint64 round = 0; IsRunning(solutions) && !toStop_.load() && round < maxRound; ++round)
    {
        StopwatchTime now = Stopwatch::Now();
        if (now >= endTime)
        {
            break;
        }

        TimeSpan duration = now - lastStartTime;
        if (duration >= TimeSpan::FromMilliseconds(static_cast<double>(10 - sleepTimePer10ms_)))
        {
            Sleep(static_cast<DWORD>(sleepTimePer10ms_));
            lastStartTime = now;
        }

        for (size_t solutionIndex = 0; solutionIndex < solutions.size(); ++solutionIndex)
        {
            SimulatedAnnealingSolution & currentSASolution = solutions[solutionIndex];

            if (!currentSASolution.running_)
            {
                continue;
            }

            successfulMoves = 0;
            CandidateSolution & currentSolution = currentSASolution.solution_;
            TempSolution tempSolution(currentSolution);

            solutions[solutionIndex].initialEnergy_ = currentSolution.Energy;

            double currentTemperature = currentSASolution.temperature_;
            bool swapOnly = currentSASolution.swapOnly_;
            bool useNodeLoadAsHeuristic = currentSASolution.useNodeLoadAsHeuristic_;
            bool useRestrictedDefrag = currentSASolution.useRestrictedDefrag_;
            int maxConstraintPriority = currentSASolution.maxConstraintPriority_;

            size_t countOfPositiveTrans = 0;
            bool generateBest = false;
            for (size_t transition = 0; transition < transitionPerRound; ++transition)
            {
                if (traceSAStat)
                {
                    size_t currentTransition = transition + transitionPerRound * static_cast<size_t>(round);
                    if (currentTransition % statInterval == 0 || previousBest[solutionIndex])
                    {
                        trace_.SimulatedAnnealingStatistics(saIds[solutionIndex], currentTransition, currentTemperature, currentSolution.AvgStdDev, currentSolution.Energy, currentBestEnergy);
                    }
                }

                previousBest[solutionIndex] = false;

                bool ret = checker_->MoveSolutionRandomly(tempSolution, swapOnly, useNodeLoadAsHeuristic, maxConstraintPriority, useRestrictedDefrag, random_);
                if (ret && !tempSolution.IsEmpty)
                {
                    Score score = currentSolution.TryChange(tempSolution);

                    if (score.Energy < currentSolution.Energy || (score.Energy == currentSolution.Energy && tempSolution.ValidMoveCount < currentSolution.ValidMoveCount))
                    {
                        currentSolution.ApplyChange(tempSolution, move(score));
                        ++totalTransitions;
                        if (currentSolution.Energy < currentBestEnergy || (currentBestEnergy == currentSolution.Energy && currentSolution.ValidMoveCount < currentBestValidMoveCount))
                        {
                            currentBestEnergy = currentSolution.Energy;
                            currentBestValidMoveCount = currentSolution.ValidMoveCount;
                            currentBestCreations = currentSolution.Creations;
                            currentBestMovements = currentSolution.Migrations;
                            generateBest = true;
                            ++countOfPositiveTrans;
                            previousBest[solutionIndex] = true;
                            bestSolutionIndex = solutionIndex;
                        }
                    }
                    else if (currentTemperature > 0)
                    {
                        double energyDiff = score.Energy - currentSolution.Energy; //energyDiff should be >=0
                        double power = -energyDiff / currentTemperature;

                        double pThreshold = random_.NextDouble();
                        if (power > log(pThreshold))
                        {
                            currentSolution.ApplyChange(tempSolution, move(score));
                            ++totalTransitions;
                        }
                        else
                        {
                            currentSolution.UndoChange(tempSolution);
                        }
                    }
                    else
                    {
                        currentSolution.UndoChange(tempSolution);
                    }
                }

                if (ret)
                {
                    ++successfulMoves;
                }
                tempSolution.Clear();
                ++totalIterations;
            }
            
            totalPositiveTransitions += countOfPositiveTrans;
            successfulTriesPerSolution.at(solutionIndex) += successfulMoves;

            // check whether the running need to be continued
            double diffThisRound = currentSolution.Energy == 0 ?
                                    currentSASolution.initialEnergy_ :
                                    abs(currentSolution.Energy - currentSASolution.initialEnergy_) / currentSolution.Energy;

            if (diffThisRound < diffEachRound)
            {
                currentSASolution.noChangeRound_++;
            }
            else
            {
                currentSASolution.noChangeRound_ = 0;
            }

            if (!generateBest)
            {
                currentSASolution.noBestRound_++;
            }
            else
            {
                currentSASolution.noBestRound_ = 0;
            }

            if (currentSASolution.noChangeRound_ >= noChangeRoundToExit)
            {
                if (currentSASolution.noBestRound_ >= noChangeRoundToExit)
                {
                    currentSASolution.running_ = false;
                }
                else
                {
                    currentSASolution.temperature_ = 0;
                }
            }
            else
            {
                // change the temperature for the next round
                // if there are positive transitions, don't change the temperature
                if (countOfPositiveTrans == 0)
                {
                    currentSASolution.temperature_ *= temperatureDecayRatio;
                }
            }
        }
    }

    if (bestSolutionIndex == SIZE_MAX)
    {
        trace_.Searcher(wformatString("Search of balancing completed with {0} total iterations and {1} total transitions and {2} positive transitions, no better solution found", totalIterations, totalTransitions, totalPositiveTransitions));
    }
    else
    {
        trace_.Searcher(wformatString("Search of balancing completed with {0} total iterations and {1} total transitions and {2} positive transitions, solution picked: {3}", totalIterations, totalTransitions, totalPositiveTransitions, bestSolutionIndex));
    }

    stringstream successfulTries;
    copy(successfulTriesPerSolution.begin(), successfulTriesPerSolution.end(), std::ostream_iterator<size_t>(successfulTries, "/"));
    trace_.DetailedSimulatedAnnealingStatistic(solutions.size(), wformatString(successfulTries.str()));

    return CandidateSolution(
        solutions[0].solution_.OriginalPlacement,
        move(currentBestCreations),
        move(currentBestMovements),
        solutions[0].solution_.CurrentSchedulerAction,
        move(solutions[0].solution_.SolutionSearchInsight));
}

void Searcher::AddSimulatedAnnealingSolution(
    CandidateSolution const& solution,
    vector<SimulatedAnnealingSolution> & solutions,
    bool swapOnly,
    bool useNodeLoadAsHeuristic,
    int maxConstraintPriority,
    bool useRestrictedDefrag)
{
    solutions.push_back(SimulatedAnnealingSolution(CandidateSolution(solution), swapOnly, true, maxConstraintPriority,
        GetInitialTemperature(solution, swapOnly, true, maxConstraintPriority, useRestrictedDefrag), useRestrictedDefrag));
    if (!useNodeLoadAsHeuristic && !useRestrictedDefrag)
    {
        solutions.push_back(SimulatedAnnealingSolution(CandidateSolution(solution), swapOnly, false, maxConstraintPriority,
            GetInitialTemperature(solution, swapOnly, false, maxConstraintPriority, useRestrictedDefrag), useRestrictedDefrag));
    }
}

double Searcher::GetInitialTemperature(
    CandidateSolution const& solution,
    bool swapOnly,
    bool useNodeLoadAsHeuristic,
    int maxConstraintPriority, 
    bool useRestrictedDefrag)
{
    int targetProbes = PLBConfig::GetConfig().InitialTemperatureProbeCount;
    int validProbes = 0;
    double maxChange = 0.0;

    TempSolution tempSolution(solution);

    while (targetProbes-- > 0)
    {
        if (checker_->MoveSolutionRandomly(tempSolution, swapOnly, useNodeLoadAsHeuristic, maxConstraintPriority, useRestrictedDefrag, random_))
        {
            if (!tempSolution.IsEmpty)
            {
                Score score = solution.TryChange(tempSolution);
                maxChange = max(maxChange, fabs(score.Energy - solution.Energy));
                solution.UndoChange(tempSolution);
                ++validProbes;
            }
        }
        tempSolution.Clear();
    }

    if (validProbes == 0 || maxChange == 0.0)
    {
        return solution.Energy == 0.0 ? 1.0 : solution.Energy;
    }
    else
    {
        if (PLBConfig::GetConfig().EnableClusterSpecificInitialTemperature)
        {
            // Temperature should be lower depending on cluster state. Cluster state is checked by 2 main parameters, that have the most impact on
            // balancing effectiveness, how much is cluster already balanced (avgstddev), and how much moves are allowed (that includes cluster size
            // and how much can be moved during run. Increasing any of them will lead to lower initial temperature, to choose more heuristic moves. 
            double coef = PLBConfig::GetConfig().ClusterSpecificTemperatureCoefficient;
            double temp = maxChange *  coef / (solution.MaxNumberOfCreationAndMigration * (solution.AvgStdDev + 0.0001));
            if (temp < maxChange * 50)
            {
                return temp;
            }
            else
            {
                return maxChange * 50;
            }
        }
        else
        {
            // a 98% initial acceptance probability
            return maxChange * 50;
        }
    }
}

bool Searcher::IsRunning(vector<SimulatedAnnealingSolution> const & solutions)
{
    bool running = false;
    for (size_t i = 0; i < solutions.size(); i++)
    {
        if (solutions[i].running_)
        {
            running = true;
            break;
        }
    }

    return running;
}

vector<Guid> Searcher::TraceSimulatedAnnealingStarted(vector<SimulatedAnnealingSolution> const& solutions) const
{
    vector<Guid> saIds;

    if (PLBConfig::GetConfig().TraceSimulatedAnnealingStatistics)
    {
        saIds.reserve(solutions.size());
        for (size_t i = 0; i < solutions.size(); i++)
        {
            saIds.push_back(Guid::NewGuid());
        }
        wstring saIdText = L"";
        for_each(saIds.begin(), saIds.end(), [&saIdText](Guid id)
        {
            saIdText.append(L" ");
            saIdText.append(id.ToString());
        });
        trace_.SimulatedAnnealingStarted(solutions.size(), saIdText);
    }

    return saIds;
}
