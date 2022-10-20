// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IPAddressProvider :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(IPAddressProvider);

        public:
            // Constructor used by Hosting.
            // First parameter is the parent object for this instance.
            IPAddressProvider(
                Common::ComponentRootSPtr const & root,
                ProcessActivationManager & processActivationManager);

            // Constructor used for testing.
            // First parameter is the mock implementation of IIPAM.
            // Second parameter is the clusterId. This is used to determine if we are on azure.
            // Third parameter is to determine if the provider should be enabled.
            // Fourth parameter is the parent object for this instance.
            IPAddressProvider(
                IIPAMSPtr const & ipamClient,
                std::wstring const & clusterId,
                bool const ipAddressProviderEnabled,
                Common::ComponentRootSPtr const & root);

        virtual ~IPAddressProvider();

        public:

            // Get a flag indicating if the ipam client has been successfully initialized.
            __declspec(property(get = get_Initialized)) bool Initialized;
            bool get_Initialized() const { return this->ipamInitialized_; };

            // Get the gateway ip address for the vnet in the dot format.
            __declspec(property(get = get_GatewayIpAddress)) wstring GatewayIpAddress;
            wstring get_GatewayIpAddress() const { return this->gatewayIpAddress_; };

            // Get the subnet in CIDR format.
            __declspec(property(get = get_Subnet)) wstring Subnet;
            wstring get_Subnet() const { return this->subnet_; };

            // Acquire ip addresses for all code packages passed in. If there is a failure 
            // we release all ips acquired so far.
            Common::ErrorCode AcquireIPAddresses(
                std::wstring const & nodeId,
                std::wstring const & servicePackageId,
                std::vector<std::wstring> const & codePackageNames,
                __out std::vector<std::wstring> & assignedIps);

            // Releases ip addresses for all code packages in the service package for the given node.
            Common::ErrorCode ReleaseIpAddresses(
                std::wstring const & nodeId,
                std::wstring const & servicePackageId);

            //Release all ips assigned to Node when node crashes/fails
            void ReleaseAllIpsForNode(
                std::wstring const & nodeId);

            // Api allows consumers to enroll in refresh cycles that will
            // keep the ip address pool updated. The callback is for ghost reservations and provides
            // an opportunity to bounce processes/containers that are using stale ip addresses.
            Common::ErrorCode StartRefreshProcessing(
                Common::TimeSpan const refreshInterval,
                Common::TimeSpan const refreshTimeout,
                std::function<void(std::map<std::wstring, std::wstring>)> ghostReservationsAvailableCallback);

            // Get a copy of a map of reserved ids and code packages
            void GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> & reservedCodePackages,
                                         std::map<std::wstring, std::wstring> & reservationIdCodePackageMap);

            // GetNetworkReservedCodePackages: Get a map of flat network name and service/code packages.
            //
            // The arguments are:
            // networkReservedCodePackages: Updated with a reference to a map of flat network name and service/code packages.
            //
            void GetNetworkReservedCodePackages(std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & networkReservedCodePackages);

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
            Common::ExclusiveLock ipAddressReservationLock_;
            bool ipamInitialized_;
            IIPAMSPtr ipam_;
            std::wstring gatewayIpAddress_;
            std::wstring subnet_;
            std::wstring clusterId_;
            bool ipAddressProviderEnabled_;
            std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages_;
            std::map<std::wstring, std::wstring> reservationIdCodePackageMap_;
    };
}