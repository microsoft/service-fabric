// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace ReliabilityUnitTest
            {
                static const UpgradeStateCancelBehaviorType::Enum DefaultCancelBehaviorType = UpgradeStateCancelBehaviorType::NonCancellableWithDeferredRollback;

                inline UpgradeStateDescriptionUPtr CreateState(
                    UpgradeStateName::Enum state,
                    UpgradeStateCancelBehaviorType::Enum cancelBehaviorType)
                {
                    return Common::make_unique<UpgradeStateDescription>(state, cancelBehaviorType);
                }

                inline UpgradeStateDescriptionUPtr CreateAsyncApiState(
                    UpgradeStateName::Enum state,
                    UpgradeStateCancelBehaviorType::Enum cancelBehaviorType)
                {
                    return Common::make_unique<UpgradeStateDescription>(state, cancelBehaviorType, true);
                }

                inline UpgradeStateDescriptionUPtr CreateAsyncApiState(
                    UpgradeStateName::Enum state)
                {
                    return CreateAsyncApiState(state, DefaultCancelBehaviorType);
                }

                inline UpgradeStateDescriptionUPtr CreateState(
                    UpgradeStateName::Enum state)
                {
                    return CreateState(state, DefaultCancelBehaviorType);
                }

                class UpgradeStubBase : public IUpgrade
                {
                public:

                    static const bool DefaultSnapshotWereReplicasClosed;

                    typedef std::function<void()> ResendReplyFunctionPtr;
                    typedef std::function<UpgradeStateName::Enum(UpgradeStateName::Enum, AsyncStateActionCompleteCallback)> EnterStateFunctionPtr;

                    typedef std::function<void(UpgradeStateName::Enum)> EnterAsyncOperationStateFunctionPtr;
                    typedef std::function<UpgradeStateName::Enum(UpgradeStateName::Enum, Common::AsyncOperationSPtr const &)> ExitAsyncOperationStateFunctionPtr;
                    typedef ReconfigurationAgentComponent::ReliabilityUnitTest::AsyncApiStub<void*, void*> AsyncOperationStateAsyncApiStub;

                    UpgradeStubBase(
                        Reliability::ReconfigurationAgentComponent::ReconfigurationAgent & ra,
                        uint64 instanceId,
                        UpgradeStateDescriptionUPtr && state1 = UpgradeStateDescriptionUPtr(),
                        UpgradeStateDescriptionUPtr && state2 = UpgradeStateDescriptionUPtr(),
                        UpgradeStateDescriptionUPtr && state3 = UpgradeStateDescriptionUPtr());

                    __declspec(property(get = get_RollbackSnapshotPassedAtStart)) RollbackSnapshot * RollbackSnapshotPassedAtStart;
                    RollbackSnapshot * get_RollbackSnapshotPassedAtStart() { return rollbackSnapshotPassedAtStart_.get(); }

                    void SendReply() override;

                    UpgradeStateName::Enum GetStartState(RollbackSnapshotUPtr && rollbackSnapshot) override;

                    UpgradeStateDescription const & GetStateDescription(UpgradeStateName::Enum state) const override;

                    UpgradeStateName::Enum EnterState(UpgradeStateName::Enum state, AsyncStateActionCompleteCallback asyncCallback) override;

                    Common::AsyncOperationSPtr EnterAsyncOperationState(
                        UpgradeStateName::Enum state,
                        Common::AsyncCallback const & asyncCallback);

                    UpgradeStateName::Enum ExitAsyncOperationState(
                        UpgradeStateName::Enum state,
                        Common::AsyncOperationSPtr const & asyncOp) override;

                    std::wstring const & GetActivityId() const override { return activityId_; }

                    ResendReplyFunctionPtr ResendReplyFunction;
                    EnterStateFunctionPtr EnterStateFunction;
                    EnterAsyncOperationStateFunctionPtr EnterAsyncOperationStateFunction;
                    ExitAsyncOperationStateFunctionPtr ExitAsyncOperationStateFunction;
                    AsyncOperationStateAsyncApiStub AsyncOperationStateAsyncApi;

                    void WriteToEtw(uint16) const {}
                    void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const {}

                    RollbackSnapshotUPtr CreateRollbackSnapshot(UpgradeStateName::Enum state) const;

                    void SetRollbackSnapshot(RollbackSnapshotUPtr && rollbackSnapshot);

                private:
                    void AddToMap(UpgradeStateDescriptionUPtr && state);

                    std::map<UpgradeStateName::Enum, UpgradeStateDescriptionUPtr> stateMap_;
                    std::wstring activityId_;

                    UpgradeStateName::Enum startState_;
                    RollbackSnapshotUPtr rollbackSnapshotPassedAtStart_;
                };
            }
        }
    }
}
