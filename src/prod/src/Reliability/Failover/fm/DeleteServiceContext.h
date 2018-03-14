// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class DeleteServiceContext : public BackgroundThreadContext
        {
            DENY_COPY(DeleteServiceContext);

        public:
            DeleteServiceContext(std::wstring const & serviceName, uint64 serviceInstance);

            virtual BackgroundThreadContextUPtr CreateNewContext() const;

            virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit);

            virtual void Merge(BackgroundThreadContext const & context);

            virtual bool ReadyToComplete();

            virtual void Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted);

        private:
            std::wstring serviceName_;
            uint64 serviceInstance_;

            int failoverUnitCount_;
            FailoverUnitId failoverUnitId_;
        };
    }
}
