// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class NatIPAddressProvider :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(NatIPAddressProvider);

    public:

        NatIPAddressProvider(
            Common::ComponentRootSPtr const & root,
            ContainerActivator & owner);

        virtual ~NatIPAddressProvider();

        // Get a flag indicating if the ipam client has been successfully initialized.
        __declspec(property(get = get_Initialized)) bool Initialized;
        bool get_Initialized() const { return this->ipamInitialized_; };

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

        // Get a copy of a map of reserved ids and code packages
        void GetReservedCodePackages(std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> & reservedCodePackages,
            std::map<std::wstring, std::wstring> & reservationIdCodePackageMap);

        _declspec(property(get = get_GatewayIP)) wstring const & GatewayIP;
        inline wstring const & get_GatewayIP() const { return gatewayIp_; }

        AsyncOperationSPtr BeginDeleteNetwork(
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);

        ErrorCode EndDeleteNetwork(AsyncOperationSPtr const & operation);

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

    Common::ErrorCode NatIPAddressProvider::ReleaseIpAddressesInternal(
        std::wstring const & nodeId,
        std::wstring const & servicePackageId,
        std::map<std::wstring, vector<std::wstring>>::iterator const & servicePackageIter);

        AsyncOperationSPtr BeginCreateNetwork(
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);
        
        ErrorCode EndCreateNetwork(AsyncOperationSPtr const & operation);

        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class CreateNetworkAsyncOperation;
        class DeleteNetworkAsyncOperation;
        Common::ExclusiveLock ipAddressReservationLock_;
        ContainerActivator & containerActivator_;
        bool ipamInitialized_;
        INatIPAMSPtr ipam_;
        bool natIpAddressProviderEnabled_;
        std::wstring gatewayIp_;
        std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages_;
        std::map<std::wstring, std::wstring> reservationIdCodePackageMap_;
    };
}