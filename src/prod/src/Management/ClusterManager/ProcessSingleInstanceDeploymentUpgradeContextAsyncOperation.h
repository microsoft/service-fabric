//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation
            : public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessSingleInstanceDeploymentUpgradeContextAsyncOperation(
                __in RolloutManager &,
                __in SingleInstanceDeploymentUpgradeContext &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            __declspec(property(get=get_Context)) SingleInstanceDeploymentUpgradeContext const & Context;
            SingleInstanceDeploymentUpgradeContext const & get_Context() const { return context_; }

            void OnStart(Common::AsyncOperationSPtr const &) override;

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            class InnerApplicationUpgradeAsyncOperation;
            // This inner async op is to integrate store commit in single transaction.
            // It doesn't change functions to unprovision itself.
            class InnerUnprovisionAsyncOperation;

            void StartApplicationUpgrade(Common::AsyncOperationSPtr const &);
            void OnUpgradeApplicationComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void UnprovisionApplicationType(Common::AsyncOperationSPtr const &);
            void OnUnprovisionApplicationTypeComplete(Common::AsyncOperationSPtr const &, bool);

            // Additional handle when applicationTypeToUnprovision is not found
            void FinishUpgrade(Common::AsyncOperationSPtr const &, Store::StoreTransaction &);
            void OnFinishUpgradeComplete(Common::AsyncOperationSPtr const &, bool);

            Common::ErrorCode UpdateContextStatusAfterUnprovision(Store::StoreTransaction & storeTx);

            void TryScheduleRetry(Common::ErrorCode const &, Common::AsyncOperationSPtr const &, RetryCallback const &);
            bool IsRetryable(Common::ErrorCode const & error) const;
            void StartTimer(Common::AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback, Common::TimeSpan const delay);
            Common::ErrorCode RefreshContext();
            Common::ErrorCode RefreshApplicationUpgradeContext();

            SingleInstanceDeploymentUpgradeContext context_;
            ApplicationContext applicationContext_;
            ApplicationUpgradeContext applicationUpgradeContext_;
            ApplicationTypeContext targetApplicationTypeContext_;
            std::unique_ptr<ApplicationTypeContext> applicationTypeContextToUnprovision_;
            Common::TimerSPtr timerSPtr_;
            Common::ExclusiveLock timerLock_;
        };
    }
}
