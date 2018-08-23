// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceManager
    {
        template<typename TReceiverContext>
        using RequestHandler = std::function<AsyncOperationSPtr(
            __in Management::ResourceManager::Context & context,
            __in Transport::MessageUPtr request,
            __in std::unique_ptr<TReceiverContext> requestContext,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root)>;

        template<typename TReceiverContext>
        class ResourceManagerService
            : public Common::ComponentRoot
            , public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ResourceManager>
        {
            DENY_COPY(ResourceManagerService)

            using Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ResourceManager>::TraceId;
            using Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ResourceManager>::get_TraceId;
        public:
            // Instantiate with static Create().
            // !!! do NOT create directly !!!
            ResourceManagerService(
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Store::IReplicatedStore * replicatedStore,
                std::wstring const & resourceTypeName,
                QueryMetadataFunction const & queryResourceMetadataFunction,
                Common::TimeSpan const & maxRetryDelay);

            static std::shared_ptr<ResourceManagerService<TReceiverContext>> Create(
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Store::IReplicatedStore * replicatedStore,
                std::wstring const & resourceTypeName,
                QueryMetadataFunction const & queryResourceMetadataFunction,
                Common::TimeSpan const & maxRetryDelay);

            AsyncOperationSPtr Handle(
                __in Transport::MessageUPtr request,
                __in std::unique_ptr<TReceiverContext> requestContext,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root);

            void GetSupportedActions(std::vector<wstring> & actions);

            Common::AsyncOperationSPtr RegisterResource(
                __in std::wstring const & resourceName,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root,
                Common::ActivityId const & activityId = Common::ActivityId());

            Common::AsyncOperationSPtr RegisterResources(
                __in std::vector<std::wstring> const & resourceNames,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const & activityId = Common::ActivityId());

            Common::AsyncOperationSPtr UnregisterResource(
                __in std::wstring const & resourceName,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & root,
                Common::ActivityId const & activityId = Common::ActivityId());

            Common::AsyncOperationSPtr UnregisterResources(
                __in std::vector<std::wstring> const & resourceNames,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const & activityId = Common::ActivityId());
            
        private:
            Context context_;
            std::unordered_map<std::wstring, RequestHandler<TReceiverContext>> requestHandlerMap_;

            void InitializeRequestHandlers();
        };

        using IpcResourceManagerService = ResourceManagerService<Transport::IpcReceiverContext>;
        using IpcResourceManagerServiceSPtr = std::shared_ptr<IpcResourceManagerService>;

        using FederationResourceManagerService = ResourceManagerService<Federation::RequestReceiverContext>;
        using FederationResourceManagerServiceSPtr  = std::shared_ptr<FederationResourceManagerService>;
    }
}