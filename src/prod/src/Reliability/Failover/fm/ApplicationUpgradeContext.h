// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ApplicationUpgradeContext : public UpgradeContext
        {
            DENY_COPY(ApplicationUpgradeContext);

        public:
            ApplicationUpgradeContext(
                ApplicationInfoSPtr const & application,
                std::wstring const & currentDomain);

            virtual BackgroundThreadContextUPtr CreateNewContext() const;

            virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit);

            virtual void Merge(BackgroundThreadContext const& context);

            virtual void Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted);

        private:
            virtual bool IsReplicaSetCheckNeeded() const;

            virtual bool IsReplicaWaitNeeded(Replica const& replica) const;

            virtual bool IsReplicaMoveNeeded(Replica const& replica) const;

            void ProcessDeletedService(FailoverUnit const& failoverUnit);

            ApplicationInfoSPtr application_;

            std::map<std::wstring, uint64> servicesToDelete_;

            bool isUpgradeNeededOnCurrentUD_;
        };
    }
}
