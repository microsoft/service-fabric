// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class provides the virtual methods which can be overridden
    // in specialized platform specific classes. The virtual methods provide mechanisms for
    // creating/deleting the network, attaching/detaching containers and updating routes, if needed.
    class OverlayNetworkPlugin :
        public INetworkPlugin,
        public FabricComponent,
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(OverlayNetworkPlugin)

    public:

        OverlayNetworkPlugin(Common::ComponentRootSPtr const & root);

        virtual ~OverlayNetworkPlugin();

        // Creation of overlay network using OS specific apis.
        //
        // The arguments are:
        //  networkDefinition: Contains network set up details such as network name, 
        //  subnet, gateway, mask, mac addresses and vxlan id.
        //
        // Returns:
        //  An error code indicating if the creation was successful.
        //
        Common::AsyncOperationSPtr BeginCreateNetwork(
            OverlayNetworkDefinitionSPtr const & networkDefinition,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCreateNetwork(
            Common::AsyncOperationSPtr const & operation);

        // Deletion of overlay network.
        //
        // The arguments are:
        //  networkName: Name of the network to be deleted.
        //
        // Returns:
        //  An error code indicating if the deletion was successful.
        //
        Common::AsyncOperationSPtr BeginDeleteNetwork(
            std::wstring const & networkName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteNetwork(
            Common::AsyncOperationSPtr const & operation);

        // Attaches container to overlay network.
        //
        // The arguments are:
        //  containerNetworkParams: Contains container network set up details such as container id, network name, 
        //  subnet, gateway, ip/mac address, node ip and network policy.
        //
        // Returns:
        //  An error code indicating if the attachment was successful.
        //
        Common::AsyncOperationSPtr BeginAttachContainerToNetwork(
            OverlayNetworkContainerParametersSPtr const & containerNetworkParams,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAttachContainerToNetwork(
            Common::AsyncOperationSPtr const & operation);

        // Detaches container from overlay network.
        //
        // The arguments are:
        //  containerId: Id of container to detach from overlay network.
        //  networkName: Name of overlay network to detach from.
        //
        // Returns:
        //  An error code indicating if the attachment was successful.
        //
        Common::AsyncOperationSPtr BeginDetachContainerFromNetwork(
            std::wstring const & containerId,
            std::wstring const & networkName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDetachContainerFromNetwork(
            Common::AsyncOperationSPtr const & operation);

        // Update ARP/FDB routes for an overlay network.
        //
        // The arguments are:
        //  routes: Routing information for an overlay network.
        //
        // Returns:
        //  An error code indicating if route update was successful.
        //
        Common::AsyncOperationSPtr BeginUpdateRoutes(
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const & operation);

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

        OverlayNetworkPluginOperationsUPtr pluginOperations_;
    };
}
