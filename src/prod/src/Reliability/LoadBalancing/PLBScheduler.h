// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "PLBEventSource.h"
#include "TimedCounter.h"
#include "PLBSchedulerAction.h"
#include "FailoverUnitMovement.h"
#include "DiagnosticsDataStructures.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ISystemState;
        class PLBDiagnostics;
        typedef std::shared_ptr<PLBDiagnostics> PLBDiagnosticsSPtr;

        class PLBScheduler
        {
            DENY_COPY(PLBScheduler);

        public:
            PLBScheduler(
                std::wstring const& serviceDomainId,
                PLBEventSource const& trace,
                bool constraintCheckEnabled,
                bool balancingEnabled,
                Common::StopwatchTime lastNodeDown,
                Common::StopwatchTime lastBalancing,
                Common::StopwatchTime lastNewNode);

            PLBScheduler(PLBScheduler && other);

            __declspec (property(get=get_CurrentAction)) PLBSchedulerAction CurrentAction;
            PLBSchedulerAction get_CurrentAction() const { return currentAction_; }

            void OnNodeUp(Common::StopwatchTime timeStamp);
            void OnNodeDown(Common::StopwatchTime timeStamp);
            void OnNodeChanged(Common::StopwatchTime timeStamp);
            void OnServiceTypeChanged(Common::StopwatchTime timeStamp);

            void OnServiceChanged(Common::StopwatchTime timeStamp);
            void OnServiceDeleted(Common::StopwatchTime timeStamp);
            void OnFailoverUnitAdded(Common::StopwatchTime timeStamp, Common::Guid fuId);
            void OnFailoverUnitChanged(Common::StopwatchTime timeStamp, Common::Guid fuId);
            void OnFailoverUnitDeleted(Common::StopwatchTime timeStamp, Common::Guid fuId);
            void OnLoadChanged(Common::StopwatchTime timeStamp);
            void OnDomainInterrupted(Common::StopwatchTime timeStamp);

            // test only method
            void SetAction(PLBSchedulerActionType::Enum action);

            void RefreshAction(ISystemState const & systemState, Common::StopwatchTime timeStamp, SchedulingDiagnostics::ServiceDomainSchedulerStageDiagnosticsSPtr stageDiagnosticsSPtr = nullptr);

            void OnMovementGenerated(
                Common::StopwatchTime timeStamp,
                double newAvgStdDev = -1.0,
                FailoverUnitMovementTable const& movementList = FailoverUnitMovementTable());

            bool IsSystemChangedSince(Common::StopwatchTime timeStamp) const;
            std::set<Common::Guid> GetMovementThrottledFailoverUnits() const;
            void Merge(Common::StopwatchTime timeStamp, PLBScheduler && other);

            void SetConstraintCheckEnabled(bool constraintCheckEnabled, Common::StopwatchTime timeStamp);
            void SetBalancingEnabled(bool balancingEnabled, Common::StopwatchTime timeStamp);

            // Checks when should refresh be called next
            Common::TimeSpan GetNextRefreshInterval(Common::StopwatchTime timestamp);

        private:
            static const size_t FUMovementCounterWindowCount = 2;

            void RefreshTimedCounters(Common::StopwatchTime timeStamp);
            void SystemStateChanged(Common::StopwatchTime timeStamp);
            bool IsBalancingThrottled(Common::StopwatchTime timeStamp, ISystemState const & systemState);
            bool IsConstraintCheckThrottled(Common::StopwatchTime timeStamp);
            bool IsCreationWithMoveThrottled(ISystemState const & systemState);
            bool ThrottleBalancingForSmallStdDevChange(ISystemState const & systemState) const;
            bool AreMovementsThrottled(ISystemState const & systemState,
                size_t currentMovementCount,
                double allowedPercentageToMove,
                uint maxReplicasToMove);
            bool ThrottleGlobalMovements(ISystemState const & systemState, std::wstring const& action);
            bool ThrottleGlobalBalancingMovements(ISystemState const & systemState);
            bool ThrottleGlobalPlacementMovements(ISystemState const & systemState);
            bool ThrottleBalancingForPerFUMovements(ISystemState const & systemState);
            void UpdateMovements(Common::StopwatchTime timeStamp, FailoverUnitMovementTable const& movementList);

            Common::TimeSpan GetMinConstraintCheckInterval();
            Common::TimeSpan GetMinPlacementInterval();

            std::wstring serviceDomainId_;
            PLBEventSource const& trace_;

            bool constraintCheckEnabled_; // control whether constraint check is enabled
            bool balancingEnabled_; // control whether load balancing is enabled

            Common::StopwatchTime lastStateChange_; // the state of nodes, service types, services, failover units changed
            Common::StopwatchTime lastLoadChange_; // only the load changed
            Common::StopwatchTime lastNodeDown_;
            Common::StopwatchTime lastNewNode_;

            PLBSchedulerAction currentAction_;
            // Needed to keep correct transition between NewReplicaPlacement and NewReplicaPlacementWithMove
            PLBSchedulerAction placementAction_;
            // Needed to keep correct transition between fast and slow balancing
            PLBSchedulerAction balancingAction_;

            Common::StopwatchTime lastActionTime_;
            bool hasLastBalancingAvgStdDev_;
            double lastBalancingAvgStdDev_;

            std::map<Common::Guid, TimedCounter> movementCounterPerFU_;



            Common::StopwatchTime lastPlacement_;
            Common::StopwatchTime lastConstraintCheck_;
            Common::StopwatchTime lastBalancing_;
        };
    }
}
