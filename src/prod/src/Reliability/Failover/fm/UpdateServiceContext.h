// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class UpdateServiceContext : public BackgroundThreadContext
        {
            DENY_COPY(UpdateServiceContext);

        public:

            UpdateServiceContext(ServiceInfoSPtr const& serviceInfo);

            virtual BackgroundThreadContextUPtr CreateNewContext() const;

            virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit);

            virtual void Merge(BackgroundThreadContext const& context);

            virtual bool ReadyToComplete();

            virtual void Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted);

        private:

            void AddReadyNode(Federation::NodeId nodeId);

            ServiceInfoSPtr serviceInfo_;

            std::set<Federation::NodeId> readyNodes_;
            int pendingCount_;

            FailoverUnitId failoverUnitId_;
        };
    }
}
