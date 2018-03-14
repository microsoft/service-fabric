// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class FailoverUnit;

        class Service;

        class FailoverUnitMovement;
        typedef std::map<Common::Guid, FailoverUnitMovement> FailoverUnitMovementTable;

        class MovePlan
        {
            DENY_COPY(MovePlan);

        public:

            MovePlan();

            MovePlan(MovePlan && other);

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return pendingMovements_.empty() && ongoingMovements_.empty(); }

            __declspec (property(get = get_NumberOfPendingMovements)) size_t NumberOfPendingMovements;
            size_t get_NumberOfPendingMovements() const { return pendingMovements_.size(); }

            void Merge(MovePlan && other);

            void OnNodeUp();
            void OnNodeDown();
            void OnNodeChanged();
            void OnServiceTypeChanged();

            void OnServiceChanged(Service const& service);
            void OnServiceDeleted(std::wstring const& serviceName);
            void OnFailoverUnitAdded(FailoverUnit const& failoverUnit);
            void OnFailoverUnitChanged(FailoverUnit const& oldFU, FailoverUnitDescription const& newFU);
            void OnFailoverUnitDeleted(FailoverUnit const& failoverUnit);
            void OnLoadChanged();
            void OnConstraintCheckDisabled();
            void OnBalancingDisabled();

            void OnMovementGenerated(FailoverUnitMovementTable && movementList, Common::StopwatchTime timestamp);

            void Refresh(Common::StopwatchTime now);

            FailoverUnitMovementTable GetMovements(Common::StopwatchTime timestamp);

            void ClearOngoingMovements();

        private:
            void ClearPendingMovements();

            FailoverUnitMovementTable pendingMovements_;
            Common::StopwatchTime pendingMovementsTime_;
            std::set<Common::Guid> ongoingMovements_;
            Common::StopwatchTime ongoingMovementsTime_;

            // TODO: update the two count when we allow dynamic configuration on the two throttling
            std::set<std::wstring> inBuildThrottledServiceNames_;
            int64 inBuildReplicaCount_;
            std::set<std::wstring> swapPrimaryThrottledServiceNames_;
            int64 swapPrimaryReplicaCount_;
        };
    }
}
