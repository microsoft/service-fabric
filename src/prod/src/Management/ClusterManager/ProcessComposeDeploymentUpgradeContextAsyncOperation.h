// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessComposeDeploymentUpgradeAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessComposeDeploymentUpgradeAsyncOperation(
                __in RolloutManager &,
                __in ComposeDeploymentUpgradeContext &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessComposeDeploymentUpgradeAsyncOperation() { }

            __declspec(property(get=get_Context)) ComposeDeploymentUpgradeContext & ComposeUpgradeContext;
            ComposeDeploymentUpgradeContext & get_Context() const { return context_; }

            void OnStart(Common::AsyncOperationSPtr const &) override;

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);
        private:
            class InnerProvisionAsyncOperation;
            class InnerApplicationUpgradeAsyncOperation;
            class InnerUnprovisionAsyncOperation;

            void StartProvisionApplicationType(Common::AsyncOperationSPtr const &);
            void OnProvisionComposeDeploymentTypeComplete(Common::AsyncOperationSPtr const &, bool);

            void StartUpgradeApplication(Common::AsyncOperationSPtr const &);
            void OnUpgradeApplicationComplete(Common::AsyncOperationSPtr const &, bool);

            void StartUnprovisionApplicationType(Common::AsyncOperationSPtr const &);
            void InnerUnprovisionApplicationType(Common::AsyncOperationSPtr const &);
            void OnInnerUnprovisionComplete(Common::AsyncOperationSPtr const &, bool);

            void FinishUpgrade(Common::AsyncOperationSPtr const &, Store::StoreTransaction &);
            void OnFinishUpgradeComplete(Common::AsyncOperationSPtr const &, bool);

            Common::ErrorCode RefreshApplicationUpgradeContext();

            Common::ErrorCode GetStoreDataComposeDeploymentInstance(StoreDataComposeDeploymentInstance &);
            Common::ErrorCode GetApplicationTypeContextForUnprovisioning(Store::StoreTransaction const &);
            Common::ErrorCode UpdateContextStatusAfterUnprovision(Store::StoreTransaction &storeTx);

            ComposeDeploymentUpgradeContext &context_;
            ApplicationTypeContext targetApplicationTypeContext_;
            std::unique_ptr<ApplicationTypeContext> applicationContextToUnprovision_;
            std::unique_ptr<ApplicationUpgradeContext> applicationUpgradeContext_;
        };
    }
}
