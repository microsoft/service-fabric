// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FabricUpgradeContext : public UpgradeContext
        {
            DENY_COPY(FabricUpgradeContext);

        public:
            FabricUpgradeContext(
                std::wstring const& fmId,
                FabricUpgrade const& upgrade,
                std::wstring const & currentDomain);

            FabricUpgradeContext(
                std::wstring const& fmId,
                FabricUpgrade const& upgrade,
                std::wstring const & currentDomain,
                int voterCount);

            virtual BackgroundThreadContextUPtr CreateNewContext() const;

            // Initialize does the following:
            //
            // 1. Adds all the nodes in the current domain to the ready list.
            //    This is because it is possible that there are some node that
            //    do not have any replica hosted on them. If this is the the case
            //    then these nodes are not added during background processing.
            //    However, if there are any exceptions (because of inducing quorum
            //    loss), such nodes are moved to the pending list during Merge.
            //
            // 2. Processes the seed nodes to ensure that fabric upgrade does not
            //    cause global lease loss.
            //
            // 3. Try to lock all the nodes that the in the current UD. This is to
            //    ensure that there is no node which was locked when the upgrade was
            //    in the previous UD and that could have incorrectly passed the
            //    gatekeeping.
            bool Initialize(FailoverManager & fm);

            virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit);

            virtual void Merge(BackgroundThreadContext const& context);

            virtual void Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted);

        private:
            virtual bool IsReplicaSetCheckNeeded() const;

            virtual bool IsReplicaWaitNeeded(Replica const& replica) const;

            virtual bool IsReplicaMoveNeeded(Replica const& replica) const;

            void ProcessSeedNodes(FailoverManager & fm);

            std::wstring fmId_;

            FabricUpgrade upgrade_;

            bool isSafeToUpgrade_;

            int voterCount_;

            static std::wstring const SeedNodeTag;
        };
    }
}
