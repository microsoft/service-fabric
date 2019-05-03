// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class provides the following features:
    // 1) Encapsulates the overlay IPAM client.
    // 2) Exposes apis such as acquire and release overlay network resources (ip and mac address pairs).
    class OverlayNetworkResourceProvider :
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(OverlayNetworkResourceProvider);

        public:

            OverlayNetworkResourceProvider(
                Common::ComponentRootSPtr const & root,
                __in OverlayNetworkDefinitionSPtr networkDefinition,
                InternalReplenishNetworkResourcesCallback const & internalReplenishNetworkResourcesCallback,
                GhostChangeCallback const & ghostChangeCallback);

            // Constructor used for testing.
            // First parameter is the mock implementation of IIPAM.
            // Second parameter is the parent object for this instance.
            OverlayNetworkResourceProvider(
                Common::ComponentRootSPtr const & root,
                __in IOverlayIPAMSPtr ipamClient,
                __in OverlayNetworkDefinitionSPtr networkDefinition,
                InternalReplenishNetworkResourcesCallback const & internalReplenishNetworkResourcesCallback,
                GhostChangeCallback const & ghostChangeCallback);

            virtual ~OverlayNetworkResourceProvider();

            // Get a flag indicating if the ipam client has been successfully initialized.
            __declspec(property(get = get_Initialized)) bool Initialized;
            bool get_Initialized() const { return this->ipamInitialized_; };

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

            // GetReservedCodePackages: Get a copy of a map of reserved ids and code packages.
            //
            // The arguments are:
            // reservedCodePackages: Updated with a reference to a copy of the internal reservedCodePackages map.
            // reservationIdCodePackageMap: Updated with a reference to a copy of the internal reservationIdCodePackageMap map.
            //
            void GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> & reservedCodePackages,
                                         std::map<std::wstring, std::wstring> & reservationIdCodePackageMap);

            // OnNewIpamData: This method is called when network resource data has been 
            // updated by the Network Inventory Manager. It is responsible for maintaining 
            // a running list of network resources that can be applied to the reservation pool.
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

            class AcquireNetworkResourcesAsyncOperation;
            class ReleaseNetworkResourcesAsyncOperation;
            class ReleaseAllNetworkResourcesForNodeAsyncOperation;

            // Populate ip/mac pair into network resources set
            std::unordered_set<OverlayNetworkResourceSPtr> PopulateNetworkResources(std::map<std::wstring, std::wstring> const & ipMacAddressMap);

            // Lock used for access to network resources
            Common::ExclusiveLock networkResourceProviderLock_;

            // Overlay network definition
            OverlayNetworkDefinitionSPtr networkDefinition_;

            // Indicator for ipam client initialization
            bool ipamInitialized_;

            // Overlay IPAM client interface
            IOverlayIPAMSPtr ipam_;

            // Mapping of node id, service package id and code packages.
            std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages_;

            // Mapping of reservation id and code package
            std::map<std::wstring, std::wstring> reservationIdCodePackageMap_;

            // Overlay network manager callback to replenish network resources
            InternalReplenishNetworkResourcesCallback internalReplenishNetworkResourcesCallback_;

            // This contains a reference to the method to call whenever the set of
            // ghost reservations changes.
            //
            GhostChangeCallback ghostChangeCallback_;
    };
}