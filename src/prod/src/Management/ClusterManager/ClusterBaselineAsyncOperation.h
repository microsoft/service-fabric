// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClusterBaselineAsyncOperation : public ProcessRolloutContextAsyncOperationBase
        {
        private:
            static Common::GlobalWString TargetPackageFilename;
            static Common::GlobalWString PackageFilenamePattern;
            static Common::GlobalWString TargetClusterManifestFilename;

        public:
            ClusterBaselineAsyncOperation(
                __in RolloutManager & rolloutManager,
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

        private:

            void StartBaselineIfNeeded(Common::AsyncOperationSPtr const &);
            void InitializePaths(Common::AsyncOperationSPtr const &);
            Common::ErrorCode InitializeMsiPath();

            void UploadIfNeeded(Common::AsyncOperationSPtr const &);
            void StartProvisionIfNeeded(Common::AsyncOperationSPtr const &);

            void OnCommitProvisionComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void OnProvisionComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void StartUpgradeIfNeeded(Common::AsyncOperationSPtr const &);
            void OnCommitUpgradeComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            static bool ShouldDoBaseline(FabricUpgradeContext const &);
            static bool ShouldAbortBaseline(Common::ErrorCode const &);

        private:
            std::wstring dataRoot_;
            std::wstring msiPath_;
            std::wstring clusterManifestPath_;
            Common::FabricVersion baselineVersion_;
            std::unique_ptr<FabricUpgradeContext> upgradeContext_;
            std::unique_ptr<FabricProvisionContext> provisionContext_;
        };
    }
}
