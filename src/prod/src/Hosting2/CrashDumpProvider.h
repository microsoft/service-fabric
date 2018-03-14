// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CrashDumpProvider : 
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(CrashDumpProvider)

    public:
        explicit CrashDumpProvider(
            Common::ComponentRoot const & root,
            EnvironmentManager & environmentManager);
        virtual ~CrashDumpProvider();

        void SetupApplicationCrashDumps(
            std::wstring const & applicationId,
            ServiceModel::ApplicationPackageDescription const & applicationPackageDescription);

        void CleanupApplicationCrashDumps(std::wstring const & applicationId);

        Common::AsyncOperationSPtr BeginSetupServiceCrashDumps(
            ServiceModel::ServicePackageDescription const & servicePackageDescription,
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndSetupServiceCrashDumps(
            Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginCleanupServiceCrashDumps(
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndCleanupServiceCrashDumps(
            Common::AsyncOperationSPtr const & asyncOperation);

        void CleanupServiceCrashDumps(
            ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext
            );

    protected:
        // ****************************************************
        // AsyncFabricComponent specific methods
        // ****************************************************
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndOpen(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndClose(
            Common::AsyncOperationSPtr const & operation);

        virtual void OnAbort();

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class SetupCrashDumpAsyncOperation;
        class CleanupCrashDumpAsyncOperation;

    private:
        bool ShouldConfigureCrashDumpCollection(std::wstring const & applicationId);

    private:
        EnvironmentManager & environmentManager_;
        std::set<std::wstring> appSetCollectingCrashDumps_;
        Common::RwLock appSetCollectingCrashDumpsLock_;
        bool appSetCollectingCrashDumpsIsClosed_;
    };
}
