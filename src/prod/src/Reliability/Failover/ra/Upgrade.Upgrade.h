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
            typedef std::function<void (UpgradeStateName::Enum)> AsyncStateActionCompleteCallback;

            // Represents an upgrade on the node
            class IUpgrade
            {
                DENY_COPY(IUpgrade);

            public:
                virtual ~IUpgrade()
                {
                }

                // Called by the state machine if a resend of the upgrade reply is needed
                // This is only called after the state machine is completed and a duplicate message is received from FM for upgrade
                // This usually indicates that the original upgrade reply from RA was dropped
                // This cannot perform an async operation
                virtual void SendReply() = 0;

                // Return the upgrade state description for a state name
                virtual UpgradeStateDescription const & GetStateDescription(UpgradeStateName::Enum state) const = 0;

                // Enter a specific state
                // This should return the next state as its return value if the processing was synchronous
                // Otherwise it should return UpgradeStateName::Invalid and call the callback with the target state if processing was asynchronous
                // Capturing the asyncCallback by value guarantees the lifetime of the IUpgrade object
                virtual UpgradeStateName::Enum EnterState(UpgradeStateName::Enum state, AsyncStateActionCompleteCallback asyncCallback) = 0;

                virtual Common::AsyncOperationSPtr EnterAsyncOperationState(
                    UpgradeStateName::Enum state,
                    Common::AsyncCallback const & asyncCallback) = 0;

                virtual UpgradeStateName::Enum ExitAsyncOperationState(
                    UpgradeStateName::Enum state,
                    Common::AsyncOperationSPtr const & asyncOp) = 0;

                __declspec(property(get=get_InstanceId)) uint64 InstanceId;
                uint64 get_InstanceId() const { return instanceId_; }

                virtual UpgradeStateName::Enum GetStartState(RollbackSnapshotUPtr && rollbackSnapshot) = 0;

                virtual RollbackSnapshotUPtr CreateRollbackSnapshot(UpgradeStateName::Enum state) const = 0;

                virtual std::wstring const & GetActivityId() const = 0;

                virtual void WriteToEtw(uint16 contextSequenceId) const = 0;
                virtual void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const = 0;
                void TraceLifeCycle(Common::StringLiteral const & tag);
                void TraceAsyncOperationCancel(UpgradeStateName::Enum state, Common::AsyncOperation& op);
                void TraceAsyncOperationStart(UpgradeStateName::Enum state, Common::AsyncOperation& op);
                void TraceAsyncOperationEnd(UpgradeStateName::Enum state, Common::AsyncOperation& op, Common::ErrorCode const & error);

            protected:
                IUpgrade(ReconfigurationAgent & ra, uint64 instanceId)
                : ra_(ra),
                  instanceId_(instanceId)
                {
                }

                ReconfigurationAgent & ra_;

            private:
                uint64 const instanceId_;
            };
        }
    }
}

