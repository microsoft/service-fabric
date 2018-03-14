// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClearApplicationUpgradeContextAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ClearApplicationUpgradeContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationUpgradeContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode ClearUpgradingStatus(Store::StoreTransaction const &storeTx, ApplicationUpgradeContext &context, bool);

        private:
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            ApplicationUpgradeContext & context_;
        };
    }
}

