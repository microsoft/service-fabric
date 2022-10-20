// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class provides the following features - 
    // 1) Encapsulates the network definition.
    // 2) Encapsulates network resource manager that is responsible for allocation/deallocation of resources.
    // 3) Provides apis such as create/delete network, attach/detach container and update routes which invoke relevant
    // apis on the overlay network plugin to perform platform specific operations.
    // 4) Maintains mapping between code packages and ip/mac addresses.
    // 5) References the platform specific network plugin to invoke create/delete network, attach/detach container and update routes.
    class OverlayNetworkDriver :
        public Common::RootedObject,
        public Common::FabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(OverlayNetworkDriver)

    public:

        OverlayNetworkDriver(
            Common::ComponentRootSPtr const & root,
            __in OverlayNetworkManager & networkManager,
            __in OverlayNetworkPluginSPtr plugin,
            __in OverlayNetworkDefinitionSPtr networkDefinition,
            ReplenishNetworkResourcesCallback const & replenishNetworkResourcesCallback);
        
        OverlayNetworkDriver(
            Common::ComponentRootSPtr const & root,
            __in OverlayNetworkManager & networkManager,
            __in OverlayNetworkPluginSPtr plugin);

        virtual ~OverlayNetworkDriver();

        // Get overlay network definition.
        __declspec(property(get = get_NetworkDefinition)) OverlayNetworkDefinitionSPtr NetworkDefinition;
        OverlayNetworkDefinitionSPtr get_NetworkDefinition() const { return this->networkDefinition_; };

        // Attaches container to network defined by this driver. The network has already been
        // set up as part of the instantiation of this class.
        //
        // The arguments are:
        //  containerId: Id of the container.
        //  ipAddress: IP address of the container.
        //  macAddress: MAC address of the container.
        //
        // Returns:
        //  An error code indicating if the container network stack was set up.
        //
        Common::AsyncOperationSPtr BeginAttachContainerToNetwork(
            std::wstring const & containerId,
            std::wstring const & ipAddress,
            std::wstring const & macAddress,
            std::wstring const & dnsServerList,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAttachContainerToNetwork(
            Common::AsyncOperationSPtr const & operation);

        // Detaches container from network defined by this driver. If this is the last container
        // on the network, then delete network is invoked as well.
        //
        // The arguments are:
        //  containerId: Id of the container.
        //
        // Returns:
        //  An error code indicating if the container network stack was deleted.
        //
        Common::AsyncOperationSPtr BeginDetachContainerFromNetwork(
            std::wstring const & containerId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDetachContainerFromNetwork(
            Common::AsyncOperationSPtr const & operation);

        // Updates route changes corresponding to network topology changes in the cluster.
        //
        // The arguments are:
        //  routes: Routing information for the cluster.
        //
        // Returns:
        //  An error code indicating if the routes were updated.
        //
        Common::AsyncOperationSPtr BeginUpdateRoutes(
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const & operation);

        // AcquireNetworkResource: Acquire network resources for all code packages passed in. If there is a failure 
        // we release all network resources acquired so far.
        //
        // The arguments are:
        //  nodeId: Id of the node.
        //  servicePackageId: Id of the service package being deployed.
        //  codePackageNames: Code packages being deployed that are part of the service package.
        //
        // Returns:
        //  An error code indicating if the acquisition was successful.
        //
        Common::AsyncOperationSPtr BeginAcquireNetworkResources(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & codePackageNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAcquireNetworkResources(
            Common::AsyncOperationSPtr const & operation,
            __out std::vector<std::wstring> & assignedNetworkResources);

        // ReleaseNetworkResources: Releases network resources for all code packages in the service package for the given node.
        //
        // The arguments are:
        //  nodeId: Id of the node.
        //  servicePackageId: Id of the service package being deployed.
        //
        // Returns:
        //  An error code indicating if the release was successful.
        //
        Common::AsyncOperationSPtr BeginReleaseNetworkResources(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReleaseNetworkResources(
            Common::AsyncOperationSPtr const & operation);

        // ReleaseAllNetworkResourcesForNode: Release all network resources assigned to the node when node crashes/fails.
        //
        // The arguments are:
        //  nodeId: Id of the node.
        //
        // Returns:
        //  An error code indicating if the release was successful.
        //
        Common::AsyncOperationSPtr BeginReleaseAllNetworkResourcesForNode(
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReleaseAllNetworkResourcesForNode(
            Common::AsyncOperationSPtr const & operation);

        // GetReservedCodePackages: Get a map of node and service/code packages.
        //
        // The arguments are:
        // reservedCodePackages: Updated with a reference to a map of node and service/code packages.
        //
        void GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & reservedCodePackages);

        // OnNewIpamData: This method is called when network resource data has been 
        // updated by the Network Inventory Manager. 
        //
        // The arguments are:
        //  ipMacAddressMapToBeAdded: The collection of ip/mac pairs to be added.
        //  ipMacAddressMapToBeRemoved: The collection of ip/mac pairs to be deleted.
        //
        void OnNewIpamData(std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeAdded, 
            std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeRemoved);

    protected:

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:

        class UpdateRoutesAsyncOperation;
        class CreateNetworkAsyncOperation;
        class DeleteNetworkAsyncOperation;
        class AttachContainerAsyncOperation;
        class DetachContainerAsyncOperation;

        // Invoked replenish network resources on the network manager.
        void ReplenishNetworkResources();

        // Overlay network plugin
        OverlayNetworkPluginSPtr networkPlugin_;

        // Overlay network definition
        OverlayNetworkDefinitionSPtr networkDefinition_;

        // Overlay network manager callback to replenish network resources
        ReplenishNetworkResourcesCallback replenishNetworkResourcesCallback_;

        // Internal callback to replenish network resources
        InternalReplenishNetworkResourcesCallback internalReplenishNetworkResourcesCallback_;

        // This contains a reference to the method to call whenever the set of
        // ghost reservations changes.
        //
        GhostChangeCallback ghostChangeCallback_;

        // Overlay network resource provider
        OverlayNetworkResourceProviderSPtr networkResourceProvider_;

        // Overlay network driver lock
        Common::ExclusiveLock networkDriverLock_;

        // Overlay network manager
        OverlayNetworkManager & networkManager_;
    };
}
