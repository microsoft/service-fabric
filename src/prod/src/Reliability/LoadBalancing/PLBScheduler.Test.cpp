// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestUtility.h"
#include "PLBScheduler.h"
#include "ISystemState.h"
#include "PLBEventSource.h"
#include "PLBConfig.h"
#include "DiagnosticsDataStructures.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Reliability::LoadBalancingComponent;

    StringLiteral const PLBSchedulerTestSource("PLBSchedulerTest");

    class TestPLBScheduler
    {
    protected:
        TestPLBScheduler() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);
        ~TestPLBScheduler() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_CLEANUP(TestCleanup);

        class TestSystemState : public ISystemState
        {
        public:
            virtual bool HasNewReplica() const { return hasNewReplica_; }
            virtual bool HasMovableReplica() const { return hasMovableReplica_; }
            virtual bool HasPartitionWithReplicaInUpgrade() const {return hasInUpgradePartition_;}
            virtual bool HasExtraReplicas() const { return hasExtraReplicas_; }
            virtual bool HasPartitionInAppUpgrade() const { return hasPartitionInAppUpgrade_; }
            virtual size_t ExistingReplicaCount() const { return existingReplicaCount_; }

            virtual bool IsConstraintSatisfied(std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & constraintsDiagnosticsData) const 
            {
                UNREFERENCED_PARAMETER(constraintsDiagnosticsData);
                return isConstraintSatisfied_;
            }

            virtual bool IsBalanced() const { return isBalanced_; }
            virtual double GetAvgStdDev() const { return avgStdDev_; }
            virtual set<Guid> GetImbalancedFailoverUnits() const { return imbalancedFailoverUnits_; }
            virtual size_t BalancingMovementCount() const { return balancingMoveCount_; }
            virtual size_t PlacementMovementCount() const { return placementMoveCount_; }

            void Reset()
            {
                hasNewReplica_ = false;
                hasMovableReplica_ = true;
                hasInUpgradePartition_ = false;
                hasExtraReplicas_ = false;
                isConstraintSatisfied_ = true;
                isBalanced_ = true;
                avgStdDev_ = 0.0;
                imbalancedFailoverUnits_.clear();
                balancingMoveCount_ = 0;
                placementMoveCount_ = 0;
                hasPartitionInAppUpgrade_ = false;
            }
            void SetHasNewReplica(bool value) { hasNewReplica_ = value; }
            void SetHasMovableReplica(bool value) { hasMovableReplica_ = value; }
            void SetIsConstraintSatisfied(bool value) { isConstraintSatisfied_ = value; }
            void SetIsBalanced(bool value) { isBalanced_ = value; }
            void SetAvgStdDev(double value) { avgStdDev_ = value; }
            void InsertImbalancedFailoverUnit(Guid fuId) { imbalancedFailoverUnits_.insert(fuId); }
            void SetBalancingMoveCount(size_t value) { balancingMoveCount_ = value; }
            void SetPlacementMoveCount(size_t value) { placementMoveCount_ = value; }
            void SetHasPartitionInAppUpgrade(bool value) { hasPartitionInAppUpgrade_ = value; }
            void SetExistingReplicaCount(size_t value) { existingReplicaCount_ = value; }

        private:
            bool hasNewReplica_;
            bool hasMovableReplica_;
            bool hasInUpgradePartition_;
            bool hasExtraReplicas_;
            bool isConstraintSatisfied_;
            bool isBalanced_;
            double avgStdDev_;
            set<Guid> imbalancedFailoverUnits_;
            size_t balancingMoveCount_;
            size_t placementMoveCount_;
            size_t existingReplicaCount_;
            bool hasPartitionInAppUpgrade_;
        };

        FailoverUnitMovement GetMovement(size_t id) const;
        Guid GetGuid(size_t id) const;
        void ActionExecutedWithMovePlan(
            StopwatchTime timeStamp,
            double newAvgStdDev,
            map<Guid, FailoverUnitMovement> const& movementList);

        StopwatchTime GetCurrentTime() const
        {
            return StopwatchTime(1000742000000LL);
        }

        void VerifyScheduler(PLBSchedulerActionType::Enum expectedAction, bool expectedSkip = false)
        {
            VERIFY_ARE_EQUAL(expectedAction, plbScheduler_->CurrentAction.Action);
            VERIFY_ARE_EQUAL(expectedSkip, plbScheduler_->CurrentAction.IsSkip);
        }
        void BalancingConfigVariableBalancingDisabledThrottle(bool ignore);

        static PLBEventSource const PLBTrace;
        unique_ptr<PLBScheduler> plbScheduler_;
        TestSystemState systemState_;
        TimeSpan oldPlacementInterval_;
        TimeSpan oldConstraintCheckInterval_;
        TimeSpan oldBalancingInterval_;
        TimeSpan oldRefreshInterval_;
        int oldActionRetryTimes_;
    };

    PLBEventSource const TestPLBScheduler::PLBTrace(TraceTaskCodes::PLB);

    BOOST_FIXTURE_TEST_SUITE(TestPLBSchedulerSuite,TestPLBScheduler)

    BOOST_AUTO_TEST_CASE(BalancingConfigVariableBalancingDisabledThrottle2)
    {
            BalancingConfigVariableBalancingDisabledThrottle(false);
    }


    BOOST_AUTO_TEST_CASE(GetActionNoneTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionNoneTest");

        auto now = GetCurrentTime();
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        plbScheduler_->OnMovementGenerated(now);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
    }

    BOOST_AUTO_TEST_CASE(GetActionCreationTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionCreationTest");
        auto now = GetCurrentTime();
        systemState_.SetHasNewReplica(true);

        PLBConfigScopeChange(PreventTransientOvercommit, bool, true);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
        now += PLBConfig::GetConfig().MinPlacementInterval;
        // CreationWithMove will happen although intermediate overcommits are prevented
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacementWithMove);

        //CreationwithMove cant be done if movements are throttled
        PLBConfigScopeModify(PreventTransientOvercommit, false);
        systemState_.SetPlacementMoveCount(PLBConfig::GetConfig().GlobalMovementThrottleThreshold);
        now += PLBConfig::GetConfig().MinPlacementInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);

        //CreationwithMove cant be done if movements are throttled based on percentage
        // Throttle on 100 replicas * 10%
        systemState_.SetExistingReplicaCount(100);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.1);
        systemState_.SetPlacementMoveCount(11);
        now += PLBConfig::GetConfig().MinPlacementInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdPercentage, 0.0);

        // CreationWithMove can't be done if movements are throttled for placement based on percentage
        systemState_.SetExistingReplicaCount(100);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForPlacement, double, 0.1);
        systemState_.SetPlacementMoveCount(11);
        now += PLBConfig::GetConfig().MinPlacementInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdPercentageForPlacement, 0.0);

        // CreationWithMove can't be done if movements are throttled for placement based on percentage
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForPlacement, uint, 10);
        systemState_.SetPlacementMoveCount(11);
        now += PLBConfig::GetConfig().MinPlacementInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdForPlacement, 0);

        // querying again returns CreationWithMove since last action was Creation
        systemState_.SetPlacementMoveCount(0);
        now += PLBConfig::GetConfig().MinPlacementInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacementWithMove);

        // after CreationWithMove execution, return to Creation action
        now += PLBConfig::GetConfig().MinPlacementInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);

        // There are no replicas to place, so there should be no action needed.
        systemState_.SetHasNewReplica(false);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Next time we want to do creation, we will do regular Creation since last placement managed to place all replicas
        systemState_.SetHasNewReplica(true);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);

        // We are done with new replicas
        systemState_.SetHasNewReplica(false);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
    }

    BOOST_AUTO_TEST_CASE(GetActionPerActionThrottling)
    {
        // Test case: per-action throttles will be OK, global throttle will not be satisfied.
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionPerActionThrottling");

        PLBConfigScopeChange(PreventTransientOvercommit, bool, false);

        // Load change to enable balancing
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));
        now = now + TimeSpan::FromSeconds(1);

        // System has new replicas, and it is imbalanced.
        systemState_.SetHasNewReplica(true);
        systemState_.SetIsBalanced(false);

        // Do one round of creation as it won't be throttled anyway (no moves involved)
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);

        now += PLBConfig::GetConfig().MinPlacementInterval;
        now += PLBConfig::GetConfig().MinLoadBalancingInterval;

        // In the past, balancing had 500 movements, placement another 500.
        // Total throttle is 1000 (not OK), and per-action throttle is 1000 as well (OK).
        systemState_.SetBalancingMoveCount(500);
        systemState_.SetPlacementMoveCount(500);

        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 1000);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForPlacement, uint, 1000);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 1000);

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);

        // Change the load again, so that we do not throttle balancing.
        plbScheduler_->OnLoadChanged(now + TimeSpan::FromMilliseconds(1));
        now = now + TimeSpan::FromSeconds(1);

        // Move the time so that both placement and balancing are expired
        now += PLBConfig::GetConfig().MinPlacementInterval;
        now += PLBConfig::GetConfig().MinLoadBalancingInterval;

        // Repeat the experiment, but use percentages instead of absolute values
        PLBConfigScopeModify(GlobalMovementThrottleThreshold, 0);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdForPlacement, 0);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdForBalancing, 0);

        systemState_.SetExistingReplicaCount(10000);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.1);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForPlacement, double, 0.1);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForBalancing, double, 0.1);

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
    }

    BOOST_AUTO_TEST_CASE(GetActionConstraintCheckTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionConstraintCheckTest");
        auto now = GetCurrentTime();
        PLBConfig const& config = PLBConfig::GetConfig();

        // ConstraintCheck action only occurs in ConstraintCheck stage
        // Set placement to very high interval so they will not take precedence of ConstraintCheck stage
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5));
        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));

        systemState_.SetIsConstraintSatisfied(false);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::ConstraintCheck);
        // Constraint check should not be allowed to retry immediately
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Constraint check should not retry regardless of whether it made any movements (here it did)
        plbScheduler_->OnMovementGenerated(now);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        now = now + config.MinConstraintCheckInterval;
        plbScheduler_->OnFailoverUnitChanged(now, GetGuid(0));
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::ConstraintCheck);

        now = now + config.MinConstraintCheckInterval;
        systemState_.SetHasMovableReplica(false);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingBasicTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingBasicTest");
        // ensure the throttle mechanism does not kick in
        double balancedAvgStdDev = 0.1;
        systemState_.SetAvgStdDev(10);

        // Balancing now does not do placement & constraintcheck
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));
        now = now + TimeSpan::FromSeconds(1);

        systemState_.SetIsBalanced(false);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        now += PLBConfig::GetConfig().MinLoadBalancingInterval;
        plbScheduler_->OnMovementGenerated(now, balancedAvgStdDev);
        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);

        plbScheduler_->OnMovementGenerated(now, balancedAvgStdDev);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        now = now + PLBConfig::GetConfig().MinLoadBalancingInterval / 2;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        now = now + config.MinLoadBalancingInterval / 2;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // The throttle above update the lastBalacing_ so we need to go to the next MinLoadBalancingInterval
        now = now + config.MinLoadBalancingInterval;
        plbScheduler_->OnLoadChanged(now);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        now = now + config.BalancingDelayAfterNodeDown;

        plbScheduler_->OnLoadChanged(now);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);

        plbScheduler_->OnNodeChanged(now);
        plbScheduler_->OnNodeUp(now);

        now += config.BalancingDelayAfterNewNode;
        plbScheduler_->RefreshAction(systemState_, now);
        systemState_.SetHasPartitionInAppUpgrade(true);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        plbScheduler_->OnNodeChanged(now);
        plbScheduler_->OnNodeUp(now);

        now += config.BalancingDelayAfterNewNode;
        systemState_.SetHasPartitionInAppUpgrade(true);
        PLBConfigScopeChange(AllowBalancingDuringApplicationUpgrade, bool, false);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        systemState_.SetHasPartitionInAppUpgrade(false);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingAvgStdDevBasedThrottleTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingAvgStdDevBasedThrottleTest");
        PLBConfigScopeChange(AvgStdDevDeltaThrottleThreshold, double, 0.01);

        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));
        now = now + TimeSpan::FromSeconds(1);
        systemState_.SetIsBalanced(false);

        double balancedAvgStdDev = 0.1;

        // Balancing now does not do placement & constraintcheck
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        now = now + config.MinLoadBalancingInterval;
        plbScheduler_->OnLoadChanged(now);

        systemState_.SetAvgStdDev(balancedAvgStdDev * (1 + config.AvgStdDevDeltaThrottleThreshold) + 1e-5);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);

        systemState_.SetAvgStdDev(balancedAvgStdDev * (1 + config.AvgStdDevDeltaThrottleThreshold) - 1e-5);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // The throttle above updates the lastBalacing_ so we need to go to the next MinLoadBalancingInterval
        now = now + config.MinLoadBalancingInterval;

        plbScheduler_->OnFailoverUnitChanged(now, GetGuid(0));
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        plbScheduler_->OnMovementGenerated(now, balancedAvgStdDev);
        now = now + config.MinLoadBalancingInterval;

        plbScheduler_->OnNodeUp(now);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Bypass the throttle by nothing changed since last check
        now = now + TimeSpan::FromSeconds(1);
        plbScheduler_->OnFailoverUnitChanged(now, GetGuid(0));

        now += max(config.BalancingDelayAfterNewNode, config.MinLoadBalancingInterval) + TimeSpan::FromSeconds(1);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingAvgStdDevBasedThrottleDisabledTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingAvgStdDevBasedThrottleDisabledTest");
        PLBConfig const& config = PLBConfig::GetConfig();
        PLBConfigScopeChange(AvgStdDevDeltaThrottleThreshold, double, -1.0);
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));
        now = now + TimeSpan::FromSeconds(1);
        double avgStdDev = 0.1;

        systemState_.SetIsBalanced(false);
        systemState_.SetAvgStdDev(avgStdDev);

        // Balancing now does not do placement & constraintcheck
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        plbScheduler_->OnMovementGenerated(now, avgStdDev);
        now = now + config.MinLoadBalancingInterval;
        plbScheduler_->OnLoadChanged(now);

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingGlobalMovementThrottleTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingGlobalMovementThrottleTest");
        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now);
        now = now + TimeSpan::FromSeconds(1);
        systemState_.SetIsBalanced(false);
        systemState_.SetAvgStdDev(10);

        // Balancing now does not do placement & constraintcheck
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        map<Guid, FailoverUnitMovement> movementList;
        for (size_t i = 0; i < config.GlobalMovementThrottleThreshold; i++)
        {
            auto movement = GetMovement(i);
            Guid fuId(movement.FailoverUnitId);
            systemState_.InsertImbalancedFailoverUnit(fuId);
            movementList.insert(make_pair(fuId, move(movement)));
        }

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        // System state is changed, but number of movements is throttling the balancing.
        plbScheduler_->OnServiceChanged(now);
        now += PLBConfig::GetConfig().MinLoadBalancingInterval;
        systemState_.SetBalancingMoveCount(PLBConfig::GetConfig().GlobalMovementThrottleThreshold);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // System state is changed, but number of movements is throttling the balancing based on percentage.
        plbScheduler_->OnServiceChanged(now);
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentage, double, 0.1);
        systemState_.SetBalancingMoveCount(11);
        systemState_.SetExistingReplicaCount(100);
        now += PLBConfig::GetConfig().MinLoadBalancingInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdPercentage, 0.0);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingMovementThrottleTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingMovementThrottleTest");
        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now);
        now = now + TimeSpan::FromSeconds(1);
        systemState_.SetIsBalanced(false);
        systemState_.SetAvgStdDev(10);

        // Balancing now does not do placement & constraintcheck
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        // Do not throttle on global movements, but on balancing movements only
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdForBalancing, uint, 100);

        map<Guid, FailoverUnitMovement> movementList;
        for (size_t i = 0; i < config.GlobalMovementThrottleThresholdForBalancing; i++)
        {
            auto movement = GetMovement(i);
            Guid fuId(movement.FailoverUnitId);
            systemState_.InsertImbalancedFailoverUnit(fuId);
            movementList.insert(make_pair(fuId, move(movement)));
        }

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        // System state is changed, but number of movements is throttling the balancing.
        plbScheduler_->OnServiceChanged(now);
        now += PLBConfig::GetConfig().MinLoadBalancingInterval;
        systemState_.SetBalancingMoveCount(PLBConfig::GetConfig().GlobalMovementThrottleThresholdForBalancing);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // System state is changed, but number of movements is throttling the balancing based on percentage.
        plbScheduler_->OnServiceChanged(now);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdForBalancing, 0);
        PLBConfigScopeChange(GlobalMovementThrottleThresholdPercentageForBalancing, double, 0.1);
        systemState_.SetBalancingMoveCount(11);
        systemState_.SetExistingReplicaCount(100);
        now += PLBConfig::GetConfig().MinLoadBalancingInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
        PLBConfigScopeModify(GlobalMovementThrottleThresholdPercentageForBalancing, 0.0);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingFUMovementThrottleTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingFUMovementThrottleTest");
        PLBConfigScopeChange(GlobalMovementThrottleThreshold, uint, 1000000);
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now);
        now = now + TimeSpan::FromSeconds(1);
        systemState_.SetIsBalanced(false);
        systemState_.SetAvgStdDev(10);

        // Balancing now does not do placement & constraintcheck
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        size_t fuBaseCount = 100;
        PLBConfig const& config = PLBConfig::GetConfig();
        for (uint k = 0; k < config.MovementPerPartitionThrottleThreshold; k++)
        {
            map<Guid, FailoverUnitMovement> movementList;
            for (size_t i = 0; i < fuBaseCount * 3; i++)
            {
                uint count;
                if (i % 3 == 0 || i % 3 == 1) count = config.MovementPerPartitionThrottleThreshold / 2;
                else count = config.MovementPerPartitionThrottleThreshold;

                auto movement = GetMovement(i);
                Guid fuId(movement.FailoverUnitId);
                if (k < count)
                {
                    movementList.insert(make_pair(fuId, move(movement)));
                }
                systemState_.InsertImbalancedFailoverUnit(fuId);
            }

            plbScheduler_->SetAction(PLBSchedulerActionType::QuickLoadBalancing);
            ActionExecutedWithMovePlan(now, 0.0, movementList);
        }

        plbScheduler_->OnLoadChanged(now);
        now = now + config.MovementPerPartitionThrottleCountingInterval / 2 - config.MinLoadBalancingInterval;
        // the number of movement throttled FUs is small enough so we still do balancing
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        auto throttled = plbScheduler_->GetMovementThrottledFailoverUnits();
        set<Guid> expectedThrottled;
        for (size_t i = 0; i < fuBaseCount * 3; i++)
        {
            if (i % 3 == 2)
            {
                expectedThrottled.insert(GetGuid(i));
            }
        }
        VERIFY_ARE_EQUAL(expectedThrottled, throttled);

        // add more moves so that movement throttled FUs become too many
        for (uint k = 0; k < config.MovementPerPartitionThrottleThreshold / 2 + 1; k++)
        {
            map<Guid, FailoverUnitMovement> movementList;
            for (size_t i = 0; i < fuBaseCount * 3; i++)
            {
                if (i % 3 == 1)
                {
                    auto movement = GetMovement(i);
                    Guid fuId(movement.FailoverUnitId);
                    movementList.insert(make_pair(fuId, move(movement)));
                }
            }

            plbScheduler_->SetAction(PLBSchedulerActionType::QuickLoadBalancing);
            ActionExecutedWithMovePlan(now, 0.0, movementList);
        }

        plbScheduler_->OnLoadChanged(now);
        now = now + config.MinLoadBalancingInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        now = now + max(config.MovementPerPartitionThrottleCountingInterval, config.MinLoadBalancingInterval) + TimeSpan::FromSeconds(1);
        plbScheduler_->RefreshAction(systemState_, now);
        throttled = plbScheduler_->GetMovementThrottledFailoverUnits();
        VERIFY_ARE_EQUAL(0u, throttled.size());
    }

    BOOST_AUTO_TEST_CASE(GetActionSlowBalancingGlobalMovementThrottleTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionSlowBalancingGlobalMovementThrottleTest");
        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now);
        now = now + TimeSpan::FromSeconds(1);
        systemState_.SetIsBalanced(false);
        systemState_.SetAvgStdDev(10);

        // Balancing now does not do placement & constraintcheck
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        map<Guid, FailoverUnitMovement> movementList;
        for (size_t i = 0; i < config.GlobalMovementThrottleThreshold; i++)
        {
            auto movement = GetMovement(i);
            Guid fuId(movement.FailoverUnitId);
            systemState_.InsertImbalancedFailoverUnit(fuId);
            movementList.insert(make_pair(fuId, move(movement)));
        }

        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        ActionExecutedWithMovePlan(now, 0.0, movementList);
        plbScheduler_->OnLoadChanged(now);
        now = now + config.MinLoadBalancingInterval;
        systemState_.SetBalancingMoveCount(PLBConfig::GetConfig().GlobalMovementThrottleThreshold);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        plbScheduler_->OnMovementGenerated(now, 0.0);

        now = now + max(config.GlobalMovementThrottleCountingInterval, config.MinLoadBalancingInterval) + TimeSpan::FromSeconds(1);
        plbScheduler_->OnLoadChanged(now);
        systemState_.SetBalancingMoveCount(0);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingAfterServiceUpdate)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingAfterServiceUpdate");
        // ensure the throttle mechanism does not kick in
        double balancedAvgStdDev = 0.1;
        systemState_.SetAvgStdDev(10);

        // Do not do constraint check or placement
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::MaxValue);

        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();
        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));
        now = now + TimeSpan::FromSeconds(1);

        // First time we will do balancing
        systemState_.SetIsBalanced(false);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);
        plbScheduler_->OnMovementGenerated(now, balancedAvgStdDev);

        // System is still not balanced, but there are no updates since last balancing
        now += config.MinLoadBalancingInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Update a service to trigger system state change
        now = now + PLBConfig::GetConfig().MinLoadBalancingInterval / 2;
        plbScheduler_->OnServiceChanged(now);

        // System is not balanced, and service is updated - we should balance
        // Maximum number of retries for QuickLoadBalancing is 1, so we should do slow
        now = now + config.MinLoadBalancingInterval / 2;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);
    }

    BOOST_AUTO_TEST_CASE(PlacementAndConstraintCheckWithBalancingDisabledAndViolations)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting PlacementAndConstraintCheckWithBalancingDisabledAndViolations");
        auto now = GetCurrentTime();
        Common::TimeSpan timeIncrement = Common::TimeSpan::FromSeconds(1.0);

        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1.0));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1.0));
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1.0));
        PLBConfigScopeChange(PLBRefreshInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1.0));

        // Disable balancing
        plbScheduler_->OnLoadChanged(now);
        plbScheduler_->SetBalancingEnabled(false, now);

        // There are no new replicas, constraint is not satisfied and system is not balanced
        systemState_.SetHasNewReplica(false);
        systemState_.SetIsConstraintSatisfied(false);
        systemState_.SetIsBalanced(false);

        // Constraint check is executed, but constraints cannot be fixed
        now += timeIncrement;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::ConstraintCheck);
        plbScheduler_->OnMovementGenerated(now);

        // New replica arrives
        plbScheduler_->OnFailoverUnitChanged(now, GetGuid(0));
        systemState_.SetHasNewReplica(true);

        // Placement needs to be allowed to run, since ConstraintCheck has already tried in the previous round.
        now += timeIncrement;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
        plbScheduler_->OnMovementGenerated(now);

        now += timeIncrement;
        // We have managed to place new replicas, hence an update from FM
        plbScheduler_->OnFailoverUnitChanged(now, GetGuid(0));
        systemState_.SetHasNewReplica(false);

        // Constraint check needs to be allowed to run again
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::ConstraintCheck);
        plbScheduler_->OnMovementGenerated(now);
    }

    BOOST_AUTO_TEST_CASE(GetActionBalancingWhenInterrupted)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting GetActionBalancingWhenInterrupted");
        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();

        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));

        // System is not balanced, constraint check and placement are not needed
        systemState_.SetIsBalanced(false);
        systemState_.SetAvgStdDev(10.0);

        // Balancing is executed and interrupted, no movements are generated
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);
        plbScheduler_->OnDomainInterrupted(now);

        // Next time we will be allowed to do balancing since previous run was interrupted
        now += config.MinLoadBalancingInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);

        // Search is not interrupted, searcher generated some movements
        plbScheduler_->OnMovementGenerated(now, 0.1);

        // System is still not balanced, but we are not allowed to do balancing because there was no change
        now += config.MinLoadBalancingInterval;
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
    }

    BOOST_AUTO_TEST_CASE(MultipleTimersTest)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting MultipleTimersTest");
        PLBConfig const& config = PLBConfig::GetConfig();
        auto now = GetCurrentTime();

        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1.01));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5));

        PLBConfigScopeChange(MoveExistingReplicaForPlacement, bool, false);

        plbScheduler_->OnLoadChanged(now - TimeSpan::FromMilliseconds(1));

        // Begin of the test, all timers expired so placement is executed
        systemState_.SetHasNewReplica(true);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
        plbScheduler_->OnMovementGenerated(now);

        // All timers are updated since there is no action needed
        systemState_.SetHasNewReplica(false);
        plbScheduler_->OnMovementGenerated(now);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Placement timer expires, and there is a new replica to be placed
        systemState_.SetHasNewReplica(true);
        plbScheduler_->RefreshAction(systemState_, now + config.MinPlacementInterval);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);

        // Creation & ConstraintCheck need to run
        // ConstraintCheck is executed because there is nothing to place
        systemState_.SetHasNewReplica(false);
        systemState_.SetIsConstraintSatisfied(false);
        plbScheduler_->RefreshAction(systemState_, now + config.MinConstraintCheckInterval);
        VerifyScheduler(PLBSchedulerActionType::ConstraintCheck);
        plbScheduler_->OnMovementGenerated(now + config.MinConstraintCheckInterval);

        // Setup this situation: when balancing expires (next round), all other timers have smaller values
        systemState_.SetHasNewReplica(false);
        systemState_.SetIsConstraintSatisfied(true);
        plbScheduler_->RefreshAction(systemState_, now + config.MinLoadBalancingInterval - TimeSpan::FromSeconds(1));
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
    }

    BOOST_AUTO_TEST_CASE(BalancingNodeUpThrottle)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting BalancingNodeUpThrottle");
        auto now = GetCurrentTime();

        // System is not balanced throughout the test
        systemState_.SetIsBalanced(false);

        // Placement and constraint check should not run during this test
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));

        // Set the MinLoadBalancingInterval so that it is smaller than node up delay
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5));
        PLBConfigScopeChange(BalancingDelayAfterNewNode, Common::TimeSpan, Common::TimeSpan::FromSeconds(10));

        // Report that node is up
        plbScheduler_->OnNodeUp(now);

        // First balancing run should be throttled
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Second balancing run should pick up state change from node up, and should run
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        // Now set the min balancing interval to be larger than node up delay and add new node
        ScopeChangeObjectMinLoadBalancingInterval.SetValue(Common::TimeSpan::FromSeconds(11));

        plbScheduler_->OnNodeUp(now);

        // Balancing should run since we have a new node - slow balancing since previous run did not generate moves
        now = now + Common::TimeSpan::FromSeconds(12);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::LoadBalancing);

        // Just update the timer since we balanced the system, don't do anything else
        systemState_.SetIsBalanced(true);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Next time balancing should not run, since we do not have any significant change in the system
        systemState_.SetIsBalanced(false);
        now = now + Common::TimeSpan::FromSeconds(12);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);
    }

    BOOST_AUTO_TEST_CASE(BalancingNodeDownThrottle)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting BalancingNodeDownThrottle");
        auto now = GetCurrentTime();

        // System is not balanced throughout the test
        systemState_.SetIsBalanced(false);

        // Placement and constraint check should not run during this test
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));

        // Set the MinLoadBalancingInterval so that it is smaller than node down delay
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5));
        PLBConfigScopeChange(BalancingDelayAfterNodeDown, Common::TimeSpan, Common::TimeSpan::FromSeconds(10));

        // Report that node is up
        plbScheduler_->OnNodeDown(now);

        // First balancing run should be throttled
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Second balancing run should pick up state change from node up, and should run
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);
    }

    BOOST_AUTO_TEST_CASE(BalancingLocalVariableBalancingDisabledThrottle)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting BalancingLocalVariableBalancingDisabledThrottle");
        // Test the scheduler when balancing is enabled or disabled via PLBScheduler local variable
        auto now = GetCurrentTime();

        // System is not balanced throughout the test
        systemState_.SetIsBalanced(false);

        // Placement and constraint check should not run during this test
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5));

        // Disable balancing
        plbScheduler_->SetBalancingEnabled(false, now);

        // Report that load has changed, so that we have a change in the system
        plbScheduler_->OnLoadChanged(now);

        // First balancing run should be throttled
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Enable balancing
        plbScheduler_->SetBalancingEnabled(true, now);

        // Second balancing run should see that balancing is enabled and pick up load change, and should run
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);
    }


    BOOST_AUTO_TEST_CASE(PlacementWithInterruptions)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting PlacementWithInterruptions");
        // Test the scheduler when balancing is enabled or disabled via PLBConfig setting
        auto now = GetCurrentTime();

        // All actions will have equal frequency
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));

        // Report that load has changed, so that we have a change in the system
        plbScheduler_->OnLoadChanged(now);

        // System is not balanced throughout the test
        systemState_.SetIsBalanced(false);

        // Balancing should run, but is interrupted, so no movements are reported to the scheduler
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        now = now + Common::TimeSpan::FromSeconds(1);

        // Update comes for one FailoverUnit, asking for placement due to failover
        plbScheduler_->OnFailoverUnitChanged(now, GetGuid(0));
        systemState_.SetHasNewReplica(true);

        // Load is changed for another FailoverUnit, generating constraint violation
        plbScheduler_->OnLoadChanged(now);
        systemState_.SetIsConstraintSatisfied(false);

        // QuickLoadBalancing had finished iteration 0, but it should not be allowed to proceed to iteration 1
        // Instead, scheduler should pick placement to place new replicas
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NewReplicaPlacement);
    }

    BOOST_AUTO_TEST_CASE(ConstraintCheckWithInterruptions)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting ConstraintCheckWithInterruptions");
        // Test the scheduler when balancing is enabled or disabled via PLBConfig setting
        auto now = GetCurrentTime();

        // All actions will have equal frequency
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1));

        // Report that load has changed, so that we have a change in the system
        plbScheduler_->OnLoadChanged(now);

        // System is not balanced throughout the test
        systemState_.SetIsBalanced(false);

        // Balancing should run, but is interrupted, so no movements are reported to the scheduler
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);

        now = now + Common::TimeSpan::FromSeconds(1);

        // Load is changed for one FailoverUnit, generating constraint violation
        plbScheduler_->OnLoadChanged(now);
        systemState_.SetIsConstraintSatisfied(false);

        // QuickLoadBalancing had finished iteration 0, but it should not be allowed to proceed to iteration 1
        // Instead, scheduler should pick constraint check to fix capacity violation
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::ConstraintCheck);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestPLBScheduler::TestSetup()
    {
        plbScheduler_ = make_unique<PLBScheduler>(L"ServiceDomain", PLBTrace, true, true, StopwatchTime::Zero, StopwatchTime::Zero, StopwatchTime::Zero);
        systemState_.Reset();
        oldActionRetryTimes_ = PLBConfig::GetConfig().PLBActionRetryTimes;
        PLBConfig::GetConfig().PLBActionRetryTimes = 1;

        oldPlacementInterval_ = PLBConfig::GetConfig().MinPlacementInterval;
        oldConstraintCheckInterval_ = PLBConfig::GetConfig().MinConstraintCheckInterval;
        oldBalancingInterval_ = PLBConfig::GetConfig().MinLoadBalancingInterval;
        oldRefreshInterval_ = PLBConfig::GetConfig().PLBRefreshInterval;

        PLBConfig::GetConfig().MinPlacementInterval = TimeSpan::FromSeconds(0);
        PLBConfig::GetConfig().MinConstraintCheckInterval = TimeSpan::FromSeconds(0);
        PLBConfig::GetConfig().MinLoadBalancingInterval = TimeSpan::FromMinutes(1);
        // Set refresh interval to default value, so that we use new timers by default
        PLBConfig::GetConfig().PLBRefreshInterval = PLBConfig::GetConfig().PLBRefreshIntervalEntry.DefaultValue;

        return true;
    }

    bool TestPLBScheduler::TestCleanup()
    {
        PLBConfig::GetConfig().PLBActionRetryTimes = oldActionRetryTimes_;

        PLBConfig::GetConfig().MinPlacementInterval = oldPlacementInterval_;
        PLBConfig::GetConfig().MinConstraintCheckInterval = oldConstraintCheckInterval_;
        PLBConfig::GetConfig().MinLoadBalancingInterval = oldBalancingInterval_;
        PLBConfig::GetConfig().PLBRefreshInterval = oldRefreshInterval_;
        return true;
    }

    void TestPLBScheduler::BalancingConfigVariableBalancingDisabledThrottle(bool /*ignore*/)
    {
        Trace.WriteInfo(PLBSchedulerTestSource, "Starting BalancingConfigVariableBalancingDisabledThrottle");
        // Test the scheduler when balancing is enabled or disabled via PLBConfig setting
        auto now = GetCurrentTime();

        // System is not balanced throughout the test
        systemState_.SetIsBalanced(false);

        // Placement and constraint check should not run during this test
        PLBConfigScopeChange(MinPlacementInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));
        PLBConfigScopeChange(MinConstraintCheckInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(1000));
        PLBConfigScopeChange(MinLoadBalancingInterval, Common::TimeSpan, Common::TimeSpan::FromSeconds(5));

        // Disable balancing via config switch
        PLBConfigScopeChange(LoadBalancingEnabled, bool, false);


        // Report that load has changed, so that we have a change in the system
        plbScheduler_->OnLoadChanged(now);

        // First balancing run should be throttled
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::NoActionNeeded);

        // Enable balancing
        ScopeChangeObjectLoadBalancingEnabled.SetValue(true);

        // Second balancing run should see that balancing is enabled and pick up load change, and should run
        now = now + Common::TimeSpan::FromSeconds(6);
        plbScheduler_->RefreshAction(systemState_, now);
        VerifyScheduler(PLBSchedulerActionType::QuickLoadBalancing);
    }

    FailoverUnitMovement TestPLBScheduler::GetMovement(size_t id) const
    {
        return FailoverUnitMovement(
            GetGuid(id),
            wstring(L"Service"),
            true,
            0,
            false,
            vector<FailoverUnitMovement::PLBAction>());
    }

    Guid TestPLBScheduler::GetGuid(size_t id) const
    {
        return Guid(wformatString("00000000-0000-0000-0000-{0:012}", id));
    }

    void TestPLBScheduler::ActionExecutedWithMovePlan(
        StopwatchTime timeStamp,
        double newAvgStdDev,
        map<Guid, FailoverUnitMovement> const& movementList)
    {
        plbScheduler_->OnMovementGenerated(timeStamp, newAvgStdDev, movementList);
    }
}
