// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            // This class processes Upgrade messages for the RA
            // When an upgrade arrives at the RA the RA should construct 
            // the appropriate state machine and pass that in
            class UpgradeMessageProcessor
            {
                DENY_COPY(UpgradeMessageProcessor);
            public:              

                struct UpgradeElement
                {
                    UpgradeStateMachineSPtr Current;
                    UpgradeStateMachineSPtr Queued;
                    ICancelUpgradeContextSPtr CancellationContext;
                public:

                    bool CanProcess(uint64 incomingInstanceId)
                    {
                        return !HasQueuedOperation && !IsStale(incomingInstanceId);
                    }

                private:
                    __declspec(property(get=get_HasQueuedOperation)) bool HasQueuedOperation;
                    bool get_HasQueuedOperation() const 
                    { 
                        return Current != nullptr && (Queued != nullptr || CancellationContext != nullptr); 
                    }

                    bool IsStale(uint64 incomingInstanceId)
                    {
                        if (Current != nullptr && incomingInstanceId < Current->InstanceId)
                        {
                            return true;
                        }

                        if (CancellationContext != nullptr && incomingInstanceId < CancellationContext->InstanceId)
                        {
                            return true;
                        }

                        return false;
                    }
                };
                
                typedef std::map<std::wstring, UpgradeElement> UpgradeMessageProcessorMap;

                UpgradeMessageProcessor(ReconfigurationAgent & ra);

                void Close();

                // Return whether the new upgrade was started or not
                bool ProcessUpgradeMessage(std::wstring const & applicationId, UpgradeStateMachineSPtr const & stateMachine);

                void ProcessCancelUpgradeMessage(std::wstring const & applicationId, ICancelUpgradeContextSPtr const & cancelUpgradeContext);

                // For writing tests
                __declspec(property(get=get_Test_UpgradeMap)) UpgradeMessageProcessorMap const & Test_UpgradeMap;
                UpgradeMessageProcessorMap const & get_Test_UpgradeMap() const { return upgrades_; }

                UpgradeStateName::Enum Test_GetUpgradeState(std::wstring const & identifier);

                void OnCancellationComplete(std::wstring const & applicationId, UpgradeStateMachineSPtr const &);

                // Returns whether the app id is in flight
                // The state should not be completed
                bool IsUpgrading(std::wstring const & applicationId) const;

            private:

                template<typename T> 
                static uint64 GetInstanceId(std::shared_ptr<T> const & ptr)
                {
                    return ptr != nullptr ? ptr->InstanceId : 0;
                }

                class Action
                {
                public:
                    enum Enum
                    {
                        None = 0,
                        Close = 1,
                        SendReplyAndClose = 2,
                        Start = 3,
                        StartRollback = 4,
                        Queue = 5,
                    };
                };

                bool DecideCancelAction(
                    std::wstring const & applicationId,
                    ICancelUpgradeContextSPtr const & incomingCancelContext,
                    Action::Enum & existingUpgradeAction,
                    UpgradeStateMachineSPtr & existingUpgrade);

                void DecideUpgradeAction(
                    std::wstring const & applicationId, 
                    uint64 instanceId, 
                    UpgradeStateMachineSPtr const & newUpgrade,
                    Action::Enum & newUpgradeAction,
                    UpgradeStateMachineSPtr & oldUpgrade,
                    Action::Enum & oldUpgradeAction);

                void TraceUpgradeAction(
                    Common::StringLiteral const & source, 
                    Common::StringLiteral const & tag, 
                    UpgradeStateMachineSPtr const & existing, 
                    UpgradeStateMachineSPtr const & incomingUpgrade, 
                    ICancelUpgradeContextSPtr const & existingcancelContext,
                    ICancelUpgradeContextSPtr const & incomingCancelContext);

                static void PerformAction(Action::Enum action, UpgradeStateMachineSPtr const & upgrade, UpgradeStateMachineSPtr const & previousUpgrade);

                mutable Common::RwLock lock_;
                UpgradeMessageProcessorMap upgrades_;
                ReconfigurationAgent & ra_;
                bool isClosed_;
            };
        }
    }
}

