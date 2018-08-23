// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo parent for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceManager
    {
        class ResourceManagerCore
            : public Common::ComponentRoot
            , public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ResourceManager>
        {
            using Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ResourceManager>::TraceId;
            using Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ResourceManager>::get_TraceId;

            DENY_COPY(ResourceManagerCore)
        public:
            ResourceManagerCore(
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Store::IReplicatedStore * replicatedStore,
                std::wstring const & resourceTypeName);

            Common::AsyncOperationSPtr RegisterResource(
                __in std::wstring const & resourceName,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const &);

            Common::AsyncOperationSPtr RegisterResources(
                __in std::vector<std::wstring> const & resourceNames,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const &);

            Common::AsyncOperationSPtr UnregisterResource(
                __in std::wstring const & resourceName,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const &);

            Common::AsyncOperationSPtr UnregisterResources(
                __in std::vector<std::wstring> const & resourceNames,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const &);

            Common::AsyncOperationSPtr ClaimResource(
                __in Claim const & claim,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const &);

            Common::AsyncOperationSPtr ReleaseResource(
                __in Claim const & claim,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const &);
        private:
            std::wstring const resourceTypeName_;
            Store::PartitionedReplicaId const partitionReplicaId_;
            Store::IReplicatedStore * replicatedStore_;

            Store::StoreTransaction CreateStoreTransaction(ActivityId const & activityId);
            Common::ErrorCode InternalUnregisterResource(
                Store::StoreTransaction & storeTx,
                std::wstring const & resourceName);
        };
    }
}