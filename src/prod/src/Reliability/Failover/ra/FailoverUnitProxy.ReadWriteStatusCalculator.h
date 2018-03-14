// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class FailoverUnitProxy::ReadWriteStatusCalculator 
        {
        public:
            ReadWriteStatusCalculator();

            std::pair<AccessStatus::Enum, AccessStatus::Enum> ComputeReadAndWriteStatus(Common::AcquireExclusiveLock & fupLock, FailoverUnitProxy const & fup) const;

        private:
            class LifeCycleState
            {
            public:
                enum Enum
                {
                    OpeningPrimary,
                    ReadyPrimary,
                    Other,
                    COUNT = Other + 1
                };
            };

            class ReconfigurationState
            {
            public:
                enum Enum
                {
                    PreWriteStatusCatchup = 0,
                    TransitioningRole,
                    CatchupInProgress,
                    CatchupCompleted,
                    Completed,
                    COUNT = Completed + 1
                };
            };

            class AccessStatusValue
            {
            public:
                AccessStatusValue();
                AccessStatusValue(AccessStatus::Enum value, bool isDynamic, bool needsPC_, bool isInvalid);

                AccessStatus::Enum Get(Common::AcquireExclusiveLock & fupLock, FailoverUnitProxy const & owner) const;

            private:

                AccessStatus::Enum value_;
                bool isDynamic_;
                bool needsPC_;
                bool isInvalid_;
            };

            struct AccessStatusValueCollection
            {
                AccessStatusValue LifeCycleState[LifeCycleState::COUNT];
                AccessStatusValue PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::COUNT];
                AccessStatusValue DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::COUNT];
                AccessStatusValue NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::COUNT];
                AccessStatusValue IdleToActiveReconfigurationAccessState[ReconfigurationState::COUNT];
            };

            std::pair<AccessStatus::Enum, AccessStatus::Enum> ComputeAccessStatus(
                AccessStatusValueCollection const & readStateValue,
                AccessStatusValueCollection const & writeStateValue,
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup) const;

            std::pair<AccessStatus::Enum, AccessStatus::Enum> ComputeAccessStatusHelper(
                AccessStatusValue const & readStateValue,
                AccessStatusValue const & writeStateValue,
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup
                ) const;

            bool TryGetLifeCycleState(
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup,
                __out LifeCycleState::Enum & lifeCycleState) const;

            bool TryGetPromoteToPrimaryReconfigurationState(
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup,
                __out ReconfigurationState::Enum & reconfigurationState) const;

            bool TryGetDemoteToSecondaryReconfigurationState(
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup,
                __out ReconfigurationState::Enum & reconfigurationState) const;

            bool TryGetNoPrimaryChangeReconfigurationState(
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup,
                __out ReconfigurationState::Enum & reconfigurationState) const;

            bool TryGetIdleToActiveReconfigurationState(
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup,
                __out ReconfigurationState::Enum & reconfigurationState) const;

            ReconfigurationState::Enum GetReconfigurationStateForPrimary(
                Common::AcquireExclusiveLock & fupLock,
                FailoverUnitProxy const & fup) const;

            AccessStatusValueCollection readStatus_;
            AccessStatusValueCollection writeStatus_;
        };
    }
}
