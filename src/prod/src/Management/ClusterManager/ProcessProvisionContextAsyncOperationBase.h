// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        //
        // This is the base class for 
        // 1. Provision application type triggered directly by user.
        // 2. Implicit provision triggered when creating an application described by
        // a docker compose file.
        //
        // In case of 1. the ApplicationTypeContext is persisted when the provision operation is accepted
        // In case of 2. the ApplicationTypeContext is persisted only when the provision operation succeeds.
        // The details necessary to restart the provision are persisted in the ContainerApplicationContext.
        //
        class ProcessProvisionContextAsyncOperationBase :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessProvisionContextAsyncOperationBase(
                __in RolloutManager &,
                __in ApplicationTypeContext &,
                Common::StringLiteral const traceComponent,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const &) override;

            virtual void StartProvisioning(Common::AsyncOperationSPtr const &) = 0;
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            bool ShouldRefreshAndRetry(Common::ErrorCode const &);
            Common::ErrorCode CheckApplicationTypes_IgnoreCase();

            //
            // Properties
            //
            __declspec (property(get=get_TraceComponent)) Common::StringLiteral const & TraceComponent;
            Common::StringLiteral const & get_TraceComponent() const { return traceComponent_; }

            __declspec (property(get=get_AppTypeContext)) ApplicationTypeContext & AppTypeContext;
            ApplicationTypeContext & get_AppTypeContext() { return context_; }

        private:
            bool ShouldCleanupApplicationPackage() const;

        protected:
            bool startedProvision_;
            std::shared_ptr<StoreDataApplicationManifest> appManifest_;
            std::shared_ptr<StoreDataServiceManifest> svcManifest_;
            Common::ErrorCode provisioningError_;

        private:
            Common::StringLiteral traceComponent_;
            ApplicationTypeContext & context_;
        };
    }
}
