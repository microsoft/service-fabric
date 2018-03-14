// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Closes a set of partitions with the specified close mode
        // and then monitors the close for completion
        // monitoring is done usin gthe CloseCompletionCheckAsyncop
        // the monitoring part of this async op can be cancelled
        class MultipleReplicaCloseAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(MultipleReplicaCloseAsyncOperation);

        public:
            struct Parameters
            {
                Parameters() :
                    MonitoringIntervalEntry(nullptr),
                    MaxWaitBeforeTerminationEntry(nullptr),
                    RA(nullptr),
                    TerminateReason(Hosting::TerminateServiceHostReason::FabricUpgrade),
                    JobItemCheck(Infrastructure::JobItemCheck::None)
                {
                }

                Infrastructure::IClockSPtr Clock;
                ReconfigurationAgent * RA;

                // ActivityId: Used for tracing
                std::wstring ActivityId;

                // The FTs that are being closed
                Infrastructure::EntityEntryBaseList FTsToClose;

                // Job Item check to be used to close or query close completion
                Infrastructure::JobItemCheck::Enum JobItemCheck;

                // The close mode to be used
                ReplicaCloseMode CloseMode;

                // See MultipleReplicaCloseCompletionCheckAsyncOperation for the below entries
                TimeSpanConfigEntry const * MonitoringIntervalEntry;

                TimeSpanConfigEntry const * MaxWaitBeforeTerminationEntry;

                MultipleReplicaCloseCompletionCheckAsyncOperation::PendingReplicaCallback Callback;

                Hosting::TerminateServiceHostReason::Enum TerminateReason;

                MultipleReplicaCloseCompletionCheckAsyncOperation::IsReplicaClosedFunctionPtr IsReplicaClosedFunction;
            };

            MultipleReplicaCloseAsyncOperation(
                Parameters && parameters,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            __declspec(property(get = get_CloseCompletionCheckAsyncOperation)) MultipleReplicaCloseCompletionCheckAsyncOperationSPtr CloseCompletionCheckAsyncOperation;
            MultipleReplicaCloseCompletionCheckAsyncOperationSPtr get_CloseCompletionCheckAsyncOperation() const
            {
                return completionCheckAsyncOp_.lock();
            }

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

            void OnCancel() override;

        private:
            void OnCloseCompleted(Common::AsyncOperationSPtr const & thisSPtr);

            bool Processor(
                Infrastructure::HandlerParameters & handlerParameters,
                JobItemContextBase &);

            MultipleReplicaCloseCompletionCheckAsyncOperation::Parameters parameters_;

            // Weak ptr to avoid leaks
            MultipleReplicaCloseCompletionCheckAsyncOperationWPtr completionCheckAsyncOp_;
        };
    }
}



