// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace Common;

namespace Management
{
    namespace NetworkInventoryManager
    {
        class InternalError
        {
        public:
            __declspec (property(get = get_ErrorCode, put = set_ErrorCode)) const ErrorCode Error;
            const ErrorCode & get_ErrorCode() const { return errorCode_; }
            void set_ErrorCode(const ErrorCode & value) { errorCode_ = value; }

            __declspec (property(get = get_UpdateNeeded, put = set_UpdateNeeded)) bool UpdateNeeded;
            bool & get_UpdateNeeded() { return updateNeeded_; }
            void set_UpdateNeeded(bool & value) { updateNeeded_ = value; }

        private:
            ErrorCode errorCode_;
            bool updateNeeded_;
        };


        class NetworkInventoryService;

        //--------
        // This class manages the address allocations (ips and macs) for a specific network,
        // keeps track of per node allocation and computes the global network mapping table
        // to be distributed to nodes.
        //
        class NetworkManager : public Common::TextTraceComponent<Common::TraceTaskCodes::FM>,
            public std::enable_shared_from_this<NetworkManager>
        {
            DENY_COPY(NetworkManager)

        public:
            NetworkManager(NetworkInventoryService & service);
            NetworkManager(NetworkManager &&) = default;
            NetworkManager & operator=(NetworkManager &&) = default;

            Common::ErrorCode Initialize(NIMNetworkDefinitionStoreDataSPtr network,
                NIMNetworkMACAddressPoolStoreDataSPtr macPool,
                bool isLoading = false);

            //---------
            // Set the Node -> allocation data (used when loading from store).
            void SetNodeAllocMap(NIMNetworkNodeAllocationStoreDataSPtr & nodeAllocMap);

            //--------
            // Set the IPv4 address pool (used when loading from store).
            void SetIPv4AddressPool(NIMNetworkIPv4AddressPoolStoreDataSPtr & ipPool);

            //--------
            // Acquire a pool of IP/MACs for the requesting node.
            ErrorCode AcquireIPBlockAsync(
                const Management::NetworkInventoryManager::NetworkAllocationRequestMessage & request,
                __out std::shared_ptr<NetworkAllocationResponseMessage> & reply,
                Common::AsyncOperationSPtr const & operation);

            //--------
            // Remove the network and its allocations.
            ErrorCode NetworkRemoveAsync(
                const Management::NetworkInventoryManager::NetworkRemoveRequestMessage & request,
                __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply,
                Common::AsyncOperationSPtr const & operation);

            //--------
            // Remove the node from the network and its allocations.
            ErrorCode NetworkNodeRemoveAsync(
                const Management::NetworkInventoryManager::NetworkRemoveRequestMessage & request,
                __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply,
                Common::AsyncOperationSPtr const & operation);

            ErrorCode CleanUpAsync(
                const std::vector<NodeInfoSPtr> & request,
                __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply,
                Common::AsyncOperationSPtr const & operation);


            //--------
            // Start the BeginAcquireEntryBlock invoke async operation.
            Common::AsyncOperationSPtr BeginAcquireEntryBlock(
                NetworkAllocationRequestMessage & request,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete invoke request and get the reply.
            Common::ErrorCode EndAcquireEntryBlock(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<NetworkAllocationResponseMessage> & reply);

            //--------
            // Start the BeginRemoveNetwork invoke async operation.
            Common::AsyncOperationSPtr BeginNetworkRemove(
                NetworkRemoveRequestMessage & request,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete invoke request and get the reply.
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

            //Release all ips assigned to a node.
            Common::ErrorCode ReleaseAllIpsForNodeAsync(
                NodeInfoSPtr & nodeInfo,
                bool releaseAllInstances,
                __out std::shared_ptr<InternalError> & response,
                Common::AsyncOperationSPtr const & operation);

            //--------
            // Start the BeginReleaseAllIpsForNode invoke async operation.
            Common::AsyncOperationSPtr BeginReleaseAllIpsForNode(
                NodeInfoSPtr & nodeInfo,
                bool releaseAllInstances,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete invoke request and get the reply.
            Common::ErrorCode EndReleaseAllIpsForNode(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<InternalError> & reply);

            ErrorCode UpdatePersistentDataAsync(
                const std::vector<NIMNetworkNodeAllocationStoreDataSPtr> & nodeEntries,
                Common::AsyncOperationSPtr const & operation);


            //--------
            // Start the BeginUpdatePersistentData invoke async operation.
            Common::AsyncOperationSPtr BeginUpdatePersistentData(
                std::vector<NIMNetworkNodeAllocationStoreDataSPtr> & request,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback);

            //--------
            // Complete invoke request and get the reply.
            Common::ErrorCode EndUpdatePersistentData(
                Common::AsyncOperationSPtr const & operation,
                __out std::shared_ptr<ErrorCode> & reply);


            //--------
            // Get the network mapping table to distribute to every node that belongs to the network
            ErrorCode GetNetworkMappingTable(std::vector<Federation::NodeInstance>& nodeInstances,
                __out std::vector<Management::NetworkInventoryManager::NetworkAllocationEntrySPtr>& networkMappingTable);

            //--------
            // Persist the network data to the store
            ErrorCode PersistData();

            __declspec(property(get = get_NetworkDefinition)) const NetworkDefinitionSPtr NetworkDefinition;
            const NetworkDefinitionSPtr get_NetworkDefinition() { return network_->NetworkDefinition; }

            //--------
            // Get the sequence number for the network changes
            int64 GetSequenceNumber() const;

            // Associate/dissociate an app from the network
            void AssociateApplication(
                std::wstring const & applicationName,
                bool isAssociation);

            Common::StringCollection GetAssociatedApplications();
            Common::StringCollection GetAssociatedNodes();

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
            {
                w.Write("xxxx: {0}", 123);
            }

        private:
            Federation::NodeInstance GetNodeInstance(const std::wstring & nodeName);

            //--------
            // Get a node allocation map for the specify nodeName.
            NIMNetworkNodeAllocationStoreDataSPtr GetOrCreateNodeAllocationMap(const std::wstring & networkId,
                const std::wstring & nodeName);
            void ReleaseEntryAllocations(
                NIMNetworkNodeAllocationStoreDataSPtr & nodeEntry,
                const std::map<std::wstring, std::wstring> & ipMacMap);

            NIMNetworkDefinitionStoreDataSPtr network_;
            NIMNetworkMACAddressPoolStoreDataSPtr macPool_;
            NIMNetworkIPv4AddressPoolStoreDataSPtr ipPool_;

            std::map<std::wstring, NIMNetworkNodeAllocationStoreDataSPtr> nodeAllocationMap_;

            NetworkInventoryService & service_;
            
            template <class TRequest, class TReply>
            class InvokeActionAsyncOperation;


            ExclusiveLock managerLock_;
        };

        typedef std::shared_ptr<NetworkManager> NetworkManagerSPtr;
        typedef Reliability::FailoverManagerComponent::LockedCacheEntry<NetworkManager> LockedNetworkManager;
        typedef std::shared_ptr<LockedNetworkManager> LockedNetworkManagerSPtr;
        
        typedef Reliability::FailoverManagerComponent::LockedCacheEntry<NIMNetworkMACAddressPoolStoreData> LockedNetworkMACAddressPool;
        typedef std::shared_ptr<LockedNetworkMACAddressPool> LockedNetworkMACAddressPoolSPtr;
    }
}