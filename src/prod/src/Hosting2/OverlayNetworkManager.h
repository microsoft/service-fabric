// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class provides the following features - 
    // 1) Initializes the network driver for the overlay network, if one does not exist.
    // 2) Creation of an overlay network, needs network definition, which this class will retrieve
    //    from the Network Inventory Manager via the Network Inventory Manager Agent.
    // 3) Maintains a mapping between network name and network driver.
    // 4) Supports api to retrieve the network driver given the network name.
    // 5) Provides a callback to the overlay network driver. This callback is used by the network driver
    //    when it has exhausted the allocated ip and mac addresses. The callback will invoke the Network Inventory Manager
    //    via the Network Inventory Manager Agent, to get the next batch of ip and mac addresses.
    class OverlayNetworkManager :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(OverlayNetworkManager)

    public:

        OverlayNetworkManager(
            Common::ComponentRootSPtr const & root,
            ProcessActivationManager & processActivationManager);

        virtual ~OverlayNetworkManager();

        // Get the network plugin.
        __declspec(property(get = get_NetworkPlugin)) OverlayNetworkPluginSPtr NetworkPlugin;
        OverlayNetworkPluginSPtr get_NetworkPlugin() const { return this->networkPlugin_; };

        // Retrieves the network driver for the given network name.
        // If the network driver does not exist, one is set up. For a new network driver, network definition
        // is needed. This is retrieved through the Network Inventory Manager Agent talking to the Network
        // Inventory Manager.
        //
        // The arguments are:
        //  networkName: Name of the network.
        //
        // Returns:
        //  An error code indicating if the retrieval/creation was successful.
        //  networkDriver: Updated with the reference to the network driver, either existing or newly created.
        //
        Common::AsyncOperationSPtr BeginGetOverlayNetworkDriver(
            std::wstring const & networkName,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetOverlayNetworkDriver(
            Common::AsyncOperationSPtr const & operation,
            OverlayNetworkDriverSPtr & networkDriver);

        // Update ARP/FDB routes for an overlay network.
        // This api on the overlay network manager is invoked by the Network Inventory Manager Agent.
        //
        // The arguments are:
        //  routingInfo: Routing information is comprised of node ip, container ip and mac address.
        //
        // Returns:
        //  An error code indicating if the update was successful.
        //
        Common::AsyncOperationSPtr BeginUpdateOverlayNetworkRoutes(
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateOverlayNetworkRoutes(
            Common::AsyncOperationSPtr const & operation);

        // Delete network from the Network Inventory Manager.
        //
        // The arguments are:
        //  networkName: Name of the network.
        //  nodeId: Id of the node where network is to be deleted.
        //
        // Returns:
        //  An error code indicating if the delete was successful.
        //
        Common::AsyncOperationSPtr BeginDeleteOverlayNetworkDefinition(
            std::wstring const & networkName,
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteOverlayNetworkDefinition(
            Common::AsyncOperationSPtr const & operation);

        // Release all network resources for the node.
        // 
        Common::AsyncOperationSPtr BeginReleaseAllNetworkResourcesForNode(
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndReleaseAllNetworkResourcesForNode(
            Common::AsyncOperationSPtr const & operation);

        // Callback method invoked by network plugin process manager to request network tables
        // to be published.
        void RequestPublishNetworkTables();

        // Retrieve gateway ip addresses for all networks.
        //
        void RetrieveGatewayIpAddress(std::vector<std::wstring> const & networkNames, __out std::vector<std::wstring> & gatewayIpAddresses);

        // Get a map of network names and service/code packages.
        //
        void GetNetworkReservedCodePackages(std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & networkReservedCodePackages);

        // Get network names.
        //
        void GetNetworkNames(std::vector<std::wstring> & networkNames);

    protected:

        // AsyncFabricComponent methods
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndOpen(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndClose(
            Common::AsyncOperationSPtr const & operation);

        virtual void OnAbort();

    private:

        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class UpdateOverlayNetworkRoutesAsyncOperation;
        class GetOverlayNetworkDefinitionAsyncOperation;
        class DeleteOverlayNetworkDefinitionAsyncOperation;
        class RequestPublishNetworkTablesAsyncOperation;
        class GetOverlayNetworkDriverAsyncOperation;
        class ReplenishNetworkResourcesAsyncOperation;
        class ReleaseAllNetworkResourcesForNodeAsyncOperation;

        // Gets additional network resources from the Network Inventory Manager.
        //
        // The arguments are:
        //  networkName: Name of the network.
        //
        // Returns:
        //  An error code indicating if the replenish was successful.
        //
        Common::AsyncOperationSPtr BeginReplenishNetworkResources(
            std::wstring const & networkName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReplenishNetworkResources(
            Common::AsyncOperationSPtr const & operation);

        // Callback method invoked by network driver to replenish network resources.
        void ReplenishNetworkResources(std::wstring const & networkName);

        // Request network tables to be published for all networks on the node 
        // where the plugin process has restarted.
        //
        // Returns:
        // An error code indicating if the publish request was successful.
        //
        Common::AsyncOperationSPtr OverlayNetworkManager::BeginRequestPublishNetworkTables(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode OverlayNetworkManager::EndRequestPublishNetworkTables(
            Common::AsyncOperationSPtr const & operation);
        
        // Maintains mapping between network name and the driver.
        std::map<std::wstring, OverlayNetworkDriverSPtr> networkDriverMap_;

        // Maintains mapping between network name and the lightweight driver (with no network definition).
        // This is used when an update routes request comes in while we are fetching network definition for
        // the actual network driver instantiation.
        std::map<std::wstring, OverlayNetworkDriverSPtr> lightWeightNetworkDriverMap_;

        // This is the callback registered with the overlay network driver.
        // It is used by the driver when the resource pool has exhausted ip and mac addresses.
        ReplenishNetworkResourcesCallback replenishNetworkResourcesCallback_;

        // Network plugin singleton instance
        OverlayNetworkPluginSPtr networkPlugin_;

        // Config enabled flag to initialize the overlay network manager and supporting classes.
        bool overlayNetworkManagerEnabled_;

        // Flag indicating if network manager has initialized.
        bool initialized_;

        Common::ExclusiveLock lock_;

        // Component root
        ComponentRootSPtr root_;

        // Connector to fabric activator client
        ProcessActivationManager & processActivationManager_;
    };
}
