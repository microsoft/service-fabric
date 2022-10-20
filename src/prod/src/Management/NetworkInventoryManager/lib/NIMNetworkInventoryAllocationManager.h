#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        class NIMPublishMappingJobQueueConfigSettingsHolder : public Common::JobQueueConfigSettingsHolder
        {
            DENY_COPY(NIMPublishMappingJobQueueConfigSettingsHolder)
        public:
            NIMPublishMappingJobQueueConfigSettingsHolder() {}
            ~NIMPublishMappingJobQueueConfigSettingsHolder() {}

            int get_MaxQueueSize() const override
            {
                return 100;// NamingConfig::GetConfig().RequestQueueSize;
            }

            int get_MaxParallelPendingWorkCount() const override
            {
                return 100;// NamingConfig::GetConfig().MaxPendingRequestCount;
            }

        protected:
            int get_MaxThreadCountValue() const override
            {
                return 5;// NamingConfig::GetConfig().RequestQueueThreadCount;
            }
        };


        class NetworkInventoryService;
        typedef std::function<bool(ServiceModel::ModelV2::NetworkResourceDescriptionQueryResult const &)> const NetworkQueryFilter;

        //--------
        // This class handles operations on a set of networks.
        class NetworkInventoryAllocationManager : public Common::TextTraceComponent<Common::TraceTaskCodes::FM>
        {
        public:
            NetworkInventoryAllocationManager(NetworkInventoryService & service);
            ~NetworkInventoryAllocationManager();

            void Close();

            //--------
            // Initializes the node manager with the global mac address pool.
            ErrorCode Initialize(MACAllocationPoolSPtr macPool,
                VSIDAllocationPoolSPtr vsidPool);

            ErrorCode LoadFromStore(MACAllocationPoolSPtr macPool,
                VSIDAllocationPoolSPtr vsidPool);

            //--------
            // Start the BeginNetworkCreate invoke async operation.
            Common::AsyncOperationSPtr BeginNetworkCreate(
                NetworkCreateRequestMessage & request,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete the invoke request and get the reply.
            Common::ErrorCode EndNetworkCreate(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<NetworkCreateResponseMessage> & reply);

            //--------
            // Start the BeginNetworkRemove invoke async operation.
            Common::AsyncOperationSPtr BeginNetworkRemove(
                NetworkRemoveRequestMessage & request,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete the invoke request and get the reply.
            Common::ErrorCode EndNetworkRemove(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply);

            //--------
            // Start the BeginNetworkNodeRemove invoke async operation.
            Common::AsyncOperationSPtr BeginNetworkNodeRemove(
                NetworkRemoveRequestMessage & request,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete the invoke request and get the reply.
            Common::ErrorCode EndNetworkNodeRemove(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply);

            //--------
            // Start the BeginNetworkEnumerate invoke async operation.
            Common::AsyncOperationSPtr BeginNetworkEnumerate(
                NetworkEnumerateRequestMessage & request,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete the invoke request and get the reply.
            Common::ErrorCode EndNetworkEnumerate(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<NetworkEnumerateResponseMessage> & reply);

            //--------
            // Start the BeginAssociateApplication invoke async operation.
            Common::AsyncOperationSPtr BeginAssociateApplication(
                std::wstring const & networkName,
                std::wstring const & applicationName,
                bool isAssociation,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
                
            //--------
            // Complete the invoke request and get the reply.
            Common::ErrorCode EndAssociateApplication(
                Common::AsyncOperationSPtr const & operation);

            //--------
            // Start the BeginAcquireEntryBlock invoke async operation.
            Common::AsyncOperationSPtr BeginAcquireEntryBlock(
                NetworkAllocationRequestMessage & allocationRequest,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete the invoke request and get the reply.
            Common::ErrorCode EndAcquireEntryBlock(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<NetworkAllocationResponseMessage> & reply);

            //--------
            // Get a network by name
            NetworkDefinitionSPtr GetNetworkByName(const std::wstring & networkName);

            //--------
            // Enumerates the defined networks with filters.
            std::vector<ServiceModel::ModelV2::NetworkResourceDescriptionQueryResult> NetworkEnumerate(const NetworkQueryFilter & filter);

            std::vector<ServiceModel::NetworkApplicationQueryResult> GetNetworkApplicationList(const std::wstring & networkName);
            std::vector<ServiceModel::ApplicationNetworkQueryResult> GetApplicationNetworkList(const std::wstring & applicationName);
            std::vector<ServiceModel::NetworkNodeQueryResult> GetNetworkNodeList(const std::wstring & networkName);
            std::vector<ServiceModel::DeployedNetworkQueryResult> GetDeployedNetworkList();

            //--------
            // Enqueue a job to send the mapping table.
            Common::ErrorCode EnqueuePublishNetworkMappings(const std::wstring& networkName);

            //--------
            // Clean up stale network allocations.
            Common::ErrorCode CleanUpNetworks();

        private:
            //--------
            // Enqueue a job to send the mapping table.
            void EnqueuePublishNetworkMappings(
                const LockedNetworkManagerSPtr & lockedNetworkMgr);

            Common::ErrorCode ReleaseAllIpsForNodeCallback(
                const LockedNetworkManagerSPtr & lockedNetworkMgr,
                const std::vector<NodeInfoSPtr> & nodeInfos,
                int index,
                AsyncOperationSPtr const & contextSPtr);

            //--------
            // Creates a new network.
            Common::ErrorCode NetworkCreateAsync(
                NetworkCreateRequestMessage & request,
                Common::AsyncOperationSPtr const & operation);

            //--------
            // Remove a network.
            Common::ErrorCode NetworkRemoveAsync(
                NetworkRemoveRequestMessage & request,
                Common::AsyncOperationSPtr const & operation);
            
            //--------
            // Remove a node from the network.
            Common::ErrorCode NetworkNodeRemoveAsync(
                NetworkRemoveRequestMessage & request,
                Common::AsyncOperationSPtr const & operation);

            //--------
            // Enumerates the defined networks.
            Common::ErrorCode NetworkEnumerateAsync(
                NetworkEnumerateRequestMessage & request,
                Common::AsyncOperationSPtr const & operation);

            //--------
            // Adquires a block of IP and MAC addresses from a network for a particular node
            Common::ErrorCode AcquireEntryBlockAsync(
                NetworkAllocationRequestMessage & allocationRequest,
                Common::AsyncOperationSPtr const & operation);

            std::unordered_map<std::wstring, NetworkManagerSPtr> networkMgrMap_;
            NIMNetworkMACAddressPoolStoreDataSPtr macPool_;
            NetworkInventoryService & service_;

            template <class TRequest, class TReply>
            class InvokeActionAsyncOperation;
            class InvokeAcquireEntryBlock;
            class InvokeRemoveNetwork;

            class PublishNetworkMappingJobItem;
            class AssociateApplicationAsyncOperation;

            using PublishNetworkMappingAsyncJobQueue = Common::AsyncWorkJobQueue<PublishNetworkMappingJobItem>;
            using PublishNetworkMappingAsyncJobQueueSPtr = std::shared_ptr<PublishNetworkMappingAsyncJobQueue>;

             // Queue for processing sending network mapping tables to hosts.
            PublishNetworkMappingAsyncJobQueueSPtr publishNetworkMappingJobQueue_;

            ExclusiveLock managerLock_;
        };
    }
}