// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Placement.h"
#include "CandidateSolution.h"
#include "Checker.h"
#include "PLBSchedulerAction.h"
#include "BoundedSet.h"
#include "PLBConfig.h"
#include "client/ClientPointers.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Searcher;
        class PLBDiagnostics;

        typedef std::unique_ptr<Searcher> SearcherUPtr;
        typedef std::shared_ptr<PLBDiagnostics> PLBDiagnosticsSPtr;
        typedef std::wstring DomainId;

        class ViolationList;

        class Searcher
        {
            DENY_COPY(Searcher);

        public:
            size_t GetMoveNumber(double maxPercentageToMove);

            Searcher(
                PLBEventSource const& trace,
                Common::atomic_bool const& toStop,
                Common::atomic_bool const& balancingEnabled,
                PLBDiagnosticsSPtr const& plbDiagnosticsSPtr,
                size_t sleepTimePer10ms = 7,
                int randomSeed = 12345
            );

            __declspec (property(get = get_Placement)) Placement const& OriginalPlacement;
            Placement const& get_Placement() const { return *placement_; }

            __declspec (property(get = get_RandomSeed)) int RandomSeed;
            int get_RandomSeed() const { return randomSeed_; }

            __declspec (property(get = get_BatchIndex, put = set_BatchIndex)) size_t BatchIndex;
            size_t get_BatchIndex() const { return batchIndex_; }
            void set_BatchIndex(size_t index) { batchIndex_ = index; }

            CandidateSolution SearchForSolution(
                PLBSchedulerAction const & searchType,
                Placement const & placement,
                Checker & checker,
                DomainId const & domainId,
                bool containsDefragMetric,
                size_t allowedMovements);

            bool IsInterrupted();

            bool UseBatchPlacement() const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            std::wstring ToString() const;

        private:

            struct PlacementSolution;
            void SearchForUpgrade(CandidateSolution& solution);
            void Searcher::TraceUpgradeSwapPrimary(std::vector<PartitionEntry const*> const& unplacedPartitions, bool relaxCapacity) const;

            void SearchForExtraReplicas(CandidateSolution& solution);

            CandidateSolution SearchForPlacement(
                double maxPercentageToMove,
                Common::TimeSpan timeout,
                uint64 maxRound,
                size_t placementSearchIterationsPerRound,
                size_t simuAnnealingTransitionsPerRound,
                double temperatureDecayRatio,
                bool creationWithMove,
                PLBSchedulerActionType::Enum currentSchedulerAction,
                size_t noChangeRoundToExit = 10,
                double diffEachRound = 0.001);

            void RandomWalkPlacementWithReplicaMove(
                CandidateSolution & solution,
                int & maxConstraintPriority,
                Common::StopwatchTime endTime,
                uint64 maxRound,
                size_t transitionPerRound,
                bool totalRandom,
                bool usePartialClosure);

            size_t CountPlaceNewReplica(TempSolution & solution, int maxConstraintPriority);

            void RandomWalkPlacement(
                CandidateSolution & solution,
                int & maxConstraintPriority,
                Common::StopwatchTime endTime,
                uint64 maxRound,
                size_t transitionPerRound,
                bool relaxAffinity = false);

            // The Equivalent of RandomWalkPlacement when the DummyPLB is Enabled. Skips the Random Walks
            void DummyWalkPlacement(
                CandidateSolution & solution,
                int & maxConstraintPriority);

            // The Portion of RandomWalkPlacement that generates Tracing and Diagnostics
            void TraceWalkPlacement(
                CandidateSolution & solution);

            // The first portion of RandomWalkPlacement, which tries a simple attempt at placing
            void InitialWalk(
                CandidateSolution & solution,
                std::vector<PlacementSolution> & solutions,
                int & maxPriority,
                bool enableVerboseTracing = false,
                bool relaxAffinity = false,
                bool modifySolution = true);

            // The second portion of RandomWalkPlacement, which retries placement attempts repeatedly
            // and for each solution identifies the one with the largest number of placed replicas
            void RandomWalking(
                CandidateSolution & solution,
                std::vector<PlacementSolution> & solutions,
                Common::StopwatchTime endTime,
                uint64 maxRound,
                size_t transitionPerRound,
                bool relaxAffinity = false);

            // The last portion of RandomWalkPlacement, which selects the placement attempt that respects the most constraints
            void ChooseWalk(
                CandidateSolution & solution,
                std::vector<PlacementSolution> & solutions,
                int & maxConstraintPriority,
                int maxPriority,
                bool modifySolution = true);

            static bool IsRunning(std::vector<PlacementSolution> const & solutions);

            struct ConstraintCheckSolution;
            CandidateSolution SearchForConstraintCheck(
                Common::TimeSpan timeout,
                uint64 maxRound,
                size_t constraintCheckIterationsPerRound,
                size_t simuAnnealingTransitionsPerRound,
                double temperatureDecayRatio,
                PLBSchedulerAction const & searchType,
                size_t noChangeRoundToExit = 10,
                double diffEachRound = 0.001);

            void RandomWalkConstraintCheck(
                CandidateSolution & solution,
                int & maxConstraintPriority,
                Common::StopwatchTime endTime,
                uint64 maxRound,
                size_t transitionPerRound,
                bool constraintFixLight);

            static bool IsRunning(std::vector<ConstraintCheckSolution> const & solutions);

            struct SimulatedAnnealingSolution;
            CandidateSolution SearchForBalancing(
                bool useNodeLoadAsHeuristic,
                double maxPercentageToMove,
                Common::TimeSpan timeout,
                uint64 maxRound,
                size_t transitionPerRound,
                double temperatureDecayRatio,
                bool containsDefragMetric,
                PLBSchedulerActionType::Enum currentSchedulerAction,
                size_t noChangeRoundToExit = 10,
                double diffEachRound = 0.001);

            CandidateSolution SimulatedAnnealing(
                std::vector<SimulatedAnnealingSolution> && solutions,
                Common::StopwatchTime endTime,
                uint64 maxRound,
                size_t transitionPerRound,
                double temperatureDecayRatio,
                size_t noChangeRoundToExit,
                double diffEachRound);

            static bool IsRunning(std::vector<SimulatedAnnealingSolution> const & solutions);

            // If metric is considered for balancing, useNodeLoadAsHeuristic will prefer swaps/moves from overloaded to underloaded nodes.
            // If metric is considered for defrag, useNodeLoadAsHeuristic will prefer swaps/moves from underloaded to overloaded nodes.
            // Heuristics are used for fast balancing - during QuickLoadBalancing and at the end of Placement and ConstraintCheck phases.
            // During LoadBalancing heuristic should not be used - engine should try swaps/moves from any to any node.
            void AddSimulatedAnnealingSolution(
                CandidateSolution const& solution,
                std::vector<SimulatedAnnealingSolution> & solutions,
                bool swapOnly,
                bool useNodeLoadAsHeuristic,
                int maxConstraintPriority,
                bool useRestrictedDefrag);

                double GetInitialTemperature(
                    CandidateSolution const& solution,
                    bool swapOnly,
                    bool useNodeLoadAsHeuristic,
                    int maxConstraintPriority,
                    bool useRestrictedDefrag);

                    std::vector<Common::Guid> TraceSimulatedAnnealingStarted(std::vector<SimulatedAnnealingSolution> const& solutions) const;

                    PLBDiagnosticsSPtr plbDiagnosticsSPtr_;
                    PLBEventSource const& trace_;
                    Common::atomic_bool const& toStop_;
                    Common::atomic_bool const& balancingEnabled_;
                    Placement const * placement_;
                    Checker * checker_;
                    DomainId domainId_;

                    size_t sleepTimePer10ms_;
                    int randomSeed_;

                    Common::Random random_;

                    size_t movementsAllowedGlobally_;

                    size_t batchIndex_;

        };
    }
}
