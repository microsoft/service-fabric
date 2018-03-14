// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Query
        {
            class HostQueryAsyncOperation : public Common::AsyncOperation
            {
                DENY_COPY(HostQueryAsyncOperation)
            public:
                HostQueryAsyncOperation(
                    ReconfigurationAgent & ra,
                    std::wstring activityId,
                    Infrastructure::EntityEntryBaseSPtr const & entityEntry,
                    wstring const & runtimeId,
                    FailoverUnitDescription const & failoverUnitDescription,
                    ReplicaDescription const & replicaDescription,
                    wstring const & hostId,
                    Common::AsyncCallback const& callback,
                    Common::AsyncOperationSPtr const& parent);

                ~HostQueryAsyncOperation();

                ServiceModel::DeployedServiceReplicaDetailQueryResult GetReplicaDetailQueryResult();

            protected:
                void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;

            private:
                void OnRequestReplyCompleted(
                    Common::AsyncOperationSPtr const & operation);

                Common::ErrorCode ProcessReply(
                    Common::AsyncOperationSPtr const & operation);

            private:
                ReconfigurationAgent & ra_;
                std::wstring activityId_;
                Infrastructure::EntityEntryBaseSPtr const & entityEntry_;
                std::wstring const & runtimeId_;
                FailoverUnitDescription const & failoverUnitDescription_;
                ReplicaDescription const & replicaDescription_;
                std::wstring const & hostId_;
                ServiceModel::DeployedServiceReplicaDetailQueryResult replicaDetailQueryResult_;
            };
        }
    }
}
